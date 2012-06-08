#pragma once

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
  int (*get)(const struct dictionary* this, const char* key, size_t keyLen, const char** val, size_t* valLen);

  /** Set the copy of val as the key.
   *  Dictionary will copy both key and val.
   * @param keyLen  Length of key or -1 (use strlen+1).
   * @param valLen  Length of val or -1 (use strlen+1).
   */
  void (*set)(struct dictionary* this, const char* key, size_t keyLen, const char* val, size_t valLen);

  /** Free all memory. */
  void (*free)(struct dictionary* this);
} dictionary;


dictionary* dictionary_new();
