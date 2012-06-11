#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "dictionary.h"


// Resize() trigger at: stat.elements > DEFAULT_LOAD_FACTOR * stat.buckets
#define DEFAULT_LOAD_FACTOR (0.9f)
#define INITIAL_SIZE (10)


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

  result->key = malloc(keyLen ? keyLen : 1);
  memcpy(result->key, key, keyLen);
  result->keyLen = keyLen;

  result->val = malloc(valLen ? valLen : 1);
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
  struct dictionaryStat stat;
  
  size_t slotCount;
  pair** slots;

  size_t resizeThreshold;
} dictImpl;
size_t _dic_slotOf(dictImpl* this, hash_t hash){ return hash % this->slotCount; }
pair* _dic_lookup(dictImpl* this, const char* key, size_t keyLen, pair** prev){
  this->stat.lookups++;

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

    this->stat.lookupChains++;
    if(prev) *prev = cur;
    cur = cur->next;
  }
  return NULL;  // Not found  
}
void _dic_resize(dictImpl* this, size_t slotCount){
  this->stat.resizes++;
  this->resizeThreshold = (size_t)(DEFAULT_LOAD_FACTOR * slotCount);

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
void _dic_resize_auto(dictImpl* this){
  if(this->resizeThreshold >= this->stat.elements) return;
  
  // FIXME: Use other policy ( nextPrime(slotCount) )
  size_t buckets = this->slotCount * 2;
  _dic_resize(this, buckets);
}

char* dic_dup(const struct dictionary* thisD, const char* key, size_t keyLen){
  dictImpl* this = (dictImpl*)thisD;
  if(keyLen == -1) keyLen = strlen(key) + 1;
  
  const pair* p = _dic_lookup(this, key, keyLen, NULL);
  if(p == NULL) return NULL;
  
  char* result = malloc((p->valLen) ? p->valLen : 1);
  memcpy(result, p->val, p->valLen);
  return result;
}
int dic_get(const struct dictionary* thisD, const char* key, size_t keyLen, char** val, size_t* valLen){
  dictImpl* this = (dictImpl*)thisD;
  if(keyLen == -1) keyLen = strlen(key) + 1;
  
  const pair* p = _dic_lookup(this, key, keyLen, NULL);
  if(p == NULL){
    if(val) *val = NULL;
    if(valLen) *valLen = -1;
    return 0;
  }

  if(val){
    *val = malloc((p->valLen) ? p->valLen : 1);
    memcpy(*val, p->val, p->valLen);
  }
  if(valLen) *valLen = p->valLen;
  return 1;
}
void dic_set(struct dictionary* thisD, const char* key, size_t keyLen, const char* val, size_t valLen){
  dictImpl* this = (dictImpl*)thisD;
  if(keyLen == -1) keyLen = strlen(key) + 1;
  if(valLen == -1) valLen = strlen(val) + 1;
  
  hash_t h = hash(key, keyLen);
  size_t s = _dic_slotOf(this, h);

  // set is a kind of lookup
  this->stat.lookups++;

  // ADD (no hash collision)
  pair* head = *(this->slots + s);
  if(! head){
    this->stat.elements++;
    *(this->slots + s) = pair_new(h, key, keyLen, val, valLen);

    // _dic_resize_auto() isn't needed here.
    // Because we used vacant slot!
    return;
  }
  
  // UPDATE exsiting key
  pair* last = NULL;
  this->stat.lookupChains--; // Followin loop too increments
  for(pair* cur = head; cur; cur = cur->next){
    // set is a kind of lookup
    this->stat.lookupChains++;

    last = cur;

    if(cur->hash != h) continue;
    if(cur->keyLen != keyLen) continue;
    if(memcmp(cur->key, key, keyLen) != 0) continue;

    pair_set(cur, val, valLen);
    return;
  }
  
  // ADD (hash collision)
  this->stat.elements++;
  last->next = pair_new(h, key, keyLen, val, valLen);
  _dic_resize_auto(this);
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
  
  this->stat.elements--;
  pair_free(p);
  return 1;
}

void dic_getStat(const dictionary* thisD, dictionaryStat* stat){
  dictImpl* this = (dictImpl*)thisD;
  *stat = this->stat;
  stat->buckets = this->slotCount;
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

char* dicStat_toString(const dictionaryStat* this){
  char* result;
  if(asprintf(
    &result,
    "%zd elements / %zd buckets, Lookup: %u (chain: %u), Resize: %u",
    this->elements,
    this->buckets,
    this->lookups,
    this->lookupChains,
    this->resizes
  ) == -1){
    // Value of result is undefined!
    return NULL;
  }
  return result;
}

dictionary* dictionary_new(size_t slots){
  dictImpl* d = malloc(sizeof(dictImpl));

  d->slotCount = INITIAL_SIZE;
  d->slots = malloc(sizeof(pair*) * d->slotCount);
  memset(d->slots, 0, sizeof(pair*) * d->slotCount);

  d->stat.lookups = 0;
  d->stat.lookupChains = 0;
  d->stat.resizes = 0;
  d->stat.buckets = -1;  // getStat() will update this.
  d->stat.toString = &dicStat_toString;
  
  d->public.set = &dic_set;
  d->public.dup = &dic_dup;
  d->public.get = &dic_get;
  d->public.remove = &dic_remove;
  d->public.getStat = &dic_getStat;
  d->public.free = &dic_free;

  return &d->public;
}
