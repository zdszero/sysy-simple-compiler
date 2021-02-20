#ifndef __SYMTAB_H__
#define __SYMTAB_H__ 

#include "globals.h"

enum symbolKind {
  Sym_Unknown, Sym_Func, Sym_Var, Sym_Array
};

typedef struct dimRec {
  int dim;
  struct dimRec *next;
} DimRec;

typedef struct arrayRec {
  int dims;
  DimRec *first;
} ArrayRec;

typedef struct symRec {
  char *name;
  int type;
  int kind;  // variable or function
  int scope;
  int offset;
  ArrayRec *arr;
} SymRec;

int newIdent(char *t, int kind, int type, int scope);
int getIdentId(char *);
int getIdentType(int id);
int getIdentKind(int id);
int getIdentOffset(int id);
int getIdentScope(int id);
int getArrayTotal(int id, int level);
int getArrayDimension(int id, int d);
int getArrayDims(int id);
char *getIdentName(int id);
void setArrayDimension(int id, int d, int val);
void setIdentType(int id, int type);
void setIdentKind(int id, int kind);
void setDimension(int id, int lev, int val);
void setIdentOffset(int id, int offset);
void addDimension(int id, int d);
int pointerTo(int type);
int valueAt(int type);
void printDimension(int id);

#endif
