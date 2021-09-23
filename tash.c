#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

char error_message[30] = "An error has occurred\n";
int arg_count;
char **path_arr;
int path_count;

char **parse_cmd(char *line,char *delimiter) {
  char **cmd_arr = malloc(sizeof(char *) * (strlen(line)+1));
  //char *delimiter = " ";
  char *token;
  //get first token
  token = strtok(line, delimiter);
  int i = 0;
  while (token != NULL) {
   //Removing the \n that will be added at the end of the last argument
    token[strcspn(token, "\n")] = 0;	                       
    cmd_arr[i] = token;
    arg_count++;
    i++;
    token = strtok(NULL, delimiter);
  }
    cmd_arr[i] = NULL;
    return cmd_arr;
}

int exec_cmd(char **cmd_arr) {
  //check for redirection
  int x;
  bool redirect = false;
  for(x = 0;x < arg_count; x++) {
    if(strcmp(cmd_arr[x], ">") == 0 && x == arg_count - 2) {
      redirect = true;
    }
  }
 
  //Built-in Commands: exit,cd and path implementation
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
    path_arr[0] = NULL;
    path_count = 0;
    for(i=1;i<arg_count;i++) {
      path_arr[i-1] = (char*)malloc((strlen(cmd_arr[i]) + 1) * sizeof(char));
      strcpy(path_arr[i-1], cmd_arr[i]);
      strcat(path_arr[i-1], "/");
      path_count++;
    }
  }else{
    bool check_exec = false;
    char temp_dir[100];
    char temp_cmd[100];

    int rc = fork();
    if(rc == 0) {
      //redirect the std o/p and error
      if(redirect) {
          char *rdfilename = cmd_arr[arg_count - 1];
	  int file = open(rdfilename, O_WRONLY | O_CREAT | O_TRUNC, 0777);
	  if(file<0) {
	    write(STDERR_FILENO, error_message, strlen(error_message));
	  }
	  dup2(file, STDOUT_FILENO);
	  dup2(file, STDERR_FILENO);
	  close(file);
	  //Remove the last two args that were used for redirection
	  cmd_arr[arg_count-1] = NULL;
	  cmd_arr[arg_count-2] = NULL;
       }
      //Checking for the executable in all the paths
      int i;
      for(i=0;i<path_count;i++) {
        strcpy(temp_dir, path_arr[i]);
	strcat(temp_dir, cmd_arr[0]);
	if(access(temp_dir,X_OK)==0) {
	  strcpy(temp_cmd, temp_dir);
	  check_exec = true;
	  break;
	}
      }
      if(check_exec) {
        execv(temp_cmd,cmd_arr);
      }else{
        write(STDERR_FILENO, error_message, strlen(error_message));
      }
    }else if(rc>0){
      //parent
      //wait(NULL);
      int status; 
      do {
        waitpid(rc, &status, WUNTRACED);
     }while(!WIFEXITED(status) && !WIFSIGNALED(status));
   }else {
      write(STDERR_FILENO, error_message, strlen(error_message));
    }
  }
  return 1;
}

int main(int argc, char *argv[]) {
  char *batchFileName;
  char **cmd_arr;
  char **parallel_cmdarr;
  bool batchMode = false;
  char *buffer;
  size_t buffsize = 100;
  //size_t characters;
  char *line = NULL;
  size_t len = 0;
  ssize_t nread;
  FILE *batchFileStream;
  path_arr = malloc(sizeof(char *) * 8);
  path_arr[0] = "/bin/";
  path_count = 1;
  arg_count = 0;
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
    int p=0;
    while((nread = getline(&line, &len, batchFileStream)) != -1) {
     p++;
     parallel_cmdarr = parse_cmd(line,"&");
      int i = 0;
      while(parallel_cmdarr[i] != NULL) {
	arg_count = 0;
	cmd_arr = parse_cmd(parallel_cmdarr[i]," ");
	i++;
	if(arg_count > 0){
	  exec_cmd(cmd_arr);
	}
      }
    }
    fclose(batchFileStream);
  }else {
    while(1){
      printf("tash> ");
      getline(&buffer, &buffsize, stdin);
      
      parallel_cmdarr = parse_cmd(buffer,"&");
      int i = 0;
      while(parallel_cmdarr[i] != NULL) {
        arg_count = 0;
        cmd_arr = parse_cmd(parallel_cmdarr[i]," ");
	i++;
	if(arg_count > 0) {
	  exec_cmd(cmd_arr);
	}
      }
    }
  }  
  free(line);
  free(buffer);
  return 0;
 }


