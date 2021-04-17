#ifndef __UTIL_H__
#define __UTIL_H__

#include "globals.h"

TreeNode *mkTreeNode();
void printTree(TreeNode *, int);
void printToken(int);
int getTypeSize(int type);
int alignedSize(int type);

#endif
