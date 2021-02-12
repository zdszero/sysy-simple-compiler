#include "globals.h"
#include "symtab.h"
#include <string.h>

SymRec SymTab[NSYMBOLS];

static int Symbols = 0;
static int isFirstTime = 1;

int newIdent(char *s) {
  if (isFirstTime) {
    SymTab[0].name = "printint";
    SymTab[0].type = T_Func;
    Symbols = 1;
    isFirstTime = 0;
  }
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
