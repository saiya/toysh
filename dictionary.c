#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include "dictionary.h"

#if __x86_64__
  typedef u_int64_t hash_t;
  #define FNV1_prime ((hash_t)1099511628211ULL)
  #define FNV1_offset_basis ((hash_t)14695981039346656037ULL)
#else
  typedef u_int32_t hash_t;
  #define FNV1_prime ((hash_t)16777619)
  #define FNV1_offset_basis ((hash_t)2166136261)
#endif

/** FNV-1a hash implementation.
 * see: http://isthe.com/chongo/tech/comp/fnv/
 */
hash_t fnv1a(const char* start, size_t len){
  hash_t hash = FNV1_offset_basis;
  const char* end = start + len;
  for(const char* cur = start; cur < end; cur++){
    hash = (hash ^ *cur) * FNV1_prime;
  }
  return hash;
}
hash_t hash(const char* start, size_t len){ return fnv1a(start, len); }



typedef struct pair{
  hash_t hash;
  char* key;
  size_t keyLen;

  char* val;
  size_t valLen;

  struct pair* next;
} pair;

pair* pair_new(hash_t hash, const char* key, size_t keyLen, const char* val, size_t valLen){
  pair* result = malloc(sizeof(pair));
  result->hash = hash;

  result->key = malloc(keyLen);
  memcpy(result->key, key, keyLen);
  result->keyLen = keyLen;

  result->val = malloc(valLen);
  memcpy(result->val, val, valLen);
  result->valLen = valLen;

  result->next = NULL;
  return result;
}
void pair_free(pair* this){
  free(this->key);
  free(this->val);
  free(this);
}
void pair_set(pair* this, const char* val, size_t valLen){
  if(valLen > this->valLen){
    // Don't realloc(..., 0) because it returns NULL.
    this->val = realloc(this->val, (valLen > 0) ? valLen : 1);
  }
  memcpy(this->val, val, valLen);
  this->valLen = valLen;
}

typedef struct dictImpl{
  struct dictionary public;
  
  size_t slotCount;
  pair** slots;
} dictImpl;

size_t _dic_slotOf(dictImpl* this, hash_t hash){ return hash % this->slotCount; }
pair* _dic_lookup(dictImpl* this, const char* key, size_t keyLen, pair** prev){
  hash_t h = hash(key, keyLen);
  if(prev) *prev = NULL;
  for(pair* cur = *(this->slots + _dic_slotOf(this, h)); cur; ){
    if(
       (cur->hash == h)
       && (cur->keyLen == keyLen)
       && (memcmp(cur->key, key, keyLen) == 0)
    ){
      return cur;
    }

    if(prev) *prev = cur;
    cur = cur->next;
  }
  return NULL;  // Not found  
}
void _dic_resize(dictImpl* this, size_t slotCount){
  pair** heads = malloc(sizeof(pair*) * slotCount);
  memset(heads, 0, slotCount);
  pair** tails = malloc(sizeof(pair*) * slotCount);
  memset(tails, 0, slotCount);
  
  size_t oldsc = this->slotCount;
  pair** oldslots = this->slots;
  this->slotCount = slotCount;
  this->slots = heads;

  for(size_t s = 0; s < oldsc; s++){
    for(pair* p = *(oldslots + s); p; ){
      size_t dest = _dic_slotOf(this, p->hash);
      if(! *(heads + dest)){
	*(tails + dest) = *(heads + dest) = p;
      }else{
	(*(tails + dest))->next = p;
	*(tails + dest) = p;
      }

      pair* next = p->next;
      p->next = NULL;
      p = next;
    }
  }
  
  free(oldslots);
  free(tails);
}

char* dic_dup(const struct dictionary* thisD, const char* key, size_t keyLen){
  dictImpl* this = (dictImpl*)thisD;
  if(keyLen == -1) keyLen = strlen(key) + 1;
  
  const pair* p = _dic_lookup(this, key, keyLen, NULL);
  if(! p) return NULL;
  
  char* result = malloc(p->valLen);
  memcpy(result, p->val, p->valLen);
  return result;
}
int dic_get(const struct dictionary* thisD, const char* key, size_t keyLen, const char** val, size_t* valLen){
  dictImpl* this = (dictImpl*)thisD;
  if(keyLen == -1) keyLen = strlen(key) + 1;
  
  const pair* p = _dic_lookup(this, key, keyLen, NULL);
  if(! p){
    if(val) *val = NULL;
    if(valLen) *valLen = -1;
    return 0;
  }

  if(val) *val = p->val;
  if(valLen) *valLen = p->valLen;
  return 1;
}
void dic_set(struct dictionary* thisD, const char* key, size_t keyLen, const char* val, size_t valLen){
  dictImpl* this = (dictImpl*)thisD;
  if(keyLen == -1) keyLen = strlen(key) + 1;
  if(valLen == -1) valLen = strlen(val) + 1;
  
  hash_t h = hash(key, keyLen);
  size_t s = _dic_slotOf(this, h);
  
  pair* head = *(this->slots + s);
  if(! head){
    *(this->slots + s) = pair_new(h, key, keyLen, val, valLen);
    return;
  }
  
  pair* last = NULL;
  for(pair* cur = head; cur; cur = cur->next){
    last = cur;

    if(cur->hash != h) continue;
    if(cur->keyLen != keyLen) continue;
    if(memcmp(cur->key, key, keyLen) != 0) continue;

    pair_set(cur, val, valLen);
    return;
  }
  
  last->next = pair_new(h, key, keyLen, val, valLen);
}
int dic_remove(const struct dictionary* thisD, const char* key, size_t keyLen){
  dictImpl* this = (dictImpl*)thisD;
  if(keyLen == -1) keyLen = strlen(key) + 1;
  
  pair* prev;
  pair* p = _dic_lookup(this, key, keyLen, &prev);
  if(! p) return 0;
  
  if(prev){
    prev->next = p->next;
  }else{
    *(this->slots + _dic_slotOf(this, p->hash)) = p->next;
  }
  
  pair_free(p);
  return 1;
}

void dic_free(struct dictionary* thisD){
  dictImpl* this = (dictImpl*)thisD;

  for(size_t s = 0; s < this->slotCount; s++){
    pair* cur = *(this->slots + s);
    while(cur){
      pair* next = cur->next;
      pair_free(cur);
      cur = next;
    }
  }
  
  free(this->slots);
  this->slots = NULL;        // To detect invalid access.
  this->public.free = NULL;  // To prevent double-free.
  free(this);
}
dictionary* dictionary_new(size_t slots){
  dictImpl* d = malloc(sizeof(dictImpl));
  d->public.set = &dic_set;
  d->public.dup = &dic_dup;
  d->public.get = &dic_get;
  d->public.remove = &dic_remove;
  d->public.free = &dic_free;

  d->slotCount = 7;
  d->slots = malloc(sizeof(pair*) * d->slotCount);
  memset(d->slots, 0, sizeof(pair*) * d->slotCount);
  
  return &d->public;
}
