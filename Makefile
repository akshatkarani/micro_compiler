compiler: scanner.l parser.y
	@bison -d parser.y
	@flex scanner.l
	@g++ lex.yy.c parser.tab.c -lfl -o micro utils/ast.cpp

interpreter: tinyNew.cpp
	@g++ tinyNew.cpp -o tiny

clean:
	@rm lex.yy.c micro parser.tab.c parser.tab.h
	@rm tiny
