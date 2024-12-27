%{

#include "nodes.hpp"
#include "output.hpp"

// bison declarations
extern int yylineno;
extern int yylex();

void yyerror(const char*);

// root of the AST, set by the parser and used by other parts of the compiler
std::shared_ptr<ast::Node> program;

using namespace std;

// TODO: Place any additional declarations here

%}

// TODO: Define tokens here
%token VOID
%token INT
%token BYTE
%token BOOL
%token TRUE
%token FALSE
%token RETURN
%token IF
%token ELSE
%token WHILE
%token BREAK
%token CONTINUE
%token SC
%token COMMA
%token ID
%token NUM
%token NUM_B
%token STRING

// TODO: Define precedence and associativity here
%right      ASSIGN
%left       OR
%left       AND
%left       RELOP_EQ RELOP_NE  
%left       RELOP_LT RELOP_GT RELOP_LE RELOP_GE        
%left       BINOP_ADD BINOP_SUB  
%left       BINOP_MUL BINOP_DIV
%right      NOT
%left       LBRACE RBRACE
%left       LPAREN RPAREN
%nonassoc   ELSE /* solves the dangling else problem */

%%

// While reducing the start variable, set the root of the AST
Program:        Funcs                                                           { program = $1; }
;

Funcs:          FuncDecl Funcs                                                  { auto funcsList = dynamic_pointer_cast<ast::Funcs>($2); 
                                                                                    funcsList->push_front(dynamic_pointer_cast<ast::FuncDecl>($1));
                                                                                    $$ = funcsList;
                                                                                } 
                |                                                               { $$ = make_shared<ast::Funcs>(); }           
;

FuncDecl:       RetType ID LPAREN Formals RPAREN LBRACE Statements RBRACE       { $$ = make_shared<ast::FuncDecl>(
                                                                                    dynamic_pointer_cast<ast::ID>($2),
                                                                                    dynamic_pointer_cast<ast::Type>($1),
                                                                                    dynamic_pointer_cast<ast::Formals>($4),
                                                                                    dynamic_pointer_cast<ast::Statements>($7)
                                                                                    ); 
                                                                                }
;

RetType:        Type                                                            { $$ = $1; }
                | VOID                                                          { $$ = make_shared<ast::Type>(ast::BuiltInType::VOID); }   
;

Formals:        FormalsList                                                     { $$ = $1; }      
                |                                                               { $$ = make_shared<ast::Formals>(); }
;

FormalsList:    FormalDecl                                                      { $$ = make_shared<ast::Formals>(dynamic_pointer_cast<ast::Formal>($1)); } 
                | FormalDecl COMMA FormalsList                                  { auto formalsList = dynamic_pointer_cast<ast::Formals>($3);
                                                                                    formalsList->push_back(dynamic_pointer_cast<ast::Formal>($1));
                                                                                    $$ = formalsList;
                                                                                }
;

FormalDecl:     Type ID                                                         { $$ = make_shared<ast::Formal>(
                                                                                    dynamic_pointer_cast<ast::ID>($2),
                                                                                    dynamic_pointer_cast<ast::Type>($1)
                                                                                    );
                                                                                }          
;

Statements:     Statement                                                       { $$ = make_shared<ast::Statements>(dynamic_pointer_cast<ast::Statement>($1)); }     
                | Statements Statement                                          { auto stmts = dynamic_pointer_cast<ast::Statements>($1);
                                                                                    stmts->push_back(dynamic_pointer_cast<ast::Statement>($2));
                                                                                    $$ = stmts;
                                                                                }      
;

Statement:      LBRACE Statements RBRACE                                        { $$ = $2; }         
                | Type ID SC                                                    { $$ = make_shared<ast::VarDecl>(
                                                                                    dynamic_pointer_cast<ast::ID>($2),
                                                                                    dynamic_pointer_cast<ast::Type>($1)
                                                                                    ); 
                                                                                }
                | Type ID ASSIGN Exp SC                                         { $$ = make_shared<ast::VarDecl>(
                                                                                    dynamic_pointer_cast<ast::ID>($2),
                                                                                    dynamic_pointer_cast<ast::Type>($1),
                                                                                    dynamic_pointer_cast<ast::Exp>($4)
                                                                                    );
                                                                                }
                | ID ASSIGN Exp SC                                              { $$ = make_shared<ast::Assign>(
                                                                                    dynamic_pointer_cast<ast::ID>($1),
                                                                                    dynamic_pointer_cast<ast::Exp>($3)
                                                                                    );
                                                                                }
                | Call SC                                                       { $$ = $1; }
                | RETURN SC                                                     { $$ = make_shared<ast::Return>(); }
                | RETURN Exp SC                                                 { $$ = make_shared<ast::Return>(dynamic_pointer_cast<ast::Exp>($2)); }
                | IF LPAREN Exp RPAREN Statement                                { $$ = make_shared<ast::If>(
                                                                                    dynamic_pointer_cast<ast::Exp>($3),
                                                                                    dynamic_pointer_cast<ast::Statement>($5)
                                                                                    );
                                                                                }
                | IF LPAREN Exp RPAREN Statement ELSE Statement %prec ELSE      { $$ = make_shared<ast::If>(
                                                                                    dynamic_pointer_cast<ast::Exp>($3),
                                                                                    dynamic_pointer_cast<ast::Statement>($5),
                                                                                    dynamic_pointer_cast<ast::Statement>($7)
                                                                                    );
                                                                                }
                | WHILE LPAREN Exp RPAREN Statement                             { $$ = make_shared<ast::While>(
                                                                                    dynamic_pointer_cast<ast::Exp>($3),
                                                                                    dynamic_pointer_cast<ast::Statement>($5)
                                                                                    );
                                                                                }
                | BREAK SC                                                      { $$ = make_shared<ast::Break>(); }
                | CONTINUE SC                                                   { $$ = make_shared<ast::Continue>(); }
;

Call:           ID LPAREN ExpList RPAREN                                        { $$ = make_shared<ast::Call>(
                                                                                    dynamic_pointer_cast<ast::ID>($1),
                                                                                    dynamic_pointer_cast<ast::ExpList>($3)
                                                                                    );
                                                                                }
                | ID LPAREN RPAREN                                              { $$ = make_shared<ast::Call>(
                                                                                    dynamic_pointer_cast<ast::ID>($1)
                                                                                    );
                                                                                }
;

ExpList:        Exp                                                             { $$ = make_shared<ast::ExpList>(dynamic_pointer_cast<ast::Exp>($1)); }
                | Exp COMMA ExpList                                             { auto expList = dynamic_pointer_cast<ast::ExpList>($3);
                                                                                    expList->push_back(dynamic_pointer_cast<ast::Exp>($1));
                                                                                    $$ = expList;
                                                                                }
;

Type:           INT                                                             { $$ = make_shared<ast::Type>(ast::BuiltInType::INT); }
                | BYTE                                                          { $$ = make_shared<ast::Type>(ast::BuiltInType::BYTE); }
                | BOOL                                                          { $$ = make_shared<ast::Type>(ast::BuiltInType::BOOL); }
;

Exp:            LPAREN Exp RPAREN                                               { $$ = $2; }
                | Exp BINOP_ADD  Exp                                            { $$ = make_shared<ast::BinOp>(
                                                                                    dynamic_pointer_cast<ast::Exp>($1),
                                                                                    dynamic_pointer_cast<ast::Exp>($3),
                                                                                    ast::BinOpType::ADD
                                                                                    );
                                                                                }
                | Exp BINOP_SUB  Exp                                            { $$ = make_shared<ast::BinOp>(
                                                                                    dynamic_pointer_cast<ast::Exp>($1),
                                                                                    dynamic_pointer_cast<ast::Exp>($3),
                                                                                    ast::BinOpType::SUB
                                                                                    );
                                                                                }
                | Exp BINOP_MUL  Exp                                            { $$ = make_shared<ast::BinOp>(
                                                                                    dynamic_pointer_cast<ast::Exp>($1),
                                                                                    dynamic_pointer_cast<ast::Exp>($3),
                                                                                    ast::BinOpType::MUL
                                                                                    );
                                                                                }
                | Exp BINOP_DIV  Exp                                            { $$ = make_shared<ast::BinOp>(
                                                                                    dynamic_pointer_cast<ast::Exp>($1),
                                                                                    dynamic_pointer_cast<ast::Exp>($3),
                                                                                    ast::BinOpType::DIV
                                                                                    );
                                                                                }
                | ID                                                            { $$ = $1; }
                | Call                                                          { $$ = $1; }
                | NUM                                                           { $$ = $1; }
                | NUM_B                                                         { $$ = $1; }
                | STRING                                                        { $$ = $1; }
                | TRUE                                                          { $$ = make_shared<ast::Bool>(true); }
                | FALSE                                                         { $$ = make_shared<ast::Bool>(false); }
                | NOT Exp                                                       { $$ = make_shared<ast::Not>(dynamic_pointer_cast<ast::Exp>($2)); }
                | Exp AND Exp                                                   { $$ = make_shared<ast::And>(
                                                                                    dynamic_pointer_cast<ast::Exp>($1),
                                                                                    dynamic_pointer_cast<ast::Exp>($3)
                                                                                    );
                                                                                }
                | Exp OR Exp                                                    { $$ = make_shared<ast::Or>(
                                                                                    dynamic_pointer_cast<ast::Exp>($1),
                                                                                    dynamic_pointer_cast<ast::Exp>($3)
                                                                                    );
                                                                                }
                | Exp RELOP_EQ Exp                                              { $$ = make_shared<ast::RelOp>(
                                                                                    dynamic_pointer_cast<ast::Exp>($1),
                                                                                    dynamic_pointer_cast<ast::Exp>($3),
                                                                                    ast::RelOpType::EQ
                                                                                    );
                                                                                }
                | Exp RELOP_NE Exp                                             { $$ = make_shared<ast::RelOp>(
                                                                                    dynamic_pointer_cast<ast::Exp>($1),
                                                                                    dynamic_pointer_cast<ast::Exp>($3),
                                                                                    ast::RelOpType::NE
                                                                                    );
                                                                                }
                | Exp RELOP_LT Exp                                              { $$ = make_shared<ast::RelOp>(
                                                                                    dynamic_pointer_cast<ast::Exp>($1),
                                                                                    dynamic_pointer_cast<ast::Exp>($3),
                                                                                    ast::RelOpType::LT
                                                                                    );
                                                                                }
                | Exp RELOP_GT Exp                                              { $$ = make_shared<ast::RelOp>(
                                                                                    dynamic_pointer_cast<ast::Exp>($1),
                                                                                    dynamic_pointer_cast<ast::Exp>($3),
                                                                                    ast::RelOpType::GT
                                                                                    );
                                                                                }
                | Exp RELOP_LE Exp                                              { $$ = make_shared<ast::RelOp>(
                                                                                    dynamic_pointer_cast<ast::Exp>($1),
                                                                                    dynamic_pointer_cast<ast::Exp>($3),
                                                                                    ast::RelOpType::LE
                                                                                    );
                                                                                }
                | Exp RELOP_GE Exp                                              { $$ = make_shared<ast::RelOp>(
                                                                                    dynamic_pointer_cast<ast::Exp>($1),
                                                                                    dynamic_pointer_cast<ast::Exp>($3),
                                                                                    ast::RelOpType::GE
                                                                                    );
                                                                                }
                | LPAREN Type RPAREN Exp                                        { $$ = make_shared<ast::Cast>(   
                                                                                    dynamic_pointer_cast<ast::Exp>($4),
                                                                                    dynamic_pointer_cast<ast::Type>($2)
                                                                                    );
                                                                                }
;

%%
// TODO: Place any additional code here
void yyerror(const char* message) {
    output::errorSyn(yylineno); 
    exit(1); 
}