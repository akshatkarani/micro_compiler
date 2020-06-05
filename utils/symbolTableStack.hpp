#ifndef SYMBOL_TABLE_STACK_HPP
#define SYMBOL_TABLE_STACK_HPP

#include "symbolTable.hpp"
#include <vector>
#include <stack>

class SymbolTableStack
{
    std::string error_variable = "";

public:
    std::vector<SymbolTable *> tables;
    std::stack<SymbolTable *> table_stack;
    int block_number = 1;

    void addNewTable(std::string name)
    {
        SymbolTable *temp = new SymbolTable(name);
        table_stack.push(temp);
        tables.push_back(temp);
    }

    void addNewTable()
    {
        SymbolTable *temp = new SymbolTable("BLOCK " + std::to_string(block_number));
        table_stack.push(temp);
        tables.push_back(temp);
        ++block_number;
    }

    void removeTable()
    {
        table_stack.pop();
    }

    void insertSymbol(std::string name, std::string type)
    {
        SymbolTable *table = table_stack.top();
        if (table->ifExists(name) && error_variable == "")
        {
            error_variable = name;
        }
        else
        {
            table->addEntry(name, type);
        }
    }

    void insertSymbol(std::string name, std::string type, std::string value)
    {
        SymbolTable *table = table_stack.top();
        if (table->ifExists(name) && error_variable == "")
        {
            error_variable = name;
        }
        else
        {
            table->addEntry(name, type, value);
        }
    }

    void insertSymbol(std::string name, std::string type, bool isParameter)
    {
        SymbolTable *table = table_stack.top();
        if (table->ifExists(name) && error_variable == "")
        {
            error_variable = name;
        }
        else
        {
            table->addEntry(name, type, isParameter);
        }
    }

    Entry* findEntry(std::string name)
    {
        std::stack<SymbolTable *> temp_stack = table_stack;
        while (temp_stack.size())
        {
            if (temp_stack.top()->ifExists(name))
                return temp_stack.top()->findEntry(name);
            temp_stack.pop();
        }
        return new Entry("error", "error");
    }

    std::string findType(std::string name)
    {
        return findEntry(name)->type;
    }

    void printStack()
    {
        if (error_variable != "")
            std::cout << "DECLARATION ERROR " + error_variable << std::endl;
        else
        {
            for (size_t i = 0; i != tables.size(); ++i)
            {
                tables[i]->printTable();
                if (i != tables.size() - 1)
                    std::cout << std::endl;
            }
        }
    }
};

#endif
