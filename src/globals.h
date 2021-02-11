#ifndef __DECL_H__
#define __DECL_H__ 

#define TEXTLEN 512
#define NSYMBOLS 1024
#define CHILDNUM 3

#include <stdio.h>

enum {
  T_None, T_Int, T_Void, T_Char
};

typedef struct treeNode {
  struct treeNode *children[CHILDNUM];
  struct treeNode *sibling;
  /* identify node type by tok attribute */
  int tok;
  /* type specifier for decl and func node */
  int type;
  /* attributes for leaf nodes */
  union {
    int val; // int value
    int id;  // identifier id in symtab
    char ch; // char
  } attr;
} TreeNode;

#include "parse.h"

typedef struct token {
  int intval;
  int token;  // identifier number in symtab
  char text[TEXTLEN + 1];
} Token;

extern FILE *Infile, *Outfile;
extern int lineno;
extern Token Tok;
extern TreeNode *syntaxTree;

#endif
