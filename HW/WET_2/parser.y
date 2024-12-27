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

// TODO: Define precedence and associativity here

%%

// While reducing the start variable, set the root of the AST
Program:  Funcs { program = $1; }
;

// TODO: Define grammar here

%%

// TODO: Place any additional code here