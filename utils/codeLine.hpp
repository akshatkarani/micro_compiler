#ifndef CODELINE_HPP
#define CODELINE_HPP

#include <iostream>
#include <string>

class CodeLine
{
public:
    std::string command, arg1 ,arg2, arg3;
    std::string scope;

    CodeLine(std::string scope, std::string command, std::string arg1)
    {
        this->scope = scope;
        this->command = command;
        this->arg1 = arg1;
        this->arg2 = "";
        this->arg3 = "";
    }

    CodeLine(std::string scope, std::string command, std::string arg1, std::string arg2)
    {
        this->scope = scope;
        this->command = command;
        this->arg1 = arg1;
        this->arg2 = arg2;
        this->arg3 = "";
    }

    CodeLine(std::string scope, std::string command, std::string arg1, std::string arg2, std::string arg3)
    {
        this->scope = scope;
        this->command = command;
        this->arg1 = arg1;
        this->arg2 = arg2;
        this->arg3 = arg3;
    }

    void print()
    {
        std::string print_val =  command + " " + arg1;
        if (arg2 != "")
            print_val += " " + arg2;
        if (arg3 != "")
            print_val += " " + arg3;
        std::cout << print_val << std::endl;
    }

};

#endif
