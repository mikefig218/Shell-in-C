//-----Michael Guerrero and Michael Figura-----//
//MF student ID - 702380403

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <signal.h>
#include <glob.h>
#include <utmpx.h>
#include <pthread.h>
#include <time.h>
#include <fcntl.h>
#include "sh.h"

#define BUFFERSIZE 2048

//-----INITIAL DECLARATIONS (FOR HELPER FUNCTIONS NOT CONTAINING POINTERS)-----//
void cd(char *curr, char *new);
int parse(char *command, char **arguments);
void mysh_free(struct pathelement *head);
static void *watchUser(void *arbitrary);
static void *watchMail(void *arbitrary);
void addUser(char *user);
ListOfMail_t *addMail(char *mail);
void removeUser(char *user);
void removeMail(char *mail);
char *redirectionCheck(char **args, int arguments);
void pipeFunc(char **args, int arguments);
void source(int pfd[], char **cmd, char *symbol);
void dest(int pfd[], char **cmd, char *symbol);
char *pipeCheck(char **args, int arguments);

ListOfUsers_t *userHead;
ListOfMail_t *mailHead;
pthread_mutex_t userLock;

//-----CODE FOR SHELL RUNS IN HERE-----//
int sh( int argc, char **argv, char **envp )
{
  //-----BELOW VARIABLES ALL GIVEN (MOST USED)-----//
  char *prompt = calloc(PROMPTMAX, sizeof(char));
  char *commandline = calloc(MAX_CANON, sizeof(char));
  char *command, *arg, *commandpath, *p, *pwd, *owd;
  char **args = calloc(MAXARGS, sizeof(char*));
  int uid, i, status, argsct, go = 1;
  struct passwd *password_entry;
  char *homedir;
  struct pathelement *pathlist;
  int noclobber, bgprocess, userThread;
  pthread_t ut; //user thread
  char *fileName;

  uid = getuid();
  password_entry = getpwuid(uid);               /* get passwd info */
  homedir = password_entry->pw_dir;		/* Home directory to start
						  out with*/

  if ( (pwd = getcwd(NULL, PATH_MAX+1)) == NULL )
  {
    perror("getcwd");
    exit(2);
  }
  owd = calloc(strlen(pwd) + 1, sizeof(char));
  memcpy(owd, pwd, strlen(pwd));
  prompt[0] = ' '; prompt[1] = '\0';

  /* Put PATH into a linked list */
  pathlist = get_path();

  char cwd[BUFFERSIZE];
  getcwd(cwd, sizeof(cwd));
  printf("%s [%s] -> ", prompt, cwd);

  while ( go )
  {
    signal(SIGTSTP, SIG_IGN); //handles ctrl+z signal
    signal(SIGINT, SIG_IGN); //handles ctrl+c signal;

    int flag = 0; //ctrl + d should not enter else statement
    bgprocess = 0;
    int pipeVar = 0;

    if(fgets(commandline, MAX_CANON, stdin) == NULL){ //handles ctrl+d
      printf("\nUse command 'exit' to quit\n");
      flag = 1;
    }

    if(strlen(commandline) > 0){ //changes last character from \n to \0 (null terminating character)
      commandline[strlen(commandline)-1] = '\0';
    }

    int arguments = parse(commandline, args); //uses the parse function to split text into arguments

    if(strcmp(args[arguments - 1], "&") == 0){ //if it is a background process
      bgprocess = 1; //determines which statement it will run through at a later point
      args[arguments - 1] = NULL;
      arguments --;
    }

    int fid;
    char *reArg = redirectionCheck(&args[0], arguments);

    if(pipeCheck(&args[0], arguments)){ //determines if a pipe character is found
      pipeVar = 1;
    }

    if(redirectionCheck(&args[0], arguments)){ // determines if a redirection character is found
      fileName = args[arguments - 1];
    }


    //-----EXIT-----//
    if(strcmp(args[0], "exit") == 0){
      if(arguments > 1){ //there are no arguments for exit
        fprintf(stderr, "There are no parameters allowed when using the 'exit' command\n");
      }
      else{ //exits shell successfully
        printf("Executing built-in %s\n", args[0]);
        printf("exiting shell...\n");
        exit(EXIT_SUCCESS);
      }
    }

    //-----WHICH-----//
    else if(strcmp(args[0], "which") == 0){
      if(arguments < 2){ //when using which there must be an argument to find
        fprintf(stderr, "Please provide at least one parameter when using the 'which' command\n");
      }
      else{ //if there is an argument print default location (using 'which' helper function)
      printf("Executing built-in %s\n", args[0]);
        for(int i = 1; i < arguments; i++){ //loops incase more than one argument was input
          which(args[i], pathlist);
        }
      }
    }

    //-----WHERE-----//
    else if(strcmp(args[0], "where") == 0){
      if(arguments < 2){ //when using where there must be an argument to find
        fprintf(stderr, "Please provide at least one parameter when using the 'where' command\n");
      }
      else{ //if there is an argument print all locations (using 'which' helper function)
      printf("Executing built-in %s\n", args[0]);
        for(int i = 1; i < arguments; i++){ //loops incase more than one argument was input
          where(args[i], pathlist);
        }
      }
    }

    //-----CD-----//
    else if(strcmp(args[0], "cd") == 0){
      char *path = malloc(sizeof(path));
      char directory[BUFFERSIZE];
      getcwd(directory, sizeof(directory));
      if(arguments < 2){ //when just cd is typed in the shell goes to the default (HOME) directory
        printf("Executing built-in %s\n", args[0]);
        strcpy(path, directory);
        cd(getenv("HOME"), path);
      }
      else if(arguments == 2){ //when a path is given
        printf("Executing built-in %s\n", args[0]);
        strcpy(path, directory);
        if(chdir(args[1]) != -1 || args[1] == "-"){ //changes directory in this specific case (go back)
          strcpy(path, directory);
        }
        cd(args[1], path); //changes directory by adding on path user specifies
      }
      else{
        fprintf(stderr, "Can only choose one path for the 'cd' command\n");
      }
    }

    //-----PWD-----//
    else if(strcmp(args[0], "pwd") == 0){
      if(arguments < 2){ //uses simple logic and built in command to get pwd and then display it
        printf("Executing built-in %s\n", args[0]);
        char directory[BUFFERSIZE];
        getcwd(directory, sizeof(directory));
        printf("%s\n", directory);
      }
      else{
        fprintf(stderr, "There are no parameters allowed when using the 'pwd' command\n");
      }
    }

    //-----LIST-----//
    else if(strcmp(args[0], "list") == 0){
      char directory[BUFFERSIZE];
      getcwd(directory, sizeof(directory));
      if(arguments < 2){ //lists files in current working directory
        printf("Executing built-in %s\n", args[0]);
        printf("%s\n", directory);
        list(directory);
      }
      else{ //lists files in each directory based off arguments passed in
        printf("Executing built-in %s\n", args[0]);
        for(int i = 1; i < arguments; i++){ //loops incase more than one argument was input
          printf("%s\n", args[i]);
          list(args[i]);
        }
      }
    }

    //-----PID-----//
    else if(strcmp(args[0], "pid") == 0){
      if(arguments < 2){ //prints current PID using getpid()
        printf("Executing built-in %s\n", args[0]);
        printf("%d\n", getpid());
      }
      else{
        fprintf(stderr, "There are no parameters allowed when using the 'pid' command\n");
      }
    }

    //-----KILL-----//
    else if(strcmp(args[0], "kill") == 0){
      if(arguments < 2 || arguments > 3){
        fprintf(stderr, "When using the 'kill' command, please enter a PID or a signal followed by a PID\n");
      }
      else if(arguments == 2){ //kills pid specified
        printf("Executing built-in %s\n", args[0]);
        kill(atoi(args[1]), SIGKILL);
      }
      else if(strstr(args[1], "-") != NULL){ //sends signal to kill pid specified
        printf("Executing built-in %s\n", args[0]);
        kill(atoi(args[2]), abs(atoi(args[1])));
      }
      else{
        fprintf(stderr, "When using the 'kill' command, please enter a PID or a signal followed by a PID\n");
      }
    }

    //-----PROMPT-----//
    else if(strcmp(args[0], "prompt") == 0){
      if(arguments < 2){  //asks user to input prompt prefix if it is not initially specified
        printf("Executing built-in %s\n", args[0]);
        char *prefix = malloc(sizeof(char));
        printf("input prompt prefix: ");
        fgets(prefix, MAX_CANON, stdin);
        strtok(prefix, "\n");
        strcpy(prompt, prefix); //adds the user input to the start
      }
      else if(arguments == 2){ //adds the prompt prefix to the start when it is specified
        printf("Executing built-in %s\n", args[0]);
        strcpy(prompt, args[1]);
      }
      else{
        fprintf(stderr, "Please provide no more than one prefix when using the 'prompt' command\n");
      }
    }

    //-----PRINTENV-----//
    else if(strcmp(args[0], "printenv") == 0){
      if(arguments < 2){ //prints entire environment by looping through envp parameter and displaying it all line by line
        printf("Executing built-in %s\n", args[0]);
        char **enviro = envp;
        while(*enviro){
          printf("%s\n", *enviro);
          enviro++;
        }
      }
      else if(arguments == 2){ //prints the specified part of the environment if it exists (ex. user)
        printf("Executing built-in %s\n", args[0]);
        char *enviro = getenv(args[1]);
        if(enviro != NULL){
          printf("%s\n", enviro);
        }
        else{ //if the user input is not part of the environment
          fprintf(stderr, "Please provide a valid environment\n");
        }
      }
      else{
        fprintf(stderr, "Please provide no more than one parameter when using the 'printenv' command\n");
      }
    }

    //-----SETENV-----//
    else if(strcmp(args[0], "setenv") == 0){
      if(arguments < 2){ //prints entire environment the same way that printenv does
        printf("Executing built-in %s\n", args[0]);
        char **enviro = envp;
        while(*enviro){
          printf("%s\n", *enviro);
          enviro++;
        }
      }
      else if(arguments == 2){ //sets argument, proceeds to replace the path with the user-input environment
        printf("Executing built-in %s\n", args[0]);
        setenv(args[1], " ", 1);
        if(strcmp(args[1], "PATH") == 0){
          mysh_free(pathlist);
          pathlist = get_path();
        }
      }
      else if(arguments == 3){ //similar to above but replaces the first argument's environment with the second's
        printf("Executing built-in %s\n", args[0]);
        setenv(args[1], args[2], 1);
        if(strcmp(args[1], "PATH") == 0){
          mysh_free(pathlist);
          pathlist = get_path();
        }
      }
      else{
        fprintf(stderr, "Please provide no more than two parameters when using the 'setenv' command\n");
      }
    }

    //-----NOCLOBBER-----//
    else if(strcmp(args[0], "noclobber") == 0){
      if(arguments < 2){
        if(noclobber == 0){ //turns noclobber on or off
          noclobber = 1;
          printf("Noclobber value is now %d\n", noclobber);
        }
        else{
          noclobber = 0;
          printf("Noclobber value is now %d\n", noclobber);
        }
      }
      else{
        fprintf(stderr, "There are no parameters allowed when using the 'noclobber' command\n");
      }
    }

    //-----WATCHUSER-----//
    else if(strcmp(args[0], "watchuser") == 0){
      if(arguments < 2){
        printf("Executing built-in %s\n", args[0]);
        ListOfUsers_t *temp = userHead;
        while(temp != NULL){  //we made this to test our results
          printf("%s\n", temp->name);
          temp = temp->next;
        }
      }
      else if(arguments == 2){
        printf("Executing built-in %s\n", args[0]);
        if(userThread == 0){ //creates a thread of users
          pthread_create(&ut, NULL, watchUser, "List Of Users");
          userThread = 1; //only one user thread should be created
        }
        pthread_mutex_lock(&userLock); //locks thread
        addUser(args[1]); //adds user to the watch users list
        pthread_mutex_unlock(&userLock); //unlocks thread
      }
      else if(arguments == 3 && strcmp(args[2], "off") == 0){
        printf("Executing built-in %s\n", args[0]);
        pthread_mutex_lock(&userLock); //locks thread
        removeUser(args[1]);
        pthread_mutex_unlock(&userLock); //unlocks thread
      }
      else{
        fprintf(stderr, "When using the 'watchuser' command, please enter a user or a user followed by 'off'\n");
      }
    }

    //-----WATCHMAIL-----//
    else if(strcmp(args[0], "watchmail") == 0){
      ListOfMail_t *temp = mailHead;
      if(arguments < 2){
        printf("Executing built-in %s\n", args[0]);
        while(temp != NULL){ //we made this to test our results
          printf("%s\n", temp->file);
          temp = temp->next;
        }
      }
      else if(arguments == 2){
        if(access(args[1], F_OK) == 0){ //file must exist
          temp = addMail(args[1]);
          pthread_create(&(temp->file_thread), NULL, watchMail, temp->file); //creates thread for file
        }
        else{
          fprintf(stderr, "Please provide a valid file name\n");
        }
      }
      else if(arguments == 3 && strcmp(args[2], "off") == 0){
        printf("Executing built-in %s\n", args[0]);
        while(temp != NULL){
          if(strcmp(temp->file, args[1]) == 0){ //if command matches file
            pthread_cancel(temp->file_thread);
            removeMail(temp->file); //removes that file
          }
          else{
            temp = temp->next;
          }
        }
      }
      else{
        fprintf(stderr, "When using the 'watchmail' command, please enter a file or a file followed by 'off'\n");
      }
    }




    //-----FOR COMMANDS NOT BUILT IN-----//
      else if(strncmp(args[0], "/", 1) == 0 || strncmp(args[0], "./", 2) == 0 || strncmp(args[0], "../", 3) == 0){ //checks for absolute path
        if(access(args[0], F_OK) == 0){ //existence check for file
          int pid = fork(); //creates child process
          if(pid == 0){ //child process
            if(reArg != NULL){ //redirection check won't be null if redirect char is found
              if(strcmp(reArg, ">") == 0){
                if(noclobber){
                  printf("Noclobber is on, cannot overwrite\n");
                }
                else{ //adapted from code given to us
                  fid = open(fileName, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
                  close(1);
                  dup(fid);
                  close(fid);
                }
              }
              else if(strcmp(reArg, ">&") == 0){
                if(noclobber){
                  printf("Noclobber is on, cannot overwrite\n");
                }
                else{ //adapted from code given to us
                  fid = open(fileName, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
                  close(1);
                  close(2);
                  dup(fid);
                  dup(fid);
                  close(fid);
                }
              }
              else if(strcmp(reArg, ">>") == 0){
                if(noclobber){
                  printf("Noclobber is on, cannot make a new file\n");
                }
                else{ //adapted from code given to us
                  if (access(fileName, F_OK) == 0){
                    fid = open(fileName, O_RDWR | O_APPEND, S_IRUSR | S_IWUSR, S_IRWXU);
                  }
                  else{
                    fid = open(fileName, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR, S_IRWXU);
                  }
                  close(1);
                  dup(fid);
                  close(fid);
                }
              }
              else if(strcmp(reArg, ">>&") == 0){
                if(noclobber){
                  printf("Noclobber is on, cannot make a new file\n");
                }
                else{ //adapted from code given to us
                  if (access(fileName, F_OK) == 0){
                    fid = open(fileName, O_RDWR | O_APPEND, S_IRUSR | S_IWUSR, S_IRWXU);
                  }
                  else{
                    fid = open(fileName, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR, S_IRWXU);
                  }
                  close(1);
                  close(2);
                  dup(fid);
                  dup(fid);
                  close(fid);
                }
              } //adapted from code given to us
              else if(strcmp(reArg, "<") == 0){
                fid = open(fileName, O_RDWR | S_IRUSR | S_IWUSR, S_IRWXU);
                close(0);
                dup(fid);
                close(fid);
              }
              args[arguments - 2] = NULL;
              args[arguments - 1] = NULL;
            }
            execve(args[0], args, envp); //will search for the command and execute it using execve
            fprintf(stderr, "Could not execute\n"); //will only get here if there is an issue with the command, will exit with a failure
            exit(EXIT_FAILURE);
          }
          else{ //when the child is done running the command it should return to the parent
            if(bgprocess){
              printf("process running");
            }
            else{
              int exit_val;
              waitpid(pid, &exit_val, 0); //parent waits for the child process which will exit when command is done running
              printf("[%s] -> child process exited successfully\n", args[0]);
            }
          }
        }
        else{ //existence check has failed
          fprintf(stderr, "Please check for existence of file\n");
        }
      }

      else if(flag == 0 && strcmp(args[0], "") != 0){ //had to create a flag because ctrl d signal registered as input not fitting any other requirement and automatically forked
        char *file = which(args[0], pathlist);
        if(file != NULL){ //file must exist
          if(access(file, F_OK) == 0){ //and user must have access to it
            int pid = fork(); //creates child process
            if(pid == 0){ //child process
              if(reArg != NULL){ //redirection check won't be null if redirect char is found
                if(strcmp(reArg, ">") == 0){
                  if(noclobber){
                    printf("Noclobber is on, cannot overwrite\n");
                  }
                  else{ //adapted from code given to us
                    fid = open(fileName, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
                    close(1);
                    dup(fid);
                    close(fid);
                  }
                }
                else if(strcmp(reArg, ">&") == 0){
                  if(noclobber){
                    printf("Noclobber is on, cannot overwrite\n");
                  }
                  else{ //adapted from code given to us
                    fid = open(fileName, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
                    close(1);
                    close(2);
                    dup(fid);
                    dup(fid);
                    close(fid);
                  }
                }
                else if(strcmp(reArg, ">>") == 0){
                  if(noclobber){
                    printf("Noclobber is on, cannot make a new file\n");
                  }
                  else{ //adapted from code given to us
                    if (access(fileName, F_OK) == 0){
                      fid = open(fileName, O_RDWR | O_APPEND, S_IRUSR | S_IWUSR, S_IRWXU);
                    }
                    else{
                      fid = open(fileName, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR, S_IRWXU);
                    }
                    close(1);
                    dup(fid);
                    close(fid);
                  }
                }
                else if(strcmp(reArg, ">>&") == 0){
                    if(noclobber){
                      printf("Noclobber is on, cannot make a new file\n");
                    }
                    else{ //adapted from code given to us
                      if (access(fileName, F_OK) == 0){
                        fid = open(fileName, O_RDWR | O_APPEND, S_IRUSR | S_IWUSR, S_IRWXU);
                      }
                      else{
                        fid = open(fileName, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR, S_IRWXU);
                      }
                      close(1);
                      close(2);
                      dup(fid);
                      dup(fid);
                      close(fid);
                    }
                  } //adapted from code given to us
                  else if(strcmp(reArg, "<") == 0){
                    fid = open(fileName, O_RDWR | S_IRUSR | S_IWUSR, S_IRWXU);
                    close(0);
                    dup(fid);
                    close(fid);
                  }
                  args[arguments - 2] = NULL;
                  args[arguments - 1] = NULL;
                }
                execve(file, args, envp); //will search for the command and execute it using execve
            }
            else{
              if(bgprocess){
                printf("backgorund process running \n");
                sleep(1);
              }
              else{
                int exit_val;
                waitpid(pid, &exit_val, 0); //parent waits for the child process which will exit when command is done running
                printf("[%s] -> child process exited successfully\n", args[0]);
              }
            }
          }
          else{ //existence check has failed
            fprintf(stderr, "Please check for existence of file\n");
          }
        }
      //  }
      }

      if(pipeVar == 1){
        pipeFunc(&args[0], arguments);
      }

      char cwd[BUFFERSIZE];
      getcwd(cwd, sizeof(cwd));
      waitpid(pid, NULL, WNOHANG); //allows prompt to run in foreground, will end hanging child processes
      printf("%s [%s] -> ", prompt, cwd);

      flag = 0; //resets flag meaning ctrl d hasn't been pressed
    }
} /* sh() */

//-----MY WHICH HELPER FUNCTION HAS BEEN ADAPTED FROM THE CODE GIVEN TO US IN get_path_main.c-----//
char *which(char *command, struct pathelement *pathlist ) //which helper function
{
  char *cmd = malloc(sizeof(cmd)); //had to malloc for which so that it returns a value to fetch external commands
  pathlist = get_path();
  while (pathlist) {         // WHICH
    sprintf(cmd, "%s/%s", pathlist->element, command);
    if (access(cmd, F_OK) == 0) {
      printf("[%s]\n", cmd);
      return cmd;
      break;
    }
    pathlist = pathlist->next;
  }
   /* loop through pathlist until finding command and return it.  Return
   NULL when not found. */

}  /* which() */

//-----MY WHERE HELPER FUNCTION HAS BEEN ADAPTED FROM THE CODE GIVEN TO US IN get_path_main.c-----//
char *where(char *command, struct pathelement *pathlist )
{
    char cmd[64];
    pathlist = get_path();
    while (pathlist) {         // WHERE
      sprintf(cmd, "%s/%s", pathlist->element, command);
      if (access(cmd, F_OK) == 0)
        printf("[%s]\n", cmd);
      pathlist = pathlist->next;
    }
  /* similarly loop through finding all locations of command */
} /* where() */


//-----CD HELPER FUNCTION-----//
void cd(char *curr, char *new){
  char *path = malloc(sizeof(*path));
  strcpy(path, new); //copies user input and adds it onto the path
  char directory[BUFFERSIZE];
  getcwd(directory, sizeof(directory));
  if(strcmp(path, "..") == 0){ //when '..' is typed returns to home directory as a default
    chdir(directory);
    return;
  }
  strcat(directory, "/");
  if(strcmp(path, "-")){ //when '-' is typed moves back one directory
    chdir(curr);
    return;
  }
  if(strstr(path, "/")){
    if(chdir(path) != 0){ //changes directory to path given
      chdir(path);
    }
    return;
  }
  strcat(directory, path); //then adds that path onto the directory to display
  if(chdir(directory) != 0){
    perror("cd");
  }
  else{
    chdir(directory);
  }
}

//-----LIST HELPER FUNCTION-----//
void list ( char *dir )
{
  DIR *directory = opendir(dir);
  struct dirent *curr_direct;
  if(directory == NULL){
    printf("Please check existence of file(s)\n");
  }
  else{ //reads from directory file and lists all files contained in it
    while((curr_direct = readdir(directory)) != NULL){
      printf("%s\n", curr_direct->d_name);
    }
    closedir(directory); //must be closed at end
  }
  /* see man page for opendir() and readdir() and print out filenames for
  the directory passed */
} /* list() */

//-----PARSE FUNCTION-----//
int parse(char *command, char **arguments){ //breaks up first input as command then the rest as arguments
  for(int i = 0; i < MAX_CANON; i++){
    arguments[i] = strsep(&command, " "); //places the command into an array of arguments, everytime a space comes up adds next word (argument) to array
    if(arguments[i] == NULL){
      return i;
      break;
    }
  }
}

//-----ENABLES FREEING OF PATHLIST FOR PRINTENV AND SETENV FUNCTIONS-----//
void mysh_free(struct pathelement *head){
  struct pathelement *curr = head;
  struct pathelement *temp;
  while(curr->next != NULL){ //loops through each pathelement in the list
    temp = curr;
    curr = curr->next;
    free(temp); //removes each node as it goes
  }
  free(curr); //removes final node
}

//-----WATCHUSER function has been adapted from the code given to us
static void *watchUser(void *arbitrary){
  struct utmpx *up;
  ListOfUsers_t *temp = userHead;
  while(1){ //while running
    setutxent();
    while(up = getutxent()){
      if(up->ut_type == USER_PROCESS){
        pthread_mutex_lock(&userLock); //lock thread
        while(temp != NULL){
          if(strcmp(temp->name, up->ut_user) == 0){
            printf("%s had logged on %s from %s\n", up->ut_user, up->ut_line, up->ut_host);
          }
          temp = temp->next;
        }
        pthread_mutex_unlock(&userLock); //unlock thread
      }
      temp = userHead;
    }
    sleep(20); //sleep for 20
  }
}

static void *watchMail(void *arbitrary){
  const char *fileName = arbitrary; //filename is the parameter
  off_t prevBufferSize;
  struct stat statBuffer;
  struct timeval time_value;
  if(stat(fileName, &statBuffer) < 0){
    perror("Stat Error");
    pthread_exit((void *) EXIT_FAILURE);
  }
  prevBufferSize = statBuffer.st_size;
  while(1){ //while running
    gettimeofday(&time_value, NULL);
    if(stat(fileName, &statBuffer) < 0){
      perror("Stat Error");
      pthread_exit((void *) EXIT_FAILURE);
    }
    if(prevBufferSize < statBuffer.st_size){ //if the size has increased
      printf("\n\aBEEP You've Got Mail in %s at %s\n", fileName, ctime(&(time_value.tv_sec))); //new mail in file
    }
    prevBufferSize = statBuffer.st_size;
    sleep(1); //sleep for 1
  }
}

//------ADD USER FUNCTION --- Adds a user to the watch lists-----//
void addUser(char *user){
  ListOfUsers_t *newUser = malloc(sizeof(struct ListOfUsers)); //malloc space for the new user
  ListOfUsers_t *temp;
  newUser->name = malloc(strlen(user) + 1); //malloc space for user's name
  strcpy(newUser->name, user); //copies user into the new user's name field
  newUser->next = NULL;
  newUser->prev = NULL;
  if(userHead == NULL){ //first user in the list
    userHead = newUser;
    userHead->prev = NULL;
    userHead->next = NULL;
  }
  else{
    temp = userHead;
    while(temp->next != NULL){ //loops through list to add new users at the end
      temp = temp->next;
    }
    temp->next = newUser;
    newUser->prev = temp;
  }
}

void removeUser(char *user){
  ListOfUsers_t *temp;
  if(userHead == NULL){
    printf("There's no user to remove\n");
  }
  else{
    temp = userHead; //starts at head
    while(temp != NULL){ //loops through list
      if(strcmp(temp->name, user) == 0){ //when the name is found in the list
        if(temp == userHead){ //if it's the head
          userHead = userHead->next;
          free(temp->name);
          free(temp);
        }
        else if(temp->next != NULL && temp->prev != NULL){ //if it's in the middle
          temp->next->prev = temp->prev;
          temp->prev->next = temp->next;
          free(temp->name);
          free(temp);
        }
        else if(temp->prev != NULL && temp->next == NULL){ //if it's at the end
          temp->prev->next = NULL;
          free(temp->name);
          free(temp);
        }
      }
      else{ //iterate through list
        temp = temp->next;
      }
    }
  }
}

//------ADD MAIL FUNCTION --- Adds a file to the watch lists-----//
ListOfMail_t *addMail(char *mail){
  ListOfMail_t *newMail = malloc(sizeof(struct ListOfMail)); //malloc space for the new file
  ListOfMail_t *temp;
  newMail->file = malloc(strlen(mail) + 1); //malloc space for file's name
  strcpy(newMail->file, mail); //copies mail into new file's mail field
  newMail->next = NULL;
  newMail->prev = NULL;
  if(mailHead == NULL){ //first file in the list
    mailHead = newMail;
    mailHead->prev = NULL;
    mailHead->next = NULL;
  }
  else{
    temp = mailHead;
    while(temp->next != NULL){ //loops through list to add new files at the end
      temp = temp->next;
    }
    temp->next = newMail;
    newMail->prev = temp;
  }
  return newMail;
}

void removeMail(char *mail){
  ListOfMail_t *temp;
  if(mailHead == NULL){
    printf("There's no file to remove\n");
  }
  else{
    temp = mailHead; //starts at head
    while(temp != NULL){ //loops through list
      if(strcmp(temp->file, mail) == 0){ //when the file is found in the list
        if(temp == mailHead){ //if it's the head
          mailHead = mailHead->next;
          free(temp->file);
          free(temp);
        }
        else if(temp->next != NULL && temp->prev != NULL){ //if it's in the middle
          temp->next->prev = temp->prev;
          temp->prev->next = temp->next;
          free(temp->file);
          free(temp);
        }
        else if(temp->prev != NULL && temp->next == NULL){ //if it's at the end
          temp->prev->next = NULL;
          free(temp->file);
          free(temp);
        }
      }
      else{
        temp = temp->next; //iterate through list
      }
    }
  }
}

char *redirectionCheck(char **args, int arguments){
  char *currArg = args[0];
  for(int i = 0; i < arguments; i++){ //checks all args
    if ((strcmp(currArg, ">") == 0) || (strcmp(currArg, ">&") == 0) || (strcmp(currArg, ">>") == 0) || (strcmp(currArg, ">>&") == 0) || (strcmp(currArg, "<") == 0)){
      return currArg; //returns argument when the redirection char is found
    }
    currArg = args[i];
  }
  return NULL; //returns null if char isn't found in the whole list
}


//---PIPING FUNCTIONS BELOW---------

void pipeFunc(char **args, int arguments){
  char **src = calloc(MAXARGS, sizeof(char *));
  char **dst = calloc(MAXARGS, sizeof(char *));
  char *isPipe = calloc(MAX_CANON, sizeof(char *));
  int i, pid, status, fid[2];
  int pipeCheck = 0;
  int j = 0;
  for(i = 0; i < arguments; i++){  //checks all arguments
    if ((strcmp(args[i], "|") == 0) || (strcmp(args[i], "|&") == 0)){
      isPipe = args[i]; //isPipe becomes the pipe character
      break;
    }
    src[i] = args[i];
  }
  while(args[i] != NULL){ //iterates to the end of the list sarting at i, need j to start at 0
    dst[j] = args[i];
    i++;
    j++;
  }
  pipe(fid);
  source(fid, src, isPipe);
  //dest(fid, dst, isPipe);         //infinite loop
  close(fid[0]);
  close(fid[1]);
}

void source(int pfd[], char **cmd, char *symbol){
  int pid = fork();
  if(pid == 0){ //child
    if(strcmp(symbol, "|&") == 0){
      close(2);
    }
    close(1);
    dup(pfd[1]);
    close(pfd[0]);
    execvp(cmd[0], cmd);
    perror(cmd[0]);
    kill(pid, SIGTERM);
  }
  else if(pid == -1){
    perror("Failed to create child process\n");
    exit(EXIT_FAILURE);
  }
}

void dest(int pfd[], char **cmd, char *symbol){
  int pid = fork();
  if(pid == 0){ //child
    if(strcmp(symbol, "|&") == 0){
      close(2);
    }
    close(0);
    dup(pfd[0]);
    close(pfd[1]);
    execvp(cmd[0], cmd);
    perror(cmd[0]);
    kill(pid, SIGTERM);
  }
  else if(pid == -1){
    perror("Failed to create child process");
    exit(EXIT_FAILURE);
  }
}

char *pipeCheck(char **args, int arguments){
  char *currArg = args[0];
  char *finalArg = args[0];
  for(int i = 0; i < arguments; i++){ //checks all args
    if ((strcmp(currArg, "|") == 0) || (strcmp(currArg, "|&") == 0)){
      finalArg = args[i - 1];
      return finalArg; //returns argument when the pipe char is found
    }
    currArg = args[i];
  }
  return NULL; //returns null if char isn't found in the whole list
}
