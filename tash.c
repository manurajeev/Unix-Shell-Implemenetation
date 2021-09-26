#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>

char error_message[30] = "An error has occurred\n";
int arg_count;
char **path_arr;
int path_count;
bool redirect = false;
char *rdfilename;
bool toexecute = true;

//Function to parse text using delimiter for space,&,>
char **parse_cmd(char *line, char *delimiter)
{
  char **cmd_arr = malloc(sizeof(char *) * (strlen(line) + 1));
  char *token;
  token = strtok(line, delimiter);
  int i = 0;
  while (token != NULL)
  {
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

//Function to execute parsed command
int exec_cmd(char **cmd_arr)
{
  //Built-in Commands: exit,cd and path implementation
  if (strcmp("exit", cmd_arr[0]) == 0)
  {
    if (arg_count != 1)
    {
      write(STDERR_FILENO, error_message, strlen(error_message));
    }
    else
    {
      exit(0);
    }
  }
  else if (strcmp("cd", cmd_arr[0]) == 0)
  {
    if (cmd_arr[2] != NULL)
    {
      write(STDERR_FILENO, error_message, strlen(error_message));
    }
    else
    {
      if (chdir(cmd_arr[1]) == 0)
      {
      }
      else
      {
        write(STDERR_FILENO, error_message, strlen(error_message));
      }
    }
  }
  else if (strcmp("path", cmd_arr[0]) == 0)
  {
    int i;
    path_arr[0] = NULL;
    path_count = 0;
    for (i = 1; i < arg_count; i++)
    {
      path_arr[i - 1] = (char *)malloc((strlen(cmd_arr[i]) + 1) * sizeof(char));
      strcpy(path_arr[i - 1], cmd_arr[i]);
      strcat(path_arr[i - 1], "/");
      path_count++;
    }
  }
  else
  {
    bool check_exec = false;
    char temp_dir[100];
    char temp_cmd[100];

    int rc = fork();
    if (rc == 0)
    {
      //redirect the std o/p and error
      if (redirect)
      {
        int file = open(rdfilename, O_WRONLY | O_CREAT | O_TRUNC, 0777);
        if (file < 0)
        {
          write(STDERR_FILENO, error_message, strlen(error_message));
        }
        dup2(file, STDOUT_FILENO);
        dup2(file, STDERR_FILENO);
        close(file);
      }
      //Checking for the executable in all the paths
      int i;
      for (i = 0; i < path_count; i++)
      {
        strcpy(temp_dir, path_arr[i]);
        strcat(temp_dir, cmd_arr[0]);
        if (access(temp_dir, X_OK) == 0)
        {
          strcpy(temp_cmd, temp_dir);
          check_exec = true;
          break;
        }
      }
      if (check_exec)
      {
        execv(temp_cmd, cmd_arr);
      }
      else
      {
        write(STDERR_FILENO, error_message, strlen(error_message));
      }
    }
    else if (rc > 0)
    {
      wait(NULL);
    }
    else
    {
      write(STDERR_FILENO, error_message, strlen(error_message));
    }
  }
  return 1;
}

//Function to trim leading and trailing spaces
void trim(char *s)
{
  char *p = s;
  int l = strlen(p);
  while (isspace(p[l - 1]))
    p[--l] = 0;
  while (*p && isspace(*p))
    ++p, --l;
  memmove(s, p, l + 1);
}

//Function to check for redirection
char *check_rdir(char *line)
{
  char *linecpy = malloc(sizeof(char *) * (strlen(line) + 1));
  strcpy(linecpy, line);
  if (strchr(linecpy, '>') != NULL)
  {
    redirect = true;
  }
  char *cmd_wtout_rdir = NULL;
  if (redirect)
  {
    char **rdir_cmd = parse_cmd(linecpy, ">");
    int i = 0;
    while (rdir_cmd[i] != NULL)
    {
      i++;
    }
    if (i != 2)
    {
      write(STDERR_FILENO, error_message, strlen(error_message));
      redirect = false;
      toexecute = false;
      rdfilename = NULL;
    }
    cmd_wtout_rdir = rdir_cmd[0];
    bool space = false;
    if(rdir_cmd[1] != NULL)
    {
      rdfilename = rdir_cmd[1];
      trim(rdfilename);
      //check for spaces in middle
      i=0;
      while(rdfilename[i] != '\0') {
        if(isspace(rdfilename[i])) {
          space = true;
        }
        i++;
      }
    }
    if(space){
      write(STDERR_FILENO, error_message, strlen(error_message));
      redirect = false;
      toexecute = false;
    }
    trim(cmd_wtout_rdir);
  }
  return cmd_wtout_rdir;
}

int main(int argc, char *argv[])
{
  char *batchFileName;
  char **cmd_arr;
  char **parallel_cmdarr;
  bool batchMode = false;
  char *buffer;
  size_t buffsize = 100;
  char *line = NULL;
  size_t len = 0;
  ssize_t nread;
  FILE *batchFileStream;
  path_arr = malloc(sizeof(char *) * 8);
  path_arr[0] = "/bin/";
  path_count = 1;
  arg_count = 0;
  buffer = (char *)malloc(buffsize * sizeof(char));
  char *rdirparsedcmd;
  if (buffer == NULL)
  {
    write(STDERR_FILENO, error_message, strlen(error_message));
  }

  //Handle whether to run in Batch Mode or not
  if (argc == 2)
  {
    batchMode = true;
    batchFileName = argv[1];
  }
  else if (argc != 1)
  {
    write(STDERR_FILENO, error_message, strlen(error_message));
    exit(1);
  }
  if (batchMode)
  {
    //run tash in batch mode
    batchFileStream = fopen(batchFileName, "r");
    if (batchFileStream == NULL)
    {
      write(STDERR_FILENO, error_message, strlen(error_message));
      exit(1);
    }
    int p = 0;
    while ((nread = getline(&line, &len, batchFileStream)) != -1)
    {
      p++;
      //If empty line, skip
      trim(line);
      if(strlen(line) == 0) {
       continue;
      }
      arg_count=0;
      parallel_cmdarr = parse_cmd(line, "&");
      //check for empty first command
      if(arg_count ==  0) {
	toexecute=false;
	write(STDERR_FILENO, error_message, strlen(error_message));
	continue;
      }
    
      int i = 0;
      while (parallel_cmdarr[i] != NULL)
      {
        arg_count = 0;
        //check for redirection
        redirect = false;
        rdirparsedcmd = check_rdir(parallel_cmdarr[i]);
        if (rdirparsedcmd != NULL)
        {
          cmd_arr = parse_cmd(rdirparsedcmd, " ");
        }
        else
        {
          cmd_arr = parse_cmd(parallel_cmdarr[i], " ");
        }
        i++;
        if (arg_count > 0 && toexecute)
        {
          exec_cmd(cmd_arr);
        }
      }
    }
    fclose(batchFileStream);
  }
  else
  {
    //run tash in interactive mode
    while (1)
    {
      printf("tash> ");
      getline(&buffer, &buffsize, stdin);    
      parallel_cmdarr = parse_cmd(buffer, "&");
      int i = 0;
      while (parallel_cmdarr[i] != NULL)
      {
        arg_count = 0;
        //check for redirection
        redirect = false;
        rdirparsedcmd = check_rdir(parallel_cmdarr[i]);
        if (rdirparsedcmd != NULL)
        {
          cmd_arr = parse_cmd(rdirparsedcmd, " ");
        }
        else
        {
          cmd_arr = parse_cmd(parallel_cmdarr[i], " ");
        }
        i++;
        if (arg_count > 0 && toexecute)
        {
          exec_cmd(cmd_arr);
        }
      }
    }
  }
  free(line);
  free(buffer);
  return 0;
}
