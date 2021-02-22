#ifndef __ANALYZE_H__
#define __ANALYZE_H__ 

#include "globals.h"

void checkAssign(TreeNode *t1, TreeNode *t2);
void checkCompare(TreeNode *t1, TreeNode *t2);
void checkCalc(TreeNode *t1, TreeNode *t2);
void checkHasReturn(TreeNode *t1, TreeNode *t2, int id);
void checkArray(TreeNode *t);
void checkCall(TreeNode *t);

#endif
