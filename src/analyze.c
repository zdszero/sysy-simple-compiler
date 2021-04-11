#include "symtab.h"
#include "analyze.h"
#include <stdlib.h>

int hasError = 0;

static int isPointerType(int type) {
  return (type >= T_Voidptr && type <= T_Longptr);
}

static int isNumberType(int type) {
  return (type == T_Char || type == T_Int || type == T_Long);
}

static int getScaleSize(int type) {
  if (type == T_Intptr)
    return 4;
  else if (type == T_Longptr)
    return 8;
  return 1;
}

static int isComparable(int t1, int t2) {
  if (t1 == T_Void || t2 == T_Void)
    return 0;
  if (isPointerType(t1) && isNumberType(t2))
    return 0;
  if (isNumberType(t1) && isPointerType(t2))
    return 0;
  return 1;
}

/* check type when assigning */
void checkAssign(TreeNode *t1, TreeNode *t2) {
  int kind = getIdentKind(t2->attr.id);
  if (kind == Sym_Array && !t2->children[0])
    t2->type = pointerTo(t2->type);
  if (!isComparable(t1->type, t2->type)) {
    fprintf(stderr, "Error: assignment between two types that are not compatible at line %d\n", lineno);
    hasError = 1;
  }
}

void checkCompare(TreeNode *t) {
  TreeNode *t1 = t->children[0], *t2 = t->children[1];
  if (!isComparable(t1->type, t2->type)) {
    fprintf(stderr, "Error: wrong types for comparison at line %d\n", lineno);
    hasError = 1;
  } else {
    t->type = T_Long;
  }
}

/* 1. check the type between two operands 
 * 2. set the type for the target parent node */
void checkCalc(TreeNode *t) {
  TreeNode *t1 = t->children[0], *t2 = t->children[1];
  if (isPointerType(t1->type) && isNumberType(t2->type)) {
    t2->attr.val *= getScaleSize(t1->type);
    t->type = t1->type;
  } else if (isNumberType(t1->type) && isPointerType(t2->type)) {
    t1->attr.val *= getScaleSize(t2->type);
    t->type = t2->type;
  } else if (!isComparable(t1->type, t2->type)) {
    fprintf(stderr, "Error: wrong types for arithmetic calculation at line %d\n", lineno);
    t->type = T_Long;
    hasError = 1;
  } else {
    t->type = t1->type;
  }
}

void checkReturn(TreeNode *t) {
  int type = t->children[0]->type;
  int flag = 0;
  TreeNode *tmp;
  for (tmp = t->children[1]; tmp; tmp = tmp->sibling) {
    if (tmp->tok == RETURN) {
      flag = 1;
      break;
    }
  }
  if (type == T_Void && !flag)
    return;
  if (type == T_Void && flag) {
    if (tmp->children[0]) {
      fprintf(stderr, "line %d: cannot return value in void function\n", lineno);
      hasError = 1;
    }
    return;
  }
  if (!flag || !tmp->children[0]) {
    fprintf(stderr, "line %d: return value is missing\n", lineno);
    hasError = 1;
  }
}

/* guess the first dimension of array */
void checkArray(TreeNode *t) {
  int id = t->attr.id;
  int d1 = getArrayDimension(id, 1);
  if (d1 != 0)
    return;
  if ((!t->children[0] || !t->children[0]->children[0]) && (d1 == 0)) {
    fprintf(stderr, "Error: array size cannot be gussed in line %d\n", lineno);
    hasError = 1;
    return;
  }
  d1 = 0;
  int dims = getArrayDims(id);
  for (TreeNode *tmp = t->children[0]->children[0]; tmp; tmp = tmp->sibling) {
    while (tmp && tmp->tok == NUM) {
      if (dims == 1)
        d1++;
      tmp = tmp->sibling;
    }
    if (!tmp)
      break;
    d1++;
  }
  setArrayDimension(id, 1, d1);
}

/* 1. check the count of arguments
 * 2. check the type of arguments */
void checkCall(TreeNode *t) {
  int id = t->children[0]->attr.id;
  if (id < BuildinFunc)
    return;
  int args = 0;
  TreeNode *tmp;
  for (TreeNode *tmp = t->children[1]; tmp; tmp = tmp->sibling) {
    args++;
  }
  int count = getFuncArgs(id);
  if (args < count) {
    fprintf(stderr, "Error: missing arguments when calling function %s in line %d\n", getIdentName(id), lineno);
    hasError = 1;
  } else if (args > count) {
    fprintf(stderr, "Error: too many arguments when calling function %s in line %d\n", getIdentName(id), lineno);
    hasError = 1;
  } else {
    int idx = 0;
    for (tmp = t->children[1]; tmp; tmp = tmp->sibling) {
      int type1 = tmp->type;
      int type2 = getFuncParaType(id, idx);
      if (!isComparable(type1, type2)) {
        fprintf(stderr, "Error: wrong function argument type in line %d\n", lineno);
        hasError = 1;
        break;
      }
      idx++;
    }
  }
}
