#ifndef AST_HPP
#define AST_HPP

#include "entry.hpp"
#include "codeObject.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <cmath>

class ASTNode
{
public:
    static std::string id_type;
    std::string type;
    ASTNode *left;
    ASTNode *right;

    virtual std::string generateCode(CodeObject *code)
    {
        return "Not Implemented!";
    };
};

class ASTNode_Expr : public ASTNode
{
public:
    std::string type = "EXPR";
    char optype;

    ASTNode_Expr(char optype)
    {
        this->optype = optype;
    }

    std::string generateCode(CodeObject *code)
    {
        std::string left = this->left->generateCode(code);
        std::string right = this->right->generateCode(code);
        std::string temp = code->getTemp();
        std::string command = "";
        if (ASTNode::id_type == "INT")
        {
            if (optype == '+')
                command += "ADDI";
            else if (optype == '-')
                command += "SUBI";
            else if (optype == '*')
                command += "MULTI";
            else if (optype == '/')
                command += "DIVI";
        }
        else if (ASTNode::id_type == "FLOAT")
        {
            if (optype == '+')
                command += "ADDF";
            else if (optype == '-')
                command += "SUBF";
            else if (optype == '*')
                command += "MULTF";
            else if (optype == '/')
                command += "DIVF";
        }
        code->threeAC.push_back(new CodeLine(code->symbolTableStack->table_stack.top()->scope_name, command, left, right, temp));
        return temp;
    }
};

class ASTNode_INT : public ASTNode
{
public:
    std::string type = "INT";
    int value;

    ASTNode_INT(int value)
    {
        this->value = value;
    }

    std::string generateCode(CodeObject *code)
    {
        std::string temp = code->getTemp();
        code->threeAC.push_back(new CodeLine(code->symbolTableStack->table_stack.top()->scope_name, "STOREI", std::to_string(this->value), temp));
        return temp;
    }
};

class ASTNode_FLOAT : public ASTNode
{
public:
    std::string type = "FLOAT";
    float value;

    ASTNode_FLOAT(float value)
    {
        this->value = value;
    }

    std::string generateCode(CodeObject *code)
    {
        std::string temp = code->getTemp();
        char str[10];
        sprintf(str, "%f", value);
        code->threeAC.push_back(new CodeLine(code->symbolTableStack->table_stack.top()->scope_name, "STOREF", str, temp));
        return temp;
    }
};

class ASTNode_ID : public ASTNode
{
public:
    std::string type = "ID";
    Entry *variable;

    ASTNode_ID(Entry *variable)
    {
        this->variable = variable;
    }

    std::string generateCode(CodeObject *code)
    {
        return this->variable->name;
    }
};

class ASTNode_Assign : public ASTNode
{
public:
    std::string type = "ASSIGN";

    ASTNode_Assign(Entry* var)
    {
        this->left = new ASTNode_ID(var);
        ASTNode::id_type = var->type;
    }

    std::string generateCode(CodeObject *code)
    {
        std::string command = "";
        if (ASTNode::id_type == "INT")
            command += "STOREI";
        else if (ASTNode::id_type == "FLOAT")
            command += "STOREF";
        code->threeAC.push_back(new CodeLine(code->symbolTableStack->table_stack.top()->scope_name,
                                             command, 
                                             this->right->generateCode(code),
                                             this->left->generateCode(code)));
        return "";
    }
};

class ASTNode_Return : public ASTNode
{
public:
    std::string type = "RETURN";

    //ASTNode_Return();

    std::string generateCode(CodeObject *code)
    {
        std::string command = "RET";
        code->threeAC.push_back(new CodeLine(code->symbolTableStack->table_stack.top()->scope_name,
                                             command, 
                                             this->right->generateCode(code)));
    }
};

class ASTNode_CallExpr : public ASTNode
{
public:
    std::string type = "CALL";
    std::string func_name;
    std::vector<ASTNode*>* exprlist;

    ASTNode_CallExpr(std::string func_name, std::vector<ASTNode*>* exprlist)
    {
        this->func_name = func_name;
        this->exprlist = exprlist;
    }


    std::string generateCode(CodeObject *code)
    {
        std::string command = "PUSH";
        //Push Register here. Not needed as of now
        //Pushing return 
        code->threeAC.push_back(new CodeLine(code->symbolTableStack->table_stack.top()->scope_name, "PUSH", ""));
        //Pushing arguments. Iterating vector in forward direction iterates parameters in reverse direction. So no issues as the parameters are given names in forward directio 
        for(auto &it : *exprlist)
        {
            std::string topush = (it)->generateCode(code);
            code->threeAC.push_back(new CodeLine(code->symbolTableStack->table_stack.top()->scope_name, "PUSH", topush));
        }
        //Calling function
        code->threeAC.push_back(new CodeLine(code->symbolTableStack->table_stack.top()->scope_name, "JSR", func_name));
        //Popping arguments. Don't Forget to reverse the order from pushing in case of . For now the order is same
        for(auto &it : *exprlist)
        {
            code->threeAC.push_back(new CodeLine(code->symbolTableStack->table_stack.top()->scope_name, "POP", ""));
        }
        //Popping return into register
        std::string temp = code->getTemp();
        code->threeAC.push_back(new CodeLine(code->symbolTableStack->table_stack.top()->scope_name, "POP", temp));
        return temp;

    }
};

#endif
