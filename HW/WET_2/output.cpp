#include "output.hpp"
#include <iostream>

namespace output {
    static const std::string child_prefix = "├──";
    static const std::string last_child_prefix = "└──";
    static const std::string child_indent = "│   ";
    static const std::string last_child_indent = "    ";

    /* Helper functions */

    static std::string toString(ast::BuiltInType type) {
        switch (type) {
            case ast::BuiltInType::INT:
                return "int";
            case ast::BuiltInType::BOOL:
                return "bool";
            case ast::BuiltInType::BYTE:
                return "byte";
            case ast::BuiltInType::VOID:
                return "void";
            case ast::BuiltInType::STRING:
                return "string";
            default:
                return "unknown";
        }
    }

    /* Error handling functions */

    void errorLex(int lineno) {
        std::cout << "line " << lineno << ": lexical error\n";
        exit(0);
    }

    void errorSyn(int lineno) {
        std::cout << "line " << lineno << ": syntax error\n";
        exit(0);
    }

    /* PrintVisitor implementation */

    PrintVisitor::PrintVisitor() : indents({last_child_indent}), prefixes({last_child_prefix}) {}

    void PrintVisitor::print_indented(const std::string &str) {
        for (auto it = indents.begin(); it != indents.end() - 1; ++it) {
            std::cout << *it;
        }
        std::cout << prefixes.back() << str << std::endl;
    }

    void PrintVisitor::enter_child() {
        indents.emplace_back(child_indent);
        prefixes.emplace_back(child_prefix);
    }

    void PrintVisitor::enter_last_child() {
        indents.emplace_back(last_child_indent);
        prefixes.emplace_back(last_child_prefix);
    }

    void PrintVisitor::leave_child() {
        indents.pop_back();
        prefixes.pop_back();
    }

    void PrintVisitor::visit(ast::Num &node) {
        print_indented("Num: " + std::to_string(node.value));
    }

    void PrintVisitor::visit(ast::NumB &node) {
        print_indented("NumB: " + std::to_string(node.value));
    }

    void PrintVisitor::visit(ast::String &node) {
        print_indented("String: " + node.value);
    }

    void PrintVisitor::visit(ast::Bool &node) {
        print_indented("Bool: " + std::string((node.value ? "true" : "false")));
    }

    void PrintVisitor::visit(ast::ID &node) {
        print_indented("ID: " + node.value);
    }

    void PrintVisitor::visit(ast::BinOp &node) {
        std::string op;

        switch (node.op) {
            case ast::BinOpType::ADD:
                op = "+";
                break;
            case ast::BinOpType::SUB:
                op = "-";
                break;
            case ast::BinOpType::MUL:
                op = "*";
                break;
            case ast::BinOpType::DIV:
                op = "/";
                break;
        }

        print_indented("BinOp: " + op);

        enter_child();
        node.left->accept(*this);
        leave_child();

        enter_last_child();
        node.right->accept(*this);
        leave_child();
    }

    void PrintVisitor::visit(ast::RelOp &node) {
        std::string op;

        switch (node.op) {
            case ast::RelOpType::EQ:
                op = "==";
                break;
            case ast::RelOpType::NE:
                op = "!=";
                break;
            case ast::RelOpType::LT:
                op = "<";
                break;
            case ast::RelOpType::LE:
                op = "<=";
                break;
            case ast::RelOpType::GT:
                op = ">";
                break;
            case ast::RelOpType::GE:
                op = ">=";
                break;
        }

        print_indented("RelOp: " + op);

        enter_child();
        node.left->accept(*this);
        leave_child();


        enter_last_child();
        node.right->accept(*this);
        leave_child();
    }

    void PrintVisitor::visit(ast::Type &node) {
        print_indented("Type: " + toString(node.type));
    }

    void PrintVisitor::visit(ast::Cast &node) {
        print_indented("Cast");

        enter_child();
        node.exp->accept(*this);
        leave_child();

        enter_last_child();
        node.target_type->accept(*this);
        leave_child();
    }

    void PrintVisitor::visit(ast::Not &node) {
        print_indented("Not");

        enter_last_child();
        node.exp->accept(*this);
        leave_child();
    }

    void PrintVisitor::visit(ast::And &node) {
        print_indented("And");

        enter_child();
        node.left->accept(*this);
        leave_child();

        enter_last_child();
        node.right->accept(*this);
        leave_child();
    }

    void PrintVisitor::visit(ast::Or &node) {
        print_indented("Or");

        enter_child();
        node.left->accept(*this);
        leave_child();

        enter_last_child();
        node.right->accept(*this);
        leave_child();
    }

    void PrintVisitor::visit(ast::ExpList &node) {
        print_indented("ExpList");

        for (auto it = node.exps.rbegin(); it != node.exps.rend(); ++it) {
            if (it != node.exps.rend() - 1) {
                enter_child();
            } else {
                enter_last_child();
            }
            (*it)->accept(*this);
            leave_child();
        }
    }

    void PrintVisitor::visit(ast::Call &node) {
        print_indented("Call");

        enter_child();
        node.func_id->accept(*this);
        leave_child();

        enter_last_child();
        node.args->accept(*this);
        leave_child();
    }

    void PrintVisitor::visit(ast::Statements &node) {
        print_indented("Statements");

        for (auto it = node.statements.begin(); it != node.statements.end(); ++it) {
            if (it != node.statements.end() - 1) {
                enter_child();
            } else {
                enter_last_child();
            }
            (*it)->accept(*this);
            leave_child();
        }
    }

    void PrintVisitor::visit(ast::Break &node) {
        print_indented("Break");
    }

    void PrintVisitor::visit(ast::Continue &node) {
        print_indented("Continue");
    }

    void PrintVisitor::visit(ast::Return &node) {
        print_indented("Return");

        if (node.exp) {
            enter_last_child();
            node.exp->accept(*this);
            leave_child();
        }
    }

    void PrintVisitor::visit(ast::If &node) {
        print_indented("If");

        enter_child();
        node.condition->accept(*this);
        leave_child();

        if (node.otherwise) {
            enter_child();
        } else {
            enter_last_child();
        }
        node.then->accept(*this);
        leave_child();

        if (node.otherwise) {
            enter_last_child();
            node.otherwise->accept(*this);
            leave_child();
        }
    }

    void PrintVisitor::visit(ast::While &node) {
        print_indented("While");

        enter_child();
        node.condition->accept(*this);
        leave_child();

        enter_last_child();
        node.body->accept(*this);
        leave_child();
    }

    void PrintVisitor::visit(ast::VarDecl &node) {
        print_indented("VarDecl");

        enter_child();
        node.id->accept(*this);
        leave_child();

        if (node.init_exp) {
            enter_child();
        } else {
            enter_last_child();
        }
        node.type->accept(*this);
        leave_child();

        if (node.init_exp) {
            enter_last_child();
            node.init_exp->accept(*this);
            leave_child();
        }
    }

    void PrintVisitor::visit(ast::Assign &node) {
        print_indented("Assign");

        enter_child();
        node.id->accept(*this);
        leave_child();

        enter_last_child();
        node.exp->accept(*this);
        leave_child();
    }

    void PrintVisitor::visit(ast::Formal &node) {
        print_indented("Formal");

        enter_child();
        node.id->accept(*this);
        leave_child();

        enter_last_child();
        node.type->accept(*this);
        leave_child();
    }

    void PrintVisitor::visit(ast::Formals &node) {
        print_indented("Formals");

        for (auto it = node.formals.rbegin(); it != node.formals.rend(); ++it) {
            if (it != node.formals.rend() - 1) {
                enter_child();
            } else {
                enter_last_child();
            }
            (*it)->accept(*this);
            leave_child();
        }
    }

    void PrintVisitor::visit(ast::FuncDecl &node) {
        print_indented("FuncDecl");

        enter_child();
        node.id->accept(*this);
        leave_child();

        enter_child();
        node.return_type->accept(*this);
        leave_child();

        enter_child();
        node.formals->accept(*this);
        leave_child();

        enter_last_child();
        node.body->accept(*this);
        leave_child();
    }

    void PrintVisitor::visit(ast::Funcs &node) {
        print_indented("Funcs");

        for (auto it = node.funcs.begin(); it != node.funcs.end(); ++it) {
            if (it != node.funcs.end() - 1) {
                enter_child();
            } else {
                enter_last_child();
            }
            (*it)->accept(*this);
            leave_child();
        }
    }
}
