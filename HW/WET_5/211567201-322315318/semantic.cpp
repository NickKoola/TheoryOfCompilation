#include "semantic.hpp"
#include <iostream>
SemanticVisitor::SemanticVisitor(output::CodeBuffer &buffer) : whileDepth(0), currentFunctionName(""),codeBuffer(buffer) {
    emitRuntimeHelperFunctions();
}

std::string SemanticVisitor::getLLVMType(ast::BuiltInType type) {
    switch (type) {
        case ast::BuiltInType::INT:
            return "i32";
        case ast::BuiltInType::BYTE:
            return "i8";
        case ast::BuiltInType::BOOL:
            return "i1";
        case ast::BuiltInType::STRING:
            return "i8*";
        case ast::BuiltInType::VOID:
            return "void";
        default:
            throw std::runtime_error("Unknown type");
    }
}

std::string SemanticVisitor::emitBinaryOperation(const std::string &left, const std::string &right, const std::string &op, ast::BuiltInType type) {
    std::string resultVar = codeBuffer.freshVar();
    codeBuffer.emit(resultVar + " = " + op + " " + getLLVMType(type) + " " + left + ", " + right);
    return resultVar;
} 

void SemanticVisitor::emitRuntimeHelperFunctions() {
    // Declare external functions
    codeBuffer.emit("declare void @print_error_message()");
    codeBuffer.emit("declare void @exit(i32)");
    codeBuffer.emit("declare i32 @printf(i8*, ...)");

    // Declare constants for format specifiers
    codeBuffer.emit("@.int_specifier = constant [4 x i8] c\"%d\\0A\\00\"");
    codeBuffer.emit("@.str_specifier = constant [4 x i8] c\"%s\\0A\\00\"");

    codeBuffer.emit("@.div_zero_msg = constant [24 x i8] c\"Error division by zero\\0A\\00\"");
}


// Map to convert BuiltInType to string
static const std::map<ast::BuiltInType, std::string> typeToString = {
    {ast::BuiltInType::VOID, "void"},
    {ast::BuiltInType::BOOL, "bool"},
    {ast::BuiltInType::BYTE, "byte"},
    {ast::BuiltInType::INT, "int"},
    {ast::BuiltInType::STRING, "string"}
};

static const std::map<std::string, ast::BuiltInType> stringToType = {
    {"void", ast::BuiltInType::VOID},
    {"bool", ast::BuiltInType::BOOL},
    {"byte", ast::BuiltInType::BYTE},
    {"int", ast::BuiltInType::INT},
    {"string", ast::BuiltInType::STRING}
};




void SemanticVisitor::visit(ast::Num &node) {
    node.type = ast::BuiltInType::INT;
    node.llvmValue = std::to_string(node.value);
}

void SemanticVisitor::visit(ast::NumB &node) {
    if (node.value > 255) {
        output::errorByteTooLarge(node.line, node.value);
    }
    node.type = ast::BuiltInType::BYTE;
    node.llvmValue = std::to_string(node.value);
}

void SemanticVisitor::visit(ast::String &node) {
    node.type = ast::BuiltInType::STRING;
    node.llvmValue = node.value;

}

void SemanticVisitor::visit(ast::Bool &node) {
    node.type = ast::BuiltInType::BOOL;
    node.llvmValue = node.value ? "1" : "0";
}

void SemanticVisitor::visit(ast::ID &node) {
    Sym *symbol = symbolTables.getSymbol(node.value);
    if (symbol == nullptr) {
        output::errorUndef(node.line, node.value);
    }
    node.type = symbol->getType();
    node.llvmValue = symbol->getEmittedName(); // the register name

    if (!symbol->isFunctionSymbol()) {
        string resultVar = codeBuffer.freshVar();
        if(node.llvmValue[1] == 't') {
            codeBuffer.emit(resultVar + " = load " + getLLVMType(node.type) + ", " + getLLVMType(node.type) + "* " + node.llvmValue);
            node.llvmValue = resultVar;
        }
    }
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
        //convert byte to int
        string new_reg = codeBuffer.freshVar();
        codeBuffer.emit(new_reg + " = zext i8 " + node.right->llvmValue + " to i32");
        node.right->llvmValue = new_reg;
    } else if (node.left->type == ast::BuiltInType::BYTE && node.right->type == ast::BuiltInType::INT) {
        node.type = ast::BuiltInType::INT;
        //convert byte to int
        string new_reg = codeBuffer.freshVar();
        codeBuffer.emit(new_reg + " = zext i8 " + node.left->llvmValue + " to i32");
        node.left->llvmValue = new_reg;
    } else { 
        output::errorMismatch(node.line);
    }

    string resultVar;
    
    switch (node.op) {
        case ast::BinOpType::ADD:
            resultVar = emitBinaryOperation(node.left->llvmValue, node.right->llvmValue, "add", node.type);
            break;
        case ast::BinOpType::SUB:
            resultVar = emitBinaryOperation(node.left->llvmValue, node.right->llvmValue, "sub", node.type);
            break;
        case ast::BinOpType::MUL:
            resultVar = emitBinaryOperation(node.left->llvmValue, node.right->llvmValue, "mul", node.type);
            break;
        case ast::BinOpType::DIV: {
            std::string isZeroCheck = codeBuffer.freshVar();
            std::string errorLabel = codeBuffer.freshLabel();
            std::string continueLabel = codeBuffer.freshLabel();
            
            //check div by zero
            codeBuffer.emit(isZeroCheck + " = icmp eq "+getLLVMType(node.type)+" " + node.right->llvmValue + ", 0");
            codeBuffer.emit("br i1 " + isZeroCheck + ", label " + errorLabel + ", label " + continueLabel);
            codeBuffer.emitLabel(errorLabel);
            codeBuffer.emit("call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([24 x i8], [24 x i8]* @.div_zero_msg, i32 0, i32 0))");
            codeBuffer.emit("call void @exit(i32 1)");
            codeBuffer.emit("br label " + continueLabel);

            codeBuffer.emitLabel(continueLabel);
            std::string divOp = (node.type == ast::BuiltInType::INT) ? "sdiv" : "udiv";
            resultVar = emitBinaryOperation(node.left->llvmValue, node.right->llvmValue, divOp, node.type);
            break;
        }
        default:
            throw std::runtime_error("Unknown binary operation");
    }
    node.llvmValue = resultVar;
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

    node.type = ast::BuiltInType::BOOL;
    std::string leftValue = node.left->llvmValue;
    std::string rightValue = node.right->llvmValue;

    if (node.left->type == ast::BuiltInType::BYTE && node.right->type == ast::BuiltInType::INT) {
        std::string extendedLeft = codeBuffer.freshVar();
        codeBuffer.emit(extendedLeft + " = zext i8 " + leftValue + " to i32");
        leftValue = extendedLeft;
    } else if (node.left->type == ast::BuiltInType::INT && node.right->type == ast::BuiltInType::BYTE) {
        std::string extendedRight = codeBuffer.freshVar();
        codeBuffer.emit(extendedRight + " = zext i8 " + rightValue + " to i32");
        rightValue = extendedRight;
    }

    
    std::string op;
    switch (node.op) {
        case ast::RelOpType::EQ:
            op = "eq";
            break;
        case ast::RelOpType::NE:
            op = "ne";
            break;
        case ast::RelOpType::LT:
            op = "slt";
            break;
        case ast::RelOpType::LE:
            op = "sle";
            break;
        case ast::RelOpType::GT:
            op = "sgt";
            break;
        case ast::RelOpType::GE:
            op = "sge";
            break;
        default:
            throw std::runtime_error("Unknown relational operation");
    }
    std::string resultVar = codeBuffer.freshVar();
    if(node.left->type == ast::BuiltInType::BYTE && node.right->type == ast::BuiltInType::BYTE)
    {
        codeBuffer.emit(resultVar + " = icmp " + op + " i8 " + leftValue + ", " + rightValue);
    }
    else
    {
        codeBuffer.emit(resultVar + " = icmp " + op + " i32 " + leftValue + ", " + rightValue);
    }
    
    node.llvmValue = resultVar;
}


void SemanticVisitor::visit(ast::Not &node) { 
    node.exp->accept(*this);

    if(node.exp->type == ast::BuiltInType::BOOL) {
        node.type = ast::BuiltInType::BOOL;
    } else {
        output::errorMismatch(node.line);
    }

    string resultVar = codeBuffer.freshVar();
    codeBuffer.emit(resultVar + " = xor i1 1, " + node.exp->llvmValue);
    node.llvmValue = resultVar;
}

void SemanticVisitor::visit(ast::And &node) {
    // THIS WORKS
    string trueLabel = codeBuffer.freshLabel();
    string falseLabel = codeBuffer.freshLabel();
    string endLabel = codeBuffer.freshLabel();

    string reg = codeBuffer.freshVar();
    string in_memmory = codeBuffer.freshVar();
    codeBuffer.emit(in_memmory + " = alloca i1");
    codeBuffer.emit("store i1 0 , i1* " + in_memmory);

    node.left->accept(*this);
    if(node.left->type != ast::BuiltInType::BOOL) {
        output::errorMismatch(node.line);
    }
    node.type = ast::BuiltInType::BOOL;

    codeBuffer.emit("br i1 " + node.left->llvmValue + ", label " + trueLabel + ", label " + falseLabel);

    codeBuffer.emitLabel(trueLabel);
    string true_reg = codeBuffer.freshVar();
    node.right->accept(*this);
    if(node.right->type != ast::BuiltInType::BOOL) {
        output::errorMismatch(node.line);
    }
    codeBuffer.emit(true_reg + " = and i1 " + node.left->llvmValue + ", " + node.right->llvmValue);
    codeBuffer.emit("store i1 " + true_reg + ", i1* " + in_memmory);
    codeBuffer.emit("br label " + endLabel);

    codeBuffer.emitLabel(falseLabel);
    string false_reg = codeBuffer.freshVar();
    codeBuffer.emit(false_reg + " = add i1 0, 0");
    codeBuffer.emit("store i1 " + false_reg + ", i1* " + in_memmory);
    codeBuffer.emit("br label " + endLabel);

    codeBuffer.emitLabel(endLabel);
    codeBuffer.emit(reg + " = load i1, i1* " + in_memmory);
    node.llvmValue = reg;
}

void SemanticVisitor::visit(ast::Or &node) {

    string trueLabel = codeBuffer.freshLabel();
    string falseLabel = codeBuffer.freshLabel();
    string endLabel = codeBuffer.freshLabel();

    string reg = codeBuffer.freshVar();
    string in_memmory = codeBuffer.freshVar();
    codeBuffer.emit(in_memmory + " = alloca i1");
    codeBuffer.emit("store i1 0 , i1* " + in_memmory);

    node.left->accept(*this);
    if(node.left->type != ast::BuiltInType::BOOL) {
        output::errorMismatch(node.line);
    }
    node.type = ast::BuiltInType::BOOL;

    codeBuffer.emit("br i1 " + node.left->llvmValue + ", label " + trueLabel + ", label " + falseLabel);

    codeBuffer.emitLabel(falseLabel);
    string false_reg = codeBuffer.freshVar();
    node.right->accept(*this);
    if(node.right->type != ast::BuiltInType::BOOL) {
        output::errorMismatch(node.line);
    }
    codeBuffer.emit(false_reg + " = or i1 " + node.left->llvmValue + ", " + node.right->llvmValue);
    codeBuffer.emit("store i1 " + false_reg + ", i1* " + in_memmory);
    codeBuffer.emit("br label " + endLabel);

    codeBuffer.emitLabel(trueLabel);
    string true_reg = codeBuffer.freshVar();
    codeBuffer.emit(true_reg + " = add i1 1, 0");
    codeBuffer.emit("store i1 " + true_reg + ", i1* " + in_memmory);
    codeBuffer.emit("br label " + endLabel);

    codeBuffer.emitLabel(endLabel);
    codeBuffer.emit(reg + " = load i1, i1* " + in_memmory);
    node.llvmValue = reg;

}

void SemanticVisitor::visit(ast::Type &node) {
    node.type = node.type;
}

void SemanticVisitor::visit(ast::Cast &node) {
    node.exp->accept(*this);

    std::string sourceType = getLLVMType(node.exp->type);
    std::string targetType = getLLVMType(node.target_type->type);
    //
    if(node.exp->type != node.target_type->type)
    {
        if((node.exp->type == ast::BuiltInType::INT && node.target_type->type == ast::BuiltInType::BYTE) ||
        (node.exp->type == ast::BuiltInType::BYTE && node.target_type->type == ast::BuiltInType::INT)) {
            node.type = node.target_type->type;
        } else {
            output::errorMismatch(node.line);
        }

        std::string resultVar = codeBuffer.freshVar();

        if (sourceType == "i8" && targetType == "i32") {
            if (node.exp->type == ast::BuiltInType::BYTE) {
                codeBuffer.emit(resultVar + " = zext i8 " + node.exp->llvmValue + " to i32");
            } else {
                codeBuffer.emit(resultVar + " = sext i8 " + node.exp->llvmValue + " to i32");
            }
        } else if (sourceType == "i32" && targetType == "i8") {
            codeBuffer.emit(resultVar + " = trunc i32 " + node.exp->llvmValue + " to i8");
        } else {
            throw std::runtime_error("Unsupported cast from " + sourceType + " to " + targetType);
        }
        node.llvmValue = resultVar;
    } else {
        node.type = node.target_type->type;
        node.llvmValue = node.exp->llvmValue;
        return;
    }

}

void SemanticVisitor::visit(ast::ExpList &node) {
    for(auto &exp : node.exps) {
        exp->accept(*this);
    }
}

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
                std::string extendedVar = codeBuffer.freshVar();
                codeBuffer.emit(extendedVar + " = zext i8 " +  node.args->exps[i]->llvmValue + " to i32");
                //cout<< " 1 in call" << endl;
                node.args->exps[i]->llvmValue = extendedVar;
            } else if (node.args->exps[i]->type != ast::BuiltInType::INT) {
                std::vector<std::string> expectedParamTypes = {"INT"};
                output::errorPrototypeMismatch(node.line, "printi", expectedParamTypes);
            }
            continue; // Skip further checks for printi
        }
        //allow int to byte conversion
        if (formals[i] == ast::BuiltInType::BYTE && node.args->exps[i]->type == ast::BuiltInType::INT) {
            if (auto numNode = std::dynamic_pointer_cast<ast::NumB>(node.args->exps[i])) {
                if (numNode->value > 255) {
                    output::errorByteTooLarge(node.line, numNode->value);
                }
            } 
            std::string extendedVar = codeBuffer.freshVar();
            codeBuffer.emit(extendedVar + " = trunc i32 " + node.args->exps[i]->llvmValue + " to i8");
            node.args->exps[i]->llvmValue = extendedVar;
            node.args->exps[i]->type = ast::BuiltInType::BYTE;


        } 
        if(formals[i] == ast::BuiltInType::INT && node.args->exps[i]->type == ast::BuiltInType::BYTE) {
            std::string extendedVar = codeBuffer.freshVar();
            codeBuffer.emit(extendedVar + " = zext i8 " + node.args->exps[i]->llvmValue + " to i32");
            node.args->exps[i]->llvmValue = extendedVar;
            node.args->exps[i]->type = ast::BuiltInType::INT;
        }
        else if (formals[i] != node.args->exps[i]->type) {
            output::errorPrototypeMismatch(node.line, node.func_id->value,expectedParamTypes);
        }
    }
    node.type = function->getReturnType();

    std::string emittedArgs;
    for (size_t i = 0; i < node.args->exps.size(); ++i) {
        auto &arg = node.args->exps[i];
        emittedArgs += getLLVMType(arg->type) + " " + arg->llvmValue;
        if (i != node.args->exps.size() - 1) {
            emittedArgs += ", ";
        }
    }

    std::string returnType = getLLVMType(node.type);
    std::string resultVar;

    if(node.func_id->value == "print") {
        string ptrVar = codeBuffer.freshVar(); 
        resultVar = codeBuffer.freshVar();
        string strLen = to_string(node.args->exps[0]->llvmValue.length()+1);
        string strVar = codeBuffer.emitString(node.args->exps[0]->llvmValue);
        codeBuffer.emit(ptrVar + " = getelementptr [" + strLen + " x i8], [" + strLen + " x i8]* " + strVar + ", i32 0, i32 0");//need to fix last str
        codeBuffer.emit(resultVar + " = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str_specifier, i32 0, i32 0), i8* " + ptrVar + ")");
        return;
    } else if(node.func_id->value == "printi") {
        resultVar = codeBuffer.freshVar();
        codeBuffer.emit(resultVar + " = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.int_specifier, i32 0, i32 0), i32 " + node.args->exps[0]->llvmValue + ")");
        return;
    }

    if (returnType == "void") {
        codeBuffer.emit("call void @" + node.func_id->value + "(" + emittedArgs + ")");
    } else {
        resultVar = codeBuffer.freshVar();
        codeBuffer.emit(resultVar + " = call " + returnType + " @" + node.func_id->value + "(" + emittedArgs + ")");
        node.llvmValue = resultVar;
    }
}

void SemanticVisitor::visit(ast::Statements &node) {  
    symbolTables.beginScope();

    for (auto &statement : node.statements) {
        statement->accept(*this);
    }
    symbolTables.endScope();
}

void SemanticVisitor::visit(ast::Break &node) { 
    if(whileDepth == 0) {
        output::errorUnexpectedBreak(node.line);
    } 
    codeBuffer.emit("br label " + codeBuffer.getLoopEndLabel());

}

void SemanticVisitor::visit(ast::Continue &node) { 
    if(whileDepth == 0) {
        output::errorUnexpectedContinue(node.line);
    }
    codeBuffer.emit("br label " + codeBuffer.getLoopStartLabel());
}

void SemanticVisitor::visit(ast::Return &node) { 
    Sym *currentFunction = symbolTables.getFunction(currentFunctionName, node.line);
    if (currentFunction == nullptr) {
        output::errorUndefFunc(node.line, currentFunctionName);
    }

    ast::BuiltInType expectedReturnType = currentFunction->getReturnType();

    if (node.exp != nullptr) {
        node.exp->accept(*this);
        std::string returnType = getLLVMType(expectedReturnType);
        
        if (node.exp->type != expectedReturnType) {
            if (!(node.exp->type == ast::BuiltInType::BYTE && expectedReturnType == ast::BuiltInType::INT)) {
                output::errorMismatch(node.line);
            }
            std::string mismatchReg = codeBuffer.freshVar();
            codeBuffer.emit(mismatchReg + " = zext i8 " + node.exp->llvmValue + " to i32");
            node.exp->llvmValue = mismatchReg;
            node.exp->type = ast::BuiltInType::INT;
        }
        codeBuffer.emit("ret " + returnType + " " + node.exp->llvmValue);
    } else {
        if (expectedReturnType != ast::BuiltInType::VOID) {
            output::errorMismatch(node.line);
        }
        codeBuffer.emit("ret void");
    }
}

void SemanticVisitor::visit(ast::If &node) {
    std::string trueLabel = codeBuffer.freshLabel();
    std::string falseLabel = node.otherwise ? codeBuffer.freshLabel() : "";
    std::string endLabel = codeBuffer.freshLabel();

    node.condition->accept(*this);

    if (node.condition->type != ast::BuiltInType::BOOL) {
        output::errorMismatch(node.condition->line);
    }
    symbolTables.beginScope();
    symbolTables.resetFunctionVarOffset();

    if(node.otherwise) { 
        codeBuffer.emit("br i1 " + node.condition->llvmValue + ", label " + trueLabel + ", label " + falseLabel);
    } else {
        codeBuffer.emit("br i1 " + node.condition->llvmValue + ", label " + trueLabel + ", label " + endLabel);
    }

    codeBuffer.emitLabel(trueLabel);

    node.then->accept(*this);

    codeBuffer.emit("br label " + endLabel);

    symbolTables.endScope();
    
    if (node.otherwise != nullptr) { 
        symbolTables.beginScope();
        codeBuffer.emitLabel(falseLabel);
        if(std::dynamic_pointer_cast<ast::Statements>(node.otherwise) != nullptr) {
            symbolTables.resetFunctionVarOffset();
            node.otherwise->accept(*this);
        } else {
            node.otherwise->accept(*this);
        }
        codeBuffer.emit("br label " + endLabel);
        symbolTables.endScope();
    }
    codeBuffer.emitLabel(endLabel);
    
}

void SemanticVisitor::visit(ast::While &node) {
    std::string conditionLabel = codeBuffer.freshLabel();
    std::string loopBodyLabel = codeBuffer.freshLabel();
    std::string endLabel = codeBuffer.freshLabel();

    ++whileDepth;
    codeBuffer.pushLoopLabels(conditionLabel, endLabel);


    codeBuffer.emit("br label " + conditionLabel);
    codeBuffer.emitLabel(conditionLabel);

    node.condition->accept(*this);
    
    if (node.condition->type != ast::BuiltInType::BOOL) {
        output::errorMismatch(node.condition->line);
    }

    codeBuffer.emit("br i1 " + node.condition->llvmValue + ", label " + loopBodyLabel + ", label " + endLabel);
    codeBuffer.emitLabel(loopBodyLabel);


    symbolTables.beginScope();
    symbolTables.resetFunctionVarOffset();

    node.body->accept(*this); 
    
    symbolTables.endScope();

    codeBuffer.emit("br label " + conditionLabel);
    codeBuffer.emitLabel(endLabel);
    codeBuffer.popLoopLabels();

    --whileDepth;
}


void SemanticVisitor::visit(ast::VarDecl &node) {
    if (symbolTables.isSymbolDefined(node.id->value)) {
        output::errorDef(node.line, node.id->value);
    }

    if (symbolTables.isFunctionDefined(node.id->value)) {
        output::errorDefAsFunc(node.line, node.id->value);
    }

    std::string type = getLLVMType(node.type->type);
    std::string resultVar = codeBuffer.freshVar();

    codeBuffer.emit(resultVar + " = alloca " + type);
    std::string initialValue = "0";

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
        initialValue = node.init_exp->llvmValue;

        if(node.type->type != node.init_exp->type) {
            //allow int to byte conversion
            if (!(node.type->type == ast::BuiltInType::INT && node.init_exp->type == ast::BuiltInType::BYTE)) {
                output::errorMismatch(node.line);
            } else { // convert byte to int
                std::string extendedVar = codeBuffer.freshVar();
                codeBuffer.emit(extendedVar + " = zext i8 " + initialValue + " to i32");
                initialValue = extendedVar;
            }
        }
    }


    codeBuffer.emit("store " + type + " " + initialValue + ", " + type + "* " + resultVar);

    symbolTables.insertSymbol(Sym(node.id->value, node.type->type, symbolTables.getFunctionVarOffset(), node.line, resultVar));
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
        if (!(symbol->getType() == ast::BuiltInType::INT && node.exp->type == ast::BuiltInType::BYTE)) {
            output::errorMismatch(node.line);
        }
    } 

    std::string assignedValue = node.exp->llvmValue;
    std::string leftReg = symbol->getEmittedName();
    
    if (symbol->getType() == ast::BuiltInType::INT && node.exp->type == ast::BuiltInType::BYTE) {
        if (auto numNode = std::dynamic_pointer_cast<ast::NumB>(node.exp)) {
            if (numNode->value > 255) {
                output::errorByteTooLarge(node.line, numNode->value);
            }
        }
        std::string extendedVar = codeBuffer.freshVar();
        codeBuffer.emit(extendedVar + " = zext i8 " + assignedValue + " to i32");
        assignedValue = extendedVar;
    }

    std::string llvmType = getLLVMType(symbol->getType());
    codeBuffer.emit("store " + llvmType + " " + assignedValue + ", " + llvmType + "* " + leftReg);

}

void SemanticVisitor::visit(ast::Formal &node) {
    if (symbolTables.isSymbolDefined(node.id->value)) {
        output::errorDef(node.line, node.id->value);
    }
    Sym *symbol = symbolTables.getSymbol(node.id->value);
    string reg = "%arg"+to_string(codeBuffer.getArgCount()); 

    std::string type = getLLVMType(node.type->type);
    codeBuffer<<type;
    codeBuffer<< " "+reg;

    symbolTables.insertSymbol(Sym(node.id->value, node.type->type, symbolTables.getFunctionParamOffset(), node.line, reg));
    symbolTables.decrementFunctionParamOffset();
    node.llvmValue = reg;
}

void SemanticVisitor::visit(ast::Formals &node) {
    size_t size = node.formals.size(); 
    for (size_t i = 0; i < size; ++i) {
        node.formals[i]->accept(*this);
        if (i != size - 1) { 
            codeBuffer << " ,";
        }
    }
}

void SemanticVisitor::visit(ast::FuncDecl &node) {
    currentFunctionName = node.id->value;

    std::string returnType = getLLVMType(node.return_type->type);

    symbolTables.beginScope();

    symbolTables.resetFunctionParamOffset();
    symbolTables.resetFunctionVarOffset();

    codeBuffer << "define " + returnType + " @" + currentFunctionName + "(" ;
    node.formals->accept(*this);
    codeBuffer.emit(") {");
    
    //function aruments allocation
    for(auto &formal : node.formals->formals) {
        std::string llvmType = getLLVMType(formal->type->type);
        
        std::string allocVar = codeBuffer.freshVar();
        
        codeBuffer.emit(allocVar + " = alloca " + llvmType);
        codeBuffer.emit("store " + llvmType + " " + formal->llvmValue + ", " + llvmType + "* " + allocVar);
        formal->llvmValue = allocVar;

        Sym* symbol = symbolTables.getSymbol(formal->id->value);
        symbol->setEmittedName(allocVar);
    }

    for(auto &statement : node.body->statements) {
        statement->accept(*this);
    }

    symbolTables.endScope();

    if (returnType == "void") {
        codeBuffer.emit("ret void");
    }
    else {
        codeBuffer.emit("ret " + returnType + " 0");
    }

    codeBuffer.emit("}");
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

        string FuncRegName = func->id->value;
        symbolTables.insertFunction(Sym(func->id->value, func->return_type->type, paramTypes, func->line ,FuncRegName ));
        
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
    for (const auto &func : node.funcs) {
        func->accept(*this);
    }
    std::cout << codeBuffer;
}
