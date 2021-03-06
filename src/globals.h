#ifndef __DECL_H__
#define __DECL_H__

#define TEXTLEN 512
#define NSYMBOLS 1024
#define CHILDNUM 3

#include <stdio.h>

enum {
  T_None,
  T_Void,
  T_Char,
  T_Int,
  T_Long,
  T_Voidptr,
  T_Intptr,
  T_Charptr,
  T_Longptr
};

enum { Scope_Glob, Scope_Local, Scope_Para };

typedef struct treeNode {
  struct treeNode *children[CHILDNUM];
  struct treeNode *sibling;
  /* identify node type by tok attribute */
  int tok;
  /* type specifier for decl and func node */
  int type;
  /* attributes for leaf nodes */
  union {
    long val; // number
    int id;   // identifer
    char ch;  // char
  } attr;
} TreeNode;

#include "parse.h"

typedef struct token {
  int token; // identifier number in symtab
  long numval;
  char text[TEXTLEN + 1];
} Token;

extern FILE *Infile, *Outfile;
extern int lineno;
extern Token Tok;
extern TreeNode *syntaxTree;
extern int scopeAttr;
extern int hasError;

#endif
