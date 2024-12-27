#ifndef TOKENS_HPP
#define TOKENS_HPP

enum tokentype {
    VOID = 1,
    INT,
    BYTE,
    BOOL,
    AND,
    OR,
    NOT,
    TRUE,
    FALSE,
    RETURN,
    IF,
    ELSE,
    WHILE,
    BREAK,
    CONTINUE,
    SC,
    COMMA,
    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    ASSIGN,
    RELOP,
    BINOP,
    COMMENT,
    ID,
    NUM,
    NUM_B,
    STRING,
    ERROR,
    ERROR_UNCLOSED_STRING,
    ERROR_UNDEF_ESCAPE,
    ERROR_UNDEF_HEX
};

extern int yylineno;
extern char *yytext;
extern int yyleng;

extern int yylex();

#endif //TOKENS_HPP
