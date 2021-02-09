#ifndef __DECL_H__
#define __DECL_H__ 

#define TEXTLEN 512
#define NSYMBOLS 1024
#define CHILDNUM 3

#include <stdio.h>

typedef struct treeNode {
  struct treeNode *children[CHILDNUM];
  struct treeNode *sibling;
  int tok;
  /* attributes for leaf nodes */
  union {
    int val; // int value
    int id; // identifier id in symtab
    int op; // operator value
  } attr;
} TreeNode;

#include "parse.h"

typedef struct symRec {
  char *name;
} SymRec;

typedef struct token {
  int intval;
  int token;  // identifier number in symtab
  char text[TEXTLEN + 1];
} Token;

extern FILE *Infile, *Outfile;
extern int Putback;
extern Token Tok;
extern SymRec SymTab[NSYMBOLS];
extern TreeNode *syntaxTree;

#endif
