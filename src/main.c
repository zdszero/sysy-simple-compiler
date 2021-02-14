#include "globals.h"
#include "scan.h"
#include "parse.h"
#include "util.h"
#include "codegen.h"
#include <stdlib.h>

int traceScan = 0;
int traceParse = 1;
int generateCode = 0;

FILE *Infile, *Outfile;

int main(int argc, char *argv[])
{
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
  if (traceParse)
    printTree(syntaxTree, 0);
  if (generateCode)
    genCode(syntaxTree);
  return 0;
}
