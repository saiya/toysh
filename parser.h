#pragma once

typedef struct commandLine{
  char* str;

  struct command* command;

  void (*free)(struct commandLine* this);
} commandLine;

typedef struct command{
  char* name;
  int argc;
  char** argv;
  struct command* next;
} command;


/** Parse command line.
 * @param str  Command line.
 *             This function will copy given string.
 */
commandLine* commandLine_parse(const char* str);


typedef enum commandTokenType{
  T_String,
  T_Pipe,
} commandTokenType;
typedef struct commandToken{
  const char* str_start;
  const char* str_next;
  
  commandTokenType type;
} commandToken;
commandToken* readToken(const char* str);
command* command_parse(const char* str, commandToken** nextToken);
