%{
  #include "globals.h"
  #include "util.h"
  #include "symtab.h"
  #include "scan.h"
  #include <stdio.h>
  #include <ctype.h>
  int yylex();
  void yyerror(const char *msg);
  TreeNode *syntaxTree;
%}

%define api.value.type { TreeNode * }
%define parse.error detailed
%token INT VOID IDENT NUM SEMI ASSIGN PRINT IF ELSE WHILE FOR
%token GLUE FUNC
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

program : declaration_list { syntaxTree = $1; }
        ;

declaration_list : declaration_list declaration
                   { YYSTYPE t = $1;
                     while (t->sibling)
                       t = t->sibling;
                     t->sibling = $2;
                     $$ = $1;
                   }
                 | declaration { $$ = $1; }
                 ;

declaration : var_declaraton   { $$ = $1; }
            | func_declaration { $$ = $1; }
            ;

var_declaraton : type_specifier var SEMI
                 { $$ = mkTreeNode(INT);
                   $$->children[0] = $2;
                 }
               | type_specifier var ASSIGN expression SEMI
                 { $$ = mkTreeNode(INT);
                   $$->children[0] = $2;
                   $$->children[1] = $4;
                 }
               ;

func_declaration : type_specifier IDENT LP RP compound_statement
                   { $$ = mkTreeNode(FUNC);
                     $$->children[0] = $1;
                     $$->children[1] = $5;
                   }
                 ;

type_specifier : INT
               | VOID
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

statement : var_declaraton       { $$ = $1; }
          | print_statement      { $$ = $1; }
          | assign_statement     { $$ = $1; }
          | compound_statement   { $$ = $1; }
          | if_statement         { $$ = $1; }
          | while_statement      { $$ = $1; }
          | for_statement        { $$ = $1; }
          | expression_statement { $$ = $1; }
          ;

print_statement : PRINT expression
                  { $$ = mkTreeNode(PRINT);
                    $$->children[0] = $2;
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

while_statement : WHILE LP expression RP compound_statement
                  { $$ = mkTreeNode(WHILE);
                    $$->children[0] = $3;
                    $$->children[1] = $5;
                  }
                ;

for_statement : FOR LP statement expression_statement post_statement RP compound_statement
                { $$ = mkTreeNode(GLUE);
                  $$->children[0] = $3;
                  $$->children[1] = mkTreeNode(WHILE);
                  $$->children[1]->children[0] = $4;
                  $$->children[1]->children[1] = mkTreeNode(GLUE);
                  $$->children[1]->children[1]->children[0] = $7;
                  $$->children[1]->children[1]->children[1] = $5;
                }
              ;

post_statement : var ASSIGN expression
                 { $$ = mkTreeNode(ASSIGN);
                   $$->children[0] = $1;
                   $$->children[1] = $3;
                 }
               ;

expression_statement : expression SEMI { /* skip */ }
                     | SEMI { /* skip */ }
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

void yyerror(const char *msg) {
  fprintf(stderr, "%s in line %d\n", msg, lineno);
}
