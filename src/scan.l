%option noyywrap

%{
  #include "globals.h"
  #include "scan.h"
  #include "util.h"
  #include <stdio.h>
  #include <stdlib.h>
  #include <ctype.h>
  #include <string.h>
  #define YY_DECL int scan_yylex(void)
  Token Tok;
  int lineno = 1;
  extern int traceScan;
  static int isFirstTime = 1;
  int scopeAttr;
%}

quote      \'
number     ("0"[Xx][0-9a-fA-F]+|[0-9]+)
identifier [a-zA-Z_][a-zA-Z0-9_]*
space      [ \t]+

%%

"int"         { return INT;       }
"long"        { return LONG;      }
"void"        { return VOID;      }
"char"        { return CHAR;      }
"if"          { return IF;        }
"else"        { return ELSE;      }
"while"       { return WHILE;     }
"for"         { return FOR;       }
"return"      { return RETURN;    }
"=="          { return EQ;        }
"!="          { return NE;        }
">"           { return GT;        }
">="          { return GE;        }
"<"           { return LT;        }
"<="          { return LE;        }
"&"           { char c;
                if ((c = input()) == '&')
                  return AND;
                unput(c);
                return AMPERSAND;
              }
"||"          { return OR;        }
","           { return COMMA;     }
"="           { return ASSIGN;    }
"+"           { return PLUS;      }
"-"           { return MINUS;     }
"*"           { return TIMES;     }
"/"           { return OVER;      }
";"           { return SEMI;      }
"("           { scopeAttr = Scope_Para;
                return LP;        }
")"           { scopeAttr = Scope_Glob;
                return RP;        }
"{"           { scopeAttr = Scope_Local;
                return LC;        }
"}"           { scopeAttr = Scope_Glob;
                return RC;        }
"["           { return LS;        }
"]"           { return RS;        }
\n            { lineno++;         }
<<EOF>>       { return YYEOF;     }
{space}       { /* skip */        }
{number}      { if (yytext[0] == '0' && (yytext[1] == 'x' || yytext[1] == 'X')) {
                  Tok.numval = strtol(yytext, NULL, 16);
                }
                else {
                  Tok.numval = strtol(yytext, NULL, 10);
                }
                return NUM;       }
{identifier}  { strcpy(Tok.text, yytext);
                return IDENT;     }
{quote}       { char c;
                if (!isascii(c = input()))
                  return YYUNDEF;
                Tok.text[0] = c;
                if ((c = input()) != '\'')
                  return YYUNDEF;
                return CH;
              }
"/*"          { char c;
                while ((c = input()) != EOF) {
                  if (c == '\n')
                    lineno++;
                  else if (c == '*') {
                    while ((c = input()) == '*');
                    if (c == '/')
                      break;
                  }
                }
              } 
"//".*        {    /* skip */     }
.             { return YYUNDEF;   }

%%

void scan() {
  if (isFirstTime) {
    isFirstTime = 0;
    yyin = Infile;
    yyout = Outfile;
  }
  Tok.token = scan_yylex();
  if (traceScan) {
    printf("line %d: ", lineno);
    printToken(Tok.token);
  }
}
