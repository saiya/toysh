#pragma once

typedef enum commandType {
  /** Run a command e.g. 'ls -la'. */
  C_EXEC,
  /** e.g. '|' or '|&' */
  C_PIPE,
  /** e.g. '2>&1' */
  C_REDIRECT_FD_TO_FD,
  /** e.g. '2>> file' */
  C_REDIRECT_FD_TO_FILE,
  /** e.g. '< file' */
  C_REDIRECT_FILE_TO_FD,
} command;
typedef struct command {
  commandType type;

  /** This is a view of commandLine.src, don't free. */
  char* src_start;
  /** This is a view of commandLine.src, don't free.
   * strlen( command ) == (src_next - src_start);   // src_next points next char or \0 of this command.
   */
  char* stc_next;
  
  /** Filename to exec or redirection (or NULL). */
  char* file;
  char** argv;
  
  /** -1 means NONE. */
  int fd_lhs1;
  /** If this command is '>&' or '|&', lhs1 is STDOUT and lhs2 is STDERR */
  int fd_lhs2;
  int fd_rhs;

  struct command* prev;
  struct command* next;
} command;

typedef struct commandLine {
  char* src;

  struct command* head;
} commandLine;

