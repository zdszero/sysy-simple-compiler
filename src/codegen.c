#include "globals.h"
#include "codegen.h"
#include "symtab.h"
#include <stdlib.h>

#define REGCOUNT 12

/* 0: busy     1: free */
static int regs[REGCOUNT];
static char *reglist[REGCOUNT] = {"%rax", "%rbx", "%rcx", "%rdx", "%rsi", "%rdi", "%r8", "%r9", "%r10", "%r11"};
static char *lreglist[REGCOUNT] = {"%eax", "%ebx", "%ecx", "%edx", "%esi", "%edi", "%r8d", "%r9d", "%r10d", "%r11d"};
static char *breglist[REGCOUNT] = {"%al", "%bl", "%cl", "%dl", "%sil", "%dil", "%r8b", "%r9b", "%r10b", "%r11b"};
static char *setlist[REGCOUNT] = {"sete", "setne", "setle", "setl", "setge", "setg"};
static char *cmplist[6] = {"jne", "je", "jg", "jge", "jl", "jle"};
enum {RAX, RBX, RCX, RDX, RSI, RDI, R8, R9, R10, R11};
static int labId = 1;

static void cg_label() {
  fprintf(Outfile, "L%d:\n", labId);
  labId++;
}

static void freeall_regs() {
  for (int i = 0; i < REGCOUNT; i++)
    regs[i] = 1;
}

static int allocate_reg() {
  for (int i = 0; i < REGCOUNT; i++)
    if (regs[i] == 1) {
      regs[i] = 0;
      return i;
    }
  fprintf(stderr, "Out of registers\n");
  exit(1);
}

static void free_reg(int i) {
  if (regs[i] == 1) {
    fprintf(stderr, "Error trying to free register %s which is already free\n", reglist[i]);
    exit(1);
  }
  regs[i] = 1;
}

/* add printint function */
static void cg_preamble() {
  freeall_regs();
}

void cg_func_preamble(char *name) {
  fprintf (Outfile,
    "\t.globl\t%s\n"
    "\t.type\t%s, @function\n"
    "%s:\n"
    "\tpushq\t%%rbp\n"
    "\tmovq\t%%rsp, %%rbp\n",
    name, name, name);
}

/* return 0 */
static void cg_func_postamble() {
  fputs(
    "\tpopq\t%rbp\n"
    "\tret\n",
  Outfile
  );
}

int cg_call(int r, int id) {
  fprintf(Outfile, "\tmovq\t%s, %%rdi\n", reglist[r]);
  fprintf(Outfile, "\tcall\t%s\n", getIdentName(id));
  fprintf(Outfile, "\tmovq\t%%rax, %s\n", reglist[r]);
  return r;
}

/* load the number value into a free register */
static int cg_loadnum(long value) {
  int r = allocate_reg();
  fprintf(Outfile, "\tmovq\t$%ld, %s\n", value, reglist[r]);
  return r;
}

static int cg_loadglob(int id) {
  int r = allocate_reg();
  if (getIdentType(id) == T_Char)
    fprintf(Outfile, "\tmovb\t%s(%%rip), %s\n", getIdentName(id), breglist[r]);
  else if (getIdentType(id) == T_Int)
    fprintf(Outfile, "\tmovl\t%s(%%rip), %s\n", getIdentName(id), lreglist[r]);
  else if (getIdentType(id) == T_Long)
    fprintf(Outfile, "\tmovq\t%s(%%rip), %s\n", getIdentName(id), reglist[r]);
  return r;
}

/*
 * ADD, SUB, MUL, DIV
 * r1 is the left tree node register, r2 is the right's
 * FORMAT: op %r2, %r1
 */
static int cg_add(int r1, int r2) {
  fprintf(Outfile, "\taddq\t%s, %s\n", reglist[r2], reglist[r1]);
  free_reg(r2);
  return r1;
}

static int cg_sub(int r1, int r2) {
  fprintf(Outfile, "\tsubq\t%s, %s\n", reglist[r2], reglist[r1]);
  free_reg(r2);
  return r1;
}

static int cg_mul(int r1, int r2) {
  fprintf(Outfile, "\timulq\t%s, %s\n", reglist[r2], reglist[r1]);
  free_reg(r2);
  return r1;
}

/* r1: divident    r2: divisor */
static int cg_div(int r1, int r2) {
  if (regs[RAX] == 0)
    fprintf(Outfile, "\tpushq\t%%rax\n");
  /* the divident is stored in %rax */
  fprintf(Outfile, "\tmovq\t%s, %%rax\n", reglist[r1]);
  /* change quad byte to octal byte */
  fprintf(Outfile, "\tcqo\n");
  fprintf(Outfile, "\tidivq\t%s\n", reglist[r2]);
  /* store the result back into r1 */
  fprintf(Outfile, "\tmovq\t%%rax, %s\n", reglist[r1]);
  if (regs[RAX] == 0)
    fprintf(Outfile, "\tpopq\t%%rax\n");
  free_reg(r2);
  return r1;
}

static void cg_globsym(int id) {
  if (getIdentType(id) == T_Char)
    fprintf(Outfile, "\t.comm\t%s, 1\n", getIdentName(id));
  else if (getIdentType(id) == T_Int)
    fprintf(Outfile, "\t.comm\t%s, 4\n", getIdentName(id));
  else if (getIdentType(id) == T_Long)
    fprintf(Outfile, "\t.comm\t%s, 8\n", getIdentName(id));
  else {
    fprintf(stderr, "Internal Error: variable %s is not given a type when declaring\n", getIdentName(id));
    exit(1);
  }
}

static void cg_assign(int id, int r) {
  if (getIdentType(id) == T_Char)
    fprintf(Outfile, "\tmovb\t%s, %s(%%rip)\n", breglist[r], getIdentName(id));
  else if (getIdentType(id) == T_Int)
    fprintf(Outfile, "\tmovl\t%s, %s(%%rip)\n", lreglist[r], getIdentName(id));
  else if (getIdentType(id) == T_Long)
    fprintf(Outfile, "\tmovq\t%s, %s(%%rip)\n", reglist[r], getIdentName(id));
  free_reg(r);
}

/* test the result of r1 - r2 */
static int cg_compare(int r1, int r2) {
  fprintf(Outfile, "\tcmpq\t%s, %s\n", reglist[r2], reglist[r1]);
  free_reg(r2);
  return r1;
}

/* jump to labId if test failed */
static void cg_jump(char *how, int labId) {
  if (how)
    fprintf(Outfile, "\t%s\tL%d\n", how, labId);
  else
    fprintf(Outfile, "\tjmp\tL%d\n", labId);
}

/* returned value is stored in register r, switch type according to id */
static void cg_return(int r, int type) {
  switch (type) {
    case T_Char:
      fprintf(Outfile, "\tmovb\t%s, %%al\n", breglist[r]);
      break;
    case T_Int:
      fprintf(Outfile, "\tmovl\t%s, %%eax\n", lreglist[r]);
      break;
    case T_Long:
      fprintf(Outfile, "\tmovq\t%s, %%rax\n", reglist[r]);
      break;
    default:
      fprintf(stderr, "Internal Error: unknown return type %d\n", type);
      exit(1);
  }
}

static int cg_eval(TreeNode *root) {
  int leftreg, rightreg;
  if (root->tok == CALL) {
    int r = cg_eval(root->children[1]);
    return cg_call(r, root->children[0]->attr.id);
  }
  if (root->children[0])
    leftreg = cg_eval(root->children[0]);
  if (root->children[1])
    rightreg = cg_eval(root->children[1]);
  switch (root->tok) {
    case NUM:
      return cg_loadnum(root->attr.val);
    case CH:
      return cg_loadnum(((int) root->attr.ch));
    case IDENT:
      return cg_loadglob(root->attr.id);
    case EQ:
      return cg_compare(leftreg, rightreg);
    case NE:
      return cg_compare(leftreg, rightreg);
    case GT:
      return cg_compare(leftreg, rightreg);
    case GE:
      return cg_compare(leftreg, rightreg);
    case LT:
      return cg_compare(leftreg, rightreg);
    case LE:
      return cg_compare(leftreg, rightreg);
    case PLUS:
      return cg_add(leftreg, rightreg);
    case MINUS:
      return cg_sub(leftreg, rightreg);
    case TIMES:
      return cg_mul(leftreg, rightreg);
    case OVER:
      return cg_div(leftreg, rightreg);
    default:
      fprintf(stderr, "Internal Error in function eval\nUnexpected token: %d\n", root->tok);
      exit(1);
  }
}

/* post traversal */
static void genAST(TreeNode *root) {
  if (!root)
    return;
  if (root->tok == FUNC) {
    cg_func_preamble(getIdentName(root->attr.id));
    genAST(root->children[0]);
    cg_func_postamble();
  } else if (root->tok == DECL) {
    int id = root->children[0]->attr.id;
    cg_globsym(id);
    if (root->children[1])
      cg_assign(id, cg_eval(root->children[1]));
  } else if (root->tok == ASSIGN) {
    int r = cg_eval(root->children[1]);
    int eval_tok = root->children[1]->tok;
    if (eval_tok >= EQ && eval_tok <= GT) {
      fprintf(Outfile, "\t%s\t%s\n",setlist[eval_tok-EQ] , breglist[r]);
      fprintf(Outfile, "\tandq\t$255, %s\n", reglist[r]);
    }
    cg_assign(root->children[0]->attr.id, r);
  } else if (root->tok == IF) {
    int cmpidx = root->children[0]->tok - EQ;
    int r = cg_eval(root->children[0]);
    cg_jump(cmplist[cmpidx], labId);
    free_reg(r);
    genAST(root->children[1]);
    if (root->children[2]) {
      cg_jump(NULL, labId+1);
    }
    cg_label();
    if (root->children[2]) {
      genAST(root->children[2]);
      cg_label();
    }
  } else if (root->tok == WHILE) {
    cg_label();
    int r = cg_eval(root->children[0]);
    int cmpidx = root->children[0]->tok - EQ;
    cg_jump(cmplist[cmpidx], labId);
    free_reg(r);
    genAST(root->children[1]);
    cg_jump(NULL, labId-1);
    cg_label();
  } else if (root->tok == GLUE) {
    genAST(root->children[0]);
    genAST(root->children[1]);
  } else if (root->tok == RETURN) {
    cg_return(cg_eval(root->children[0]), root->children[0]->type);
  } else if (root->tok == CALL) {
    free_reg(cg_call(cg_eval(root->children[1]), root->children[0]->attr.id));
  } else {
    fprintf(stderr, "Internal Error: unknown sibling type %d\n", root->tok);
    exit(1);
  }
  genAST(root->sibling);
}

void genCode(TreeNode *root) {
  cg_preamble();
  genAST(syntaxTree);
}
