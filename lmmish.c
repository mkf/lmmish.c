#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct trie trie;
struct trie {
  trie* _c[254];
  char* v;
} trie_empty = { { NULL }, NULL };
trie** trie_c(trie* w, char k) {
  return &(w->_c[k-1]);
}
int delete_trie_node_if_empty_else(trie* w, char k) {
  for(char i = 1; i<255; i++)
    if( (k==0 || i!=k) && *trie_c(w, i)!=NULL )
      return 1;
  free(w);
  return 0;
}
struct dead_end {
  trie* branch;
  char* rest;
};
typedef struct trie_list trie_list;
struct trie_list {
  trie* v;
  char k;
  trie_list* n;
};
trie_list* tl_new(trie* v, char k, trie_list* n) {
  trie_list* r = malloc(sizeof(trie_list));
  r->v = v;
  r->k = k;
  r->n = n;
  return r;
}
struct dead_end trie_deadend(trie* w, char* k) {
  trie* r = w;
  char* h = k;
  while(*h!=0) {
    trie* child = *trie_c(r, *(h++));
    if(child==NULL) break;
    r = child;
  }
  return (struct dead_end) { r, h };
}
trie_list* trie_branch(trie* w, char* k) {
  trie_list* r = tl_new(w, 0, NULL);
  char* h = k;
  while(*h!=0) {
    trie* child = *trie_c(r, *h);
    if(child==NULL) break;
    r = tl_new(child, *((char*)(h++)), r);
  }
  return (*h==0) ? tl_new(child->v, 0, r) : r;
}
trie* trie_go(trie* w, char* k, char altsep) {
  trie* r = w;
  char* h = k;
  while(!(*h==0 || *h==altsep) && r!=NULL)
    r = *trie_c(r, *(h++));
  return r;
}
char* trie_popjust(trie* w, char* k) {
  trie* r = trie_go(w, k);
  if(r==NULL) return NULL;
  else {
    char* v = r->v;
    r->v = NULL;
    return v;
  }
}
char* trie_popfree(trie* w, char* k) {
  trie_list* n = trie_branch(w, k);
  if(n==NULL) return NULL;
  char* result = NULL;
  do {
    trie_list nl = *n;
    free(n);
    n = n->n;
    if(result==NULL && nl.k==0) {
      result = nl.v;
    } else {
      char cur = nl->k;
      free(nl->v);
      *trie_c(n->v, cur) = NULL;
      if (delete_trie_node_if_empty_else(n->v, cur))
	break;
    }
  } while (n!=NULL)
  while(n!=NULL) {
    free(n);
    n = n->n;
  }
}
char* trie_popfreerec(trie* w, char* k) {
  trie* next = NULL;
  if(*k==0) {
    next = w;
    v = w->v;
    w->v = NULL;
  } else {
    next = *trie_c(w, *k);
    v = next==NULL ? NULL : trie_popfree(next, k+1);
    if((*k+1)==0) {
    };
  }
  trie* cur = next->c;
  for(int i=1; i<255; i++) {
    if(*cur!=NULL) break;
  }
  return v;
}
void trie_newnodesinsert(trie* w, char* k, char* v) {
  trie* r = w;
  char* h = k;
  while(*h!=0) {
    trie* t = malloc(sizeof(trie));
    r->c[*(h++)] = t;
    r = t;
  }
}
void trie_forceinsert(trie* w, char* k, char* v) {
  struct dead_end de = trie_deadend(w, k);
  trie_newnodesinsert(de.branch, de.rest, v);
}
trie tmlentities;
void make_tmlentities_trie() {
  trie_forceinsert(&tmlentities, "gt", ">");
  trie_forceinsert(&tmlentities, "lt", "<");
  trie_forceinsert(&tmlentities, "amp", "&");
  trie_forceinsert(&tmlentities, "bell", "\x07");
  trie_forceinsert(&tmlentities, "nl", "\n");
  trie_forceinsert(&tmlentities, "cr", "\r");
  trie_forceinsert(&tmlentities, "esc", "\x033");
}
char* tmlentity(char* src) {
  trie* r = trie_go(&tmlentities, src, ';');
  return r==NULL ? NULL : r->v;
}
enum dyncont_i { original, datetime, raw };
struct dyncont {
  enum dyncont typ;
  char* load;
  (struct dyncont)* next;
};
