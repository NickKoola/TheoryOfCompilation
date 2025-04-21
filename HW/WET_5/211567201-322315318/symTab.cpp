#include "symTab.hpp"
#include <iostream>

//--------------------Sym--------------------

bool Sym::isVariable() const
{
    return !isFunction;
}
std::string Sym::getName() const {
    return this->name;
}

ast::BuiltInType Sym::getType() const {
    return this->type;
}

ast::BuiltInType Sym::getReturnType() const{
    return this->ret_type;
}

int Sym::getOffset() const {
    return this->offset;
}

const std::vector<ast::BuiltInType> Sym::getFormalsTypes() const {
    return this->formals_types;
}

bool Sym::isFunctionSymbol() const{
    return this->isFunction;
}

//--------------------Table--------------------

void Table::insertSymbol(const Sym &sym) {
    if (symbols.find(sym.getName()) != symbols.end()) {
        output::errorDef(sym.getLine(), sym.getName());
    }
    symbols[sym.getName()] = sym;
}

bool Table::findSymbolInTable(const std::string &name) const {
    return symbols.find(name) != symbols.end();
}

Sym* Table::getSymbol(const std::string &name) {
    auto it = symbols.find(name);
    return (it != symbols.end()) ? &(it->second) : nullptr;
}

//--------------------Tables--------------------

Tables::Tables() : functionParamOffset(-1), functionVarOffset(0) {
    globalFunctions.insertSymbol(Sym("print", ast::BuiltInType::VOID, std::vector<ast::BuiltInType>{ast::BuiltInType::STRING}, 0, "%print"));
    globalFunctions.insertSymbol(Sym("printi", ast::BuiltInType::VOID, std::vector<ast::BuiltInType>{ast::BuiltInType::INT}, 0, "%printi"));

    scopes.emplace_back();
    scopeOffsets.push_back(0);
}

void Tables::beginScope() {
    scopes.emplace_back();
    int currentOffset = scopeOffsets.empty() ? 0 : scopeOffsets.back();
    scopeOffsets.push_back(currentOffset);

}

void Tables::endScope() {
    if (!scopes.empty()) {
        scopes.pop_back();
        scopeOffsets.pop_back();
    }
}

bool Tables::isSymbolDefined(const std::string &name) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        if (it->findSymbolInTable(name)) {
            return true;
        }
    }
    return globalFunctions.findSymbolInTable(name);
}

Sym* Tables::getSymbol(const std::string &name) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        Sym* sym = it->getSymbol(name);
        if (sym != nullptr) {
            return sym;
        }
    }
    return globalFunctions.getSymbol(name);
}

void Tables::insertSymbol(const Sym &sym) {
    for (const auto& scope : scopes) {
        if (scope.findSymbolInTable(sym.getName())) {
            output::errorDef(sym.getLine(), sym.getName());
        }
    }
  
    scopes.back().insertSymbol(sym);
    if(sym.getOffset() >=0 )
    {
        scopeOffsets.back()++;
    }
    
}

void Tables::insertFunction(const Sym &sym) {
    if (globalFunctions.findSymbolInTable(sym.getName())) {
        output::errorDef(sym.getLine(), sym.getName());
    }
    globalFunctions.insertSymbol(sym);
}

bool Tables::isFunctionDefined(const std::string &name) {
    return globalFunctions.findSymbolInTable(name);
}

Sym* Tables::getFunction(const std::string &name, int lineno) {
    Sym* func = globalFunctions.getSymbol(name);
    return func;
}

