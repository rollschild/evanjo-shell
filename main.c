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
    // exec() under normal conditions does not return
    if (execvp(args[0], args) == -1) {
      perror("esh");
    }
    exit(EXIT_FAILURE);
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

  do {
    printf("esh=$ ");
    line = EshReadLine();
    args = EshParseLine(line);
    status = EshExecute(args);

    free(line);
    free(args);
  } while (status);

  return;
}

int main(int argc, char** argv) {
  printf("Welcome to the Evanjo shell!\n");
  // Run command loop
  EshLoop();

  return EXIT_SUCCESS;
}
