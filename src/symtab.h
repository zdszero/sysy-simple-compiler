#ifndef __SYMTAB_H__
#define __SYMTAB_H__ 

#include "globals.h"

enum symbolKind {
  Sym_Unknown, Sym_Func, Sym_Var, Sym_Array
};

typedef struct dimensionRecord {
  int dim;
  struct dimensionRecord *next;
} DimRec;

typedef struct symRec {
  char *name;
  int type;
  int kind;  // variable or function
  DimRec *first;
} SymRec;

int newIdent(char *t, int kind, int type);
int getIdentId(char *);
int getIdentType(int id);
int getIdentKind(int id);
char *getIdentName(int id);
DimRec *getIdentDim(int id);
int getDimension(int id, int lev);
void setIdentType(int id, int type);
void setIdentKind(int id, int kind);
void setDimension(int id, int lev, int val);
void addDimension(int id, int d);
int pointerTo(int type);
int valueAt(int type);

#endif
