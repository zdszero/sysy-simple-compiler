#include "globals.h"
#include "codegen.h"
#include "symtab.h"
#include "util.h"
#include <stdlib.h>

#define REGCOUNT 10

enum {
  SEC_Data, SEC_Text
};

/* 0: busy     1: free */
static int regs[REGCOUNT];
static char *reglist[REGCOUNT] = {"%rax",  "%rbx", "%r10", "%r11", "%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
static char *lreglist[REGCOUNT] = {"%eax", "%ebx", "%r10d", "%r11d", "%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"};
static char *breglist[REGCOUNT] = {"%al", "%bl", "%r10b", "%r11b", "%dil", "%sil", "%dl", "%cl", "%r8b", "%r9b"};
static char *setlist[REGCOUNT] = {"sete", "setne", "setle", "setl", "setge", "setg"};
static char *movcmd[3] = {"movb", "movl", "movq"};
// static char *cmplist[6] = {"jne", "je", "jg", "jge", "jl", "jle"};
enum {RAX, RBX, R10, R11, RDI, RSI, RDX, RCX, R8, R9};
// the next available label
static int curLab = 1;
static int isAssign = 1;
static int comment = 1;
static int curSec = -1;

/********************************
 *  function declaration        *
 ********************************/

/* register management */
static int allocate_reg();
static void freeall_regs();
static void free_reg(int i);
/* section management */
static void cg_section_text();
static void cg_section_data();
/* some help functions */
static void cg_comment(char *msg);
static int getArrayIndex(TreeNode *t);
static int getArrayBase(TreeNode *t);
static int array_glob_dfs(TreeNode *t, int type, int id, int level);
static int array_local_dfs(TreeNode *t, int idx, int type, int id, int level);
static char *getSizeReg(int size, int idx);
static char *getSizeMov(int size);
/* jump */
static void cg_label(int lab);
static int cg_address(TreeNode *t);
/* function */
static void cg_preamble();
void cg_func_preamble(TreeNode *t);
static void cg_func_postamble(TreeNode *t);
/* general statments */
int cg_call(TreeNode *t);
static int cg_loadnum(long value);
static int cg_loadvar(TreeNode *t);
static void cg_type(int type, int val);
static void cg_declaration(TreeNode *t);
static void cg_assign(TreeNode *t, int r);
static int cg_add(int r1, int r2);
static int cg_sub(int r1, int r2);
static int cg_mul(int r1, int r2);
static int cg_div(int r1, int r2);
static int cg_compare(int r1, int r2, int idx);
static void cg_jump(char *how, int curLab);
static int cg_logic_and(int r1, int r2);
static int cg_logic_or(int r1, int r2);
static void cg_return(int r, int type);
static int cg_eval(TreeNode *root);
static void genAST(TreeNode *root);
void genCode(TreeNode *root);

/********************************
 *  function implementation     *
 ********************************/

static void cg_section_text() {
  if (curSec == SEC_Text)
    return;
  fprintf(Outfile, "\t.text\n");
  curSec = SEC_Text;
}

static void cg_section_data() {
  if (curSec == SEC_Data)
    return;
  fprintf(Outfile, "\t.data\n");
  curSec = SEC_Data;
}

static void cg_comment(char *msg) {
  if (comment) {
    fprintf(Outfile, "\t# %s\n", msg);
  }
}

static void cg_label(int lab) {
  fprintf(Outfile, "L%d:\n", lab);
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

void cg_func_preamble(TreeNode *t) {
  int id = t->attr.id;
  char *name = getIdentName(id);
  int offset = getIdentOffset(id);
  fprintf(Outfile,
    "\t.globl\t%s\n"
    "\t.type\t%s, @function\n"
    "%s:\n"
    "\tpushq\t%%rbp\n"
    "\tmovq\t%%rsp, %%rbp\n"
    "\taddq\t$%d, %%rsp\n",
    name, name, name, offset);
}

/* return 0 */
static void cg_func_postamble(TreeNode *t) {
  int offset = getIdentOffset(t->attr.id);
  fprintf(Outfile,
    "\taddq\t$%d, %%rsp\n"
    "\tpopq\t%%rbp\n"
    "\tret\n", -offset
  );
}

int cg_call(TreeNode *t) {
  cg_comment("func call");
  int id = t->children[0]->attr.id;
  char *name = getIdentName(id);
  int pushed[6] = {0};
  TreeNode *tmp;
  int i = 0;   // argument index
  for (tmp = t->children[1]; tmp; tmp = tmp->sibling) {
    int tmpr = cg_eval(tmp);
    // not free
    if (regs[RDI + i] == 0) {
      fprintf(Outfile, "\tpushq\t%s\n", reglist[RDI + i]);
      pushed[i] = 1;
    }
    fprintf(Outfile, "\tmovq\t%s, %s\n", reglist[tmpr], reglist[RDI + i]);
    free_reg(tmpr);
    regs[RDI + i] = 0;
    i++;
  }
  fprintf(Outfile, "\tcall\t%s\n", name);
  for (i = 0; i < 6; i++) {
    if (pushed[i]) {
      fprintf(Outfile, "\tpopq\t%s\n", reglist[RDI + i]);
    } else {
      regs[RDI + i] = 1; // set free after func call
    }
  }
  int r = allocate_reg();
  fprintf(Outfile, "\tmovq\t%%rax, %s\n", reglist[r]);
  return r;
}

/* load the number value into a free register */
static int cg_loadnum(long value) {
  int r = allocate_reg();
  fprintf(Outfile, "\tmovq\t$%ld, %s\n", value, reglist[r]);
  return r;
}

static int cg_loadvar(TreeNode *t) {
  int r;
  int id = t->attr.id;
  int scope = getIdentScope(id);
  if (scope == Scope_Para) {
    int idx = getIdentOffset(id);
    regs[RDI+idx] = 0;
    return RDI+idx;
  } else {
    int kind = getIdentKind(t->attr.id);
    if (kind == Sym_Array && !t->children[0]) {
      r = cg_address(t);
    } else {
      r = allocate_reg();
      int size = getIdentSize(id);
      int tmpr = cg_address(t);
      fprintf(Outfile, "\t%s\t(%s), %s\n", getSizeMov(size), reglist[tmpr], getSizeReg(size, r));
      free_reg(tmpr);
    }
  }
  return r;
}

static void cg_type(int type, int val) {
  switch (type) {
    case T_Char:
      fprintf(Outfile, "\t.byte\t%d\n", val);
      break;
    case T_Int:
      fprintf(Outfile, "\t.long\t%d\n", val);
      break;
    case T_Long:
    case T_Charptr:
    case T_Intptr:
    case T_Longptr:
      fprintf(Outfile, "\t.quad\t%d\n", val);
      break;
    default:
      fprintf(stderr, "Internal Error: undefined data type %d\n", type);
      exit(1);
      break;
  }
}

static int array_glob_dfs(TreeNode *t, int type, int id, int level) {
  if (!t)
    return 0;
  int total = getArrayTotal(id, level);
  int count = 0;
  while (t) {
    if (t->tok == LEVEL) {
      count += array_glob_dfs(t->children[0], type, id, level+1);
    } else {
      cg_type(type, t->attr.val);
      count++;
    }
    t = t->sibling;
  }
  while (count < total) {
    cg_type(type, 0);
    count++;
  }
  return total;
}

static int array_local_dfs(TreeNode *t, int idx, int type, int id, int level) {
  if (!t)
    return idx;
  int size = getTypeSize(type);
  int dim = getArrayDimension(id, level);
  int base = getIdentOffset(id);
  while (t) {
    if (t->tok == LEVEL) {
      idx = array_local_dfs(t->children[0], idx, type, id, level+1);
      idx = (idx + dim) / dim * dim;
    } else {
      int val = t->attr.val;
      int offset = base + idx * size;
      if (size == 1) {
        fprintf(Outfile, "\tmovb\t$%d, %d(%%rbp)\n", val, offset);
      } else if (size == 4) {
        fprintf(Outfile, "\tmovl\t$%d, %d(%%rbp)\n", val, offset);
      } else if (size == 8) {
        fprintf(Outfile, "\tmovq\t$%d, %d(%%rbp)\n", val, offset);
      }
      idx++;
    }
    t = t->sibling;
  }
  return idx - 1;
}

static void cg_declaration(TreeNode *t) {
  int id = t->attr.id;
  int type = t->type;
  int scope = getIdentScope(id);
  int kind = getIdentKind(id);
  if (kind == Sym_Array) {
    int total = getArrayTotal(id, 0);
    if (scope == Scope_Glob) {
      char *name = getIdentName(id);
      fprintf(Outfile, "%s:", name);
      int count = array_glob_dfs(t->children[0], type, id, 0);
      while (count < total) {
        cg_type(type, 0);
        count++;
      }
    } else {
      array_local_dfs(t->children[0], 0, type, id, 0);
    }
  } else if (kind == Sym_Var) {
    if (scope == Scope_Glob) {
      char *name = getIdentName(id);
      fprintf(Outfile, "\t.globl\t%s\n", name);
      fprintf(Outfile, "%s:", name);
      cg_type(type, 0);
    }
    if (t->children[0])
      cg_assign(t, cg_eval(t->children[0]));
  }
}

static void cg_assign(TreeNode *t, int r) {
  int id = t->attr.id;
  int size = getIdentSize(id);
  int tmpr = cg_address(t);
  fprintf(Outfile, "\t%s\t%s, (%s)\n", getSizeMov(size), getSizeReg(size, r), reglist[tmpr]);
  free_reg(r);
  free_reg(tmpr);
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

/* test the result of r1 - r2 */
static int cg_compare(int r1, int r2, int idx) {
  fprintf(Outfile, "\tcmpq\t%s, %s\n", reglist[r2], reglist[r1]);
  if (isAssign) {
    fprintf(Outfile, "\t%s\t%s\n", setlist[idx], breglist[r1]);
    fprintf(Outfile, "\tandq\t$255, %s\n", reglist[r1]);
  }
  free_reg(r2);
  return r1;
}

/* jump to curLab if test failed */
static void cg_jump(char *how, int curLab) {
  if (how)
    fprintf(Outfile, "\t%s\tL%d\n", how, curLab);
  else
    fprintf(Outfile, "\tjmp\tL%d\n", curLab);
}

static int cg_logic_and(int r1, int r2) {
  cg_comment("logic or");
  fprintf(Outfile, "\tandb\t%s, %s\n", breglist[r2], breglist[r1]);
  free_reg(r2);
  return r1;
}

static int cg_logic_or(int r1, int r2) {
  cg_comment("logic and");
  fprintf(Outfile, "\torb\t\t%s, %s\n", breglist[r2], breglist[r1]);
  free_reg(r2);
  return r1;
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

static int getArrayBase(TreeNode *t) {
  int r = allocate_reg();
  int id = t->attr.id;
  int scope = getIdentScope(id);
  if (scope == Scope_Glob) {
    char *name = getIdentName(id);
    fprintf(Outfile, "\tleaq\t%s(%%rip), %s\n", name, reglist[r]);
  } else {
    int startOffset = getIdentOffset(id);
    fprintf(Outfile, "\tleaq\t%d(%%rbp), %s\n", startOffset, reglist[r]);
  }
  return r;
}

/* return the register that holds the idx of current array treenode */
static int getArrayIndex(TreeNode *t) {
    int r = allocate_reg(); // index reg
    int id = t->attr.id;
    int depth = 1;
    fprintf(Outfile, "\tmovq\t$0, %s\n", reglist[r]);
    for (TreeNode *tmp = t->children[0]; tmp; tmp = tmp->sibling) {
      int dim = getArrayTotal(id, depth+1);
      int tmpr = cg_eval(tmp);
      if (dim > 1)
        fprintf(Outfile, "\timulq\t$%d, %s\n", dim, reglist[tmpr]);
      fprintf(Outfile, "\taddq\t%s, %s\n", reglist[tmpr], reglist[r]);
      free_reg(tmpr);
      depth++;
    }
    return r;
}

static char *getSizeReg(int size, int idx) {
  if (size == 1)
    return breglist[idx];
  else if (size == 4)
    return lreglist[idx];
  else
    return reglist[idx];
}

static char *getSizeMov(int size) {
  return movcmd[size / 4];
}

static int cg_address(TreeNode *t) {
  int id = t->attr.id;
  int scope = getIdentScope(id);
  int kind = getIdentKind(id);
  int type = getIdentType(id);
  int size = getTypeSize(type);
  int r;
  if (kind == Sym_Array) {
    r = getArrayBase(t);
    if (t->children[0]) {
      int ir = getArrayIndex(t);
      fprintf(Outfile, "\tleaq\t(%s,%s,%d), %s\n", reglist[r], reglist[ir], size, reglist[r]);
      free_reg(ir);
    }
  } else if (kind == Sym_Var) {
    r = allocate_reg();
    if (scope == Scope_Glob) {
      char *name = getIdentName(id);
      fprintf(Outfile, "\tleaq\t%s(%%rip), %s\n", name, reglist[r]);
    } else if (scope == Scope_Local) {
      int offset = getIdentOffset(id);
      fprintf(Outfile, "\tleaq\t%d(%%rbp), %s\n", offset, reglist[r]);
    }
  } else {
    fprintf(stderr, "Internal Error: parameter should not be here\n");
    exit(1);
  }
  return r;
}

static int cg_eval(TreeNode *root) {
  if (root->tok == CALL) {
    return cg_call(root);
  } else if (root->tok == ASTERISK) {
    cg_comment("dereference");
    int r = cg_eval(root->children[0]);
    fprintf(Outfile, "\tmovq\t(%s), %s\n", reglist[r], reglist[r]);
    return r;
  } else if (root->tok == AMPERSAND) {
    cg_comment("get address");
    return cg_address(root->children[0]);
  } else if (root->tok == IDENT) {
      return cg_loadvar(root);
  }
  int leftreg, rightreg;
  /* recursion for both left and right */
  if (root->children[0])
    leftreg = cg_eval(root->children[0]);
  if (root->children[1])
    rightreg = cg_eval(root->children[1]);
  switch (root->tok) {
    case AND:
      return cg_logic_and(leftreg, rightreg);
    case OR:
      return cg_logic_or(leftreg, rightreg);
    case NUM:
      return cg_loadnum(root->attr.val);
    case CH:
      return cg_loadnum(((int) root->attr.ch));
    case EQ:
    case NE:
    case GT:
    case GE:
    case LT:
    case LE:
      return cg_compare(leftreg, rightreg, root->tok-EQ);
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
  if (root->tok == DECL && getIdentScope(root->children[0]->attr.id) == Scope_Glob)
    cg_section_data();
  else
    cg_section_text();
  int r;
  int tmplab, tmplab2;
  switch (root->tok) {
    case FUNC:
      cg_comment("function preamble");
      cg_func_preamble(root->children[0]);
      genAST(root->children[1]);
      cg_comment("function postamble");
      cg_func_postamble(root->children[0]);
      break;
    case DECL:
      for (TreeNode *t = root->children[0]; t != NULL; t = t->sibling) {
        cg_declaration(t);
      }
      break;
    case ASSIGN:
      cg_comment("assign");
      isAssign = 1;
      r = cg_eval(root->children[1]);
      cg_assign(root->children[0], r);
      break;
    case IF:
      cg_comment("if statement");
      tmplab = curLab++;
      tmplab2 = curLab++;
      r = cg_eval(root->children[0]);
      fprintf(Outfile, "\tcmpq\t$1, %s\n", reglist[r]);
      cg_jump("je", tmplab);
      free_reg(r);
      cg_comment("else branch");
      if (root->children[2]) {
        genAST(root->children[2]);
      }
      cg_jump(NULL, tmplab2);
      cg_label(tmplab);
      cg_comment("if branch");
      tmplab = curLab++;
      genAST(root->children[1]);
      cg_label(tmplab2);
      break;
    case WHILE:
      cg_comment("while");
      tmplab = curLab++;
      tmplab2 = curLab++;
      cg_label(tmplab);
      r = cg_eval(root->children[0]);
      fprintf(Outfile, "\tcmpq\t$1, %s\n", reglist[r]);
      cg_jump("jne", tmplab2);
      free_reg(r);
      cg_comment("loop body");
      genAST(root->children[1]);
      cg_jump(NULL, tmplab);
      cg_comment("end of while");
      cg_label(tmplab2);
      break;
    case GLUE:
      genAST(root->children[0]);
      genAST(root->children[1]);
      break;
    case RETURN:
      cg_comment("return");
      cg_return(cg_eval(root->children[0]), root->children[0]->type);
      break;
    case CALL:
      free_reg(cg_call(root));
      break;
    default:
      fprintf(stderr, "Internal Error: unknown sibling type %d\n", root->tok);
      exit(1);
  }
  genAST(root->sibling);
}

void genCode(TreeNode *root) {
  cg_preamble();
  genAST(syntaxTree);
}
