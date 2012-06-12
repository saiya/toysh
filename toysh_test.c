#include <CUnit/CUnit.h>
#include <CUnit/Console.h>
#include "parser_test.h"
#include "dictionary_test.h"

int main(){
  CU_initialize_registry();
  parser_test_new();
  dictionary_test_new();
  
  CU_console_run_tests();

  CU_cleanup_registry();
  return 0;
}
