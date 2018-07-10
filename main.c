#include <setjmp.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define ESH_READ_LINE_BUFFERSIZE 1024
#define ESH_TOKENS_BUFFERSIZE 64
#define ESH_TOKEN_DELIM " \t\n\r\a"

typedef void (*sig_handler_ptr)(int);
// static jmp_buf env;
static sigjmp_buf env;
static volatile sig_atomic_t jump_active = 0;

sig_handler_ptr signal(int sig_num, sig_handler_ptr handler);

void SigintHandler(int sig_num) {
  // printf("Caught SIGINT!\n");
  // exit(1);
  // longjmp(env, 42);
  if (!jump_active) {
    return;
  }
  siglongjmp(env, 42);
}

int EshCd(char** args);
int EshHelp(char** args);
int EshExit(char** args);
int EshPwd(char** args);

char* builtin_str[] = {"cd", "help", "exit", "pwd"};

// function pointer array
int (*builtin_func_ptr[])(char**) = {&EshCd, &EshHelp, &EshExit, &EshPwd};

int NumOfBuiltinFunc() { return sizeof(builtin_str) / sizeof(char*); }

int EshHelp(char** args) {
  printf("Evanjo SHell.\n");
  printf("The followings are built-in.\n");

  for (int i = 0; i < NumOfBuiltinFunc(); ++i) {
    printf("  %s\n", builtin_str[i]);
  }

  return 1;
}

int EshCd(char** args) {
  if (args[1] == NULL) {
    fprintf(stderr, "esh: expected argument to $cd$.\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("esh: error changing directory!\n");
    }
  }

  return 1;
}

// get current working directory
int EshPwd(char** args) {
  char cwd[1024];

  if (args[0] == NULL) {
    fprintf(stderr, "esh: at least one command is required!\n");
  } else {
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
      fprintf(stdout, "Current working directory is: %s.\n", cwd);
    } else {
      perror("esh: getcwd() error!\n");
    }
  }

  return 1;
}

int EshExit(char** args) { return 0; }

int EshLaunch(char** args) {
  pid_t pid;
  pid_t wpid;
  int status = 0;

  pid = fork();

  if (pid == 0) {
    signal(SIGINT, SIG_DFL);  // restore SIGINT default behavior
    // ...inside child process

    // exec() under normal conditions does not return
    if (execvp(args[0], args) == -1) {
      perror("esh");
    }
    exit(EXIT_FAILURE);  // this only terminates the child process
  } else if (pid < 0) {
    perror("esh");
  } else {
    // Parent process
    do {
      // This will modify status
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  // returning 1 to calling function,
  // ...signalling that we should prompt for input again
  return 1;
}

int EshExecute(char** args) {
  if (args[0] == NULL) {
    return 1;
  }

  for (int i = 0; i < NumOfBuiltinFunc(); ++i) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func_ptr[i])(args);
    }
  }

  return EshLaunch(args);
}

char** EshParseLine(char* line) {
  int buffersize = ESH_TOKENS_BUFFERSIZE;
  int position = 0;
  char** tokens = malloc(buffersize * sizeof(char*));
  char* token;

  if (!tokens) {
    fprintf(stderr, "esh: allocation error!\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, ESH_TOKEN_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    ++position;

    if (position >= buffersize) {
      buffersize += ESH_TOKENS_BUFFERSIZE;
      tokens = realloc(tokens, buffersize * sizeof(char*));

      if (!tokens) {
        fprintf(stderr, "esh: allocation error!\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, ESH_TOKEN_DELIM);
  }

  tokens[position] = NULL;
  return tokens;
}

char* EshReadLine(void) {
  int buffersize = ESH_READ_LINE_BUFFERSIZE;
  char* buffer = malloc(buffersize * sizeof(char));
  int position = 0;
  int c;  // Notice this is int, NOT char

  if (!buffer) {
    fprintf(stderr, "esh: allocation error!\n");
    exit(EXIT_FAILURE);
  }

  while (true) {
    c = getchar();

    if (c == EOF || c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
      // ++position;
    }

    ++position;

    if (position >= buffersize) {
      buffersize += ESH_READ_LINE_BUFFERSIZE;
      buffer = realloc(buffer, buffersize * sizeof(char));

      if (!buffer) {
        fprintf(stderr, "esh: allocation error!|n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

// Alternative way to EshReadLine()
/*
char* EshReadLine(void) {
  char* line = NULL;
  ssize_t buffersize = 0;
  getline(&line, &buffersize, stdin);
  return line;
}
*/
// Read
// Parse
// Execute
void EshLoop(void) {
  char* line;
  char** args;
  int status;

  // sleep(10);  // TEST
  /*
  ### NOTE: if this funcion is enabled,
  ### before the jump point is set up,
  ### if the SIGINT arrives, program will exit.
  */

  // signal(SIGINT, SigintHandler);
  // signal(SIGINT, SIG_IGN);  // ignore Ctrl-C

  // Use sigaction() to realize signal handling
  struct sigaction SigAction;
  SigAction.sa_handler = SigintHandler;
  sigemptyset(&SigAction.sa_mask);
  SigAction.sa_flags = SA_RESTART;
  sigaction(SIGINT, &SigAction, NULL);

  do {
    // sleep(10);
    // right here is the correct place to test sigjmp flag

    if (sigsetjmp(env, 1) == 42) {
      // right here it blocks the following SITINT invocations
      printf("SIGINT caught. Restarting...\n");
      sleep(1);
    }
    jump_active = 1;
    printf("esh=$ ");
    line = EshReadLine();
    args = EshParseLine(line);
    status = EshExecute(args);

    free(line);
    free(args);

    // sleep(1);
  } while (status);

  return;
}

int main(int argc, char** argv) {
  printf("Welcome to the Evanjo shell!\n");
  // Run command loop
  EshLoop();

  return EXIT_SUCCESS;
}
