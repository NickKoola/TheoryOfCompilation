#ifndef OUTPUT_HPP
#define OUTPUT_HPP

#include "tokens.hpp"

namespace output {

    /* prints the token with the given line number, type, and value. For COMMENT value is ignored */
    void printToken(int lineno, enum tokentype token, const char *value);

    /* Error handling functions */

    void errorUnknownChar(char c);

    void errorUnclosedString();

    void errorUndefinedEscape(const char* sequence);
}

#endif //OUTPUT_HPP
