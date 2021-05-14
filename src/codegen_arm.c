#include "globals.h"
#include "symtab.h"
#include <stdlib.h>

#define REGCOUNT 11

// 寄存器是否空闲
static int Regs[REGCOUNT];
enum { R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10 };

void free_reg(int r) {
  if (Regs[r] == 1) {
    fprintf(stderr, "Error: R%d is already free\n", r);
    exit(1);
  }
  Regs[r] = 1;
}

int allocate_reg() {
  for (int i = 0; i < REGCOUNT; i++) {
    if (Regs[i]) {
      Regs[i] = 0;
      return i;
    }
  }
  fprintf(stderr, "Error: all registers are unavailable\n");
  exit(1);
}

static int cg_loadnum(int num) {
  int r = allocate_reg();
  fprintf(Outfile, "\tldr R%d, #%d\n", r, num);
  return r;
}

static void genAST(TreeNode *t) {

}

void genCode(TreeNode *tree) {
  for (int i = 0; i < REGCOUNT; i++) {
    Regs[i] = 1;
  }
  genAST(tree);
}
