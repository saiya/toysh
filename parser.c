#include <ctype.h>
#include <string.h>
#include <assert.h>
#include "parser.h"
#include "parser_allocator.h"

typedef enum{
  E_NONE,

  E_TOO_MANY_COMMANDS,
  E_TOO_MANY_ARGS,
  E_NO_REDIRECT_TARGET,
  E_UNCLOSED_SINGLE_QUOT,
  E_UNCLOSED_DOUBLE_QUOT,
} parseError;

typedef struct{
  parseError err;

  pool* pool;
} state;
#define ALLOC_ANY(state, size) (state->pool->alloc(state->pool, O_Generic, size))
#define ALLOC_CMD(state, size) (state->pool->alloc(state->pool, O_Command, size))
#define ALLOC_STR(state, size) (state->pool->alloc(state->pool, O_String, size))

command* parse_command(const char* cursor, state* state);
command* parse_command_pipe(const char* cursor, state* state);
command* parse_command_redirect(const char* cursor, state* state);
command* parse_command_exec(const char* cursor, state* state);
char* parse_strunit(const char** cursor, state* state);
int isspecial(char c);
char unsecape(char c);
int skip_spaces(const char** cursor);
int read_num(const char** cursor, int* num);

commandLine* parse(const char* src, pool* pool){
  state s_;
  state* s = &s_;
  s->pool = pool;
  s->err = E_NONE;

  commandLine* result = ALLOC_ANY(s, sizeof(commandLine));
  result->src = ALLOC_STR(s, strlen(src) + 1);
  strncpy(result->src, src, strlen(src) + 1);
  result->head = NULL;
  result->last = NULL;
  
  command* cmd;
  const char* cursor = src;
  while((cmd = parse_command(cursor, s))){
    cmd->next = cmd->prev = NULL;
    
    if(result->head == NULL){
      result->head = cmd;
      result->last = cmd;
    }else{
      result->last->next = cmd;
      cmd->prev = result->last;
      result->last = cmd;
    }
    cursor = cmd->src_next;
  }

  if(result->head == NULL) return NULL;
  return result;
}

command* parse_command(const char* cursor, state* state){
  command* cmd;
  
  if((cmd = parse_command_pipe(cursor, state))) return cmd;
  if((cmd = parse_command_redirect(cursor, state))) return cmd;
  if((cmd = parse_command_exec(cursor, state))) return cmd;
  
  return NULL;
}
command* parse_command_pipe(const char* cursor, state* state){
  if(! skip_spaces(&cursor)) return NULL;
  
  int fd_lhs1 = FD_NONE;
  int fd_lhs2 = FD_NONE;
  if(*cursor != '|') return NULL; // This is not pipe.
  fd_lhs1 = FD_STDOUT;
  const char* src_start = cursor;
  cursor++;

  if(*cursor == '&'){
    fd_lhs2 = FD_STDERR;
    cursor++;
  }
  
  command* result = ALLOC_CMD(state, sizeof(command));
  result->src_start = src_start;
  result->src_next = cursor;
  result->type = C_PIPE;
  result->fd_lhs1 = fd_lhs1;
  result->fd_lhs2 = fd_lhs2;
  result->fd_rhs = FD_STDIN;
  return result;
}
command* parse_command_redirect(const char* cursor, state* state){
  if(! skip_spaces(&cursor)) return NULL;
  
  int fd_lhs1 = FD_NONE;
  int fd_lhs2 = FD_NONE;
  if(! read_num(&cursor, &fd_lhs1)) fd_lhs1 = FD_NONE;
  
  if(*cursor != '>') return NULL;  // This is not redirection.
  if(fd_lhs1 == FD_NONE) fd_lhs1 = FD_STDOUT;
  const char* src_start = cursor;
  cursor++;
  
  if(*cursor == '&'){
    fd_lhs2 = FD_STDERR;
    cursor++;
  }
  
  commandType type;
  int fd_rhs = FD_NONE;
  char* file = NULL;
  if(read_num(&cursor, &fd_rhs)){
    type = C_REDIRECT_FD_TO_FD;
  }else{
    if((file = parse_strunit(&cursor, state))){
      type = C_REDIRECT_FD_TO_FILE;
    }else{
      state->err = E_NO_REDIRECT_TARGET;
      return NULL;
    }
  }
  
  command* result = ALLOC_CMD(state, sizeof(command));
  result->src_start = src_start;
  result->src_next = cursor;
  result->type = type;
  result->file = file;
  result->fd_lhs1 = fd_lhs1;
  result->fd_lhs2 = fd_lhs2;
  result->fd_rhs = fd_rhs;
  return result;
}
command* parse_command_exec(const char* cursor, state* state){
  if(! skip_spaces(&cursor)) return NULL;

  command* result = ALLOC_CMD(state, sizeof(command));
  result->type = C_EXEC;
  result->src_start = cursor;

  result->file = NULL;
  size_t argc_len = 256;
  result->argc = 0;
  result->argv = ALLOC_ANY(state, argc_len * sizeof(char*));
  
  char* str;
  while((str = parse_strunit(&cursor, state))){
    if(result->argc + 1 >= argc_len){
      state->err = E_TOO_MANY_ARGS;
      return NULL;
    }

    if(result->file == NULL){
      result->file = str;
      *(result->argv + 0) = result->file;
      result->argc = 1;
    }else{
      *(result->argv + result->argc) = str;
      result->argc++;
    }
  }
  *(result->argv + result->argc) = NULL;

  result->src_next = cursor;
  return result;
}

char* parse_strunit(const char** cursor, state* state){
  if(! skip_spaces(cursor)) return NULL;
  
  // Determine str type and start of content
  char type = '\0';  // \0, ", '
  switch(**cursor){
  case '\"': type = '\"'; (*cursor)++; break;
  case '\'': type = '\''; (*cursor)++; break;
  }
  const char* start = *cursor;
  
  // Seek the end of str
  const char* end = NULL;
  {
    int escaped = 0;
    for( ; **cursor ; (*cursor)++){
      if(escaped){
	escaped = 0;
	continue;
      }
      if(**cursor == '\\' && type != '\''){
	escaped = 1;
	continue;
      }
      if(isspecial(**cursor) && type == '\0'){ end = *cursor; break; }
      
      if(type){
	if(**cursor == type){ // Reached at closing quot.
	  end = *cursor;
	  (*cursor)++;  // Skip the quot
	  break;
	}
      }else{
	if(isspace(**cursor)){ end = *cursor; break; }
      }
    }
    if(end == NULL){
      if(type){
	state->err = (type == '\'') ? E_UNCLOSED_SINGLE_QUOT : E_UNCLOSED_DOUBLE_QUOT;
	return NULL;
      }else{
	end = *cursor;
      }
    }
  }
  assert(end != NULL);
  if(start == end && type == '\0') return NULL;

  // Copy [start, end)
  // If (type != '\''), process escpape sequence.
  char* result = ALLOC_STR(state, end - start + 1);
  {
    char* rc_max = result + (end - start);
    char* rc = result;
    *rc_max = '\0';
    int escaped = 0;
    for(const char* c = start; c < end; c++){
      if(escaped){
	escaped = 0;
	*rc = unsecape(*c);
	rc++;
	continue;
      }
      if((*c == '\\') && (type != '\'')){
	escaped = 1;
	continue;
      }

      *rc = *c;
      rc++;
    }
    assert(*rc_max == '\0');  // Possible buffer over-run
  }
  return result;
}

int isspecial(char c){
  switch(c){
  case '|':
  case '&':
  case '>':    
  case '<':
    return 1;

  default: return 0;
  }
}

char unsecape(char c){
  // FIXME: implement \r, \n, ...
  return c;  // For \\, \" or \', return c itself.
}


/** @return 0: EOS, non0: Non space char is found. */
int skip_spaces(const char** cursor){
  for( ; **cursor; (*cursor)++){
    if(! isspace(**cursor)) return 1;
  }
  return 0;
}

/**  */
int read_num(const char** cursor, int* num){
  if(**cursor < '0' || '9' < **cursor) return 0;
  
  *num = 0;
  for( ; '0' <= **cursor && **cursor <= '9'; (*cursor)++){
    *num = (*num * 10) + (**cursor - '0');
  }
  return 1;
}
