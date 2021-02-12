%option noyywrap

%{
  #include "globals.h"
  #include "scan.h"
  #include <stdio.h>
  #include <stdlib.h>
  #include <ctype.h>
  #include <string.h>
  #define YY_DECL int scan_yylex(void)
  Token Tok;
  int lineno = 1;
  static int isFirstTime = 1;
%}

quote \'

%%

"int"                  { return INT;     }
"long"                 { return LONG;    }
"void"                 { return VOID;    }
"char"                 { return CHAR;    }
"print"                { return PRINT;   }
"if"                   { return IF;      }
"else"                 { return ELSE;    }
"while"                { return WHILE;   }
"for"                  { return FOR;     }
"=="                   { return EQ;      }
"!="                   { return NE;      }
">"                    { return GT;      }
">="                   { return GE;      }
"<"                    { return LT;      }
"<="                   { return LE;      }
"="                    { return ASSIGN;  }
"+"                    { return PLUS;    }
"-"                    { return MINUS;   }
"*"                    { return TIMES;   }
"/"                    { return OVER;    }
";"                    { return SEMI;    }
"("                    { return LP;      }
")"                    { return RP;      }
"{"                    { return LC;      }
"}"                    { return RC;      }
\n                     { lineno++;       }
[ \t]+                 { /* skip */      }
[+-]?(0x)?[0-9a-f]+    { return NUM;     }
[a-zA-Z_][a-zA-Z0-9_]* { return IDENT;   }
<<EOF>>                { return YYEOF;   }
{quote}                { char c;
                         if (!isascii(c = input()))
                           return YYUNDEF;
                         Tok.text[0] = c;
                         if ((c = input()) != '\'')
                           return YYUNDEF;
                         return CH;
                       }
.                      { return YYUNDEF; }

%%

void scan() {
  if (isFirstTime) {
    isFirstTime = 0;
    yyin = Infile;
    yyout = Outfile;
  }
  Tok.token = scan_yylex();
  if (Tok.token == IDENT) {
    strcpy(Tok.text, yytext);
  } else if (Tok.token == NUM) {
    if (yytext[0] == '0' && (yytext[1] == 'x' || yytext[1] == 'X'))
      Tok.val = strtol(yytext, NULL, 16);
    else {
      char *pc = yytext;
      if (*pc == '+' || *pc == '-')
        pc++;
      for (; pc != NULL; pc++) {
        if (!(*pc >= '0' && *pc <= '9')) {
          fprintf(stderr, "Error: invalid number %s at line %d\n", yytext, lineno);
          exit(1);
        }
      }
      Tok.val = strtol(yytext, NULL, 10);
    }
  }
}
