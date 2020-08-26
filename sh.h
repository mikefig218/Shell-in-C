#include "get_path.h"

int pid;
int sh( int argc, char **argv, char **envp);
char *which(char *command, struct pathelement *pathlist);
char *where(char *command, struct pathelement *pathlist);
void list ( char *dir );
void printenv(char **envp);

#define PROMPTMAX 32
#define MAXARGS 10

typedef struct ListOfUsers{
  char *name;
  struct ListOfUsers *next;
  struct ListOfUsers *prev;
} ListOfUsers_t;

typedef struct ListOfMail{
  char *file;
  pthread_t file_thread;
  struct ListOfMail *next;
  struct ListOfMail *prev;
} ListOfMail_t;
