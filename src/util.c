#include "util.h"
#include "globals.h"
#include "symtab.h"
#include <stdlib.h>

static TreeNode *tmp = NULL;

TreeNode *mkTreeNode(int tok) {
  TreeNode *t = (TreeNode *) malloc(sizeof(TreeNode));
  t->tok = tok;
  for (int i = 0; i < CHILDNUM; i++) {
    t->children[i] = NULL;
    t->type = T_None;
  }
  t->sibling = NULL;
  return t;
}

struct record {
  int val;
  char *name;
};

struct record nameMap[] = {
  {FUNC,   "func"   },
  {DECL,   "decl"   },
  {VOID,   "void"   },
  {CHAR,   "char"   },
  {INT,    "int"    },
  {LONG,   "long"   },
  {CH,     "ch"     },
  {RETURN, "return" },
  {SEMI,   "semi"   },
  {COMMA,  "comma"  },
  {IF,     "if"     },
  {ELSE,   "else"   },
  {WHILE,  "while"  },
  {FOR,    "for"    },
  {GLUE,   "glue"   },
  {LEVEL,  "level"  },
  {CALL,   "call"   },
  {LP,     "lp"     },
  {RP,     "rp"     },
  {LC,     "lc"     },
  {RC,     "rc"     },
  {LS,     "ls"     },
  {RS,     "rs"     },
  {ASSIGN, "assign" },
  {OR,     "or"     },
  {AND,    "and"    },
  {AMPERSAND, "&"   },
  {ASTERISK,  "*"   },
  {EQ,     "eq"     },
  {NE,     "ne"     },
  {GT,     "gt"     },
  {GE,     "ge"     },
  {LT,     "lt"     },
  {LE,     "le"     },
  {PLUS,   "plus"   },
  {MINUS,  "minus"  },
  {TIMES,  "times"  },
  {OVER,   "over"   },
  {YYEOF,  "eof"    }
};

char *getTokenName(int val) {
  int n = sizeof(nameMap) / sizeof(struct record);
  for (int i = 0; i < n; i++) {
    if (val == nameMap[i].val)
      return nameMap[i].name;
  }
  return NULL;
}

static void printDimension(DimRec *dim) {
  putchar('(');
  for (DimRec *p = dim; p; p = p->next) {
    printf("%d,", p->dim);
  }
  putchar(')');
}

void printToken(int tok) {
  if (tok == IDENT) {
    if (tmp) {
      printf("id: %s->", getIdentName(tmp->attr.id));
      switch (getIdentKind(tmp->attr.id)) {
        case Sym_Func:
          printf("func");
          break;
        case Sym_Var:
          printf("var");
          break;
        case Sym_Array:
          printf("array");
          printDimension(getIdentDim(tmp->attr.id));
          break;
        default:
          printf("error");
          break;
      }
    } else {
      printf("id");
    }
    putchar('\n');
  } else if (tok == NUM) {
    if (tmp)
      printf("num: %ld\n", tmp->attr.val);
    else
      printf("num\n");
  } else {
    char *name = getTokenName(tok);
    if (name) {
      printf("%s\n", name);
    } else {
      printf("error\n");
    }
  }
}

void printTree(TreeNode *t, int dep) {
  if (!t)
    return;
  tmp = t;
  for (int i = 0; i < dep; i++)
    printf("| ");
  printToken(t->tok);
  for (int i = 0; i < CHILDNUM; i++) {
    printTree(t->children[i], dep + 1);
  }
  printTree(t->sibling, dep);
}

