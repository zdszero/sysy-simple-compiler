#include "symtab.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>

#define FuncCnt 64

int args = 0;
static int lastFn;

typedef struct funcRange {
  int flag;
  int start;
  int end;
} FuncRange;

SymRec SymTab[NSYMBOLS];
static int Symbols = 0;
static int lastLocal = NSYMBOLS - 1;
static int Locals = NSYMBOLS - 1;
static char *builtinFn[] = {"putch", "putint", "putlong", "putarray"};

FuncRange ranges[FuncCnt];

__attribute__((constructor)) static void initSymtab() {
  for (int i = 0; i < 4; i++) {
    SymTab[i].name = builtinFn[i];
    SymTab[i].kind = Sym_Func;
    SymTab[i].type = T_Void;
    SymTab[i].scope = Scope_Glob;
    if (i == 0) {
      newIdent("aaa", Sym_Var, T_Char, Scope_Para);
    } else if (i == 1) {
      newIdent("bbb", Sym_Var, T_Int, Scope_Para);
    } else if (i == 2) {
      newIdent("ccc", Sym_Var, T_Long, Scope_Para);
    } else {
      newIdent("ddd", Sym_Var, T_Int, Scope_Para);
      newIdent("eee", Sym_Var, T_Intptr, Scope_Para);
    }
    setFuncRange(i);
  }
  Symbols = 4;
}

/* range: (Locals, lastLocal] */
void setFuncRange(int id) {
  ranges[id].start = lastLocal;
  ranges[id].end = Locals;
  ranges[id].flag = 1;
  lastLocal = Locals;
}

void updateFuncRange(int id) {
  ranges[id].end = Locals;
  lastLocal = Locals;
}

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
    fprintf(stderr, "Unrecognized type %d to point to in line %d\n", type,
            lineno);
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
    fprintf(stderr, "Unrecognized type to dereference in line %d\n", lineno);
    exit(1);
  }
  return newtype;
}

int newIdent(char *s, int kind, int type, int scope) {
  int idx;
  if (scope == Scope_Glob)
    idx = Symbols++;
  else
    idx = Locals--;
  if (Locals < Symbols) {
    fprintf(stderr, "Error: run out of symbols\n");
    exit(1);
  }
  SymTab[idx].name = strdup(s);
  SymTab[idx].kind = kind;
  SymTab[idx].type = type;
  SymTab[idx].arr = NULL;
  SymTab[idx].scope = scope;
  return idx;
}

void setIdentType(int id, int type) { SymTab[id].type = type; }

void setIdentKind(int id, int kind) {
  SymTab[id].kind = kind;
  if (kind == Sym_Func)
    lastFn = id;
}

void setIdentOffset(int id, int offset) { SymTab[id].offset = offset; }

/* find identifier in symbol table and return its index */
int getIdentId(char *s) {
  int i;
  if (scopeAttr == Scope_Glob) {
    for (i = 0; i < Symbols && SymTab[i].name != NULL; i++)
      if (strcmp(s, SymTab[i].name) == 0)
        return i;
  } else {
    for (i = ranges[lastFn].start; i > Locals && SymTab[i].name != NULL; i--)
      if (strcmp(s, SymTab[i].name) == 0)
        return i;
    for (i = 0; i < Symbols && SymTab[i].name != NULL; i++)
      if (strcmp(s, SymTab[i].name) == 0)
        return i;
  }
  return -1;
}

char *getIdentName(int id) { return SymTab[id].name; }

int getIdentType(TreeNode *t) {
  int type;
  if (t->type)
    type = t->type;
  else
    type = SymTab[t->attr.id].type;
  if (SymTab[t->attr.id].kind == Sym_Array && t->children[0])
    return valueAt(type);
  return type;
}

int getIdentKind(int id) { return SymTab[id].kind; }

int getIdentOffset(int id) { return SymTab[id].offset; }

int getIdentScope(int id) { return SymTab[id].scope; }

int getIdentSize(TreeNode *t) { return getTypeSize(getIdentType(t)); }

int getArrayDimension(int id, int d) {
  DimRec *tmp = SymTab[id].arr->first;
  for (int i = 1; i < d; i++)
    tmp = tmp->next;
  return tmp->dim;
}

int getArrayTotal(int id, int level) {
  DimRec *tmp = SymTab[id].arr->first;
  for (int i = 1; i < level; i++)
    tmp = tmp->next;
  int ans = 1;
  while (tmp) {
    ans *= tmp->dim;
    tmp = tmp->next;
  }
  return ans;
}

int getArrayDims(int id) { return SymTab[id].arr->dims; }

void setArrayDimension(int id, int d, int val) {
  DimRec *tmp = SymTab[id].arr->first;
  for (int i = 1; i < d; i++)
    tmp = tmp->next;
  tmp->dim = val;
}

int getFuncArgs(int id) {
  int ans = 0;
  for (int i = ranges[id].start; i > ranges[id].end; i--) {
    if (SymTab[i].scope == Scope_Para)
      ans++;
    else
      break;
  }
  return ans;
}

int getFuncParaType(int id, int idx) {
  return SymTab[ranges[id].start - idx].type;
}

int getFuncParaSize(int id, int idx) {
  int type = getFuncParaType(id, idx);
  return getTypeSize(type);
}

void printDimension(int id) {
  if (!SymTab[id].arr)
    return;
  DimRec *tmp = SymTab[id].arr->first;
  fprintf(Outfile, "(");
  while (tmp) {
    fprintf(Outfile, "%d,", tmp->dim);
    tmp = tmp->next;
  }
  fprintf(Outfile, ")");
}

void setDimension(int id, int level, int val) {
  DimRec *tmp = SymTab[id].arr->first;
  for (int i = 1; i < level; i++)
    tmp = tmp->next;
  tmp->dim = val;
}

void addDimension(int id, int d) {
  DimRec *dr = (DimRec *)malloc(sizeof(DimRec));
  dr->dim = d;
  dr->next = NULL;
  if (!SymTab[id].arr) {
    SymTab[id].arr = (ArrayRec *)malloc(sizeof(ArrayRec));
    SymTab[id].arr->dims = 0;
  }
  DimRec *tmp = SymTab[id].arr->first;
  SymTab[id].arr->dims++;
  if (!tmp)
    SymTab[id].arr->first = dr;
  else {
    while (tmp->next)
      tmp = tmp->next;
    tmp->next = dr;
  }
}

static void printSymbol(int i) {
  printf("%s", SymTab[i].name);
  int scope = SymTab[i].scope;
  if (scope == Scope_Glob) {
    printf("(glob)");
  } else if (scope == Scope_Local) {
    printf("(local)");
  } else if (scope == Scope_Para) {
    printf("(para)");
  }
  printf(" -> ");
}

void printSymTab() {
  printf("Globals:\n");
  for (int i = 0; i < Symbols; i++)
    printSymbol(i);
  printf("\nLocals\n");
  for (int i = 0; i < FuncCnt; i++) {
    if (ranges[i].flag == 1) {
      printf("%s : ", SymTab[i].name);
      for (int j = ranges[i].start; j > ranges[i].end; j--) {
        printSymbol(j);
      }
      putchar('\n');
    }
  }
  putchar('\n');
}
