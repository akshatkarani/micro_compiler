#ifndef CodeObject_HPP
#define CodeObject_HPP

#include "codeLine.hpp"
#include "symbolTableStack.hpp"
#include <iostream>
#include <vector>
#include <string>

class CodeObject
{
    int temp_value = 0;
public:
    std::vector<CodeLine*> threeAC;
    SymbolTableStack* symbolTableStack;
    
    CodeObject(SymbolTableStack* symbolTableStack)
    {
        this->symbolTableStack = symbolTableStack;
    }

    std::string getTemp()
    {
        return "$T" + std::to_string(++temp_value);
    }

    void addRead(std::string var_name, std::string type)
    {
        //std::string stack_var_name = ((symbolTableStack->table_stack.top())->findEntry(var_name))->stackname;
        if (type == "INT")
            threeAC.push_back(new CodeLine(symbolTableStack->table_stack.top()->scope_name, "READI", var_name));
        else if (type == "FLOAT")
            threeAC.push_back(new CodeLine(symbolTableStack->table_stack.top()->scope_name, "READF", var_name));
    }

    void addWrite(std::string var_name, std::string type)
    {
        //std::string stack_var_name = ((symbolTableStack->table_stack.top())->findEntry(var_name))->stackname;
        if (type == "INT")
            threeAC.push_back(new CodeLine(symbolTableStack->table_stack.top()->scope_name, "WRITEI", var_name));
        else if (type == "FLOAT")
            threeAC.push_back(new CodeLine(symbolTableStack->table_stack.top()->scope_name, "WRITEF", var_name));
        else if (type == "STRING")
            threeAC.push_back(new CodeLine(symbolTableStack->table_stack.top()->scope_name, "WRITES", var_name));
    }

    void print()
    {
        for (auto s: threeAC)
            s->print();
    }

};

#endif
