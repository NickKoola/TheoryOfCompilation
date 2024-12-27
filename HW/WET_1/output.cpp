#include "output.hpp"
#include <iostream>

static const std::string token_names[] = {
        "__FILLER_FOR_ZERO",
        "VOID",
        "INT",
        "BYTE",
        "BOOL",
        "AND",
        "OR",
        "NOT",
        "TRUE",
        "FALSE",
        "RETURN",
        "IF",
        "ELSE",
        "WHILE",
        "BREAK",
        "CONTINUE",
        "SC",
        "COMMA",
        "LPAREN",
        "RPAREN",
        "LBRACE",
        "RBRACE",
        "ASSIGN",
        "RELOP",
        "BINOP",
        "COMMENT",
        "ID",
        "NUM",
        "NUM_B",
        "STRING"
};

void output::printToken(int lineno, enum tokentype token, const char *value) {
    if (token == COMMENT) {
        std::cout << lineno << " COMMENT //" << std::endl;
    } else {
        std::cout << lineno << " " << token_names[token] << " " << value << std::endl;
    }
}

void output::errorUnknownChar(char c) {
    std::cout << "ERROR: Unknown character " << c << std::endl;
    exit(0);
}

void output::errorUnclosedString() {
    std::cout << "ERROR: Unclosed string" << std::endl;
    exit(0);
}

void output::errorUndefinedEscape(const char *sequence) {
    std::cout << "ERROR: Undefined escape sequence " << sequence << std::endl;
    exit(0);
}
