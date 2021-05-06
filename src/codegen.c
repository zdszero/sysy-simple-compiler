#include "codegen.h"
#include "globals.h"
#include "symtab.h"
#include "util.h"
#include <stdlib.h>

#define REGCOUNT 10

enum { SEC_Data, SEC_Text };

// 0：被占用    1：空闲
static int Regs[REGCOUNT];
static char *RegList[REGCOUNT] = {"%rax", "%rbx", "%r10", "%r11", "%rdi",
                                  "%rsi", "%rdx", "%rcx", "%r8",  "%r9"};
static char *LRegList[REGCOUNT] = {"%eax", "%ebx", "%r10d", "%r11d", "%edi",
                                   "%esi", "%edx", "%ecx",  "%r8d",  "%r9d"};
static char *BRegList[REGCOUNT] = {"%al",  "%bl", "%r10b", "%r11b", "%dil",
                                   "%sil", "%dl", "%cl",   "%r8b",  "%r9b"};
static char *SetList[REGCOUNT] = {"sete", "setne", "setle",
                                  "setl", "setge", "setg"};
static char *MovList[3] = {"movb", "movl", "movq"};
static char *CmpList[3] = {"cmpb", "cmpl", "cmpq"};
static char *JmpList[6] = {"jne", "je", "jg", "jge", "jl", "jle"};
enum { RAX, RBX, R10, R11, RDI, RSI, RDX, RCX, R8, R9 };
// 下一个可以使用的Label编号
static int CurLab = 1;
// 当前程序段：data，text
static int CurSec = -1;
// 当前变量的大小
static int VarSize = 8;
// 是否存储cmp的结果到寄存器
static int IsAssign = 1;
// 是否在生成的汇编代码中使用注释
static int CommentFlag = 1;
// 暂存的函数结点
static TreeNode *TmpFn;

/********************************
 *  function declaration        *
 ********************************/

static int allocate_reg();
static void freeall_regs();
static void free_reg(int i);
static void cg_section_text();
static void cg_section_data();
static void cg_comment(char *msg);
static int array_glob_dfs(TreeNode *t, int type, int id, int level);
static int array_local_dfs(TreeNode *t, int idx, int type, int id, int level);
static char *getSizeReg(int size, int idx);
static char *getSizeMov(int size);
static char *getSizeCmp(int size);
static void cg_label(int lab);
static int cg_address(TreeNode *t);
static void cg_preamble();
static void cg_func_preamble(TreeNode *t);
static void cg_func_load_params(TreeNode *t);
static void cg_func_postamble(TreeNode *t);
static int cg_call(TreeNode *t);
static int cg_loadnum(long value);
static int cg_loadvar(TreeNode *t);
static void cg_type(int type, int val);
static void cg_declaration(TreeNode *t);
static void cg_assign(TreeNode *t, int r);
static int cg_add(int r1, int r2);
static int cg_sub(int r1, int r2);
static int cg_mul(int r1, int r2);
static int cg_div(int r1, int r2);
static int cg_mod(int r1, int r2);
static int cg_compare(int r1, int r2, int idx);
static void cg_jump(char *how, int lab);
static int cg_logic_and(int r1, int r2);
static int cg_logic_or(int r1, int r2);
static void cg_return(TreeNode *t);
static int cg_eval(TreeNode *root);
static void genAST(TreeNode *root);
void genCode(TreeNode *root);

/********************************
 *  function implementation     *
 ********************************/

static void cg_section_text() {
  if (CurSec == SEC_Text)
    return;
  fprintf(Outfile, "\t.text\n");
  CurSec = SEC_Text;
}

static void cg_section_data() {
  if (CurSec == SEC_Data)
    return;
  fprintf(Outfile, "\t.data\n");
  CurSec = SEC_Data;
}

static void cg_comment(char *msg) {
  if (CommentFlag) {
    fprintf(Outfile, "\t# %s\n", msg);
  }
}

static void cg_label(int lab) { fprintf(Outfile, ".L%d:\n", lab); }

static void freeall_regs() {
  for (int i = 0; i < REGCOUNT; i++)
    Regs[i] = 1;
}

static int allocate_reg() {
  for (int i = 0; i < REGCOUNT; i++)
    if (Regs[i] == 1) {
      Regs[i] = 0;
      return i;
    }
  fprintf(stderr, "Out of registers\n");
  exit(1);
}

static void free_reg(int i) {
  if (Regs[i] == 1) {
    fprintf(stderr, "Error trying to free register %s which is already free\n",
            RegList[i]);
    exit(1);
  }
  Regs[i] = 1;
}

static void cg_preamble() { freeall_regs(); }

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

static void cg_func_load_params(TreeNode *t) {
  int cnt = 0;
  for (TreeNode *tmp = t; tmp; tmp = tmp->sibling) {
    int id = tmp->attr.id;
    int offset = getIdentOffset(id);
    int size = getIdentSize(tmp);
    fprintf(Outfile, "\t%s\t%s, %d(%%rbp)\n", getSizeMov(size),
            getSizeReg(size, RDI + cnt), offset);
    cnt++;
  }
}

/* return 0 */
static void cg_func_postamble(TreeNode *t) {
  int offset = getIdentOffset(t->attr.id);
  fprintf(Outfile,
          "\taddq\t$%d, %%rsp\n"
          "\tpopq\t%%rbp\n"
          "\tret\n",
          -offset);
}

static int cg_call(TreeNode *t) {
  cg_comment("func call");
  int id = t->children[0]->attr.id;
  int type = t->children[0]->type;
  // 如果返回类型不为void，那么用%rax来传递返回值
  if (type != T_Void)
    Regs[RAX] = 0;
  char *name = getIdentName(id);
  TreeNode *tmp = t->children[1];
  int i = 0;
  // 依次传递函数所需要的参数
  while (tmp) {
    int tmpr = cg_eval(tmp);
    int size = getFuncParaSize(id, i);
    fprintf(Outfile, "\t%s\t%s, %s\n", getSizeMov(size), getSizeReg(size, tmpr),
            getSizeReg(size, RDI + i));
    free_reg(tmpr);
    tmp = tmp->sibling;
    i++;
  }
  fprintf(Outfile, "\tcall\t%s\n", name);
  // 传递完返回值后恢复%rax的空闲状态
  if (type != T_Void) {
    int r = allocate_reg();
    fprintf(Outfile, "\tmovq\t%%rax, %s\n", RegList[r]);
    Regs[RAX] = 1;
    return r;
  }
  // 当处理函数调用语句时，不需要返回值
  return -1;
}

/* load the number value into a free register */
static int cg_loadnum(long value) {
  int r = allocate_reg();
  fprintf(Outfile, "\tmovq\t$%ld, %s\n", value, RegList[r]);
  return r;
}

static int cg_loadvar(TreeNode *t) {
  int kind = getIdentKind(t->attr.id);
  int r;
  if (kind == Sym_Array && !t->children[0]) {
    VarSize = 64;
    int r = cg_address(t);
    return r;
  } else {
    int size = getIdentSize(t);
    VarSize = size;
    r = cg_address(t);
    fprintf(Outfile, "\t%s\t(%s), %s\n", getSizeMov(size), RegList[r],
            getSizeReg(size, r));
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
      count += array_glob_dfs(t->children[0], type, id, level + 1);
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
      idx = array_local_dfs(t->children[0], idx, type, id, level + 1);
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
    type = valueAt(type);
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
  cg_comment("assign");
  int size = getIdentSize(t);
  int scope = getIdentScope(t->attr.id);
  int tmpr = cg_address(t);
  if (t->children[0] || scope != Scope_Para) {
    fprintf(Outfile, "\t%s\t%s, (%s)\n", getSizeMov(size), getSizeReg(size, r),
            RegList[tmpr]);
    free_reg(tmpr);
  } else {
    fprintf(Outfile, "\t%s\t%s, %s\n", getSizeMov(size), getSizeReg(size, r),
            getSizeReg(size, tmpr));
  }
  free_reg(r);
}

/*
 * ADD, SUB, MUL, DIV
 * r1 is the left tree node register, r2 is the right's
 * FORMAT: op %r2, %r1
 */
static int cg_add(int r1, int r2) {
  fprintf(Outfile, "\taddq\t%s, %s\n", RegList[r2], RegList[r1]);
  free_reg(r2);
  return r1;
}

static int cg_sub(int r1, int r2) {
  fprintf(Outfile, "\tsubq\t%s, %s\n", RegList[r2], RegList[r1]);
  free_reg(r2);
  return r1;
}

static int cg_mul(int r1, int r2) {
  fprintf(Outfile, "\timulq\t%s, %s\n", RegList[r2], RegList[r1]);
  free_reg(r2);
  return r1;
}

// r1：被除数，r2：除数
static int cg_div(int r1, int r2) {
  // 如果RAX、RBX被占用且不是用于除法，保存它们的值。
  if (Regs[RAX] == 0 && r1 != RAX)
    fprintf(Outfile, "\tpushq\t%%rax\n");
  if (Regs[RDX] == 0 && r2 != RDX)
    fprintf(Outfile, "\tpushq\t%%rdx\n");
  fprintf(Outfile, "\tmovq\t%s, %%rax\n", RegList[r1]);
  fprintf(Outfile, "\tcqo\n");
  fprintf(Outfile, "\tidivq\t%s\n", RegList[r2]);
  fprintf(Outfile, "\tmovq\t%%rax, %s\n", RegList[r1]);
  if (Regs[RDX] == 0 && r2 != RDX)
    fprintf(Outfile, "\tpopq\t%%rdx\n");
  if (Regs[RAX] == 0 && r1 != RAX)
    fprintf(Outfile, "\tpopq\t%%rax\n");
  free_reg(r2);
  return r1;
}

static int cg_mod(int r1, int r2) {
  // 如果RAX、RBX被占用且不是用于除法，保存它们的值。
  if (Regs[RAX] == 0 && r1 != RAX)
    fprintf(Outfile, "\tpushq\t%%rax\n");
  if (Regs[RDX] == 0 && r2 != RDX)
    fprintf(Outfile, "\tpushq\t%%rdx\n");
  fprintf(Outfile, "\tmovq\t%s, %%rax\n", RegList[r1]);
  fprintf(Outfile, "\tcqo\n");
  fprintf(Outfile, "\tidivq\t%s\n", RegList[r2]);
  fprintf(Outfile, "\tmovq\t%%rdx, %s\n", RegList[r1]);
  if (Regs[RDX] == 0 && r2 != RDX)
    fprintf(Outfile, "\tpopq\t%%rdx\n");
  if (Regs[RAX] == 0 && r1 != RAX)
    fprintf(Outfile, "\tpopq\t%%rax\n");
  free_reg(r2);
  return r1;
}

/* test the result of r1 - r2 */
static int cg_compare(int r1, int r2, int idx) {
  fprintf(Outfile, "\t%s\t%s, %s\n", getSizeCmp(VarSize),
          getSizeReg(VarSize, r2), getSizeReg(VarSize, r1));
  if (IsAssign) {
    fprintf(Outfile, "\t%s\t%s\n", SetList[idx], BRegList[r1]);
    fprintf(Outfile, "\tandq\t$255, %s\n", RegList[r1]);
  }
  free_reg(r2);
  return r1;
}

static void cg_jump(char *how, int lab) {
  if (how)
    fprintf(Outfile, "\t%s\t.L%d\n", how, lab);
  else
    fprintf(Outfile, "\tjmp\t.L%d\n", lab);
}

static int cg_logic_and(int r1, int r2) {
  cg_comment("logic or");
  fprintf(Outfile, "\tandb\t%s, %s\n", BRegList[r2], BRegList[r1]);
  free_reg(r2);
  return r1;
}

static int cg_logic_or(int r1, int r2) {
  cg_comment("logic and");
  fprintf(Outfile, "\torb\t\t%s, %s\n", BRegList[r2], BRegList[r1]);
  free_reg(r2);
  return r1;
}

/* returned value is stored in register r, switch type according to id */
static void cg_return(TreeNode *t) {
  if (t->children[0]) {
    int r = cg_eval(t->children[0]);
    fprintf(Outfile, "\tmovq\t%s, %%rax\n", RegList[r]);
    free_reg(r);
  }
  cg_comment("function postamble");
  cg_func_postamble(TmpFn);
}

static char *getSizeReg(int size, int idx) {
  if (size == 1)
    return BRegList[idx];
  else if (size == 4)
    return LRegList[idx];
  else
    return RegList[idx];
}

static char *getSizeMov(int size) { return MovList[size / 4]; }

static char *getSizeCmp(int size) { return CmpList[size / 4]; }

static int cg_address(TreeNode *t) {
  int id = t->attr.id;
  int scope = getIdentScope(id);
  int kind = getIdentKind(id);
  int r;
  if (kind == Sym_Array) {
    int size;
    r = allocate_reg();
    // 获取数组的起始地址并且存储在寄存器r中
    if (scope == Scope_Glob) {
      char *name = getIdentName(id);
      fprintf(Outfile, "\tleaq\t%s(%%rip), %s\n", name, RegList[r]);
    } else if (scope == Scope_Para) {
      int offset = getIdentOffset(id);
      TreeNode *tmp = t->children[0];
      t->children[0] = NULL;
      size = getIdentSize(t);
      fprintf(Outfile, "\t%s\t%d(%%rbp), %s\n", getSizeMov(size), offset,
              getSizeReg(size, r));
      t->children[0] = tmp;
    } else {
      int offset = getIdentOffset(id);
      fprintf(Outfile, "\tleaq\t%d(%%rbp), %s\n", offset, RegList[r]);
    }
    size = getIdentSize(t);
    // 如果访问的是某个特定元素比如a[3]
    if (t->children[0]) {
      // 获取等价一维数组的下标并且存储在寄存器ir(index register)中
      int ir = allocate_reg();
      // 函数调用目前只支持一维数组
      if (scope == Scope_Para) {
        int tmpr = cg_eval(t->children[0]);
        fprintf(Outfile, "\t%s\t%s, %s\n", getSizeMov(size),
                getSizeReg(size, tmpr), getSizeReg(size, ir));
        free_reg(tmpr);
      } else {
        int depth = 1;
        fprintf(Outfile, "\tmovq\t$0, %s\n", RegList[ir]);
        for (TreeNode *tmp = t->children[0]; tmp; tmp = tmp->sibling) {
          int dim = getArrayTotal(id, depth + 1);
          int tmpr = cg_eval(tmp);
          if (dim > 1)
            fprintf(Outfile, "\timulq\t$%d, %s\n", dim, RegList[tmpr]);
          fprintf(Outfile, "\taddq\t%s, %s\n", RegList[tmpr], RegList[ir]);
          free_reg(tmpr);
          depth++;
        }
      }
      fprintf(Outfile, "\tleaq\t(%s,%s,%d), %s\n", RegList[r], RegList[ir],
              size, RegList[r]);
      free_reg(ir);
    }
  } else if (kind == Sym_Var) {
    r = allocate_reg();
    if (scope == Scope_Glob) {
      char *name = getIdentName(id);
      fprintf(Outfile, "\tleaq\t%s(%%rip), %s\n", name, RegList[r]);
    } else {
      int offset = getIdentOffset(id);
      fprintf(Outfile, "\tleaq\t%d(%%rbp), %s\n", offset, RegList[r]);
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
    fprintf(Outfile, "\tmovq\t(%s), %s\n", RegList[r], RegList[r]);
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
    return cg_loadnum(((int)root->attr.ch));
  case EQ:
  case NE:
  case GT:
  case GE:
  case LT:
  case LE:
    return cg_compare(leftreg, rightreg, root->tok - EQ);
    VarSize = 64;
  case PLUS:
    return cg_add(leftreg, rightreg);
  case MINUS:
    return cg_sub(leftreg, rightreg);
  case TIMES:
    return cg_mul(leftreg, rightreg);
  case OVER:
    return cg_div(leftreg, rightreg);
  case MOD:
    return cg_mod(leftreg, rightreg);
  default:
    fprintf(stderr, "Internal Error in function eval\nUnexpected token: %d\n",
            root->tok);
    exit(1);
  }
}

/* post traversal */
static void genAST(TreeNode *root) {
  if (!root)
    return;
  if (root->tok == DECL &&
      getIdentScope(root->children[0]->attr.id) == Scope_Glob)
    cg_section_data();
  else
    cg_section_text();
  int r;
  int tmplab, tmplab2;
  switch (root->tok) {
  case FUNC:
    cg_comment("function preamble");
    TmpFn = root->children[0];
    cg_func_preamble(TmpFn);
    cg_func_load_params(root->children[2]);
    genAST(root->children[1]);
    break;
  case DECL:
    for (TreeNode *t = root->children[0]; t != NULL; t = t->sibling) {
      cg_declaration(t);
    }
    break;
  case ASSIGN:
    IsAssign = 1;
    r = cg_eval(root->children[1]);
    cg_assign(root->children[0], r);
    break;
  case IF:
    cg_comment("if statement");
    tmplab = CurLab++;
    tmplab2 = CurLab++;
    r = cg_eval(root->children[0]);
    fprintf(Outfile, "\tcmpq\t$1, %s\n", RegList[r]);
    cg_jump("je", tmplab);
    free_reg(r);
    cg_comment("else branch");
    if (root->children[2]) {
      genAST(root->children[2]);
    }
    cg_jump(NULL, tmplab2);
    cg_label(tmplab);
    cg_comment("if branch");
    tmplab = CurLab++;
    genAST(root->children[1]);
    cg_label(tmplab2);
    break;
  case WHILE:
    cg_comment("while");
    tmplab = CurLab++;
    tmplab2 = CurLab++;
    cg_label(tmplab);
    r = cg_eval(root->children[0]);
    fprintf(Outfile, "\tcmpq\t$1, %s\n", RegList[r]);
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
    cg_return(root);
    break;
  case CALL:
    cg_call(root);
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
