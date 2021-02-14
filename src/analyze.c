#include "symtab.h"
#include "analyze.h"
#include <stdlib.h>

static int isComparable(int t1, int t2) {
  if (t1 == T_Void || t2 == T_Void)
    return 0;
  if (t1 == T_Func || t2 == T_Func)
    return 0;
  return 1;
}

/* return true if the last sibling is return type */
static int hasReturn(TreeNode *t) {
  while (t->sibling)
    t = t->sibling;
  if (t->tok == RETURN)
    return 1;
  return 0;
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

void typeCheck_HasReturn(TreeNode *t1, TreeNode *t2, int id) {
  if (t1->type == T_Void && hasReturn(t2)) {
    fprintf(stderr, "Error: return statment in void function %s\n", getIdentName(id));
    exit(1);
  } else if (t1->type != T_Void && !hasReturn(t2)) {
    fprintf(stderr, "Error: missing return statement in function %s\n", getIdentName(id));
    exit(1);
  }
}
