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
%token IDENT NUM SEMI INT ASSIGN PRINT IF ELSE
%token PLUS MINUS TIMES OVER
%token EQ NE LE LT GE GT
%token LP RP LC RC
%left EQ NE LE LT GE GT
%left PLUS MINUS
%left TIMES OVER
/* solve dangling else problem, prefer shift to reduce */
%precedence RP
%precedence ELSE

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

statement : print_statement    { $$ = $1; }
          | decl_statement     { $$ = $1; }
          | assign_statement   { $$ = $1; }
          | compound_statement { $$ = $1; }
          | if_statement       { $$ = $1; }
          ;

print_statement : PRINT expression SEMI
                  { $$ = mkTreeNode(PRINT);
                    $$->children[0] = $2;
                  }
                ;

decl_statement : INT var SEMI
                 { $$ = mkTreeNode(INT);
                   $$->children[0] = $2;
                 }
               | INT var ASSIGN expression SEMI
                 { $$ = mkTreeNode(INT);
                   $$->children[0] = $2;
                   $$->children[1] = $4;
                 }
               ;

assign_statement : var ASSIGN expression SEMI
                   { $$ = mkTreeNode(ASSIGN);
                     $$->children[0] = $1;
                     $$->children[1] = $3;
                   }
                 ;

compound_statement : LC statements RC { $$ = $2; }
                   ;

if_statement : if_head
               { $$ = $1; }
             | if_head ELSE compound_statement
               { $$ = $1;
                 $$->children[2] = $3;
               }
             ;

if_head : IF LP expression RP compound_statement
          { $$ = mkTreeNode(IF);
            $$->children[0] = $3;
            $$->children[1] = $5;
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
