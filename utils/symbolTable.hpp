#ifndef SYMBOL_TABLE_HPP
#define SYMBOL_TABLE_HPP

#include "entry.hpp"
#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>

class SymbolTable
{
public:
    std::string scope_name;
    
    // Map of name and Entry
    std::unordered_map<std::string, Entry *> symbols;
    int total_parameters = 0;
    int total_non_parameters = 0;

    
    // To maintain order in which symbols are added
    std::vector<Entry *> ordered_symbols;
    SymbolTable(std::string scope_name)
    {
        this->scope_name = scope_name;
    }

    Entry* findEntry(std::string name)
    {
        return symbols[name];
    }

    bool ifExists(std::string name)
    {
        if (symbols.find(name) == symbols.end())
            return false;
        else
            return true;
    }

    void addEntry(std::string name, std::string type)
    {
        total_non_parameters++;
        Entry* new_entry = new Entry(name, type);
        new_entry->stackname = "$-" + std::to_string(total_non_parameters);
        ordered_symbols.push_back(new_entry);
        symbols[name] = new_entry;
        
    }

    void addEntry(std::string name, std::string type, std::string value)
    {
        total_non_parameters++;
        Entry* new_entry = new Entry(name, type, value);
        new_entry->stackname = value;
        ordered_symbols.push_back(new_entry);
        symbols[name] = new_entry;
    }

    void addEntry(std::string name, std::string type, bool isParameter)
    {
        total_parameters++;
        Entry* new_entry = new Entry(name, type, isParameter);
        new_entry->stackname = "$" + std::to_string(total_parameters+1); //idk why +1
        ordered_symbols.push_back(new_entry);
        symbols[name] = new_entry;
    }

    int linkSize()
    {
        return total_non_parameters;
    }

    void printTable()
    {
        std::cout << "Symbol table " << scope_name << " Paras = "<< total_parameters<< " NonParas = " << total_non_parameters << std::endl;

        for (auto it = ordered_symbols.begin(); it != ordered_symbols.end(); ++it)
        {
            std::cout << "name " << (*it)->name << " type " << (*it)->type << " StackName " << (*it)->stackname;    
            if ((*it)->value != "")
                std::cout << " value " << (*it)->value;
            if((*it)->isParameter)
                std::cout << " isParameter";
            std::cout << std::endl;
        }
    }
};

#endif
