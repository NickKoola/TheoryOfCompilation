
%{

/* Declarations section */
#include <stdio.h>
//#include "tokens.hpp"
#include "output.hpp"
#include "parser.tab.h"

%}

%option yylineno
%option noyywrap
digit   		([0-9])
letter  		([a-zA-Z])
whitespace		([\t\n\r ])
string          (\"([^\n\r\"\\]|\\[rnt"\\])+\")
comment         (\/\/[^\r\n]*[ \r|\n|\r\n]?)

%%

void                                return VOID;
int                                 return INT;
byte                                return BYTE;
bool                                return BOOL;
and                                 return AND;
or                                  return OR;
not                                 return NOT;
true                                return TRUE;
false                               return FALSE;
return                              return RETURN;
if                                  return IF;
else                                return ELSE;
while                               return WHILE;
break                               return BREAK;
continue                            return CONTINUE;
;                                   return SC;
,                                   return COMMA;
\(                                  return LPAREN;
\)                                  return RPAREN;
\{                                  return LBRACE;
\}                                  return RBRACE;
"="                                 return ASSIGN;
"=="                                return RELOP_EQ;
"!="                                return RELOP_NE;
">"                                 return RELOP_GT;
"<"                                 return RELOP_LT;
"<="                                return RELOP_LE;
">="                                return RELOP_GE;
"+"                                 return BINOP_ADD;
"-"                                 return BINOP_SUB;                                    
"*"                                 return BINOP_MUL;
"/"                                 return BINOP_DIV;
{letter}({letter}|{digit})*         {yylval = std::make_shared<ast::ID>(yytext); return ID;}
0|[1-9]{digit}*                     {yylval = std::make_shared<ast::Num>(yytext); return NUM;}                                  
0b|[1-9]{digit}*b                   {yylval = std::make_shared<ast::NumB>(yytext); return NUM_B;} 
{string}                            {yylval = std::make_shared<ast::String>(yytext); return STRING;} 
{whitespace}|{comment}              {/* Skip Whitespaces and Comments */}
.                                   {output::errorLex(yylineno); exit(0);}

%%

