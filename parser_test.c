#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <CUnit/CUnit.h>
#include "parser_test.h"
#include "parser.h"

void test();
CU_pSuite parser_test_new(){
  CU_pSuite suite = CU_add_suite("parser", NULL, NULL);
  CU_add_test(suite, "test", test);
  return suite;
}


char* commandTypeToString(commandType type){
  switch(type){
  case C_INVALID: return strdup("C_INVALID");
  case C_EXEC: return strdup("C_EXEC");
  case C_PIPE: return strdup("C_PIPE");
  case C_REDIRECT_FD_TO_FD: return strdup("C_REDIRECT_FD_TO_FD");
  case C_REDIRECT_FD_TO_FILE: return strdup("C_REDIRECT_FD_TO_FILE");
  case C_REDIRECT_FILE_TO_FD: return strdup("C_REDIRECT_FILE_TO_FD");
  }
  return NULL;
}
void test(){
  pool* pool = pool_new();

  commandLine* cl = parse("ls -la |& wc > /dev/null", pool);
  CU_ASSERT(cl != NULL);

  command* ls = cl->head; CU_ASSERT(ls != NULL);
  command* pipe = ls->next; CU_ASSERT(pipe != NULL);
  command* wc = pipe->next; CU_ASSERT(wc != NULL);
  command* redirect = wc->next; CU_ASSERT(redirect != NULL);
  CU_ASSERT(cl->last == redirect);
  CU_ASSERT(redirect->next == NULL);

  CU_ASSERT(ls->type == C_EXEC);
  CU_ASSERT(strcmp(ls->file, "ls") == 0);
  CU_ASSERT(ls->argc == 2);
  CU_ASSERT(strcmp(*(ls->argv + 0), "ls") == 0);
  CU_ASSERT(strcmp(*(ls->argv + 1), "-la") == 0);

  CU_ASSERT(pipe->type == C_PIPE);
  CU_ASSERT(pipe->fd_lhs1 == FD_STDOUT);
  CU_ASSERT(pipe->fd_lhs2 == FD_STDERR);
  CU_ASSERT(pipe->fd_rhs == FD_STDIN);

  CU_ASSERT(wc->type == C_EXEC);
  CU_ASSERT(strcmp(wc->file, "wc") == 0);
  CU_ASSERT(wc->argc == 1);

  CU_ASSERT(redirect->type == C_REDIRECT_FD_TO_FILE);
  CU_ASSERT(redirect->fd_lhs1 == FD_STDOUT);
  CU_ASSERT(redirect->fd_lhs2 == FD_NONE);
  CU_ASSERT(strcmp(redirect->file, "/dev/null") == 0);

  pool->free(pool);
}

