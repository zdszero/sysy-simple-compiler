#include "globals.h"
#include "codegen.h"
#include "symtab.h"
#include <stdlib.h>

#define REGCOUNT 12

enum {
  SEC_Data, SEC_Text
};

/* 0: busy     1: free */
static int regs[REGCOUNT];
static char *reglist[REGCOUNT] = {"%rax", "%rbx", "%rcx", "%rdx", "%rsi", "%rdi", "%r8", "%r9", "%r10", "%r11"};
static char *lreglist[REGCOUNT] = {"%eax", "%ebx", "%ecx", "%edx", "%esi", "%edi", "%r8d", "%r9d", "%r10d", "%r11d"};
static char *breglist[REGCOUNT] = {"%al", "%bl", "%cl", "%dl", "%sil", "%dil", "%r8b", "%r9b", "%r10b", "%r11b"};
static char *setlist[REGCOUNT] = {"sete", "setne", "setle", "setl", "setge", "setg"};
// static char *cmplist[6] = {"jne", "je", "jg", "jge", "jl", "jle"};
enum {RAX, RBX, RCX, RDX, RSI, RDI, R8, R9, R10, R11};
static int labId = 1;
static int isAssign = 1;
static int comment = 1;
static int curSec = -1;

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
  cg_comment("call function");
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

static int getTypeSize(int type) {
  switch (type) {
    case T_Char:
      return 1;
    case T_Int:
      return 4;
    case T_Long:
    case T_Charptr:
    case T_Intptr:
    case T_Longptr:
      return 8;
    default:
      fprintf(stderr, "Internal Error: unrecgnozied type %d\n", type);
      exit(1);
  }
}

static int cg_loadglob(TreeNode *t) {
  cg_comment("load global");
  int id = t->attr.id;
  int r = allocate_reg();
  int size = getTypeSize(getIdentType(id));
  char *name = getIdentName(id);
  if (t->children[0]) {
    int idx = 0, depth = 1;
    TreeNode *tmp = t->children[0];
    while (tmp) {
      idx += tmp->attr.val * getArrayTotal(id, depth+1);
      tmp = tmp->sibling;
      depth++;
    }
    int r1 = allocate_reg(), r2 = allocate_reg();
    fprintf(Outfile, "\tleaq\t%s(%%rip), %s\n", name, reglist[r1]);
    fprintf(Outfile, "\tmovq\t$%d, %s\n", idx, reglist[r2]);
    if (size == 1) {
      fprintf(Outfile, "\tmovb\t(%s,%s,%d), %s\n", reglist[r1], reglist[r2], size, breglist[r]);
    } else if (size == 4) {
      fprintf(Outfile, "\tmovl\t(%s,%s,%d), %s\n", reglist[r1], reglist[r2], size, lreglist[r]);
    } else if (size == 8) {
      fprintf(Outfile, "\tmovq\t(%s,%s,%d), %s\n", reglist[r1], reglist[r2], size, reglist[r]);
    }
    free_reg(r1);
    free_reg(r2);
  } else {
    if (size == 1) {
      fprintf(Outfile, "\tmovb\t%s(%%rip), %s\n", name, breglist[r]);
    } else if (size == 4) {
      fprintf(Outfile, "\tmovl\t%s(%%rip), %s\n", name, lreglist[r]);
    } else if (size == 8) {
      fprintf(Outfile, "\tmovq\t%s(%%rip), %s\n", name, reglist[r]);
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

static void cg_glob_var(int id) {
  int type = getIdentType(id);
  char *name = getIdentName(id);
  fprintf(Outfile, "\t.globl\t%s\n", name);
  fprintf(Outfile, "%s:", name);
  cg_type(type, 0);
}

static int array_dfs(TreeNode *t, int type, int id, int level) {
  if (!t)
    return 0;
  int total = getArrayTotal(id, level);
  int count = 0;
  while (t) {
    if (t->tok == LEVEL) {
      count += array_dfs(t->children[0], type, id, level+1);
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

static void cg_glob_array(TreeNode *root) {
  int id = root->attr.id;
  char *name = getIdentName(id);
  fprintf(Outfile, "%s:", name);
  int type = root->type;
  int count = array_dfs(root->children[0], type, id, 0);
  int total = getArrayTotal(id, 1);
  while (count < total) {
    cg_type(type, 0);
    count++;
  }
}

static void cg_assign(int id, int r) {
  int type = getIdentType(id);
  char *name = getIdentName(id);
  switch (type) {
    case T_Char:
      fprintf(Outfile, "\tmovb\t%s, %s(%%rip)\n", breglist[r], name);
      break;
    case T_Int:
      fprintf(Outfile, "\tmovl\t%s, %s(%%rip)\n", lreglist[r], name);
      break;
    case T_Long:
    case T_Charptr:
    case T_Intptr:
    case T_Longptr:
      fprintf(Outfile, "\tmovq\t%s, %s(%%rip)\n", reglist[r], name);
      break;
    default:
      fprintf(stderr, "Internal Error: variable %s is not given a type when declaring\n", getIdentName(id));
      exit(1);
  }
  free_reg(r);
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

/* jump to labId if test failed */
static void cg_jump(char *how, int labId) {
  if (how)
    fprintf(Outfile, "\tje\tL%d\n", labId);
  else
    fprintf(Outfile, "\tjmp\tL%d\n", labId);
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

static int cg_eval(TreeNode *root) {
  if (root->tok == CALL) {
    int r = cg_eval(root->children[1]);
    return cg_call(r, root->children[0]->attr.id);
  } else if (root->tok == ASTERISK) {
    cg_comment("dereference");
    int r = cg_eval(root->children[0]);
    fprintf(Outfile, "\tmovq\t(%s), %s\n", reglist[r], reglist[r]);
    return r;
  } else if (root->tok == AMPERSAND) {
    cg_comment("get address");
    int r = allocate_reg();
    fprintf(Outfile, "\tleaq\t%s(%%rip), %s\n", getIdentName(root->children[0]->attr.id), reglist[r]);
    return r;
  } else if (root->tok == IDENT) {
      return cg_loadglob(root);
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
  if (root->tok == DECL)
    cg_section_data();
  else
    cg_section_text();
  int r;
  switch (root->tok) {
    case FUNC:
      cg_comment("function preamble");
      cg_func_preamble(getIdentName(root->children[0]->attr.id));
      genAST(root->children[1]);
      cg_comment("function postamble");
      cg_func_postamble();
      break;
    case DECL:
      cg_comment("declaration begin");
      for (TreeNode *t = root->children[0]; t != NULL; t = t->sibling) {
        int kind = getIdentKind(t->attr.id);
        if (kind == Sym_Var) {
          cg_glob_var(t->attr.id);
          if (t->children[0])
            cg_assign(t->attr.id, cg_eval(t->children[0]));
        } else if (kind == Sym_Array) {
          cg_glob_array(t);
        } else {
          fprintf(stderr, "Internal Error: wrong kind after decl codgen\n");
        }
      }
      cg_comment("declaration end");
      break;
    case ASSIGN:
      cg_comment("assign");
      isAssign = 1;
      r = cg_eval(root->children[1]);
      cg_assign(root->children[0]->attr.id, r);
      break;
    case IF:
      cg_comment("if statement");
      r = cg_eval(root->children[0]);
      cg_comment("fail jump");
      fprintf(Outfile, "\tcmpq\t$0, %s\n", reglist[r]);
      cg_jump("je", labId);
      free_reg(r);
      cg_comment("if branch");
      genAST(root->children[1]);
      if (root->children[2]) {
        cg_jump(NULL, labId+1);
      }
      cg_comment("else branch");
      cg_label();
      if (root->children[2]) {
        genAST(root->children[2]);
        cg_label();
      }
      break;
    case WHILE:
      cg_comment("while");
      cg_label();
      r = cg_eval(root->children[0]);
      fprintf(Outfile, "\tcmpq\t$0, %s\n", reglist[r]);
      cg_comment("jump out of while");
      cg_jump("je", labId);
      free_reg(r);
      genAST(root->children[1]);
      cg_comment("loop jump");
      cg_jump(NULL, labId-1);
      cg_label();
      break;
    case GLUE:
      genAST(root->children[0]);
      genAST(root->children[1]);
      break;
    case RETURN:
      cg_return(cg_eval(root->children[0]), root->children[0]->type);
      break;
    case CALL:
      free_reg(cg_call(cg_eval(root->children[1]), root->children[0]->attr.id));
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
