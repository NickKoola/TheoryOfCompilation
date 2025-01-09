#include "semantic.hpp"
#include <iostream>
SemanticVisitor::SemanticVisitor() : whileDepth(0), currentFunctionName(""), scopePrinter(output::ScopePrinter()) {
    scopePrinter.emitFunc("print", ast::BuiltInType::VOID, {ast::BuiltInType::STRING});
    scopePrinter.emitFunc("printi", ast::BuiltInType::VOID, {ast::BuiltInType::INT});
}

void SemanticVisitor::visit(ast::Num &node) {
    node.type = ast::BuiltInType::INT;
}

void SemanticVisitor::visit(ast::NumB &node) {
    if (node.value > 255) {
        output::errorByteTooLarge(node.line, node.value);
    }
    node.type = ast::BuiltInType::BYTE;
}

void SemanticVisitor::visit(ast::String &node) {
    node.type = ast::BuiltInType::STRING;
}

void SemanticVisitor::visit(ast::Bool &node) {
    node.type = ast::BuiltInType::BOOL;
}

void SemanticVisitor::visit(ast::ID &node) {
    Sym *symbol = symbolTables.getSymbol(node.value);
    if (symbol == nullptr) {
        output::errorUndef(node.line, node.value);
    }
    node.type = symbol->getType();
}



void SemanticVisitor::visit(ast::BinOp &node) {
    node.left->accept(*this);
    node.right->accept(*this);

    if(node.left->type == ast::BuiltInType::INT && node.right->type == ast::BuiltInType::INT) {
        node.type = ast::BuiltInType::INT;
    } else if (node.left->type == ast::BuiltInType::BYTE && node.right->type == ast::BuiltInType::BYTE) {
        node.type = ast::BuiltInType::BYTE;
    } else if (node.left->type == ast::BuiltInType::INT && node.right->type == ast::BuiltInType::BYTE) {
        node.type = ast::BuiltInType::INT;
    } else if (node.left->type == ast::BuiltInType::BYTE && node.right->type == ast::BuiltInType::INT) {
        node.type = ast::BuiltInType::INT;
    } else { 
        output::errorMismatch(node.line);
    }
}

void SemanticVisitor::visit(ast::RelOp &node) {
    node.left->accept(*this);
    node.right->accept(*this);

    if((node.left->type == ast::BuiltInType::INT || node.left->type == ast::BuiltInType::BYTE) && 
        (node.right->type == ast::BuiltInType::INT || node.right->type == ast::BuiltInType::BYTE)) {
        node.type = ast::BuiltInType::BOOL;
    } else {
        output::errorMismatch(node.line);
    }
}

void SemanticVisitor::visit(ast::Not &node) {
    node.exp->accept(*this);

    if(node.exp->type == ast::BuiltInType::BOOL) {
        node.type = ast::BuiltInType::BOOL;
    } else {
        output::errorMismatch(node.line);
    }
}

void SemanticVisitor::visit(ast::And &node) {
    node.left->accept(*this);
    node.right->accept(*this);

    if(node.left->type == ast::BuiltInType::BOOL && node.right->type == ast::BuiltInType::BOOL) {
        node.type = ast::BuiltInType::BOOL;
    } else {
        output::errorMismatch(node.line);
    }
}

void SemanticVisitor::visit(ast::Or &node) {
    node.left->accept(*this);
    node.right->accept(*this);

    if(node.left->type == ast::BuiltInType::BOOL && node.right->type == ast::BuiltInType::BOOL) {
        node.type = ast::BuiltInType::BOOL;
    } else {
        output::errorMismatch(node.line);
    }
}

void SemanticVisitor::visit(ast::Type &node) {
    node.type = node.type; // useless
}

void SemanticVisitor::visit(ast::Cast &node) {
    node.exp->accept(*this);
    
    if(node.exp->type != node.target_type->type)
    {
        if((node.exp->type == ast::BuiltInType::INT && node.target_type->type == ast::BuiltInType::BYTE) ||
        (node.exp->type == ast::BuiltInType::BYTE && node.target_type->type == ast::BuiltInType::INT)) {
            node.type = node.target_type->type;
        } else {
            output::errorMismatch(node.line);
        }
    }
    else
    {
        node.type = node.target_type->type;
    }
}

void SemanticVisitor::visit(ast::ExpList &node) {
    for(auto &exp : node.exps) {
        exp->accept(*this);
    }
}
// Map to convert BuiltInType to string
static const std::map<ast::BuiltInType, std::string> typeToString = {
    {ast::BuiltInType::VOID, "void"},
    {ast::BuiltInType::BOOL, "bool"},
    {ast::BuiltInType::BYTE, "byte"},
    {ast::BuiltInType::INT, "int"},
    {ast::BuiltInType::STRING, "string"}
};

void SemanticVisitor::visit(ast::Call &node) {
    Sym *function = symbolTables.getFunction(node.func_id->value, node.line);
    if (function == nullptr) { 
        Sym *symbol = symbolTables.getSymbol(node.func_id->value);
        if (symbol != nullptr) {
            output::errorDefAsVar(node.line, node.func_id->value);
        } else {
            output::errorUndefFunc(node.line, node.func_id->value);
        }
    }

    const auto &formals = function->getFormalsTypes();
    std::vector<std::string> expectedParamTypes;
    for (const auto &type : formals) {
        std::string upperCaseType = typeToString.at(type);
        std::transform(upperCaseType.begin(), upperCaseType.end(), upperCaseType.begin(), ::toupper);
        expectedParamTypes.push_back(upperCaseType);
    }
    if (formals.size() != node.args->exps.size()) {
        output::errorPrototypeMismatch(node.line, node.func_id->value, expectedParamTypes);
    }

    for (size_t i = 0; i < node.args->exps.size(); ++i) {
        node.args->exps[i]->accept(*this); 
        // Disallow string arguments for non-print functions   
        if (node.func_id->value == "print") {
            if (node.args->exps[i]->type != ast::BuiltInType::STRING) {
                std::vector<std::string> expectedParamTypes = {"STRING"};
                output::errorPrototypeMismatch(node.line, "print", expectedParamTypes);
            }
            continue; // Skip further checks for print
        }
        
        if (node.func_id->value == "printi") {
            if (node.args->exps[i]->type == ast::BuiltInType::BYTE) {
                if (auto numNode = std::dynamic_pointer_cast<ast::NumB>(node.args->exps[i])) {
                    if (numNode->value > 255) {
                        output::errorByteTooLarge(node.line, numNode->value);
                    }
                }
            } else if (node.args->exps[i]->type != ast::BuiltInType::INT) {
                std::vector<std::string> expectedParamTypes = {"INT"};
                output::errorPrototypeMismatch(node.line, "printi", expectedParamTypes);
            }
            continue; // Skip further checks for printi
        }
        //allow int to byte conversion
        if (formals[i] == ast::BuiltInType::BYTE && node.args->exps[i]->type == ast::BuiltInType::INT) {
            if (auto numNode = std::dynamic_pointer_cast<ast::Num>(node.args->exps[i])) {
                if (numNode->value > 255) {
                    output::errorByteTooLarge(node.line, numNode->value);
                }
            }
        } else if (formals[i] != node.args->exps[i]->type) {
            output::errorPrototypeMismatch(node.line, node.func_id->value,expectedParamTypes);
        }
    }
    node.type = function->getReturnType();
}

void SemanticVisitor::visit(ast::Statements &node) {  
    symbolTables.beginScope();
    scopePrinter.beginScope();

    for (auto &statement : node.statements) {
        statement->accept(*this);
    }

    scopePrinter.endScope();
    symbolTables.endScope();
}

void SemanticVisitor::visit(ast::Break &node) {
    if(whileDepth == 0) {
        output::errorUnexpectedBreak(node.line);
    } 

}

void SemanticVisitor::visit(ast::Continue &node) {
    if(whileDepth == 0) {
        output::errorUnexpectedContinue(node.line);
    }
}

void SemanticVisitor::visit(ast::Return &node) { 
    Sym *currentFunction = symbolTables.getFunction(currentFunctionName, node.line);
    if (currentFunction == nullptr) {
        output::errorUndefFunc(node.line, currentFunctionName);
    }

    ast::BuiltInType expectedReturnType = currentFunction->getReturnType();

    if (node.exp != nullptr) {
        node.exp->accept(*this);
        if (node.exp->type != expectedReturnType) {
            if (!(node.exp->type == ast::BuiltInType::BYTE && expectedReturnType == ast::BuiltInType::INT)) {
                output::errorMismatch(node.line);
            }
        }
    } else {
        if (expectedReturnType != ast::BuiltInType::VOID) {
            output::errorMismatch(node.line);
        }
    }
}

void SemanticVisitor::visit(ast::If &node) {
    node.condition->accept(*this);

    if (node.condition->type != ast::BuiltInType::BOOL) {
        output::errorMismatch(node.condition->line);
    }
    //scope for the `if` condition
    symbolTables.beginScope();
    scopePrinter.beginScope();
    
    symbolTables.resetFunctionVarOffset();

    if(std::dynamic_pointer_cast<ast::Statements>(node.then) != nullptr) {
        node.then->accept(*this);
    } else {
        node.then->accept(*this);
    }

    scopePrinter.endScope();
    symbolTables.endScope();
    //same but for 'else' 
    if (node.otherwise != nullptr) { 
        symbolTables.beginScope();
        scopePrinter.beginScope();
        if(std::dynamic_pointer_cast<ast::Statements>(node.otherwise) != nullptr) {
            symbolTables.resetFunctionVarOffset();

            node.otherwise->accept(*this);
        } else {
            node.otherwise->accept(*this);
        }
        scopePrinter.endScope();
        symbolTables.endScope();
    }
}

void SemanticVisitor::visit(ast::While &node) {
    node.condition->accept(*this);
    if (node.condition->type != ast::BuiltInType::BOOL) {
        output::errorMismatch(node.condition->line);
    }
    //scope for the `while` condition
    symbolTables.beginScope();
    scopePrinter.beginScope();
     
    symbolTables.resetFunctionVarOffset();

    ++whileDepth;
    node.body->accept(*this);
    --whileDepth;

    scopePrinter.endScope();
    symbolTables.endScope();
}

void SemanticVisitor::visit(ast::VarDecl &node) {
    if (symbolTables.isSymbolDefined(node.id->value)) {
        output::errorDef(node.line, node.id->value);
    }

    if (symbolTables.isFunctionDefined(node.id->value)) {
        output::errorDefAsFunc(node.line, node.id->value);
    }

    if(node.init_exp != nullptr) {
        node.init_exp->accept(*this);
        if (auto idNode = std::dynamic_pointer_cast<ast::ID>(node.init_exp)) {
            if (symbolTables.isFunctionDefined(idNode->value)) {
                output::errorDefAsFunc(idNode->line, idNode->value);
            }
            if (!symbolTables.isSymbolDefined(idNode->value)) {
                output::errorUndef(idNode->line, idNode->value);
            }
        }
    
        if(node.type->type != node.init_exp->type) {
            //allow int to byte conversion
            if (!(node.type->type == ast::BuiltInType::INT && node.init_exp->type == ast::BuiltInType::BYTE)) {
                output::errorMismatch(node.line);
            } 
        }
    }
    scopePrinter.emitVar(node.id->value, node.type->type, symbolTables.getFunctionVarOffset());
    symbolTables.insertSymbol(Sym(node.id->value, node.type->type, symbolTables.getFunctionVarOffset(), node.line));
}

void SemanticVisitor::visit(ast::Assign &node) {
    Sym *symbol = symbolTables.getSymbol(node.id->value);
    if (symbol == nullptr) {
        output::errorUndef(node.line, node.id->value);
    }

    node.exp->accept(*this);

    if (symbol->isFunctionSymbol()) {
        output::errorDefAsFunc(node.line, node.id->value);
    } 
    if (auto idNode = std::dynamic_pointer_cast<ast::ID>(node.exp)) {
            if (symbolTables.isFunctionDefined(idNode->value)) {
                output::errorDefAsFunc(idNode->line, idNode->value);
            }
            if (!symbolTables.isSymbolDefined(idNode->value)) {
                output::errorUndef(idNode->line, idNode->value);
            }
    }
    if (symbol->getType() != node.exp->type) {
        //allow int to byte conversion
        if (!(symbol->getType() == ast::BuiltInType::INT && node.exp->type == ast::BuiltInType::BYTE)) {
            output::errorMismatch(node.line);
        }
    } 
    if (symbol->getType() == ast::BuiltInType::INT && node.exp->type == ast::BuiltInType::BYTE) {
        if (auto numNode = std::dynamic_pointer_cast<ast::Num>(node.exp)) {
            if (numNode->value > 255) {
                output::errorByteTooLarge(node.line, numNode->value);
            }
        }
    }
}

void SemanticVisitor::visit(ast::Formal &node) {
    if (symbolTables.isSymbolDefined(node.id->value)) {
        output::errorDef(node.line, node.id->value);
    }
    scopePrinter.emitVar(node.id->value, node.type->type, symbolTables.getFunctionParamOffset());
    symbolTables.insertSymbol(Sym(node.id->value, node.type->type, symbolTables.getFunctionParamOffset(), node.line));
    symbolTables.decrementFunctionParamOffset();
}

void SemanticVisitor::visit(ast::Formals &node) {
    for (auto &formal : node.formals) {
        formal->accept(*this);
    }
}

void SemanticVisitor::visit(ast::FuncDecl &node) {
    currentFunctionName = node.id->value;

    symbolTables.beginScope();
    scopePrinter.beginScope();

    symbolTables.resetFunctionParamOffset();
    symbolTables.resetFunctionVarOffset();

    node.formals->accept(*this);

    for(auto &statement : node.body->statements) {
        statement->accept(*this);
    }

    scopePrinter.endScope();
    symbolTables.endScope();
    
    currentFunctionName = "";
}

void SemanticVisitor::visit(ast::Funcs &node) { 
    bool hasMain = false;

    for (const auto &func : node.funcs) {
        if (func->id->value == "print" || func->id->value == "printi") {
            continue;  // Skip built-in functions
        }
        if (symbolTables.isFunctionDefined(func->id->value)) {
            output::errorDef(func->id->line, func->id->value);
        }

        std::vector<ast::BuiltInType> paramTypes;
        for (const auto &formal : func->formals->formals) {
            paramTypes.push_back(formal->type->type);
        }

        scopePrinter.emitFunc(func->id->value, func->return_type->type, paramTypes);
        symbolTables.insertFunction(Sym(func->id->value, func->return_type->type, paramTypes, func->line));
        
        if (func->id->value == "main") {
            hasMain = true;
            if (!func->formals->formals.empty() || func->return_type->type != ast::BuiltInType::VOID) {
                output::errorMainMissing();
            }
        }
    }

    if (!hasMain) {
        output::errorMainMissing();
    }
    // Second pass
    for (const auto &func : node.funcs) {
        func->accept(*this);
    }
    std::cout << scopePrinter;
}
