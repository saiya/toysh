#include <stdlib.h>
#include <stdio.h>
#include <CUnit/CUnit.h>
#include <CUnit/Console.h>
#include "dictionary_test.h"
#include "dictionary.h"

void test_random();

CU_pSuite dictionary_test_new(){
  CU_pSuite suite = CU_add_suite("dictionary", NULL, NULL);
  CU_add_test(suite, "Random data test", test_random);
  return suite;
}

typedef struct myString{
  int printable;
  size_t len;  // Contains \0
  char* buff;
} myString;
void myString_free(myString* str){ free(str->buff); }

#define test_random_strlen_max (100)
#define test_random_entry_min (100)
#define test_random_entry_max (300)
char test_randon_randChar(unsigned int* rnd, int is_printable){
  const static char printables[] = 
    " \t\r\n"
    "!\"#$%&\'()=~|-^\\`{+*}<>?_@[;:],./"
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  ;
  if(is_printable){
    return printables[(rand_r(rnd) * sizeof(printables)) / RAND_MAX];
  }else{
    return (char)((rand_r(rnd) * 255) / RAND_MAX);
  }
}
myString test_random_newRandStr(unsigned int* rnd, size_t length){
  if(length == -1) length = (size_t)((rand_r(rnd) / (float)RAND_MAX) * test_random_strlen_max);
  int is_printable = (int)((rand_r(rnd) / (float)RAND_MAX) >= 0.5f);
  if(is_printable && length == 0) length = 1;
  
  myString result;
  result.printable = is_printable;
  result.len = length;
  result.buff = malloc((length ? length : 1) * sizeof(char));
  for(size_t i = 0; i < length; i++){
    if(is_printable && i == length - 1){
      *(result.buff + i) = '\0';
    }else{
      *(result.buff + i) = test_randon_randChar(rnd, is_printable);
    }
  }
  // if(is_printable) printf("Random string (%zd): %s\n", length, result.buff);
  return result;
}
void test_random(){
  unsigned int rnd = (unsigned int)rand();
  // printf("\tSeed: %d\n", rnd);
  
  size_t entries = (size_t)((rand_r(&rnd) / (float)RAND_MAX) * (test_random_entry_max - test_random_entry_min)) + test_random_entry_min;
  // printf("\tEntries: %zd\n", entries);

  size_t nullVal_at = (rand_r(&rnd) * entries) / RAND_MAX;
  size_t nullBoth_at = (rand_r(&rnd) * entries) / RAND_MAX;
  myString* keys = malloc(entries * sizeof(myString));
  myString* vals = malloc(entries * sizeof(myString));
  for(size_t i = 0; i < entries; i++){
    while(1){
      if(i == nullVal_at){
	*(keys + i) = test_random_newRandStr(&rnd, -1);
	*(vals + i) = test_random_newRandStr(&rnd, 0);
      }else if(i == nullBoth_at){
	*(keys + i) = test_random_newRandStr(&rnd, 0);
	*(vals + i) = test_random_newRandStr(&rnd, 0);
      }else{
	*(keys + i) = test_random_newRandStr(&rnd, -1);
	*(vals + i) = test_random_newRandStr(&rnd, -1);
      }
      
      // Retry if the key duplicates
      int unique = 1;
      for(size_t j = 0; j < i; j++){
	if(
	   ((keys + i)->len == (keys + j)->len)
	   && (memcmp((keys + i)->buff, (keys + j)->buff, (keys + i)->len) == 0)
	){
	  unique = 0;
	  break;
	}
      }
      if(unique) break;

      // Before retry...
      myString_free(keys + i);
      myString_free(vals + i);
    }
  }
  
  
  dictionary* dict = dictionary_new();
  size_t tmpLen;
  char* tmpStr;
  for(size_t i = 0; i < entries; i++){
    myString key = *(keys + i);
    myString val = *(vals + i);

    // Should not be found
    CU_ASSERT(dict->dup(dict, key.buff, key.len) == NULL);
    CU_ASSERT(dict->get(dict, key.buff, key.len, NULL, NULL) == 0);
    
    dict->set(dict, key.buff, key.len, val.buff, val.len);
    
    // Should be found
    tmpStr = dict->dup(dict, key.buff, key.len);
    CU_ASSERT(strncmp(tmpStr, val.buff, val.len) == 0);
    free(tmpStr);
    CU_ASSERT(dict->get(dict, key.buff, key.len, &tmpStr, &tmpLen) != 0);
    CU_ASSERT(tmpLen == val.len);
    CU_ASSERT(strncmp(tmpStr, val.buff, val.len) == 0);
    if(strncmp(tmpStr, val.buff, val.len) != 0){
      if(val.printable){
	printf("\tExpected: \"%s\"\n", val.buff);
	printf("\tActual: \"%s\"\n", tmpStr);
      }
    }
    free(tmpStr);
  }

  {
    dictionaryStat stat;
    dict->getStat(dict, &stat);
    char* statStr = stat.toString(&stat);
    puts("");
    printf("Stat: %s\n", statStr);
    free(statStr);
  }

  for(size_t i = 0; i < entries; i++){
    myString key = *(keys + i);
    myString val = *(vals + i);

    // Sould be found
    CU_ASSERT(dict->get(dict, key.buff, key.len, NULL, NULL) != 0);

    dict->remove(dict, key.buff, key.len);
    
    // Should not be found
    CU_ASSERT(dict->dup(dict, key.buff, key.len) == NULL);
    CU_ASSERT(dict->get(dict, key.buff, key.len, NULL, NULL) == 0);
  }
  
  dict->free(dict);
  for(size_t i = 0; i < entries; i++){
    myString_free(keys + i);
    myString_free(vals + i);
  }
  free(keys);
  free(vals);
}
