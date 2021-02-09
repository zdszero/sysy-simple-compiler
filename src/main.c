#include "globals.h"
#include "scan.h"
#include "parse.h"
#include "util.h"
#include "codegen.h"
#include <stdlib.h>

#define TEST_SCAN 0
#define TEST_PARSE 1
#define TEST_CODEGEN 0

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
#if TEST_SCAN
  while (Tok.token != YYEOF) {
    scan();
    printToken(Tok.token);
  }
  return 0;
#endif
  yyparse();
#if TEST_PARSE
  printTree(syntaxTree, 0);
#endif
#if TEST_CODEGEN
  genCode(syntaxTree);
#endif
  return 0;
}
