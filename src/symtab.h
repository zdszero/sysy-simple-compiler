#ifndef __SYMTAB_H__
#define __SYMTAB_H__ 

#include "globals.h"

enum symbolKind {
  Sym_Unknown, Sym_Func, Sym_Var
};

typedef struct symRec {
  char *name;
  int type;
  int kind;  // variable or function
} SymRec;

int newIdent(char *t, int kind, int type);
int getIdentId(char *);
int getIdentType(int id);
char *getIdentName(int id);
void setIdentType(TreeNode *t, int kind, int type, int dep);
int pointerTo(int type);
int valueAt(int type);

#endif
