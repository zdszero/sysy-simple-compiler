#ifndef __ANALYZE_H__
#define __ANALYZE_H__

#include "globals.h"

void checkAssign(TreeNode *t1, TreeNode *t2);
void checkCompare(TreeNode *t);
void checkCalc(TreeNode *t);
void checkReturn(TreeNode *t);
void checkArray(TreeNode *t);
void checkCall(TreeNode *t);

#endif
