%{
  #include "globals.h"
  #include "util.h"
  #include "symtab.h"
  #include "scan.h"
  #include <stdio.h>
  #include <ctype.h>
  int yylex();
  void yyerror(char *msg);
  TreeNode *syntaxTree;
%}

%define api.value.type { TreeNode * }
%token IDENT NUM SEMI INT ASSIGN PRINT
%token PLUS MINUS TIMES OVER
%token EQ NE LE LT GE GT
%token LP RP
%left EQ NE LE LT GE GT
%left PLUS MINUS
%left TIMES OVER

%%

program : statements { syntaxTree = $1; }
        ;

statements : statements statement
             { YYSTYPE t = $1;
               while (t->sibling)
                 t = t->sibling;
               t->sibling = $2;
               $$ = $1;
             }
           | statement { $$ = $1; }
           ;

statement : PRINT expression SEMI
            { $$ = mkTreeNode(PRINT);
              $$->children[0] = $2;
            }
          | INT var SEMI
            { $$ = mkTreeNode(INT);
              $$->children[0] = $2;
            }
          | var ASSIGN expression SEMI
            { $$ = mkTreeNode(ASSIGN);
              $$->children[0] = $1;
              $$->children[1] = $3;
            }
          ;

var : IDENT
      { $$ = mkTreeNode(IDENT);
        $$->attr.id = findIdent(Tok.text);
      }
    ;

expression : expression EQ expression
             { $$ = mkTreeNode(EQ);
               $$->children[0] = $1;
               $$->children[1] = $3;
             }
           | expression NE expression
             { $$ = mkTreeNode(NE);
               $$->children[0] = $1;
               $$->children[1] = $3;
             }
           | expression GT expression
             { $$ = mkTreeNode(GT);
               $$->children[0] = $1;
               $$->children[1] = $3;
             }
           | expression GE expression
             { $$ = mkTreeNode(GE);
               $$->children[0] = $1;
               $$->children[1] = $3;
             }
           | expression LT expression
             { $$ = mkTreeNode(LT);
               $$->children[0] = $1;
               $$->children[1] = $3;
             }
           | expression LE expression
             { $$ = mkTreeNode(LE);
               $$->children[0] = $1;
               $$->children[1] = $3;
             }
           | expression PLUS expression
             { $$ = mkTreeNode(PLUS);
               $$->children[0] = $1;
               $$->children[1] = $3;
             }
           | expression MINUS expression
             { $$ = mkTreeNode(MINUS);
               $$->children[0] = $1;
               $$->children[1] = $3;
             }
           | expression TIMES expression
             { $$ = mkTreeNode(TIMES);
               $$->children[0] = $1;
               $$->children[1] = $3;
             }
           | expression OVER expression
             { $$ = mkTreeNode(OVER);
               $$->children[0] = $1;
               $$->children[1] = $3;
             }
           | LP expression RP { $$ = $2; }
           | NUM
             { $$ = mkTreeNode(NUM);
               $$->attr.val = Tok.intval;
             }
           | IDENT
             { $$ = mkTreeNode(IDENT);
               $$->attr.id = findIdent(Tok.text);
             }
           ;

%%

int yylex() {
  scan();
  return Tok.token;
}

void yyerror(char *msg) {
  fprintf(stderr, "%s\n", msg);
}
