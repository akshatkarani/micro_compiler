%{
    #include <iostream>
    #include <string>
    #include <vector>
    #include "utils/symbolTableStack.hpp"
    #include "utils/codeObject.hpp"
    #include "utils/ast.hpp"
    #include "utils/assemblyCode.hpp"

    void yyerror(const char *msg);

    extern "C" int yyparse();
    extern "C" FILE *yyin;
    extern "C" int yylex();

    SymbolTableStack *tableStack = new SymbolTableStack();
    CodeObject *threeAC = new CodeObject(tableStack);
    AssemblyCode *assembly_code = new AssemblyCode();
%}

%token _PROGRAM
%token _BEGIN
%token _END
%token _FUNCTION
%token _READ
%token _WRITE
%token _IF
%token _ELSE
%token _FI
%token _FOR
%token _ROF
%token _RETURN
%token <intval> _INT
%token _STRING
%token _VOID
%token <floatval> _FLOAT
%token <intval> INTLITERAL
%token <strval> STRINGLITERAL
%token <floatval> FLOATLITERAL
%token <strval> IDENTIFIER
%token ASSIGN_OP
%token ADD_OP
%token SUB_OP
%token MULT_OP
%token DIV_OP
%token EQ_OP
%token NOTEQ_OP
%token LT_OP
%token GT_OP
%token OPEN_PARENTHESIS
%token CLOSED_PARENTHESIS
%token SEMICOLON
%token COMMA
%token LTE_OP
%token GTE_OP

%type <strval> str id
%type <strlist> id_list id_tail
%type <intval> var_type
%type <astnode> mulop addop primary postfix_expr factor_prefix factor expr_prefix expr return_stmt call_expr
%type <astlist> expr_list expr_list_tail

%union {
    int intval;
    float floatval;
    std::string* strval;
    std::vector<std::string*> *strlist;
    ASTNode *astnode;
    std::vector<ASTNode*> *astlist;
}

%%
program:            _PROGRAM id _BEGIN
                    {
                        tableStack->addNewTable("GLOBAL");
                    }
                    pgm_body _END
                    {
                        tableStack->removeTable();
                    };
id:                 IDENTIFIER ;
pgm_body:           decl func_declarations ;
decl:               string_decl decl | var_decl decl | ;

string_decl:        _STRING id ASSIGN_OP str SEMICOLON
                    {
                        tableStack->insertSymbol(*($2), "STRING", *($4));
                    };
str:                STRINGLITERAL ;

var_decl:           var_type id_list SEMICOLON
                    {
                        std::string str_type = "";
                        std::vector<std::string*> list = *$2;
                        for (int i = list.size(); i != 0; --i)
                        {
                            if ($1 == _FLOAT)
                            {
                                tableStack->insertSymbol(*(list[i-1]), "FLOAT");
                            }
                            else if ($1 == _INT)
                            {
                                tableStack->insertSymbol(*(list[i-1]), "INT");
                            }
                        }
                    };
var_type:           _FLOAT { $$ = _FLOAT; } | _INT { $$ = _INT; };
any_type:           var_type | _VOID ;
id_list:            id id_tail
                    {
                        $$ = $2;
                        $$->push_back($1);
                    } ;
id_tail:            COMMA id id_tail
                    {
                        $$ = $3;
                        $$->push_back($2);
                    } |
                    {
                        std::vector<std::string*>* temp = new std::vector<std::string*>;
                        $$ = temp;
                    };

param_decl_list:    param_decl param_decl_tail | ;
param_decl:         var_type id
                    {
                        if ($1 == _FLOAT)
                            tableStack->insertSymbol(*$2, "FLOAT", true);
                        else if ($1 == _INT)
                            tableStack->insertSymbol(*$2, "INT", true);
                    };
param_decl_tail:    COMMA param_decl param_decl_tail | ;

func_declarations:  func_decl func_declarations | ;
func_decl:          _FUNCTION any_type id
                    {
                        tableStack->addNewTable(*($3));
                    }
                    OPEN_PARENTHESIS param_decl_list CLOSED_PARENTHESIS _BEGIN func_body _END
                    {
                        tableStack->removeTable();
                    };
func_body:          decl stmt_list ;

stmt_list:          stmt stmt_list | ;
stmt:               base_stmt | if_stmt | for_stmt ;
base_stmt:          assign_stmt | read_stmt | write_stmt | return_stmt ;

assign_stmt:        assign_expr SEMICOLON ;
assign_expr:        id ASSIGN_OP expr
                    {
                        ASTNode * node = new ASTNode_Assign(tableStack->findEntry(*$1));
                        node->right = $3;
                        node->generateCode(threeAC);
                    };
read_stmt:          _READ OPEN_PARENTHESIS id_list CLOSED_PARENTHESIS SEMICOLON
                    {
                        std::vector<std::string*> list = *($3);
                        for (int i = list.size(); i != 0; --i)
                        {
                            std::string name = *(list[i-1]);
                            std::string type = tableStack->findType(name);
                            threeAC->addRead(name, type);
                        }
                    };
write_stmt:         _WRITE OPEN_PARENTHESIS id_list CLOSED_PARENTHESIS SEMICOLON
                    {
                        std::vector<std::string*> list = *($3);
                        for (int i = list.size(); i != 0; --i)
                        {
                            std::string name = *(list[i-1]);
                            std::string type = tableStack->findType(name);
                            threeAC->addWrite(name, type);
                        }
                    };
return_stmt:        _RETURN expr SEMICOLON
                    {
                        ASTNode *retnode = new ASTNode_Return();
						retnode->right = $2;
						retnode->generateCode(threeAC);
                    };

expr:               expr_prefix factor
                    {
                        if ($1 == nullptr)
                            $$ = $2;
                        else
                        {
                            $1->right = $2;
                            $$ = $1;
                        }
                    };
expr_prefix:        expr_prefix factor addop
                    {
                        if ($1 == nullptr)
                            $3->left = $2;
                        else
                        {
                            $1->right = $2;
                            $3->left = $1;
                        }
                        $$ = $3;
                    } |
                    { $$ = nullptr; };
factor:             factor_prefix postfix_expr
                    {
                        if ($1 == nullptr)
                            $$ = $2;
                        else
                        {
                            $1->right = $2;
                            $$ = $1;
                        }
                    };
factor_prefix:      factor_prefix postfix_expr mulop
                    {
                        if ($1 == nullptr)
                            $3->left = $2;
                        else
                        {
                            $1->right = $2;
                            $3->left = $1;
                        }
                        $$ = $3;
                    } |
                    { $$ = nullptr; };
postfix_expr:       primary { $$ = $1; } | call_expr { $$ = $1; } ;
call_expr:          id OPEN_PARENTHESIS expr_list CLOSED_PARENTHESIS
                    {
                        $$ = new ASTNode_CallExpr(*($1), $3);
                    };
expr_list:          expr expr_list_tail
                    {
                        $$ = $2;
                        $$->push_back($1);
                    } |
                    {
                        std::vector<ASTNode *> *node = new std::vector<ASTNode*>;
                        $$ = node;
                    };
expr_list_tail:     COMMA expr expr_list_tail
                    {
                        $$ = $3;
                        $$->push_back($2);
                    } |
                    {
                        std::vector<ASTNode *> *node = new std::vector<ASTNode*>;
                        $$ = node;
                    };
primary:            OPEN_PARENTHESIS expr CLOSED_PARENTHESIS
                    {
                        $$ = $2;
                    } |
                    id
                    {
                            $$ = new ASTNode_ID(tableStack->findEntry(*$1));
                    } |
                    INTLITERAL
                    {
                        $$ = new ASTNode_INT($1);
                    } |
                    FLOATLITERAL
                    {
                        $$ = new ASTNode_FLOAT($1);
                    };
addop:              ADD_OP { $$ = new ASTNode_Expr('+'); } |
                    SUB_OP { $$ = new ASTNode_Expr('-'); };
mulop:              MULT_OP { $$ = new ASTNode_Expr('*'); } |
                    DIV_OP { $$ = new ASTNode_Expr('/'); };

if_stmt:            _IF
                    {
                        tableStack->addNewTable();
                    }
                    OPEN_PARENTHESIS cond CLOSED_PARENTHESIS decl stmt_list
                    {
                        tableStack->removeTable();
                    }
                    else_part _FI;
else_part:          _ELSE
                    {
                        tableStack->addNewTable();
                    }
                    decl stmt_list
                    {
                        tableStack->removeTable();
                    } | ;
cond:               expr compop expr ;
compop:             LT_OP | GT_OP | EQ_OP | NOTEQ_OP | LTE_OP | GTE_OP ;

init_stmt:          assign_expr | ;
incr_stmt:          assign_expr | ;

for_stmt:           _FOR
                    {
                        tableStack->addNewTable();
                    }
                    OPEN_PARENTHESIS init_stmt SEMICOLON cond SEMICOLON incr_stmt CLOSED_PARENTHESIS decl stmt_list _ROF
                    {
                        tableStack->removeTable();
                    };
%%

int main(int argc, char* argv[])
{
    extern FILE *yyin;
    yyin = fopen(argv[1], "r");
    int result = yyparse();
    fclose(yyin);

    // threeAC->print();
    assembly_code->generateCode(threeAC, tableStack);
    assembly_code->print();
}

void yyerror(const char *msg)
{
    int err = 1;
}
