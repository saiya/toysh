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
void toysh_run(const commandLine* cline);
void toysh();
