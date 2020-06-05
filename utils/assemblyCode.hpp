#ifndef ASSEMBLYCODE_HPP
#define ASSEMBLYCODE_HPP

#include "codeObject.hpp"
#include "symbolTable.hpp"
#include <string>
#include <unordered_map>

class AssemblyCode
{
    std::vector<CodeLine *> assembly;
    int register_no = -1;
    std::unordered_map<std::string, std::string> temp_to_reg;
public:
    
    std::string getRegister(std::string temporary)
    {   
        if (tempExists(temporary))
            return temp_to_reg.find(temporary)->second;
        else
        {
            std::string reg = "r" + std::to_string(++register_no);
            this->temp_to_reg[temporary] = reg;
            return reg;
        }
    }

    std::string getNewRegister()
    {
        return "r" + std::to_string(++register_no);
    }

    bool isTemporary(std::string temp)
    {
        return (temp[0] == '$');
    }

    bool tempExists(std::string temp)
    {
        return (temp_to_reg.find(temp) != temp_to_reg.end());
    }

    void generateCode(CodeObject *code, SymbolTableStack *tableStack)
    {
    	
        std::vector<CodeLine *> threeAC = code->threeAC;

        for (auto table : tableStack->tables)
        {
        	if(table->scope_name != "GLOBAL")
        	{
        		assembly.push_back(new CodeLine("GLOBAL", "label", table->scope_name));
        		assembly.push_back(new CodeLine("GLOBAL","link", std::to_string(table->linkSize())));
        	}

            for (auto entry : table->ordered_symbols)
            {
                if (entry->type == "STRING")
                    assembly.push_back(new CodeLine(table->scope_name, "str", entry->name, entry->value));
            }
            if(table->scope_name == "GLOBAL")
        	{
        		assembly.push_back(new CodeLine("GLOBAL", "push",""));
        		assembly.push_back(new CodeLine("GLOBAL", "jsr", "main"));
        		assembly.push_back(new CodeLine("GLOBAL", "sys", "halt"));
        	}

	        for (auto code_line : threeAC)
	        {
	            std::string command = code_line->command;
	            if(code_line->scope == table->scope_name)
	            {
	            	std::string new_arg1 = code_line->arg1;
	            	std::string new_arg2 = code_line->arg2;

	            	if(table->ifExists(new_arg1))
	            	{
	            		new_arg1 = table->findEntry(new_arg1)->stackname;
	            	}
	            	if(table->ifExists(new_arg2))
	            	{
	            		new_arg2 = table->findEntry(new_arg2)->stackname;
	            	}


		            if (command == "READI")
		            {
		                assembly.push_back(new CodeLine(code_line->scope, "sys", "readi", new_arg1));
		            }
		            else if (command == "READF")
		            {
		                assembly.push_back(new CodeLine(code_line->scope, "sys", "readr", new_arg1));
		            }
		            else if (command == "WRITEI")
		            {
		                assembly.push_back(new CodeLine(code_line->scope, "sys", "writei", new_arg1));
		            }
		            else if (command == "WRITEF")
		            {
		                assembly.push_back(new CodeLine(code_line->scope, "sys", "writer", new_arg1));
		            }
		            else if (command == "WRITES")
		            {
		                assembly.push_back(new CodeLine(code_line->scope, "sys", "writes", new_arg1));
		            }
		            else if (command == "STOREI" || command == "STOREF")
		            {
		                new_arg1 = isTemporary(code_line->arg1) ? getRegister(code_line->arg1) : new_arg1;
		                new_arg2 = isTemporary(code_line->arg2) ? getRegister(code_line->arg2) : new_arg2;
		                assembly.push_back(new CodeLine(code_line->scope, "move", new_arg1, new_arg2));
		            }
		            else if (command == "RET")
		            {
		            	new_arg1 = isTemporary(code_line->arg1) ? getRegister(code_line->arg1) : new_arg1;
		            	std::string arg1 = getNewRegister();
		            	assembly.push_back(new CodeLine(code_line->scope, "move", new_arg1, arg1));
		            	std::string retstack = "$" + std::to_string(table->total_parameters + 2);
		            	assembly.push_back(new CodeLine(code_line->scope, "move", arg1, retstack));

		            	if(table->scope_name != "GLOBAL")
			        	{
			        		assembly.push_back(new CodeLine(code_line->scope,"unlnk",""));
			        	}

		            	assembly.push_back(new CodeLine(code_line->scope, "ret", ""));
		            }
		            else if (command == "PUSH")
		            {
		            	new_arg1 = isTemporary(code_line->arg1) ? getRegister(code_line->arg1) : new_arg1;
		            	assembly.push_back(new CodeLine(code_line->scope, "push", new_arg1));
		            }
		            else if (command == "JSR")
		            {
		            	assembly.push_back(new CodeLine(code_line->scope, "jsr", new_arg1));
		            }
		            else if (command == "POP")
		            {
		            	new_arg1 = isTemporary(code_line->arg1) ? getRegister(code_line->arg1) : new_arg1;
		            	assembly.push_back(new CodeLine(code_line->scope, "pop", new_arg1));
		            }
		            else
		            {   std::string command;
		                new_arg1 = isTemporary(code_line->arg1) ? getRegister(code_line->arg1) : new_arg1;
		                std::string arg2 = getNewRegister();
		                assembly.push_back(new CodeLine(code_line->scope, "move", new_arg1, arg2));

		                if (code_line->command == "ADDI")
		                    command = "addi";
		                else if (code_line->command == "SUBI")
		                    command = "subi";
		                else if (code_line->command == "MULTI")
		                    command = "muli";
		                else if (code_line->command == "DIVI")
		                    command = "divi";
		                else if (code_line->command == "ADDF")
		                    command = "addr";
		                else if (code_line->command == "SUBF")
		                    command = "subr";
		                else if (code_line->command == "MULTF")
		                    command = "mulr";
		                else if (code_line->command == "DIVF")
		                    command = "divr";

		                new_arg1 = isTemporary(code_line->arg2) ? getRegister(code_line->arg2) : new_arg2;
		                assembly.push_back(new CodeLine(code_line->scope, command, new_arg1, arg2));
		                if (isTemporary(code_line->arg3))
		                    temp_to_reg[code_line->arg3] = arg2;
		            }
		        }
	        }
	        
	    }
    	assembly.push_back(new CodeLine("idk", "end", ""));
    }

    void printMap()
    {
        for (auto it: temp_to_reg)
            std::cout << it.first << " " << it.second << std::endl;
        std::cout << "PRINTED" << std::endl;
    }

    void print()
    {
        for (auto c: assembly)
            c->print();
    }

};

#endif
