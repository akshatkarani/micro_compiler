#include <list>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <stdlib.h>

using namespace std;

//global variables
int line, debug;  // source line number
bool parseerror; // stop after parsing if set to true

  // vvvvvvvvvvvvvvvvvvvvvvvv Statistics: constants vvvvvvvvvvvvvvvvv
int stats;
#define NUMREGS 200
#define LAT_MOV_rl 1
#define LAT_MOV_m 5
#define LAT_INT_rl 1
#define LAT_INT_m 6
#define LAT_FP_rl 3
#define LAT_FP_m 8
  // ^^^^^^^^^^^^^^^^^^^^^^^^ Statistics: constants ^^^^^^^^^^^^^^^^^
int latestTime = 0;

void passert(bool condition, string errormsg){
  if (!condition) {
    cout<<"error on line "<<line<<" : "<<errormsg<<'\n';
    parseerror = true;
  }
}

void fatal(string errormsg){
    cout<<"error on line "<<line<<" : "<<errormsg<<'\n';
    exit(1);
}


class machine {
  int ival1, ival2;
  float rval1,rval2;
  bool comparingint;
  int registeri[NUMREGS];
  float registerr[NUMREGS];
  int registerFreeTime[NUMREGS];
public:
  machine(){
    ival1=0;ival2=0;rval1=0;rval2=0;
    for (int i=0; i<NUMREGS; i++)
      {
	registeri[i]=0;
	registerr[i]=0;
	registerFreeTime[i]=0;
      }
  }  
  void setstatusi(int i1, int i2)
  {
    ival1=i1; ival2=i2; comparingint=true;
  }
  void setstatusr(float r1, float r2){
    rval1=r1; rval2=r2; comparingint=false;
  }
  bool gt() {if (comparingint) return ival1>ival2; else return rval1>rval2;}
  bool ge() {if (comparingint) return ival1>=ival2; else return rval1>=rval2;}
  bool lt() {if (comparingint) return ival1<ival2; else return rval1<rval2;}
  bool le() {if (comparingint) return ival1<=ival2; else return rval1<=rval2;}
  bool eq() {if (comparingint) return ival1==ival2; else return rval1==rval2;}
  bool ne() {if (comparingint) return ival1!=ival2; else return rval1!=rval2;}
  void setregi(int reg, int i){registeri[reg]=i;}
  void setregr(int reg, float r){registerr[reg]=r;}
  int regi(int reg){return registeri[reg];}
  float regr(int reg){return registerr[reg];}
  // vvvvvvvvvvvvvvvvvvvvvvvv Statistics vvvvvvvvvvvvvvvvv
  void setFreeTime(int reg, int ft)
  {
    registerFreeTime[reg] = ft;
    latestTime = max(latestTime,ft);
  }
  int getFreeTime(int reg) { return registerFreeTime[reg]; }
  void printFreeTime(int reg)
  {
    cout << registerFreeTime[reg];
  }
  // ^^^^^^^^^^^^^^^^^^^^^^^^ Statistics ^^^^^^^^^^^^^^^^^
  void print(){
    cout<<"status:"<<ival1<<','<<ival2<<"  "<<rval1<<','<<rval2;
    if (comparingint) cout<<" int\n"; else cout<<" float\n";
    cout<<"regs "<<registeri[0]<<' '<<registeri[1]<<' '<<registeri[2]<<' '<<registeri[3]<<"   ";
    cout<<registerr[0]<<' '<<registerr[1]<<' '<<registerr[2]<<' '<<registerr[3]<<'\n';
  }
};

namespace opcodes {
enum optype {var,str, label, move, addi,addr, subi,subr, muli,mulr, divi,divr, 
             inci, deci, cmpi, push, pop, ret, link, unlnk, cmpr, 
             jsr, jmp, jgt, jlt, jge, jle, jeq, jne, 
             // all jumps must be between jsr..jne 
	     sys, endofprogram, emptyline, unknown };
	}
enum operandtype {id,stackref,reg,num,strval,empty,nonknown};
enum syscalls {SCreadi,SCreadr,SCwritei,SCwriter,SCwrites,SChalt,SCunknown};


class opcode;

class StackElement {
  int ivalue;
  float rvalue;
  list<opcode>::iterator address;
  enum stackcontent {dataitem, adritem, fpitem} elementtype;
  // vvvvvvvvvvvvvvvvvvvvvvvv Statistics vvvvvvvvvvvvvvvvv
  int freeTime;
  // ^^^^^^^^^^^^^^^^^^^^^^^^ Statistics ^^^^^^^^^^^^^^^^^

public:
  StackElement(int ii, float ff){freeTime = 0; ivalue=ii; rvalue=ff; elementtype=dataitem;}
  StackElement(int fp){freeTime = 0; ivalue=fp; elementtype=fpitem;}
  StackElement(list<opcode>::iterator adr){freeTime = 0; address=adr; elementtype=adritem;}
  void setival(int i){
    passert(elementtype==dataitem,"illegal data stack reference"); 
    ivalue=i;
  }
  void setrval(float r){
    passert(elementtype==dataitem,"illegal data stack reference"); 
    rvalue=r;
  }
  int ival()  {passert(elementtype==dataitem,"illegal int stack reference"); 
               return ivalue;}
  float rval(){passert(elementtype==dataitem,"illegal float stack reference"); 
               return rvalue;}
  int fval(){passert(elementtype==fpitem,"illegal fp stack reference"); 
             return ivalue;}
  // vvvvvvvvvvvvvvvvvvvvvvvv Statistics vvvvvvvvvvvvvvvvv
  int getFreeTime()
  {
    return freeTime;
  }
  void setFreeTime(int ft)
  {
    freeTime = ft;
  }
  void printFreeTime()
  {
    cout << freeTime;
  }
  // ^^^^^^^^^^^^^^^^^^^^^^^^ Statistics ^^^^^^^^^^^^^^^^^
  
  list<opcode>::iterator aval(){
               passert(elementtype==adritem,"illegal pc stack reference"); 
               return address;}
};

// GLOBAL VARIABLE. MACHINE AND SYSTEM STATE:
machine cpu;
vector<StackElement> stack; 
int fp = 0;
//for the user, the stack will grow downwards, however the implementation
// is a list that starts from 1. Offsets will need to be negated.
  


class Symbol {
  string nam;
  int ivalue;
  float rvalue;
  string svalue;
  bool sconst;
  // vvvvvvvvvvvvvvvvvvvvvvvv Statistics vvvvvvvvvvvvvvvvv
  int freeTime; // Time this variable will be free.
  // ^^^^^^^^^^^^^^^^^^^^^^^^ Statistics ^^^^^^^^^^^^^^^^^
public:
  Symbol(string s){freeTime=0; nam=s; ivalue=0;rvalue=0;sconst=false;}
  Symbol(string s1, string s2){freeTime=0; nam=s1; ivalue=0;rvalue=0;svalue=s2;sconst=true;}
  void setival(int i){ivalue=i;}
  void setrval(float r){rvalue=r;}
  string name(){return nam;}
  int ival(){return ivalue;}
  float rval(){return rvalue;}
  string sval(){return svalue;}
  bool strconst() {return sconst;}
  // vvvvvvvvvvvvvvvvvvvvvvvv Statistics vvvvvvvvvvvvvvvvv
  void setFreeTime(int ft)
  {
    freeTime = ft;
    latestTime = max(latestTime,freeTime);
  }
  int getFreeTime(){return freeTime;}
  void printFreeTime(){
    cout << freeTime;
  }
  // ^^^^^^^^^^^^^^^^^^^^^^^^ Statistics ^^^^^^^^^^^^^^^^^

  void print(){cout<<nam<<':'; if (sconst) cout<<svalue; else cout<<ivalue<<'/'<<rvalue; }
};
  

class operand {
  string opname;
  int stackoffset;
  float literalvalue;
  list<Symbol>::iterator varref;
  operandtype ot;
public:
  int registernumber;
  string name(){return opname;}

  // functions used during parsing
  void settype(operandtype t){ot=t;}
  void setreg(int regno){ot=reg; registernumber=regno;}
  void setlit(float r){ot=num; literalvalue=r;}
  void setstackref(int ofs){ot=stackref; stackoffset=ofs;}
  void setname(string ss){opname=ss;}
  void setvarref(list<Symbol>::iterator vr){varref=vr;}
  operandtype type(){return ot;}

  // functions used during "execution"
  // generic operations (operate on both int and float field)
  void setval(operand &op){setival(op.ival());setrval(op.rval());}
  void add(operand &op){ 
    setival(ival()+op.ival()); 
    setrval(rval()+op.rval());
  }
  void sub(operand &op){
    setival(ival()-op.ival());
    setrval(rval()-op.rval());
  }
  void mul(operand &op){
    setival(ival()*op.ival()); 
    setrval(rval()*op.rval());
  }
  void divi(operand &op){setival(ival()/op.ival());}
  void divr(operand &op){setrval(rval()/op.rval());}
  void inci(){setival(ival()+1);}
  void deci(){setival(ival()-1);}

  // set/get individual int and float field
  void setival(int i){
    if (ot==id) {varref->setival(i);}
    else if (ot==reg) cpu.setregi(registernumber,i);
    else if (ot==stackref) stack[fp-stackoffset].setival(i); 
    else fatal("setival: illegal operand type");
  }
  void setrval(float r){
    if (ot==id) {varref->setrval(r);}
    else if (ot==stackref) stack[fp-stackoffset].setrval(r); 
    else if (ot==reg) cpu.setregr(registernumber,r);
    else fatal("setrval: illegal operand type");
  }
  int ival(){
    if (ot==id) {return varref->ival();}
    else if (ot==stackref) return stack[fp-stackoffset].ival(); 
    else if (ot==reg) return cpu.regi(registernumber);
    else if (ot==num) return (int) literalvalue;
    else fatal("operand::ival: illegal operand type");
  }
  float rval(){
    if (ot==id) {return varref->rval();}
    else if (ot==stackref) return stack[fp-stackoffset].rval(); 
    else if (ot==reg) return cpu.regr(registernumber);
    else if (ot=num) return literalvalue;
    else fatal("operand::rval: illegal operand type");
  }
  string sval(){
    if (ot==id) {return varref->sval();}
    else fatal("operand::sval: illegal operand type");
  }
  // vvvvvvvvvvvvvvvvvvvvvvvv Statistics vvvvvvvvvvvvvvvvv
  void setFreeTime(int ft){
    switch (ot) {
    case id:
      {
	varref->setFreeTime(ft);
	break;
      }
    case reg:
      {
	cpu.setFreeTime(registernumber,ft);
	break;
      }
    case stackref:
      {
	stack[fp-stackoffset].setFreeTime(ft);
	break;
      }
    }
  }
  int getFreeTime(){
    switch(ot) {
    case id: {return varref->getFreeTime();}
    case reg: {return cpu.getFreeTime(registernumber);}
    case stackref: {return stack[fp-stackoffset].getFreeTime();}
    default: {return 0;}
    }
  }

  void printFreeTime(){
    switch (ot) {
    case id: {varref->printFreeTime();break;}
    case reg: {cpu.printFreeTime(registernumber);break;}
    case stackref: {stack[fp-stackoffset].printFreeTime(); break; }
    }
  }
  // ^^^^^^^^^^^^^^^^^^^^^^^^ Statistics ^^^^^^^^^^^^^^^^^
  void print(){
    switch (ot) {
    case id: {cout<<"id:"<<opname;break;}
    case stackref: {cout<<"stackref:"<<opname;break;}
    case reg: {cout<<"reg"<<registernumber;break;}
    case num: {cout<<"num:"<<literalvalue;break;}
    case strval: {cout<<"str:"<<opname;break;}
    case empty: {if (debug>=4) cout<<"emptyop"; break;}
    case nonknown: {cout<<"unknownop";break;}
    }
  }
};


class opcode {
  int srcline;
  opcodes::optype typ;
  operand op1,op2;
  list<opcode>::iterator jumptarget; //for jump instructions
  string opname(opcodes::optype t){
    switch (t) {
    case opcodes::var: {return "var";}
    case opcodes::str: {return "str";}
    case opcodes::label: {return "label";}
    case opcodes::move: {return "move";}
    case opcodes::addi: {return "addi";}
    case opcodes::addr: {return "addr";}
    case opcodes::subi: {return "subi";}
    case opcodes::subr: {return "subr";}
    case opcodes::muli: {return "muli";}
    case opcodes::mulr: {return "mulr";}
    case opcodes::divi: {return "divi";}
    case opcodes::divr: {return "divr";}
    case opcodes::inci: {return "inci";}
    case opcodes::deci: {return "deci";}
    case opcodes::cmpi: {return "cmpi";}
    case opcodes::cmpr: {return "cmpr";}
    case opcodes::push: {return "push";}
    case opcodes::pop: {return "pop";}
    case opcodes::jsr: {return "jsr";}
    case opcodes::ret: {return "ret";}
    case opcodes::link: {return "link";}
    case opcodes::unlnk: {return "unlnk";}
    case opcodes::jmp: {return "jmp";}
    case opcodes::jgt: {return "jgt";}
    case opcodes::jlt: {return "jlt";}
    case opcodes::jge: {return "jge";}
    case opcodes::jle: {return "jle";}
    case opcodes::jeq: {return "jeq";}
    case opcodes::jne: {return "jne";}
    case opcodes::sys: {return "sys";}
    default: {return "unknown";}
    }
  }
public:
  opcode(opcodes::optype t,operand o1, operand o2): typ(t),op1(o1),op2(o2),srcline(line){}
  int src(){return srcline;}
  void settarget(list<opcode>::iterator pc){jumptarget=pc;}
  list<opcode>::iterator target(){return jumptarget;}
  string name() {return opname(typ);}
  opcodes::optype code(){return typ;}
  operand & o1(){return op1;}
  operand & o2(){return op2;}
  void print(){
    cout<<"line:"<<srcline<<"  "<<opname(typ)<<"  ";
    op1.print(); cout<<"  ";
    op2.print(); 
  }
  void print(int thyme){
    cout<<"time:"<<thyme<<" line:"<<srcline<<"  "<<opname(typ)<<"  ";
    op1.print(); cout<<"  ";
    op2.print(); 
  }
};




// class jumptarget {
//   string labl;
//   list<opcode>::iterator target;
// public:
//   jumptarget(string l, list<opcode>::iterator t){labl=l,target=t;}
//   string label(){return labl;}
//   list<opcode>::iterator  targetpc(){return target;}
// };


int isRegister(char * s, int& regno) {
  // return true if s is a string of the form r<no> or R<no>, 
  // where <no> is a register number in the range 0..NUMREGs-1
  if (tolower(s[0])!='r') return false;
  for (int i=1;s[i]!=0;i++) if (!isdigit(s[i])) return false;
  regno = atoi(&s[1]);
  return (regno>=0 && regno<NUMREGS);
}

void parseline(opcodes::optype & code, operand& op1, operand& op2, ifstream & src, int line) {
  op1.settype(empty);
  op2.settype(empty);

  if (src.eof()) {code=opcodes::endofprogram; return;}
  string _str;
  char *p;
  char sc[40], ch;
  int is;
  string ss;
  int ip =0;
  //src.get(buf,200,'\n'); // read a source line
  getline (src, _str);
  //  while (src.get(ch)) if (! isspace(ch)){ src.putback(ch); break;} // skip end of line
  //src.get(ch); 
//if (ch!='\n' && !src.eof()) {cout<<"warning: '"<<(int) ch<<"' found at end of line "<<line<<'\n';}
  if (debug==4)  cout << "debug: "<< _str;

  const char * buf = _str.c_str();

  // get opcode
  while (buf[ip]==' '){ip++;} // eat white space
  is = 0;
  while (isalpha(buf[ip])){sc[is++]=tolower(buf[ip++]);} // move opcode to ss, move to lower case
  sc[is]=0;
  ss=sc; // for some operations it's more convenient to have it in a string type

  if (debug==4)  cout << "debug: "<<ss;
  if (ss=="end") code = opcodes::endofprogram;  
  else if (ss=="") code = opcodes::emptyline;
  else if (ss=="var") code = opcodes::var;
  else if (ss=="str") code = opcodes::str;
  else if (ss=="label") code = opcodes::label;
  else if (ss=="move") code = opcodes::move;
  else if (ss=="addi") code = opcodes::addi;
  else if (ss=="addr") code = opcodes::addr;
  else if (ss=="subi") code = opcodes::subi;
  else if (ss=="subr") code = opcodes::subr;
  else if (ss=="muli") code = opcodes::muli;
  else if (ss=="mulr") code = opcodes::mulr;
  else if (ss=="divi") code = opcodes::divi;
  else if (ss=="divr") code = opcodes::divr;
  else if (ss=="inci") code = opcodes::inci;
  else if (ss=="deci") code = opcodes::deci;
  else if (ss=="cmpi") code = opcodes::cmpi;
  else if (ss=="cmpr") code = opcodes::cmpr;
  else if (ss=="push") code = opcodes::push;
  else if (ss=="pop") code = opcodes::pop;
  else if (ss=="jsr") code = opcodes::jsr;
  else if (ss=="ret") code = opcodes::ret;
  else if (ss=="link") code = opcodes::link;
  else if (ss=="unlnk") code = opcodes::unlnk;
  else if (ss=="jmp") code = opcodes::jmp;
  else if (ss=="jgt") code = opcodes::jgt;
  else if (ss=="jlt") code = opcodes::jlt;
  else if (ss=="jge") code = opcodes::jge;
  else if (ss=="jle") code = opcodes::jle;
  else if (ss=="jeq") code = opcodes::jeq;
  else if (ss=="jne") code = opcodes::jne;
  else if (ss=="sys") code = opcodes::sys;
  else code = opcodes::unknown;
  
  // read first operand
  while (buf[ip]==' '){ip++;} // eat white space
  is = 0;
  if (buf[ip]!=';') {
    while (isgraph(buf[ip])) {sc[is++] = buf[ip++];} // move operand to sc
  }
  sc[is]=0;
  ss=sc; 
  if (debug==4)  cout << "debug: "<<ss;

  int regno;
  if (isRegister(sc,regno)) {op1.setreg(regno);}
  else if (isalpha(sc[0])) {op1.settype(id);}
  else if (sc[0]=='$') {op1.setstackref(atoi(&sc[1]));}
  else if (sc[0]=='+' || sc[0]=='-' || isdigit(sc[0])) { // assume it's a number
    op1.setlit(strtod(sc,0)); // literals are always represented as floats
  }
  else if (ss!="") {op1.settype(nonknown);}
  op1.setname(ss);
  
  // read second operand
  while (buf[ip]==' '){ip++;} // eat white space
  is = 0;
  if (isalpha(buf[ip])||buf[ip]=='$') {  // alpha,$ followed by alphanum with punctuation
    while (isgraph(buf[ip])) {sc[is++]=buf[ip++];} 
    sc[is]=0;
    ss=sc; 
    if (debug==4)  cout << "debug: "<<ss;
    if (isRegister(sc,regno)) {op2.setreg(regno);}
    else if (sc[0]=='$') {op2.setstackref(atoi(&sc[1]));}
    else  {op2.settype(id);}
    op2.setname(ss);
  }
  else if (buf[ip]=='"') { // doublequote-delimited string
    op2.settype(strval);
    ip++; // skip leading 
    is=0;
	ss = "";
    while (buf[ip]!='"') {
      if (buf[ip-1]=='\\' && buf[ip]=='n') {ss[is-1]='\n'; ip++;}
	  else {ss += buf[ip++]; is++;}
      // else sc[is++]=buf[ip++];
    }
    // sc[is]=0; 
	ip++;
    op2.setname(ss);
  }

  while (isspace(buf[ip])){ip++;} // eat white space
  if (buf[ip] != ';' && buf[ip] != 0) cout<< "line "<<line<<" warning: non comment found at end of line\n";

}

syscalls checksyscall(operand op){
  if (op.name()=="readi"){return SCreadi;}
  if (op.name()=="readr"){return SCreadr;}
  if (op.name()=="writei"){return SCwritei;}
  if (op.name()=="writer"){return SCwriter;}
  if (op.name()=="writes"){return SCwrites;}
  if (op.name()=="halt"){return SChalt;}
  return SCunknown;
}

void linkvariable(list<Symbol>&table, operand & o, int srcline){
  // if the operand is an id, find the variable in the table and make a 
  // reference to it in the operand
  if (o.type()==id) {
    for (list<Symbol>::iterator it=table.begin();it!=table.end();it++){
      if (it->name()==o.name()) {
	o.setvarref(it);
	return;
      }
    }
    cout << "error on line "<<srcline<<" identifier "<<o.name()<<" not defined\n";
    parseerror = true;
  }
}


int main(int argc, char *argv[]){
  if (argc==1) {cerr<<"usage: tiny srcfile [stats|nostats|d1|d2|d3 [mix]]\n"; exit(1);}

  bool mix = false; // allow mixed declarations and code
  debug = 0;
  stats = 1;

  if (argc>=3){
    if (string(argv[2])=="d1") debug=1;
    else if (string(argv[2])=="d2") debug=2;
    else if (string(argv[2])=="d3") debug=3;
    else if (string(argv[2])=="d4") debug=4;
    else if (string(argv[2])=="stats") stats=1; // Print statistics
    else if (string(argv[2])=="nostats") stats=0; // Don't print statistics
  }
  if (argc>=4){
    if (string(argv[3])=="mix") mix=true;
  }
  ifstream src(argv[1]);
  if (!src) {cerr<<argv[1]<<" not found\n"; exit(1);}

  line = 0;
  parseerror = false;
  list<opcode> program;
  list<Symbol> symboltable;
  //  list<jumptarget> labeltab;
  operand op1,op2;
  opcodes::optype  code;
  bool done = false; 
  bool declarations = true; // parsing is in the declarations section
  
  while (!done) {
    line ++;
    parseline(code,op1,op2,src,line);
    if (code==opcodes::endofprogram) {done=true;}
    else if (code==opcodes::emptyline) {}
    else {
      //      checkoperands(op1,op2); 
      // op1 can be num/reg/id, op2 can be reg/id/strval/empty
      // only one of them can be an id
      // there must be 1 or 2 operands
      
      switch (code) {
      case opcodes::var:  {
	passert(declarations||mix,"declarations must preceed all code"); 
	passert(op1.type()==id,"identifier operand expected"); 
	passert(op2.type()==empty,"only one operand expected"); 
	symboltable.push_back(Symbol(op1.name())); break;
      }
      case opcodes::str :  {
	passert(declarations||mix,"declarations must preceed all code"); 
	passert(op1.type()==id,"1st operand must be indentifier"); 
	passert(op2.type()==strval,"2nd operand must be string"); 
	symboltable.push_back(Symbol(op1.name(),op2.name())); 
	break;
      }
      case opcodes::label : {
	passert(op1.type()==id,"1st operand must be indentifier"); 
	passert(op2.type()==empty,"only one operand expected"); 
	//	labeltab.push_back(op1.name());  // may not need this
        declarations = false;
	program.push_back(opcode(opcodes::label,op1,op2));
	break;
      }
      case opcodes::move: {
	passert( (op1.type()==num || op1.type()==reg || 
		  op1.type()== id || op1.type()== stackref)
		 && (op2.type()==reg || op2.type()== id|| op2.type()== stackref),
		 "illegal operand type");
	passert(!((op1.type()==id || op1.type()== stackref)&& 
		(op2.type()==id || op2.type()== stackref)),
		"both  operands are memory refs");
        declarations = false;
	program.push_back(opcode(code,op1,op2)); 
	break;
      }
      case opcodes::inci: 
      case opcodes::deci: {
	passert(op1.type()==reg,"operand must be a register"); 
	passert(op2.type()==empty,"only one operand expected"); 
        declarations = false;
	program.push_back(opcode(code,op1,op2)); 
	break;
      }
      case opcodes::addi:
      case opcodes::addr: 
      case opcodes::subr:
      case opcodes::subi:
      case opcodes::muli:
      case opcodes::mulr:
      case opcodes::divi:
      case opcodes::divr:
      case opcodes::cmpi:
      case opcodes::cmpr: {
	passert( (op1.type()==num || op1.type()==reg || 
		  op1.type()== id || op1.type()== stackref)
		 && (op2.type()==reg), "illegal operand type");
        declarations = false;
	program.push_back(opcode(code,op1,op2)); 
	break;
      }
      case opcodes::push: {
	passert(op2.type()==empty,"zero or one operand expected"); 
	passert(op1.type()==num || op1.type()==reg || op1.type()== id
		|| op1.type()== stackref || op1.type()==empty ,
		"illegal operand type"); 
	program.push_back(opcode(code,op1,op2));
        declarations = false;
	break;
      }
      case opcodes::pop:{
	passert(op2.type()==empty,"zero or one operand expected"); 
	passert(op1.type()==reg || op1.type()== id || 
		op1.type()==stackref ||op1.type()==empty, 
		"illegal operand type"); 
	program.push_back(opcode(code,op1,op2));
        declarations = false;
	break;
      }
      case opcodes::jsr: {
	passert(op2.type()==empty,"only one operand expected"); 
	passert(op1.type()==id,"operand must be an identifier"); 
	program.push_back(opcode(code,op1,op2));
        declarations = false;
	break;
      }
      case opcodes::ret: {
	passert(op1.type()==empty && op2.type()==empty,"no operand expected"); 
	program.push_back(opcode(code,op1,op2));
        declarations = false;
	break;
      }
      case opcodes::link: {
	passert(op1.type()==num && op2.type()==empty,"illegal operand"); 
	program.push_back(opcode(code,op1,op2));
        declarations = false;
	break;
      }
      case opcodes::unlnk: {
	passert(op1.type()==empty && op2.type()==empty,"no operand expected"); 
	program.push_back(opcode(code,op1,op2));
        declarations = false;
	break;
      }
      case opcodes::jmp:
      case opcodes::jgt:
      case opcodes::jlt:
      case opcodes::jge:
      case opcodes::jle:
      case opcodes::jne:
      case opcodes::jeq: {
	passert(op1.type()==id,"operand must be an identifier"); 
	passert(op2.type()==empty,"only one operand expected"); 
        declarations = false;
	program.push_back(opcode(code,op1,op2));
	break;
      }
      case opcodes::sys : {
        syscalls scl = checksyscall(op1);
	passert(scl != SCunknown,"unknown system call");
	if (scl == SChalt) {
	  passert(op2.type()==empty,"only one operand expected");
	} 
	// operand types are not fully checked
        declarations = false;
	program.push_back(opcode(code,op1,op2)); break;
      }
      case opcodes::unknown : {
	passert(false,"unknown opcode");
      }
      }
    }
  }
  
  if (debug>=1) { // print all identifiers
    for (list<Symbol>::iterator it  = symboltable.begin(); it!=symboltable.end(); it++) {
      cout<<"id "; it->print();cout<<'\n';
    }
    for (list<opcode>::iterator it  = program.begin(); it!=program.end(); it++) {
    it->print(); cout<<'\n';
    }
  }
  
  // check if all ids are defined; set jump target pcs:
  
  for (list<opcode>::iterator it  = program.begin(); it!=program.end(); it++) {
    if (it->code()>=opcodes::jsr && it->code()<=opcodes::jne) {
      // for all jump instructions, insert the jump target
      list<opcode>::iterator jt = program.begin();
      // find the opcode that has this label 
      bool labelfound = false;
      for (  ; jt!=program.end() && ! labelfound; jt++) {
	if (jt->code()==opcodes::label && jt->o1().name()==it->o1().name()) {
	  it->settarget(jt);
	  labelfound = true;
	}
      }
      if (!labelfound) {
	cout<<"error on line "<<it->src()<<" jump target is not defined\n";
	parseerror=true;
      }
    }
    else if (it->code()==opcodes::sys){
      // for sys calls the first operand is the syscall name, although its operand
      // type id "id", so only check the 2nd operand
      linkvariable(symboltable,it->o2(),it->src());
    }
    else if (it->code()!=opcodes::label) {
      // in all other cases, except labels, check both operands
      linkvariable(symboltable,it->o1(),it->src());
      linkvariable(symboltable,it->o2(),it->src());
    }
  }
  
  if (parseerror) exit(1);
  
  // execute the program
  // vvvvvvvvvvvvvvvvvvvvvvvv Statistics: initialization vvvvvvvvvvvvvvvvv
  int CNT_register[NUMREGS];
  int CNT_mem = 0; 
  int CNT_iOps = 0; // Count integer instructions.
  int CNT_rOps = 0; // Count real instructions.
  int CNT_peeps = 0; // Count peephole inci/deci optimizations.
  int CNT_branches = 0; // Count branches.
  int CNT_instructions = 0; // Count total # of instructions.
  int CNT_cycles = 0; // The cycles.
  int CNT_target = 0; // The target of the command, if any.
  // CNT_target ==0 means no target, ==1 means op1, ==2 means op2.
  // Use CNT_target to update the time a symbol/register is free.
  int CNT_cmp = 0; // Since cmpi ... jge ... is a two-part command, need to keep track of the time the cmpi ends.
  int CNT_latency = 0; // Latency for an instruction.
  int CNT_MOV_rl = 0;
  int CNT_MOV_m = 0;
  int CNT_INT_rl = 0;
  int CNT_INT_m = 0;
  int CNT_FP_rl = 0;
  int CNT_FP_m = 0;
  int i,allregs;
  // ^^^^^^^^^^^^^^^^^^^^^^^^ Statistics: initialization ^^^^^^^^^^^^^^^^^

  bool eop = false;
  int itemp;
  float rtemp;

  // vvvvvvvvvvvvvvvvvvvvvvvv Statistics: initialization vvvvvvvvvvvvvvvvv
  if(stats) {
  for (i=0;i<NUMREGS;i++)
    {
      CNT_register[i]=0;
    }
  }
  // ^^^^^^^^^^^^^^^^^^^^^^^^ Statistics: initialization ^^^^^^^^^^^^^^^^^

  list<opcode>::iterator pc = program.begin();
  while (!eop && pc != program.end()) {
    if (debug>=3) {
      cpu.print();
      for (list<Symbol>::iterator it  = symboltable.begin(); it!=symboltable.end(); it++) {
	it->print(); cout <<"  ";
      }
      cout<<'\n';
    }
    if (debug >=2)
      {
	if(stats)
	  {
	    pc->print(CNT_cycles); cout<<'\n';
	  } else {
	    pc->print(); cout<<'\n';
	  }
      }
    line = pc->src();
    // vvvvvvvvvvvvvvvvvvvvvvvv Statistics vvvvvvvvvvvvvvvvv
    if(stats) {
    // I just do statistics in this first switch statement
    CNT_latency = 0;
    CNT_target = 0;
    switch (pc->code())
      {
      case opcodes::label:
	// No time
	break;
      case opcodes::move: 
      {
	CNT_cycles = max(pc->o1().getFreeTime(),CNT_cycles);
	CNT_cycles = max(pc->o2().getFreeTime(),CNT_cycles);
	if(debug >= 2) {
	  cout << "FREETIME "<< CNT_cycles << " move (";
	  pc->o1().printFreeTime();
	  cout << ",";
	  pc->o2().printFreeTime();
	  cout << ")\n";
	}
	CNT_instructions++;
	switch(pc->o1().type())
	  {
	  case id:
	    CNT_mem++;
	    CNT_latency = LAT_MOV_m;
	    break;
	  case reg:
	    CNT_register[pc->o1().registernumber]++;
	  default:
	    CNT_latency = LAT_MOV_rl;
	  }
	switch(pc->o2().type())
	  {
	  case id:
	    CNT_mem++;
	    CNT_latency = max(LAT_MOV_m,CNT_latency);
	    break;
	  case reg:
	    CNT_register[pc->o2().registernumber]++;
	  default:
	    CNT_latency = max(LAT_MOV_rl,CNT_latency);
	  }
	if(CNT_latency > LAT_MOV_rl)
	  {
	    CNT_MOV_m++;
	  } else {
	    CNT_MOV_rl++;
	  }
	CNT_target = 2;
	break;
      }
      //    Peephole Ops
      case opcodes::inci:
      case opcodes::deci:
	CNT_cycles = max(pc->o1().getFreeTime(),CNT_cycles);
	if(debug >= 2) {
	  cout << "FREETIME "<< CNT_cycles << " inci/deci (";
	  pc->o1().printFreeTime();
	  cout << ")\n";
	}
	CNT_instructions++;
	CNT_iOps++;
	CNT_peeps++;
	CNT_INT_rl++;
	CNT_latency = LAT_INT_rl;
	CNT_target = 1;
	break;
      case opcodes::cmpi: 
	{
	  CNT_cycles = max(pc->o1().getFreeTime(),CNT_cycles);
	  CNT_cycles = max(pc->o2().getFreeTime(),CNT_cycles);
	  if(debug >= 2) {
	    cout << "FREETIME "<< CNT_cycles << " intOp (";
	    pc->o1().printFreeTime();
	    cout << ",";
	    pc->o2().printFreeTime();
	    cout << ")\n";
	  }
	  CNT_instructions++;
	  CNT_iOps++;
	  switch(pc->o1().type())
	    {
	    case reg:
	      CNT_register[pc->o1().registernumber]++;
	    case num:
	      CNT_latency = LAT_INT_rl;
	      break;
	    case stackref:
	    case id:
	      CNT_mem++;
	      CNT_latency = LAT_INT_m;
	      break;
	    default:
	      fatal("STATISTICS: unknown op1 used in an integer op");
	    }
	  switch(pc->o2().type())
	    {
	    case reg:
	      CNT_register[pc->o2().registernumber]++;
	    case num:
	      CNT_latency = max(CNT_latency,LAT_INT_rl);
	      break;
	    case stackref:
	    case id:
	      CNT_mem++;
	      CNT_latency = max(LAT_INT_m,CNT_latency);
	      break;
	    default:
	      fatal("STATISTICS: unknown op2 used in an integer op");
	    }
	  if (CNT_latency > LAT_INT_rl) {
	    CNT_INT_m++;
	  } else {
	    CNT_INT_rl++;
	  }
	  // Compares have internal registers as the target.
	  CNT_cmp = CNT_cycles + CNT_latency; // jge, ... must wait for the compare.
	  break;
	}
      // Integer Ops
      case opcodes::addi:
      case opcodes::subi:
      case opcodes::muli:
      case opcodes::divi:
      {
	CNT_cycles = max(pc->o1().getFreeTime(),CNT_cycles);
	CNT_cycles = max(pc->o2().getFreeTime(),CNT_cycles);
	if(debug >= 2) {
	  cout << "FREETIME "<< CNT_cycles << " intOp (";
	  pc->o1().printFreeTime();
	  cout << ",";
	  pc->o2().printFreeTime();
	  cout << ")\n";
	}
	CNT_instructions++;
	CNT_iOps++;
	switch(pc->o1().type())
	  {
	  case reg:
	    CNT_register[pc->o1().registernumber]++;
	  case num:
	    CNT_latency = LAT_INT_rl;
	    break;
	  case stackref:
	  case id:
	    CNT_mem++;
	    CNT_latency = LAT_INT_m;
	    break;
	  default:
	    fatal("STATISTICS: unknown op1 used in an integer op");
	  }
	switch(pc->o2().type())
	  {
	  case reg:
	    CNT_register[pc->o2().registernumber]++;
	  case num:
	    CNT_latency = max(CNT_latency,LAT_INT_rl);
	    break;
	  case stackref:
	  case id:
	    CNT_mem++;
	    CNT_latency = max(LAT_INT_m,CNT_latency);
	    break;
	  default:
	    fatal("STATISTICS: unknown op2 used in an integer op");
	  }
	if (CNT_latency > LAT_INT_rl) {
	  CNT_INT_m++;
	} else {
	  CNT_INT_rl++;
	}
	CNT_target = 2;
	break;
      }
      case opcodes::cmpr:
	{
	  CNT_cycles = max(pc->o1().getFreeTime(),CNT_cycles);
	  CNT_cycles = max(pc->o2().getFreeTime(),CNT_cycles);
	  if(debug>=2){
	    cout << "FREETIME "<< CNT_cycles << " fpOp (";
	    pc->o1().printFreeTime();
	    cout << ",";
	    pc->o2().printFreeTime();
	    cout << ")\n";
	  }
	  CNT_instructions++;
	  CNT_rOps++;
	  CNT_latency = 0;
	switch(pc->o1().type())
	  {
	  case reg:
	    CNT_register[pc->o1().registernumber]++;
	  case num:
	    CNT_latency = LAT_FP_rl;
	    break;
	  case stackref:
	  case id:
	    CNT_mem++;
	    CNT_latency = LAT_FP_m;
	    break;
	  default:
	    fatal("STATISTICS: unknown op1 used in an integer op");
	  }
	switch(pc->o2().type())
	  {
	  case reg:
	    CNT_register[pc->o2().registernumber]++;
	  case num:
	    CNT_latency = max(CNT_latency,LAT_FP_rl);
	    break;
	  case stackref:
	  case id:
	    CNT_mem++;
	    CNT_latency = max(LAT_FP_m,CNT_latency);
	    break;
	  default:
	    fatal("STATISTICS: unknown op2 used in an integer op");
	  }
	if(CNT_latency > LAT_FP_rl) {
	  CNT_FP_m++;
	} else {
	  CNT_FP_rl++;
	}
	// Compares have internal registers as the target.
	CNT_cmp = CNT_cycles + CNT_latency; // jge, ... must wait for the compare.
	break;
	}
      // Real Ops
      case opcodes::addr:
      case opcodes::subr: 
      case opcodes::mulr:
      case opcodes::divr :
      {
	CNT_cycles = max(pc->o1().getFreeTime(),CNT_cycles);
	CNT_cycles = max(pc->o2().getFreeTime(),CNT_cycles);
	if(debug>=2){
	  cout << "FREETIME "<< CNT_cycles << " fpOp (";
	  pc->o1().printFreeTime();
	  cout << ",";
	  pc->o2().printFreeTime();
	  cout << ")\n";
	}
	CNT_instructions++;
	CNT_rOps++;
	CNT_latency = 0;
	switch(pc->o1().type())
	  {
	  case reg:
	    CNT_register[pc->o1().registernumber]++;
	  case num:
	    CNT_latency = LAT_FP_rl;
	    break;
	  case stackref:
	  case id:
	    CNT_mem++;
	    CNT_latency = LAT_FP_m;
	    break;
	  default:
	    fatal("STATISTICS: unknown op1 used in an integer op");
	  }
	switch(pc->o2().type())
	  {
	  case reg:
	    CNT_register[pc->o2().registernumber]++;
	  case num:
	    CNT_latency = max(CNT_latency,LAT_FP_rl);
	    break;
	  case stackref:
	  case id:
	    CNT_mem++;
	    CNT_latency = max(LAT_FP_m,CNT_latency);
	    break;
	  default:
	    fatal("STATISTICS: unknown op2 used in an integer op");
	  }
	if(CNT_latency > LAT_FP_rl) {
	  CNT_FP_m++;
	} else {
	  CNT_FP_rl++;
	}
	CNT_target = 2;
	break;
      }
      case opcodes::pop:
	CNT_target = 1;
      case opcodes::push:
	// These are simply moves to the stack.
      {
	CNT_cycles = max(pc->o1().getFreeTime(),CNT_cycles);
	if(debug>=2){
	  cout << "FREETIME "<< CNT_cycles << " push/pop (";
	  pc->o1().printFreeTime();
	  cout << ")\n";
	}
	CNT_instructions++;
	switch(pc->o1().type())
	  {
	  case id:
	    CNT_mem++;
	    CNT_latency = LAT_MOV_m;
	    break;
	  case reg:
	    CNT_register[pc->o1().registernumber]++;
	  default:
	    CNT_latency = LAT_MOV_rl;
	  }
	if(CNT_latency > LAT_MOV_rl)
	  {
	    CNT_MOV_m++;
	  } else {
	    CNT_MOV_rl++;
	  }
	break;
      }
      case opcodes::link:
      case opcodes::unlnk:
      { 
	CNT_instructions++;
	CNT_latency = LAT_INT_rl;
	CNT_INT_rl++;
	break;
      }
      case opcodes::jgt:
      case opcodes::jlt:
      case opcodes::jge:
      case opcodes::jle:
      case opcodes::jeq:
      case opcodes::jne:
	{
	  CNT_cycles = max(CNT_cycles,CNT_cmp);
	  if(debug>=2){
	    cout << "FREETIME "<< CNT_cycles << " cmp/jgt... (Wait for comparison)\n";
	  }
	}
      case opcodes::jmp:
      case opcodes::jsr:
	// pushes already wait for their operands so jsr can execute immediately.
	{
	  CNT_branches++;
	  CNT_instructions++;
	  CNT_latency = 1;
	  CNT_INT_rl++;
	  break;
	}
      case opcodes::ret:
      {
	CNT_cycles = max(CNT_cycles,latestTime); // Wait for all pending instructions before returning.
	  if(debug>=2){
	    cout << "FREETIME "<< CNT_cycles << " return (Wait for pending instructions before returning)\n";
	  }
	CNT_instructions++;
	CNT_latency = 1;
	CNT_INT_rl++;
	break;
      }
      case opcodes::sys:
      { 
	switch (checksyscall(pc->o1()))
	  {
	  case SCreadi : 
	  case SCwritei :
	    {
	      CNT_instructions++;
	      switch(pc->o1().type())
		{
		case id:
		  {
		    CNT_mem++;
		    CNT_latency = LAT_INT_m;
		    break;
		  }
		case reg:
		  {
		    CNT_register[pc->o1().registernumber]++;
		  }
		default:
		  {
		    CNT_latency = LAT_INT_rl;
		  }
		}
	      if (CNT_latency > LAT_INT_rl) {
		CNT_INT_m++;
	      } else {
		CNT_INT_rl++;
	      }
	      break;
	    }
	  case SCreadr : 
	  case SCwriter : 
	    {
	      CNT_instructions++;
	      CNT_rOps++;
	      CNT_latency = 0;
	      switch(pc->o1().type())
		{
		case reg:
		  CNT_register[pc->o1().registernumber]++;
		case num:
		  CNT_latency = LAT_FP_rl;
		  break;
		case id:
		  CNT_mem++;
		  CNT_latency = LAT_FP_m;
		  break;
		default:
		  fatal("STATISTICS: unknown op1 used in an integer op");
		}
	      if (CNT_latency > LAT_FP_rl) {
		CNT_FP_m++;
	      } else {
		CNT_FP_rl++;
	      }
	      break;
	    }
	  case SCwrites :
	    {
	      CNT_instructions++;
	      CNT_iOps++;
	      CNT_latency = 0;
	      CNT_mem++;
	      CNT_latency = LAT_INT_m;
	      CNT_INT_m++;
	      break;
	    } 
	  case SChalt :
	    {
	      CNT_latency = 1;
	      CNT_instructions++;
	      CNT_INT_rl++;
	      CNT_cycles = max(latestTime,CNT_cycles);
	      break;
	    }
	  }
      }
      }
    if(CNT_target > 0)
      {
	if(CNT_target == 1)
	  {
	    pc->o1().setFreeTime(CNT_cycles+CNT_latency);
	  } else {
	    pc->o2().setFreeTime(CNT_cycles+CNT_latency);
	  }
	if (debug>=2) {
	  cout << "Target will be free at cycle " << CNT_cycles+CNT_latency << "\n";
	}
      }
    CNT_cycles += 1;
    }
  // ^^^^^^^^^^^^^^^^^^^^^^^^ Statistics ^^^^^^^^^^^^^^^^^

    switch (pc->code()) {
    case opcodes::move : 
      {
	pc->o2().setval(pc->o1());
	pc++;
	break;
      }
    case opcodes::label : {pc++;break;}
    case opcodes::addi:
    case opcodes::addr : {pc->o2().add(pc->o1()); pc++; break;}
    case opcodes::subi:
    case opcodes::subr : {pc->o2().sub(pc->o1()); pc++; break;}
    case opcodes::muli:
    case opcodes::mulr : {pc->o2().mul(pc->o1());  pc++; break;}
    case opcodes::divi:  {pc->o2().divi(pc->o1()); pc++; break;}
    case opcodes::divr : {pc->o2().divr(pc->o1()); pc++; break;}
    case opcodes::inci : {pc->o1().inci(); pc++; break;}
    case opcodes::deci : {pc->o1().deci(); pc++; break;}
    case opcodes::cmpi       : {cpu.setstatusi(pc->o1().ival(),pc->o2().ival());  pc++; break;}
    case opcodes::cmpr       : {cpu.setstatusr(pc->o1().rval(),pc->o2().rval());  pc++; break;}
    case opcodes::push: {
      if (pc->o1().type() != empty) 
	stack.push_back(StackElement(pc->o1().ival(),pc->o1().rval()));
      else
	stack.push_back(StackElement(0,0));
      pc++;
      break;
    }
    case opcodes::pop: {
      if (pc->o1().type() != empty) {
	pc->o1().setival(stack.back().ival());
	pc->o1().setrval(stack.back().rval());
      }
      stack.pop_back();
      pc++;
      break;
    }
    case opcodes::jsr: {
      list<opcode>::iterator subrpc=pc->target();
      pc++;
      stack.push_back(StackElement(pc));
      pc = subrpc;
      break;
    }
    case opcodes::ret: {
      pc = stack.back().aval();
      stack.pop_back();
      break;
    }
    case opcodes::link: { 
      // push(fp); fp <- sp; reserve space on stack for local variables;
      stack.push_back(StackElement(fp));  
      fp = stack.size()-1;
      for (int ii=1;ii<=pc->o1().ival();ii++) {
	stack.push_back(StackElement(0,0));
      }
      pc++;
      break;
    }
    case opcodes::unlnk: {
      // clean stack up to fp; pop(fp)
      while (stack.size()>fp+1) stack.pop_back();
      fp = stack.back().fval();
      stack.pop_back();
      pc++;
      break;
    }
    case opcodes::jmp :       {pc=pc->target(); break;}
    case opcodes::jgt:       {if (cpu.gt()) pc=pc->target(); else pc++; break;}
    case opcodes::jlt:       {if (cpu.lt()) pc=pc->target(); else pc++; break;}
    case opcodes::jge:       {if (cpu.ge()) pc=pc->target(); else pc++; break;}
    case opcodes::jle:       {if (cpu.le()) pc=pc->target(); else pc++; break;}
    case opcodes::jeq:       {if (cpu.eq()) pc=pc->target(); else pc++; break;}
    case opcodes::jne:       {if (cpu.ne()) pc=pc->target(); else pc++; break;}
    case opcodes::sys:  { 
      switch (checksyscall(pc->o1())) {
      case SCreadi : {cin >> itemp; pc->o2().setival(itemp); pc++; break;}
      case SCreadr : {cin >> rtemp; pc->o2().setrval(rtemp); pc++; break;}
      case SCwriter : {cout << pc->o2().rval(); pc++; break;}
      case SCwritei : {cout << pc->o2().ival(); pc++; break;}
      case SCwrites : {cout << pc->o2().sval(); pc++; break;} 
      case SChalt : {eop=true; break;}
      }
    }
    }
  }  
  // vvvvvvvvvvvvvvvvvvvvvvvv Statistics: Print vvvvvvvvvvvvvvvvv
  if(stats) {
  cout << "\nSTATISTICS _____________________________\n";
  cout << "   #Instructions:" << CNT_instructions <<"\n";
  cout << "    (move-ops mem:" << CNT_MOV_m << ", reglit:" << CNT_MOV_rl << ")\n";
  cout << "    ( int-ops mem:" << CNT_INT_m << ", reglit:" << CNT_INT_rl << ")\n";
  cout << "    (  fp-ops mem:" << CNT_FP_m  << ", reglit:" << CNT_FP_rl  << ")\n";
  for(i=0;i<NUMREGS;i++)
    allregs += CNT_register[i];
  cout << "   Memory Usage (mem:" << CNT_mem <<",reg:"<<allregs<<")\n";
  cout << "      register-use[" << CNT_register[0];
  for(i=1;i<NUMREGS;i++)
    cout << "," << CNT_register[i];
  cout << "]\n";
  cout << "   Total Cycles = "<<CNT_cycles<<"\n";
  cout << "OTHER STATSvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n";
  cout << "    (int-ops:" << CNT_iOps << ", fp-ops:"<< CNT_rOps <<")\n";
  cout << "    (branches:"<< CNT_branches<<")\n";
  cout << "      peephole-ops:"<<CNT_peeps<<"\n";
  }
  // ^^^^^^^^^^^^^^^^^^^^^^^^ Statistics ^^^^^^^^^^^^^^^^^

}
