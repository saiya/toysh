#pragma once

typedef struct dictionaryStat{
  unsigned int lookups;
  unsigned int lookupChains;
  
  unsigned int resizes;

  size_t elements;
  size_t buckets;

  char* (*toString)(const struct dictionaryStat* this);
} dictionaryStat;

/** char* to char* dictionary. */
typedef struct dictionary{

  /** Returns copy of the value (char*) of NULL.
   *  You have to free the result (char*).
   * @param keyLen  Length of key or -1 (use strlen+1).
   */
  char* (*dup)(const struct dictionary* this, const char* key, size_t keyLen);
  /** Copies the value and length of the value.
   * @param val     If this is NULL, this won't copy the value itself.
   *                When the key isn't found, this will copy NULL into here.
   * @param valLen  If this is NULL, this won't copy the length.
   *                When the key isn't found, this will copy -1 into here.
   * @return 0: not found, not0: found.
   */
  int (*get)(const struct dictionary* this, const char* key, size_t keyLen, char** val, size_t* valLen);

  /** Set the copy of val as the key.
   *  Dictionary will copy both key and val.
   * @param keyLen  Length of key or -1 (use strlen+1).
   * @param valLen  Length of val or -1 (use strlen+1).
   */
  void (*set)(struct dictionary* this, const char* key, size_t keyLen, const char* val, size_t valLen);

  /** 
   * @param keyLen  Length of key or -1 (use strlen+1).
   * @return 0: removed, not0: not found
   */
  int (*remove)(const struct dictionary* this, const char* key, size_t keyLen);

  /**  */
  void (*getStat)(const struct dictionary* this, dictionaryStat* stat);

  /** Free all memory. */
  void (*free)(struct dictionary* this);
} dictionary;

dictionary* dictionary_new();
