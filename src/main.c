#include "codegen.h"
#include "globals.h"
#include "parse.h"
#include "scan.h"
#include "symtab.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// 打印源文件的符号
int traceScan = 0;
// 打印语法树
int traceParse = 0;
// 打印符号表
int traceSymbols = 0;
// 生成汇编代码，不生成可执行文件
int traceAsm = 0;
// SysY源文件，生成的汇编文件
FILE *Infile, *Outfile;

void printHelp();

int main(int argc, char *argv[]) {
  int c;
  // 解析命令行参数并且设置对应的标识
  char *inFileName = NULL, *outFileName = NULL, *exeFileName = NULL;
  while ((c = getopt(argc, argv, "hsptSo:")) != -1) {
    switch (c) {
    case 'h':
      printHelp();
      return 0;
    case 's':
      traceScan = 1;
      break;
    case 'p':
      traceParse = 1;
      break;
    case 't':
      traceSymbols = 1;
      break;
    case 'S':
      traceAsm = 1;
      break;
    case 'o':
      exeFileName = strdup(optarg);
      break;
    default:
      fprintf(stderr, "Error In Argument Parsing\n");
      exit(1);
    }
  }
  // 判断SysY源文件是否给定
  if (optind == argc) {
    fprintf(stderr, "Error: no input file\n");
    exit(1);
  } else {
    // 判断源文件是否存在
    inFileName = strdup(argv[optind]);
    Infile = fopen(inFileName, "r");
    if (!Infile) {
      fprintf(stderr, "Error: file %s not found\n", inFileName);
      exit(1);
    }
  }
  // 判断文件后缀是否为sy
  char *extension = strrchr(inFileName, '.');
  if (strcmp(extension + 1, "sy") != 0) {
    fprintf(stderr, "Error: %s: file format not recognized\n", inFileName);
    exit(1);
  }
  // 默认生成prefix.s文件
  outFileName = strdup(inFileName);
  int len = strlen(outFileName);
  outFileName[len - 1] = '\0';
  outFileName[len - 2] = 's';
  Outfile = fopen(outFileName, "w");
  Tok.token = -1;
  yyparse();
  // 判断编译是否出现错误
  if (hasError)
    exit(1);
  if (traceParse)
    printTree(syntaxTree, 0);
  if (traceSymbols)
    printSymTab();
  // 如果制定了-S选项，只编译而不进行汇编和链接
  genCode(syntaxTree);
  fclose(Infile);
  fclose(Outfile);
  // 否则就生成可执行文件，删除汇编文件
  if (!traceScan && !traceParse && !traceParse && !traceAsm) {
    char cmd[128];
    // 如果-o选项没有指定参数，默认生成a.out可执行文件
    if (!exeFileName)
      exeFileName = "a.out";
    sprintf(cmd, "gcc -o %s %s ../lib/printint.c", exeFileName, outFileName);
    system(cmd);
    remove(outFileName);
  }
  return 0;
}

void printHelp() {
  printf("Usage: cm [options] file\n"
         "Options:\n"
         "  -h\tDisplay help information\n"
         "  -s\tScanning only, print all tokens\n"
         "  -p\tPrint abstract syntax tree\n"
         "  -t\tPrint symbol table\n"
         "  -o\tSpecify executable filename\n"
         "  -S\tCompile only, do not assemble or link\n");
}
