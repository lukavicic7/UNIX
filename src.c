#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
/*
  Function Declarations for builtin shell commands:
 */
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);
int lsh_clear();
int lsh_mkdir(char **args);
int lsh_rmdir(char **args);
int lsh_rm(char **args);
int lsh_touch(char **args);
int lsh_pwd();
int lsh_echo(char **args);
int lsh_cat(char **args);


/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] = {
  "cd",
  "help",
  "exit",
  "clear",
  "mkdir",
  "rmdir",
  "rm",
  "touch",
  "pwd",
  "echo",
  "cat",

};

int (*builtin_func[]) (char **) = {
  &lsh_cd,
  &lsh_help,
  &lsh_exit,
  &lsh_clear,
  &lsh_mkdir,
  &lsh_rmdir,
  &lsh_rm,
  &lsh_touch,
  &lsh_pwd,
  &lsh_echo,
  &lsh_cat
};

int lsh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

/*
  Builtin function implementations.
*/
int lsh_cat(char **args){
    if(args[1] == NULL){
        fprintf(stderr, "lsh_cat: expected argument to cat\n");
    }
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    fp = fopen(args[1], "r");
    if (fp == NULL){
        printf("File doesn't exist\n");
        return 1;
    }
    else{
      while ((read = getline(&line, &len, fp)) != -1) {
        printf("%s", line);
      }
    }
    fclose(fp);
    return 1;

}
int lsh_echo(char **args){
    int i=1;
    while (args[i] != NULL){
        printf("%s ", args[i]);
        i++;    
    }
    printf("\n");  
    return 1;
}

int lsh_pwd() 
{ 
    char cwd[1024]; 
    getcwd(cwd, sizeof(cwd)); 
    printf("%s \n", cwd); 
} 
  
int lsh_touch(char **args){
    if(args[1] == NULL){
        fprintf(stderr, "lsh_touch: expected argument to touch\n");
    }
    else{
        if(creat(args[1], 0755) != 0){
            perror("lsh");
        }
    }
    return 1; 
}

int lsh_rm(char **args){
    if(args[1] == NULL){
        fprintf(stderr, "lsh_rm: expected argument to rm\n");
    }
    if(remove(args[1]) == 0){
        printf("File deleted successfully\n");
    }
    else{
        printf("Error: unable to delete\n");
    }
    return 1;
}
int lsh_rmdir(char **args){
    if(args[1] == NULL){
        fprintf(stderr, "lsh_rmdir: expected argument to rmdir\n");
    }
    if(rmdir(args[1]) == 0){
        printf("Directory deleted successfully\n");
    }
    else{
        printf("Error: unable to delete\n");
    }
    return 1;
}

int lsh_mkdir(char **args){
    if(args[1] == NULL){
        fprintf(stderr, "lsh_mkdir: expected argument to mkdir \n");   
    }
    else{
        if (mkdir(args[1], 0755) != 0){
            perror("lsh");
        }
    }
    return 1;        
}

int lsh_clear(){
    system("clear");
    return 1;
}

int lsh_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "lsh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("lsh");
    }
  }
  return 1;
}

int lsh_history(){
  char str[30];
  FILE *file;
  file = fopen("history.txt" , "r");
  if (file) {
      while (fscanf(file, "%s", str)!=EOF)
          printf("%s",str);
    }
  fclose(file);
  return 1;
}

int lsh_help(char **args)
{
  int i;
  printf("Stephen Brennan's LSH\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < lsh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

int lsh_exit(char **args)
{
  return 0;
}

char *lsh_read_line(void)
{
  char *line = NULL;
  ssize_t bufsize = 0; // have getline allocate a buffer for us
  getline(&line, &bufsize, stdin);
  return line;
}




#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
char **lsh_split_line(char *line)
{
  int bufsize = LSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token;

  if (!tokens) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, LSH_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += LSH_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, LSH_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}


int lsh_launch(char **args)
{
  pid_t pid, wpid;
  int status;

  pid = fork();
  if (pid == 0) {
    // Child process
    if (execvp(args[0], args) == -1) {
      perror("lsh");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // Error forking
    perror("lsh");
  } else {
    // Parent process
    do {
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

int lsh_execute(char **args)
{
  int i;

  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < lsh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return lsh_launch(args);
}

void lsh_loop()
{

  char *line;
  char **args;
  int status;
  FILE *fp;

  fp = fopen("history.txt", "w+");
  do {
    printf("> ");
    line = lsh_read_line();
    if (line != NULL){
      fputs(line,fp);
    }
    //lsh_history();
    args = lsh_split_line(line);
    status = lsh_execute(args);

    free(line);
    free(args);
    
  } while (status);
   fclose(fp);
}

int main(int argc, char **argv)
{

  // Load config files, if any.

  // Run command loop.
  lsh_loop();

  // Perform any shutdown/cleanup.

  return EXIT_SUCCESS;
}
