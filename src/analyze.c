#include "symtab.h"
#include "analyze.h"
#include <stdlib.h>

static int isComparable(int t1, int t2) {
  if (t1 == VOID || t2 == VOID)
    return 0;
  return 1;
}

/* check type when assigning */
void typeCheck_Assign(TreeNode *t1, TreeNode *t2) {
  if (!isComparable(t1->type, t2->type)) {
    fprintf(stderr, "Error: assign between two types that are not compatible at line %d\n", lineno);
    exit(1);
  }
}

void typeCheck_Compare(TreeNode *t1, TreeNode *t2) {
  if (!isComparable(t1->type, t2->type)) {
    fprintf(stderr, "Error: wrong types for comparison at line %d", lineno);
    exit(1);
  }
}

void typeCheck_Calc(TreeNode *t1, TreeNode *t2) {
  if (!isComparable(t1->type, t2->type)) {
    fprintf(stderr, "Error: wrong types for arithmetic calculation at line %d", lineno);
    exit(1);
  }
}
