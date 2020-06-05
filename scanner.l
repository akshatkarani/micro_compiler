%{
    #include <iostream>
    #include <vector>
    #include <string>
    #include "utils/ast.hpp"
    #include "parser.tab.h"

    extern "C" int yylex();
%}

%option noyywrap
%option yylineno

DIGIT               [0-9]
ID                  [a-zA-Z][a-zA-Z0-9]*
COMMENT             #.*\n
ADD_OP              "+"
SUB_OP              "-"
MULT_OP             "*"
DIV_OP              "/"
EQ_OP               "="
NOTEQ_OP            "!="
LT_OP               "<"
GT_OP               ">"
OPEN_PARENTHESIS    "("
CLOSED_PARENTHESIS  ")"
SEMICOLON          ";"
COMMA               ","
LTE_OP              "<="
GTE_OP              ">="
ASSIGN_OP           ":="

%%

{COMMENT}                       {}

{ADD_OP}                        { return ADD_OP; }
{SUB_OP}                        { return SUB_OP; }
{MULT_OP}                       { return MULT_OP; }
{DIV_OP}                        { return DIV_OP; }
{EQ_OP}                         { return EQ_OP; }
{NOTEQ_OP}                      { return NOTEQ_OP; }
{LT_OP}                         { return LT_OP; }
{GT_OP}                         { return GT_OP; }
{OPEN_PARENTHESIS}              { return OPEN_PARENTHESIS; }
{CLOSED_PARENTHESIS}            { return CLOSED_PARENTHESIS; }
{SEMICOLON}                     { return SEMICOLON; }
{COMMA}                         { return COMMA; }
{LTE_OP}                        { return LTE_OP; }
{GTE_OP}                        { return GTE_OP; }
{ASSIGN_OP}                     { return ASSIGN_OP; }

PROGRAM							{ return _PROGRAM; }
BEGIN							{ return _BEGIN; }
END								{ return _END; }
FUNCTION						{ return _FUNCTION; }
READ							{ return _READ; }
WRITE							{ return _WRITE; }
IF								{ return _IF; }
ELSE							{ return _ELSE; }
FI							    { return _FI; }
FOR							    { return _FOR; }
ROF						        { return _ROF; }
RETURN							{ return _RETURN; }
INT								{ return _INT; }
VOID							{ return _VOID; }
STRING							{ return _STRING; }
FLOAT							{ return _FLOAT; }

{DIGIT}+                        { yylval.intval = atoi(yytext);
                                  return INTLITERAL; }

\"[^"]*\"                       { yylval.strval = new std::string(yytext);
                                  return STRINGLITERAL; }

{ID}                            { yylval.strval = new std::string(yytext); 
                                  return IDENTIFIER; }

{DIGIT}*\.{DIGIT}+              { yylval.floatval = atof(yytext);
                                  return FLOATLITERAL; }


.|\n                            {}

%%
