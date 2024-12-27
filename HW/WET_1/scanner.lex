
%{

/* Declarations section */
#include <stdio.h>
#include "tokens.hpp"

%}

%option yylineno
%option noyywrap
digit   		([0-9])
letter  		([a-zA-Z])
whitespace		([\t\n\r ])
string     ((([   !#-\[\]-~ ])|\\([\\ntr\"0])|\\(x[2-6][0-9A-Fa-f])|\\(x7[0-9A-Ea-e])|\\(x0[9ADad]))*)
undef_hex   (\\x([^2-7][0-9A-Ea-e]|[2-7][^0-7A-Ea-e]|[^2-7][^2-9A-Ea-e]|[^2-9A-Ea-e]))



%%


void            return VOID;
int             return INT;
byte            return BYTE;
bool            return BOOL;
and             return AND;
or              return OR;
not             return NOT;
true            return TRUE;
false           return FALSE;
return          return RETURN;
if              return IF;
else            return ELSE;
while           return WHILE;
break           return BREAK;
continue        return CONTINUE;
;               return SC;
,               return COMMA;
\(               return LPAREN;
\)               return RPAREN;
\{               return LBRACE;
\}               return RBRACE;
=               return ASSIGN;
==|!=|<|>|<=|>=         return RELOP;
\+|\-|\*|\/                 return BINOP;
\/\/[^\r\n]*               return COMMENT;
{letter}({letter}|{digit})*       return ID;
[1-9]{digit}*|0               return NUM;
[1-9]{digit}*b|0b               return NUM_B;
\"{string}\"           return STRING;
\"{string}           return ERROR_UNCLOSED_STRING;
\"{string}\\[^\\ntr0"]    return ERROR_UNDEF_ESCAPE;
\"{string}{undef_hex}    return ERROR_UNDEF_HEX;
{whitespace}                                ;
.                   return ERROR;

%%

