#ifndef SYMTAB_HPP
#define SYMTAB_HPP

#include <vector>
#include <stack>
#include <string>
#include <unordered_map>
#include "nodes.hpp"
#include "output.hpp"

#include <iostream>

class Sym{
private:
    std::string name;
    ast::BuiltInType type;
    int offset;

    bool isFunction;
    ast::BuiltInType ret_type;
    std::vector<ast::BuiltInType> formals_types;

    int line;

public:
    Sym() : name(""), type(ast::BuiltInType::VOID), offset(0), isFunction(false), ret_type(ast::BuiltInType::VOID) {}

    Sym(const std::string &name, ast::BuiltInType type, int offset, int line) 
        : name(name), type(type), offset(offset), isFunction(false), ret_type(ast::BuiltInType::VOID), line(line) {}

    Sym(const std::string &name, ast::BuiltInType ret_type, const std::vector<ast::BuiltInType> &formals_types, int line)
        : name(name), type(ast::BuiltInType::VOID), offset(0), isFunction(true), ret_type(ret_type), formals_types(formals_types), line(line) {}

    bool isVariable() const;
    std::string getName() const;
    ast::BuiltInType getType() const;
    ast::BuiltInType getReturnType() const;
    int getOffset() const;
    const std::vector<ast::BuiltInType> getFormalsTypes() const;
    bool isFunctionSymbol() const;
    int getLine() const { return line; }
    void setOffset(int offset) { this->offset = offset; }
};
// A Signle Scope Table
class Table{
private:
    std::unordered_map<std::string, Sym> symbols;

public:
    Table() = default;
    ~Table() = default;
    
    void insertSymbol(const Sym &sym);
    bool findSymbolInTable(const std::string &name) const;
    Sym* getSymbol(const std::string &name);
};

class Tables{
private:
    std::vector<Table> scopes;
    Table globalFunctions;
    std::vector<int> scopeOffsets;
    int functionParamOffset;
    int functionVarOffset;

public:
    Tables();
    ~Tables() = default;
    void beginScope();
    void endScope();

    bool isSymbolDefined(const std::string &name);
    Sym* getSymbol(const std::string &name);
    void insertSymbol(const Sym &sym);
    
    void insertFunction(const Sym &sym);
    bool isFunctionDefined(const std::string &name);
    Sym* getFunction(const std::string &name, int lineno);

    void resetFunctionParamOffset() { functionParamOffset = -1; }
    void resetFunctionVarOffset() { functionVarOffset = 0; }

    int getFunctionParamOffset() const { return functionParamOffset; }
    int getFunctionVarOffset() const { return scopeOffsets.back(); }

    void decrementFunctionParamOffset() { functionParamOffset--; }
};

#endif //SYMTAB_HPP