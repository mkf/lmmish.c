#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const struct tmlentityt {
  const char* key;
  const char value;
} const tmlentities[] =
  {
   { "gt", '>' },
   { "lt", '<' },
   { "amp", '&' },
   { "bell", 7 },
   { "nl", '\n' },
   { "cr", '\r' },
   { "esc", 33 }
  };
#define DEFTMLENTCOUNT 7
#define MAXTMLENTLEN 4

char tmlentity(char* src) {
  int i;
  for(i = 1; i<=MAXTMLENTLEN; i++)
    if(src[i]==';') goto thereis;
  return 0;
 thereis:
  src[i]=0;
  char result = 0;
  int j;
  for(j = 0; j<DEFTMLENTCOUNT; j++)
    if(strcmp(tmlentities[j].key, src)==0)
      result = tmlentities[j].value;
  src[i]=';';
  return result;
}
enum dyncont_i { original, raw, color, datetime };
struct dyncont {
  enum dyncont_i typ;
  void* load;
  struct dyncont* next;
};
struct dyncont* justtags(char* src) {
  
};
