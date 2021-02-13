#include "symtab.h"
#include <string.h>
#include <stdlib.h>

SymRec SymTab[NSYMBOLS];

static int Symbols = 0;
static int isFirstTime = 1;

int pointerTo(int type) {
  int newtype;
  switch (type) {
    case T_Void:
      newtype = T_Voidptr;
      break;
    case T_Char:
      newtype = T_Charptr;
      break;
    case T_Int:
      newtype = T_Intptr;
      break;
    case T_Long:
      newtype = T_Longptr;
      break;
    default:
      fprintf(stderr, "Unrecognized type %d to point to in line %d\n", type, lineno);
      exit(1);
  }
  return newtype;
}

int valueAt(int type) {
  int newtype;
  switch (type) {
    case T_Voidptr:
      newtype = T_Void;
      break;
    case T_Charptr:
      newtype = T_Char;
      break;
    case T_Intptr:
      newtype = T_Int;
      break;
    case T_Longptr:
      newtype = T_Long;
      break;
    default:
      fprintf(stderr, "Unrecognized type to dereference\n");
      exit(1);
  }
  return newtype;
}

int newIdent(char *s, int kind, int type) {
  if (isFirstTime) {
    SymTab[0].name = "printint";
    SymTab[0].type = T_Func;
    Symbols = 1;
    isFirstTime = 0;
  }
  SymTab[Symbols].name = strdup(s);
  SymTab[Symbols].kind = kind;
  SymTab[Symbols].type = type;
  return Symbols++;
}

void setIdentType(TreeNode *t, int kind, int type, int dep) {
  t->type = type;
  for (int i = 0; i < dep; i++) {
    t->type = pointerTo(t->type);
  }
  SymTab[t->attr.id].kind = kind;
  SymTab[t->attr.id].type = t->type;
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
