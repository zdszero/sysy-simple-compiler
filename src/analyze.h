#ifndef __ANALYZE_H__
#define __ANALYZE_H__ 

#include "globals.h"

void typeCheck_Assign(TreeNode *, TreeNode *);
void typeCheck_Compare(TreeNode *, TreeNode *);
void typeCheck_Calc(TreeNode *, TreeNode *);
void typeCheck_HasReturn(TreeNode *, TreeNode *, int);
void checkArray(TreeNode *);

#endif
