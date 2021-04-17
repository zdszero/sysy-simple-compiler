#include "codegen.h"
#include "globals.h"
#include "parse.h"
#include "scan.h"
#include "util.h"
#include <stdlib.h>

extern void printSymTab();
int traceScan = 0;
int traceParse = 0;
int traceSymbols = 0;
int generateCode = 1;

FILE *Infile, *Outfile;

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "usage: %s <filename>\n", argv[0]);
    exit(1);
  }
  Infile = fopen(argv[1], "r");
  if (!Infile) {
    fprintf(stderr, "%s does not exist\n", argv[1]);
    exit(1);
  }
  Outfile = stdout;
  Tok.token = -1;
  yyparse();
  if (hasError)
    exit(1);
  if (traceParse)
    printTree(syntaxTree, 0);
  if (traceSymbols)
    printSymTab();
  if (generateCode)
    genCode(syntaxTree);
  return 0;
}
