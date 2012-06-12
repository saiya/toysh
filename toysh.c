#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <readline/history.h>
#include <readline/readline.h>
#include "toysh.h"
#include "parser.h"
#include "dictionary.h"


void commandLineHandle_wait(commandLineHandle* this){
  for(size_t i = 0; i < this->procs; i++){
    waitpid(*(this->pids + i), (this->codes + i), 0);
  }
}

typedef struct{
  pid_t pid;

  int stdout;
} procIO;
procIO procIO_empty(){
  procIO p;
  p.pid = -1;
  p.stdout = -1;
  return p;
}
procIO procIO_fail(){ return procIO_empty(); } 
void procIO_closeAll(procIO p){
  if(p.stdout != -1){
    close(p.stdout);
  }
}
procIO toysh_start_exec(command* cmd, procIO former){
  procIO result = procIO_empty();
  int redirectionMap[100];

  command* pipe_cmd = NULL;
  for(command* succ = cmd->next; succ; succ = succ->next){
    int endOfSucc = 0;
    switch(succ->type){
    case C_PIPE: pipe_cmd = succ; break;
    
    case C_REDIRECT_FD_TO_FD:
    case C_REDIRECT_FD_TO_FILE:
    case C_REDIRECT_FILE_TO_FD:
      break;

    default: endOfSucc = 1; break;
    }
    if(endOfSucc) break;
  }

  int pipe_fds[2];  // [reader, writer]
  if(pipe_cmd){
    if(pipe(pipe_fds) == -1){
      puts("pipe() failed.");
      return procIO_fail();
    }
    result.stdout = pipe_fds[0];
  }
  
  result.pid = fork();
  switch(result.pid){
  case -1:  // fork failure!
    puts("fork() failed.");
    return procIO_fail();

  case 0:   // Child
    // Build redirection map of outputs.
    for(int fd = 0; fd < 100; fd++){ redirectionMap[fd] = -1; }
    if(former.stdout != -1){ // Initial valud of stdin sould be input pipe.
      redirectionMap[STDIN_FILENO] = former.stdout;
    }
    if(pipe_cmd){  // Initial value of stdout should be output pipe.
      if(pipe_cmd->fd_lhs1 != -1) redirectionMap[pipe_cmd->fd_lhs1] = pipe_fds[1];
      if(pipe_cmd->fd_lhs2 != -1) redirectionMap[pipe_cmd->fd_lhs2] = pipe_fds[1];
    }

    for(command* succ = cmd->next; succ; succ = succ->next){
      int endOfSucc = 0;
      switch(succ->type){
      case C_REDIRECT_FD_TO_FD:
	if(succ->fd_lhs1 != -1) redirectionMap[succ->fd_rhs] = redirectionMap[succ->fd_lhs1];
	if(succ->fd_lhs2 != -1) redirectionMap[succ->fd_rhs] = redirectionMap[succ->fd_lhs2];
	break;

      case C_REDIRECT_FD_TO_FILE: {
	FILE* file = fopen(succ->file, "w");
	if(succ->fd_lhs1 != -1) redirectionMap[succ->fd_lhs1] = fileno(file);
	if(succ->fd_lhs2 != -1) redirectionMap[succ->fd_lhs2] = fileno(file);
	break;
      }
      case C_REDIRECT_FILE_TO_FD: {
	FILE* file = fopen(succ->file, "r");
	if(succ->fd_rhs != -1) redirectionMap[succ->fd_rhs] = fileno(file);
	break;
      }
      
      default: endOfSucc = 1; break;
      }
      if(endOfSucc) break;
    }

    // Apply redirectionMap
    for(int fd = 0; fd < 100; fd++){
      if(redirectionMap[fd] != -1){
	// printf("%s: dup2(%d, %d); // former.stdout: %d \n", cmd->file, redirectionMap[fd], fd, former.stdout);
	dup2(redirectionMap[fd], fd);
      }
    }
    if(former.stdout != -1){ close(former.stdout); }
    if(pipe_cmd){ close(pipe_fds[0]); close(pipe_fds[1]); }

    execvp(cmd->file, cmd->argv);

    // Error
    printf("`%s` exec error (%d): %s\n", cmd->file, errno, strerror(errno));
    exit(EXIT_FAILURE);

  default:  // Parent
    if(pipe_cmd){ close(pipe_fds[1]); }
    return result;
  }  
}
commandLineHandle* toysh_start(const commandLine* cl, pool* p){
  commandLineHandle* h = p->alloc(p, O_Generic, sizeof(commandLine));
  h->procs = 0;
  h->pids = NULL;
  h->codes = NULL;
  h->wait = commandLineHandle_wait;
  
  for(command* cmd = cl->head; cmd; cmd = cmd->next){
    switch(cmd->type){
    case C_EXEC:
      h->procs++;
      break;
    default: continue;
    }
  }
  h->pids = p->alloc(p, O_Generic, h->procs * sizeof(pid_t));
  h->codes = p->alloc(p, O_Generic, h->procs * sizeof(int));

  pid_t* p_pid = h->pids;
  procIO proc = procIO_empty();
  for(command* cmd = cl->head; cmd; cmd = cmd->next){
    switch(cmd->type){
    case C_INVALID: continue;

    case C_PIPE:
    case C_REDIRECT_FD_TO_FD:
    case C_REDIRECT_FD_TO_FILE:
    case C_REDIRECT_FILE_TO_FD:
      continue;
      
    case C_EXEC:
      {
	procIO former = proc;
	proc = toysh_start_exec(cmd, proc);
	*p_pid = proc.pid;
	p_pid++;
	procIO_closeAll(former);
	break;
      }
    }
  }
  procIO_closeAll(proc);

  return h;
}

void toysh(){
  pool* p = pool_new();

  char *prompt = "\e[1;34m""toysh % ""\e[0m"; // getenv("PS2");
  char *line = NULL;
  while((line = readline(prompt))){
    commandLine* cl = parse(line, p);
    free(line);

    if(! cl) continue;
    
    add_history(cl->src);
    commandLineHandle* h = toysh_start(cl, p);
    h->wait(h);
  }

  p->free(p);
}
int main(){
  toysh();
}
