#include "globals.h"
#include "symtab.h"
#include <string.h>

SymRec SymTab[NSYMBOLS];

static int Symbols = 0;

int newIdent(char *s) {
  SymTab[Symbols].name = strdup(s);
  SymTab[Symbols].type = T_None;
  return Symbols++;
}

/* find identifier in symbol table and return its index */
int getIdentId(char *s) {
  int i;
  for (i = 0; i < Symbols && SymTab[i].name != NULL; i++) {
    if (strcmp(s, SymTab[i].name) == 0)
      return i;
  }
  return -1;
}

char *getIdentName(int id) {
  return SymTab[id].name;
}

int getIdentType(int id) {
  return SymTab[id].type;
}

void setIdentType(int id, int type) {
  SymTab[id].type = type;
}
