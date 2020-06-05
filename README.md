# MICRO Compiler

A compiler for MICRO language designed using C++, Flex and Bison.

## Scope
This compiler compiles correctly for code including Statements, Expressions and Function calls. Conditionals and loops are not implemented.
To see sample code files see [sample_input](/sample_input)

## Build the Compiler

To build the compiler first install flex and bison

``sudo apt-get install flex``

``sudo apt-get install bison``

Then clone this repository and run

``make compiler``

## Generate the assembly code

The above command will generate file named `micro`. This is our compiler.
To compile your MICRO code use following command

``./micro path/to/input_file``

You can test using the sample input files provided.

## Interpreting the assembly code

The compiler will generate assembly level code. To interpret the assembly level code we will use Tiny (a very simple assembly code interpreter). You can check it's the documentation of [here](/tinyDoc.txt)

Build the interpreter using

``make interpreter``

and then run

``./tiny input file``

The input file in this step is the output generated from above step.

## Understanding the code

Main parts of this compiler are

1. Lexical analyzer: This is written using flex and code is in the file [scanner.l](/scanner.l)
2. Parser: This is written using bison and code is in the file [parser.y](parser.y)
3. Code Generation: Code Generation is divided into two parts. First Three Address Code in generated which is then converted into assembly level code. This code can be found in [utils](/utils)

## About

This project was done as part of CS406 Compilers course at IIT Dharwad in Spring 2020 Semester.
The interpreter in not written by me and was provided.
