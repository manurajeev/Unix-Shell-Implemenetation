#include<stdio.h>
#include<stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

char **parseCmd(char *line) {
  char **cmdArr = malloc(sizeof(char *) * 8);
  char *delimiter = " ";
  char *token;
  //get first token
  token = strtok(line, delimiter);
  int i = 0;
  while (token != NULL) {
   //Removing the \n that will be added at the end of the last argument
    token[strcspn(token, "\n")] = 0;

     cmdArr[i] = token;
     i++;
     token = strtok(NULL, delimiter);
    }
    cmdArr[i] = NULL;
    return cmdArr;
  }

int main(int argc, char *argv[]) {
  char *batchFileName;
  char **cmdArr;
  char error_message[30] = "An error has occurred\n";
  bool batchMode = false;
  char *buffer;
size_t buffsize = 32;
size_t characters;
  char *line = NULL;
  size_t len = 0;
  ssize_t nread;
  FILE *batchFileStream;
  buffer = (char *)malloc(buffsize * sizeof(char));
  if(buffer == NULL){
    write(STDERR_FILENO, error_message, strlen(error_message));
  }

  //Handle whether to run in Batch Mode or not
   if(argc==2) {
      //run shell in batch mode
      batchMode = true;
      batchFileName = argv[1];
    }else if(argc != 1) {
      //Throw error
      write(STDERR_FILENO, error_message, strlen(error_message));
      exit(1);
   }

  if(batchMode) {
    batchFileStream = fopen(batchFileName, "r");
    if(batchFileStream == NULL) {
      write(STDERR_FILENO, error_message, strlen(error_message));
      exit(1);
    }
    while((nread = getline(&line, &len, batchFileStream)) != -1) {
      cmdArr = parseCmd(line);
//execute
      printf("Batch file exec to be implemented for %s\n",cmdArr[0]);
    }
    fclose(batchFileStream);
  }else {
    do{
      printf("tash> ");
      characters = getline(&buffer, &buffsize, stdin);

      cmdArr = parseCmd(buffer);
      printf("Execute the command %s\n",cmdArr[0]);
   }while(strcmp("exit", cmdArr[0]));
  }
  free(line);
  free(buffer);
  return 0;
 }
