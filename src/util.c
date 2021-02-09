#include "globals.h"
#include "symtab.h"
#include "util.h"
#include <stdlib.h>

static TreeNode *tmp = NULL;

TreeNode *mkTreeNode(int tok) {
  TreeNode *t = (TreeNode *) malloc(sizeof(TreeNode));
  t->tok = tok;
  for (int i = 0; i < CHILDNUM; i++)
    t->children[i] = NULL;
  t->sibling = NULL;
  return t;
}

void printToken(int tok) {
  switch (tok) {
    case IDENT:
      {
        if (tmp)
          fprintf(Outfile, "id: %s\n", getIdent(tmp->attr.id));
        else
          fprintf(Outfile, "id\n");
      }
      break;
    case NUM:
      {
        if (tmp)
          fprintf(Outfile, "num: %d\n", tmp->attr.val);
        else
          fprintf(Outfile, "num\n");
      }
      break;
    case SEMI:
      fprintf(Outfile, "semi\n");
      break;
    case INT:
      fprintf(Outfile, "int\n");
      break;
    case IF:
      fprintf(Outfile, "if\n");
      break;
    case ELSE:
      fprintf(Outfile, "else\n");
      break;
    case LP:
      fprintf(Outfile, "lp\n");
      break;
    case RP:
      fprintf(Outfile, "rp\n");
      break;
    case LC:
      fprintf(Outfile, "lc\n");
      break;
    case RC:
      fprintf(Outfile, "rc\n");
      break;
    case ASSIGN:
      fprintf(Outfile, "assign\n");
      break;
    case PRINT:
      fprintf(Outfile, "print\n");
      break;
    case EQ:
      fprintf(Outfile, "eq\n");
      break;
    case NE:
      fprintf(Outfile, "ne\n");
      break;
    case GT:
      fprintf(Outfile, "gt\n");
      break;
    case GE:
      fprintf(Outfile, "ge\n");
      break;
    case LT:
      fprintf(Outfile, "lt\n");
      break;
    case LE:
      fprintf(Outfile, "le\n");
      break;
    case PLUS:
      fprintf(Outfile, "plus\n");
      break;
    case MINUS:
      fprintf(Outfile, "minus\n");
      break;
    case TIMES:
      fprintf(Outfile, "times\n");
      break;
    case OVER:
      fprintf(Outfile, "over\n");
      break;
    case YYEOF:
      fprintf(Outfile, "eof\n");
      break;
    default:
      fprintf(Outfile, "error\n");
      break;
  }
}

void printTree(TreeNode *t, int dep) {
  if (!t)
    return;
  tmp = t;
  for (int i = 0; i < dep; i++)
    fprintf(Outfile, "| ");
  printToken(t->tok);
  for (int i = 0; i < CHILDNUM; i++) {
    printTree(t->children[i], dep+1);
  }
  printTree(t->sibling, dep);
}
