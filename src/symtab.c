#include "globals.h"
#include "symtab.h"
#include <string.h>

SymRec SymTab[NSYMBOLS];

int findIdent(char *s) {
  int i;
  for (i = 0; i < NSYMBOLS && SymTab[i].name != NULL; i++) {
    if (strcmp(s, SymTab[i].name) == 0)
      return i;
  }
  if (i == NSYMBOLS) {
    return -1;
  } else {
    SymTab[i].name = strdup(s);
    return i;
  }
}

char *getIdent(int id) {
  return SymTab[id].name;
}
