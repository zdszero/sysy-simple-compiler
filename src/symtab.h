#ifndef __SYMTAB_H__
#define __SYMTAB_H__

#include "globals.h"

#define BuildinFunc 4

enum symbolKind { Sym_Unknown, Sym_Func, Sym_Var, Sym_Array };

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
  int kind;
  int scope;
  int offset;
  ArrayRec *arr;
} SymRec;

int newIdent(char *t, int kind, int type, int scope);
int getIdentId(char *);
int getIdentType(TreeNode *t);
int getIdentKind(int id);
int getIdentOffset(int id);
int getIdentScope(int id);
int getIdentSize(TreeNode *t);
int getArrayTotal(int id, int level);
int getArrayDimension(int id, int d);
int getArrayDims(int id);
int getFuncArgs(int id);
int getFuncParaType(int id, int idx);
char *getIdentName(int id);
int getFuncParaSize(int id, int idx);
void setArrayDimension(int id, int d, int val);
void setIdentType(int id, int type);
void setIdentKind(int id, int kind);
void setDimension(int id, int lev, int val);
void setIdentOffset(int id, int offset);
void setFuncRange(int id);
void updateFuncRange(int id);
void addDimension(int id, int d);
int pointerTo(int type);
int valueAt(int type);
void printDimension(int id);
void printSymTab();

#endif
