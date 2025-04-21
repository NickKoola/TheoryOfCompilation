#include "output.hpp"
#include "nodes.hpp"
#//include "codeGenerator.hpp"
#include "semantic.hpp"
#include <iostream>

// Extern from the bison-generated parser
extern int yyparse();

extern std::shared_ptr<ast::Node> program;

int main() {
    // Parse the input. The result is stored in the global variable `program`
    yyparse();

    //SemanticVisitor semanticVisitor;
    //program->accept(semanticVisitor);

    output::CodeBuffer codeBuffer;
    SemanticVisitor codeGeneratorVisitor(codeBuffer);
    program->accept(codeGeneratorVisitor);
    //std::cout << codeBuffer;
}
