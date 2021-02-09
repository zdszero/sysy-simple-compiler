%option noyywrap

%{
  #include "globals.h"
  #include "scan.h"
  #include <stdio.h>
  #include <string.h>
  #define YY_DECL int scan_yylex(void)
  Token Tok;
  static int lineno;
  static int isFirstTime = 1;
%}

%%

"int"                  { return INT;     }
"print"                { return PRINT;   }
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
[ \t\n]+               { /* skip */      }
[0-9]+                 { return NUM;     }
[a-zA-Z_][a-zA-Z0-9_]* { return IDENT;   }
<<EOF>>                { return YYEOF;   }
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
    Tok.intval = atoi(yytext);
  }
}
