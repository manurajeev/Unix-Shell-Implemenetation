#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

char error_message[30] = "An error has occurred\n";
int arg_count;
char **path_arr;
int path_count;

char **parse_cmd(char *line) {
  char **cmd_arr = malloc(sizeof(char *) * 8);
  char *delimiter = " ";
  char *token;
  //get first token
  token = strtok(line, delimiter);
  int i = 0;
  while (token != NULL) {
   //Removing the \n that will be added at the end of the last argument
    token[strcspn(token, "\n")] = 0;
    arg_count++;
    cmd_arr[i] = token;
    i++;
    token = strtok(NULL, delimiter);
  }
    cmd_arr[i] = NULL;
    return cmd_arr;
}

int exec_cmd(char **cmd_arr) {
  if(strcmp("exit",cmd_arr[0]) == 0){
    if(arg_count != 1) {
      write(STDERR_FILENO, error_message, strlen(error_message));
    }else{
      exit(0);
    }
  } else if(strcmp("cd",cmd_arr[0]) == 0) {
    if(cmd_arr[2] != NULL) {
      write(STDERR_FILENO, error_message, strlen(error_message));
    }else {
     // if(cmd_arr[1] != NULL)
      if(chdir(cmd_arr[1]) == 0) {

       } else{
       write(STDERR_FILENO, error_message, strlen(error_message));
     }
   }
 }else if(strcmp("path",cmd_arr[0]) == 0){
    int i;
    for(i=1;i<arg_count;i++) {
      strcpy(path_arr[i], cmd_arr[i]);
      strcat(path_arr[i], "/");
      path_count++;
    }
  }else{
    bool check_exec = false;
    char temp_dir[100];
    //Handle Redirection later

    int rc = fork();
    if(rc == 0) {
      //Checking for the executable in all the paths
      int i;
      for(i=0;i<path_count;i++) {
        strcpy(temp_dir, path_arr[i]);
        strcat(temp_dir, cmd_arr[0]);
        if(access(temp_dir,X_OK)==0) {
          strcpy(cmd_arr[0], temp_dir);
          check_exec = true;
          break;
        }
      }
      if(check_exec) {
        execv(cmd_arr[0],cmd_arr);
      }else{
        write(STDERR_FILENO, error_message, strlen(error_message));
      }
    }
  }
  return 0;
}

int main(int argc, char *argv[]) {
  char *batchFileName;
  char **cmd_arr;
  bool batchMode = false;
  char *buffer;
  size_t buffsize = 32;
  //size_t characters;
  char *line = NULL;
  size_t len = 0;
  ssize_t nread;
  FILE *batchFileStream;
  path_arr = malloc(sizeof(char *) * 8);
  path_arr[0] = "/bin/";
  path_count = 1;
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
  arg_count = 0;
  if(batchMode) {
    batchFileStream = fopen(batchFileName, "r");
    if(batchFileStream == NULL) {
      write(STDERR_FILENO, error_message, strlen(error_message));
      exit(1);
    }
    while((nread = getline(&line, &len, batchFileStream)) != -1) {
      cmd_arr = parse_cmd(line);
      //execute
      exec_cmd(cmd_arr);
      printf("Batch file exec for %s\n",cmd_arr[0]);
    }
    fclose(batchFileStream);
  }else {
    while(1){
      arg_count = 0;
      printf("tash> ");
     //characters = getline(&buffer, &buffsize, stdin);
      getline(&buffer, &buffsize, stdin);

      cmd_arr = parse_cmd(buffer);
      exec_cmd(cmd_arr);
    }
    //while(strcmp("exit", cmd_arr[0]));
  }
  free(line);
  free(buffer);
  return 0;
 }
