#ifndef SEMANTIC_HPP
#define SEMANTIC_HPP

#include <map>
#include <algorithm>
#include "visitor.hpp"
#include "nodes.hpp"
#include "output.hpp"
#include "symTab.hpp"
#include <set>

class SemanticVisitor : public Visitor {
public:
    int whileDepth = 0;
    //output::ScopePrinter scopePrinter;       
    Tables symbolTables;
    string currentFunctionName;
    output::CodeBuffer &codeBuffer;
    std::string getLLVMType(ast::BuiltInType type);
    std::string emitBinaryOperation(const std::string &left, const std::string &right, const std::string &op, ast::BuiltInType type);
    void emitRuntimeHelperFunctions();

    //SemanticVisitor();
    explicit SemanticVisitor(output::CodeBuffer &buffer);

    void visit(ast::Num &node) override;

    void visit(ast::NumB &node) override;

    void visit(ast::String &node) override;

    void visit(ast::Bool &node) override;

    void visit(ast::ID &node) override;

    void visit(ast::BinOp &node) override;

    void visit(ast::RelOp &node) override;

    void visit(ast::Not &node) override;

    void visit(ast::And &node) override;

    void visit(ast::Or &node) override;

    void visit(ast::Type &node) override;

    void visit(ast::Cast &node) override;

    void visit(ast::ExpList &node) override;

    void visit(ast::Call &node) override;

    void visit(ast::Statements &node) override;

    void visit(ast::Break &node) override;

    void visit(ast::Continue &node) override;

    void visit(ast::Return &node) override;

    void visit(ast::If &node) override;

    void visit(ast::While &node) override;

    void visit(ast::VarDecl &node) override;

    void visit(ast::Assign &node) override;

    void visit(ast::Formal &node) override;

    void visit(ast::Formals &node) override;

    void visit(ast::FuncDecl &node) override;

    void visit(ast::Funcs &node) override;
};

class PhiTrackingVisitor : public SemanticVisitor {
private:
    std::set<std::string> modifiedVars;  // Set of variables modified inside the loop

public:
    explicit PhiTrackingVisitor(output::CodeBuffer &buffer, Tables &symbolTables)
        : SemanticVisitor(buffer), modifiedVars() {}

    void visit(ast::Assign &node) override {
        modifiedVars.insert(node.id->value);  // Track assigned variables
        SemanticVisitor::visit(node);         // Call base class implementation
    }

    void visit(ast::Statements &node) override {
        for (const auto &stmt : node.statements) {
            stmt->accept(*this);
        }
    }

    const std::set<std::string>& getModifiedVars() const {
        return modifiedVars;
    }

    // Use the base class implementation for other AST nodes
    using SemanticVisitor::visit;
};


#endif //SEMANTIC_HPP