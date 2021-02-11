#include "symtab.h"
#include "analyze.h"
#include <stdlib.h>

/* check type when assigning */
void typeCheck_Assign(TreeNode *t1, TreeNode *t2) {
  if (t1->type == t2->type) {
    /* set symbol type if none is in symbol table */
    if (getIdentType(t1->attr.id) == T_None)
      setIdentType(t1->attr.id, t1->type);
  } else if (t1->type == T_Char && t2->type == T_Int) {
    t1->type = T_Char;
    setIdentType(t1->attr.id, t1->type);
  } else if (t1->type == T_Int && t2->type == T_Char) {
    t1->type = T_Int;
    setIdentType(t1->attr.id, t1->type);
  } else {
    fprintf(Outfile, "assign between two types that are not compatible at line %d\n", lineno);
    exit(1);
  }
}

void typeCheck_Compare(TreeNode *t1, TreeNode *t2) {
  if (!isComparable(t1->type, t2->type)) {
    fprintf(Outfile, "Error: wrong types for comparison at line %d", lineno);
    exit(1);
  }
}

void typeCheck_Calc(TreeNode *t1, TreeNode *t2) {
  if (!isComparable(t1->type, t2->type)) {
    fprintf(Outfile, "Error: wrong types for arithmetic calculation at line %d", lineno);
    exit(1);
  }
}

int isComparable(int t1, int t2) {
  if (t1 == VOID || t2 == VOID)
    return 0;
  return 1;
}
