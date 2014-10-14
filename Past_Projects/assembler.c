/*This is an Assembler for the didactic 'LC3-b' assembly language
used at UT Austin for computer architecture courses
*/
#include <stdlib.h>
#include <stdio.h>
/*#include "../tests/test.c" */
#define INT_MAX 65536
#define NOT_IMMEDIATE INT_MAX+1
#define UNPARSED -1
#define NOT_LABEL NOT_IMMEDIATE
/*  --------structs, inits, enums ---------*/

size_t strlen();
typedef struct {
	int beg;
	int end;
}snippet;
typedef struct{
	int line;
	snippet symb;
}symbol;
typedef enum{false,true} bool;
typedef enum{NOT_REG=-1,R0,R1,R2,R3,R4,R5,R6,R7} REG;
typedef enum{INVALID_OP=-1, ORIG, END, GETC, OUT, PUTS, IN, HALT, RET, RTI, FILL, TRAP, BR, BRn, BRz,
BRnz, BRp, BRnp, BRzp, BRnzp, JSR, JMP, JSRR, NOT, LEA, ANDr, ADDr, XORr, LSHF, RSHL, RSHA, ANDi,
ADDi, XORi, LDB, LDW, STB, STW} OPCODE;
typedef enum{A,B,C,D,E,F,G,H}BITSTENCIL;
int beginaddress;
int codebegin;
int codend_line;
int codebegin_line;
int line;
int out_line;
int num_tokens;
int num_symbols;
int FP;
int sym_index;

char* infile;
char* outfile;
snippet* tokens;
symbol* symbols;



/* ------------ Utilities --------------*/



/*f*/
int
toNum( char * pStr )
{
   char * t_ptr;
   char * orig_pStr;
   int t_length,k;
   int lNum, lNeg = 0;
   long int lNumLong;

   orig_pStr = pStr;
   if( *pStr == '#' )				/* decimal */
   { 
     pStr++;
     if( *pStr == '-' )				/* dec is negative */
     {
       lNeg = 1;
       pStr++;
     }
     t_ptr = pStr;
     t_length = strlen(t_ptr);
     for(k=0;k < t_length;k++)
     {
       if (!isdigit(*t_ptr))
       {
	 printf("Error: invalid decimal operand, %s\n",orig_pStr);
	 exit(4);
       }
       t_ptr++;
     }
     lNum = atoi(pStr);
     if (lNeg)
       lNum = -lNum;
 
     return lNum;
   }
   else if( *pStr == 'x' )	/* hex     */
   {
     pStr++;
     if( *pStr == '-' )				/* hex is negative */
     {
       lNeg = 1;
       pStr++;
     }
     t_ptr = pStr;
     t_length = strlen(t_ptr);
     for(k=0;k < t_length;k++)
     {
       if (!isxdigit(*t_ptr))
       {
	 printf("Error: invalid hex operand, %s\n",orig_pStr);
	 exit(4);
       }
       t_ptr++;
     }
     lNumLong = strtol(pStr, NULL, 16);    /* convert hex string into integer */
     lNum = (lNumLong > INT_MAX)? INT_MAX : lNumLong;
     if( lNeg )
       lNum = -lNum;
     return lNum;
   }
   else
   {
	printf( "Error: invalid operand, %s\n", orig_pStr);
	exit(4); 
   }
}

bool
cmpChars(char a, char b)
{
	return (a==b || (a+('a'-'A'))==b || (a-('a'-'A')) ==b );
}
bool
cmpSnipvStr(snippet snip, char* str)
{
	int slen = strlen(str);
	if( (snip.end-snip.beg)!= slen ){return false;}
	int i = 0;
	for(;i<slen;i++)
	{
		if( !cmpChars(infile[snip.beg+i],str[i]) ){return false;}
	}
	return true;
}
REG
whichReg(snippet token)
{
	if( (token.end-token.beg) != 2)
		return NOT_REG;
	if( !cmpChars(infile[token.beg],'R') )
		return NOT_REG;
	int regNum = infile[token.beg+1] - '0';
	if( regNum<0 || regNum>7)
		return NOT_REG;
	
	else return regNum;
}
int
whichImmediate(snippet token, OPCODE op)
{
	int imm_val;
	if( infile[token.beg] == '#' || infile[token.beg] == 'x')
	{
		int size = (token.end-token.beg);
		char s_token[size];
		memcpy(s_token, infile+token.beg, token.end-token.beg);
		s_token[size] = 0;
		imm_val = toNum(s_token);
	}
	else{ return NOT_IMMEDIATE; }

	if( imm_val > (1<<15) )
	{
		printf("%d error: Immediate value %d is too large for LC3's 16 bits!", line, imm_val);
		exit(3);
	}
	if(op==FILL){return imm_val;}
	if(op==ORIG)
	{
		if(imm_val<0)
		{
			printf("%d error: .ORIG argument cannot be negative!",line);
			exit(3);
		}
		return imm_val;
	}

	if(op==TRAP)
	{
		if( infile[token.beg] != 'x')
		{
			printf("%d error: TRAP requires a hexadecimal argument!", line);
			exit(4);
		}
		if( ! (imm_val>0 && imm_val<0xFF) )
		{
			printf("%d error: Invalid TRAP vector!",line);
			exit(3);
		}
		return imm_val;
	}
	if(op>=LSHF && op<=RSHA)
	{
		if( imm_val > (1<<3) || imm_val<0 )
		{
			printf("%d error: Invalid shift value (too large or negative).", line, imm_val);
			exit(3);
		}
		return imm_val;
	}
	if(op>=ANDi && op<=XORi)
	{
		if( imm_val > (1<<4) )
		{
			printf("%d error: Immediate value %d is too large for this OpCode's 5 bits!", line, imm_val);
			exit(3);
		}
		imm_val = (imm_val<0) ? imm_val&0x001F : imm_val;
		return imm_val;
	}
	if(op>=LDB && op<=STW)
	{
		if( imm_val > (1<<5) )
		{
			printf("%d error: Immediate value %d is too large for this OpCode's 6 bits!", line, imm_val);
			exit(3);
		}
		imm_val = (imm_val<0) ? imm_val&0x001F : imm_val;
		return imm_val;
	}
	printf("whichImmediate() is broken");
	exit(4);
}
OPCODE
BRnzpOp(snippet snip)
{
	int n = 0;
	int z = 0;
	int p = 0;
	int addendums = (snip.end-snip.beg)-2;
	int i =0;
	for(;i<addendums;i++)
	{
		if(cmpChars(infile[snip.beg+2+i],'n')){n=1;}
		if(cmpChars(infile[snip.beg+2+i],'z')){z=2;}
		if(cmpChars(infile[snip.beg+2+i],'p')){p=4;}
	}

	switch(n|z|p)
	{
	case 1:
		return BRn;
	case 2:
		return BRz;
	case 3:
		return BRnz;
	case 4:
		return BRp;
	case 5:
		return BRnp;
	case 6:
		return BRzp;
	case 7:
		return BRnzp;
	default:
		return INVALID_OP;
	}


}
OPCODE
isOp(snippet snip)
{

	if(cmpSnipvStr(snip, ".orig")){return ORIG;}
	if(cmpSnipvStr(snip, ".end")){
		return END;
	}
	if(cmpSnipvStr(snip, "getc")){return GETC;}
	if(cmpSnipvStr(snip, "out")){return OUT;}
	if(cmpSnipvStr(snip, "puts")){return PUTS;}
	if(cmpSnipvStr(snip, "in")){return IN;}
	if(cmpSnipvStr(snip, "halt")){return HALT;}
	if(cmpSnipvStr(snip, "ret")){return RET;}
	if(cmpSnipvStr(snip, "rti")){return RTI;}
	if(cmpSnipvStr(snip, ".fill")){return FILL;}
	if(cmpSnipvStr(snip, "trap")){return TRAP;}
	if(cmpSnipvStr(snip, "br")){return BR;}
	if(cmpSnipvStr(snip, "jsr")){return JSR;}
	if(cmpSnipvStr(snip, "jmp")){return JMP;}
	if(cmpSnipvStr(snip, "jsrr")){return JSRR;}
	if(cmpSnipvStr(snip, "not")){return NOT;}
	if(cmpSnipvStr(snip, "lea")){return LEA;}
	if(cmpSnipvStr(snip, "and")){return ANDi;} /*be sure to do checks on these*/
	if(cmpSnipvStr(snip, "add")){return ADDi;}
	if(cmpSnipvStr(snip, "xor")){return XORi;}
	if(cmpSnipvStr(snip, "lshf")){return LSHF;}
	if(cmpSnipvStr(snip, "rshfl")){return RSHL;}
	if(cmpSnipvStr(snip, "rshfa")){return RSHA;}
	if(cmpSnipvStr(snip, "ldb")){return LDB;}
	if(cmpSnipvStr(snip, "ldw")){return LDW;}
	if(cmpSnipvStr(snip, "stb")){return STB;}
	if(cmpSnipvStr(snip, "stw")){return STW;}
	if( cmpChars(infile[snip.beg+0],'b') && cmpChars(infile[snip.beg+1],'r') )
	{
		return BRnzpOp(snip);
	}
	return INVALID_OP;

}



OPCODE	
whichOp(snippet snip)
{
	OPCODE op = isOp(snip);
	if( (op==ANDi || op==ADDi || op==XORi) )
	{

		if(whichReg(tokens[num_tokens-1])!=NOT_REG) {return (ANDr-ANDi)+op; /*enum offset:imm to reg*/ }
		if(whichImmediate(tokens[num_tokens-1],op)!=NOT_IMMEDIATE) {return op;}
		else
		{
			printf("%d error: Invalid last operand!", line);
			exit(4);
		}
	}
	return op;
}


bool isDelim(char c, char* delimeters)
{
	int d_index = 0;
	
	while( delimeters[d_index] != 0)
	{
		if(delimeters[d_index]==c)
			return true;
		d_index++;
	}
	return false;
}

void
seekToEOL()
{
	while(infile[FP] != '\n')
	{
		if( infile[FP]==0 ){return;}
		FP++;
	}
	FP++;
	return;
}
/*updates line, returns FP at the beginning of the newest line or at EOF indicating 0, with tokens containing up to 5 "snippets"*/
void
tokenize(char* delimeters) /*FPorig is actually just an offset for the string containing the file, not a real file pointer*/
{	
	/*int FP = FPorig?; /*questions arose about thread safety for using a global for this FP*/
	num_tokens = 0;
	for(;;) /*this is a while(1), right?*/
	{	
		if( num_tokens >= 5){return;} /*don't buffer overflow me, bro*/
		while( isDelim(infile[FP], delimeters)==true ){
			FP++;
		}
		if( infile[FP] == '\n' ) /* newline after delimeters, no new tokens to report*/
		{
			FP++;
			line++;
			return;
		}
		if( infile[FP] == 0) /* EOF after delimeters, no new tokens to report*/
		{
			return;
		}
		if( infile[FP] == ';')/* henceforth comments, stop tokenizing this line */
		{
			seekToEOL();
			line = infile[FP]==0 ? line :line+1;
			return;
		}
		tokens[num_tokens].beg = FP;
		
		while( !isDelim(infile[FP], delimeters) )
		{
			if( infile[FP] == '\n' || infile[FP]==0) /*newline or EOF after non-delims, report one last token*/
			{
				tokens[num_tokens].end = FP;
				FP++;
				num_tokens++;
				line = infile[FP]==0 ? line :line+1;
				return;
			}
			if( infile[FP] == ';')
			{
				tokens[num_tokens].end = FP;
				num_tokens++;
				seekToEOL();
				line = infile[FP]==0 ? line :line+1;
				return;
			}
			FP++;

		}
		tokens[num_tokens].end = FP;
		num_tokens++;
	}


		
}
void
seekToCode()
{
	if( codebegin==UNPARSED)
	{
	FP=0;
	while(1)
	{

		tokenize(" \t,");
		if (whichOp(tokens[1])==ORIG)
		{
			printf("%d error: .ORIG may not have a label or invalid token preceding!");
			exit(4);

		}
		if( whichOp(tokens[0])==ORIG)
		{
			int token2 = whichImmediate(tokens[1], ORIG);
			if( infile[FP]==0 )
			{
				printf("%d error: .ORIG present, but no .END!",line);
				exit(4);
			}
			codebegin = FP;
			codebegin_line=line;
			beginaddress = whichImmediate(tokens[1],ORIG);
			return;
		}
		if( infile[FP]==0)
		{
			printf("error: Input file must contain starting point .ORIG!");
			exit(4);
		}
		if( num_tokens>0 )
		{
			printf("%d warning: Tokens ignored before .ORIG found",line);
		}

	}

	}
	FP = codebegin;
	line = codebegin_line;
}
/*f*/
char* readFile(char* _filename){
	FILE* infile = fopen(_filename,"r");
	
  	fseek (infile , SEEK_SET , SEEK_END);
	long size = ftell (infile);
	rewind (infile);

  	char* strfile = malloc (sizeof(char)*size);
	if (strfile == NULL)
	{
		fputs ("Memory error",stderr); exit (2);
	}
	fread(strfile,1,size,infile);	
	return strfile;
}

void
addSymb(snippet snip)
{
	symbol* s = symbols+sym_index;
	s->line = line;
	s->symb = snip;
	sym_index++;
}
bool
cmpSnips(snippet snip1, snippet snip2)
{
	if(snip1.end-snip1.beg != snip2.end-snip2.beg)
	{
		return false;
	}
	int i = 0;
	for(;i<snip1.end-snip1.beg;i++)
	{
		if( !cmpChars(infile[snip1.beg+i],infile[snip2.beg+i]) ){return false;}
	}
	return true;
}
int
whichLabel(snippet snip)
{
	int i = 0;
	for(;i<num_symbols;i++)
	{
		snippet sym = symbols[i].symb;
		if(cmpSnips(sym,snip)){return i;}
	}
	return NOT_LABEL;
}
/* ---------- Utilities for Tests --------------*/

void
printsnippet(snippet snip)
{
	int fetcher = snip.beg;
	while(fetcher < snip.end)
	{
		if(infile[fetcher]=='\n')
			printf("There is a newline in the token!");
		putc(infile[fetcher],stdout);
		fetcher++;
	}
	putc(' ',stdout);
	fflush(stdout);
}
void
printtokens()
{
	int i=0;
	for(i; i<num_tokens; i++)
	{
		printsnippet(tokens[i]);
				
	}
	/*printf("\n");*/
}

void
printSymbols()
{
	int i = 0;
	for(;i<num_symbols;i++)
	{
		printsnippet(symbols[i].symb);

	}

}

/* ------------- Tests --------------*/

void
tokenTest()
{
	printf("TokenTest:\n");
	/*remember to check num_tokens is 5 or less, so that special exit cases don't need clunky looking checks in tokenize*/
	char* test = "    abc \n";
	char* test2 = "token1 token2 token3 token4 token5 toomanytoken!";
	char* test3 = "\tdoop,chyuahhh@#$,;comment fake!";
	char* test4 = "";
	char* test5 = "L342 random 	,	welp ba;d betternotbeshown!";
	char* test6 = "one two three";
	char* test7 = " ,\tsimple simple\n\t, FUUUUUUUUUUUU, nUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUU!!!!!! ,\t\nADd R3 R3 #123\nFirst sec;nd";
	
	FP = 0;
	infile = test7;
	while(infile[FP] != 0)
	{
		tokenize(" \t,");
		printtokens();
		fflush(stdout);
		printf("\n");
	}
	fflush(stdout);


}

void
delimTest()
{
	printf("DelimTest:\n");
	bool test = isDelim('c'," \t,");
	bool test2 = isDelim('\n'," \t,");
	bool test3 = isDelim(' '," \t,");
	
	printf("test1 passed?: %d\n", false==test);
	printf("test2 passed?: %d\n", false==test2);
	printf("test3 passed?: %d\n\n", true==test3);
}

void
cmpCharTest()
{
	bool test = cmpChars('x','x');
	bool test2 = cmpChars('a','A');
	bool test3 = cmpChars('B','b');
	bool test4 = cmpChars('\n','n');

	printf("test1 passed?: %d\n", true==test);
	printf("test2 passed?: %d\n", true==test2);
	printf("test3 passed?: %d\n", true==test3);
	printf("test4 passed?: %d\n\n", false==test4);
}

void
regTest()
{
	infile = " r0 R6 R16 r9 R\t";
	tokenize(" ,\t");
	REG test = whichReg(tokens[0]);
	REG test2 = whichReg(tokens[1]);
	REG test3 = whichReg(tokens[2]);
	REG test4 = whichReg(tokens[3]);
	REG test5 = whichReg(tokens[4]);

	printf("test1 passed?: %d\n", R0==test && 0==test);
	printf("test2 passed?: %d\n", R6==test2 && 6==test2);
	printf("test3 passed?: %d\n", NOT_REG==test3);
	printf("test4 passed?: %d\n", NOT_REG==test4 && -1==test4);
	printf("test5 passed?: %d\n", NOT_REG==test5);

}
void
seekToCodeTest()
{
	seekToCode();

}
void
cmpSnipsTest()
{
	infile = "wor del2 am del2";
	snippet snip1;
	snip1.beg = 4;
	snip1.end = 8;
	snippet snip2;
	snip2.beg = 12;
	snip2.end = 16;
	printsnippet(snip1);
	printsnippet(snip2);
	printf("cmpSnips test1: %d",cmpSnips(snip1,snip2));
	fflush(stdout);
	exit(0);
}
void
tests()
{
	delimTest();
	tokenTest();
	cmpCharTest();
	regTest();
	seekToCodeTest();

}
/* --------- Control ----------- */

int
calcLabel(int labelnum, OPCODE op)
{

	symbol s = symbols[labelnum];
	int offset = s.line-(line+1);

	if(op != JSR) /*BRs included in this*/
	{

		if(offset < 0)
		{
			if( (offset*-1) > 0x01FF )
			{
				printf("%d error: Offset too large!",line);
				exit(3);
			}
			return offset&0x1FF;
		}
		else if(offset> 0x01FF)
		{
			printf("%d error: Offset too large!",line);
			exit(3);
		}
		return offset;
	}

	if(op==JSR)
	{
		if(offset < 0)
		{
			if( (offset*-1) > 0x07FF )
			{
				printf("%d error: JSR offset too large!",line);
				exit(3);
			}
			return offset&0x7FF;
		}
		else if(offset> 0x07FF)
		{
			printf("%d error: JSR offset too large!",line);
			exit(3);
		}
		return offset;
	}


}
int
fourTokens()
{
	int op = whichOp(tokens[0]);
	if(op==ANDr || op== ADDr || op==XORr)
	{
		int reg1 = whichReg(tokens[1]);
		int reg2 = whichReg(tokens[2]);
		int reg3 = whichReg(tokens[3]);

		if(reg1==NOT_REG || reg2==NOT_REG || reg3==NOT_REG)
		{
			printf("%d error: Invalid arguments");
			exit(4);
		}
		switch(op)
		{
		case ANDr:
			return (0x5000 | reg1<<9 | reg2<<6 | reg3);
		case ADDr:
			return (0x1000 | reg1<<9 | reg2<<6 | reg3);
		case XORr:
			return (0x9000 | reg1<<9 | reg2<<6 | reg3);

		}
	}
		if(op==ANDi || op==ADDi || op == XORi || op==LDB || op==LDW || op==LSHF || op==RSHL || op==RSHA
			|| op==LDW || op==LDB	|| op==STB || op==STW)
		{
			int reg1 = whichReg(tokens[1]);
			int reg2 = whichReg(tokens[2]);
			int imm3 = whichImmediate(tokens[3],op);

			if(reg1==NOT_REG || reg2==NOT_REG || imm3==NOT_IMMEDIATE)
			{
				printf("%d error: Invalid arguments");
				exit(4);
			}
			switch(op)
			{
			case ANDi:
				imm3 = whichImmediate(tokens[3],ANDi);
				return (0x5000 | reg1<<9 | reg2<<6 | 0x0020 | imm3);
			case ADDi:
				imm3 = whichImmediate(tokens[3],ADDi);
				return (0x1000 | reg1<<9 | reg2<<6 | 0x0020 | imm3);
			case XORi:
				imm3 = whichImmediate(tokens[3],XORi);
				return (0x9000 | reg1<<9 | reg2<<6 | 0x0020 | imm3);
			case LDB:
				imm3 = whichImmediate(tokens[3],LDB);
				return (0x2000 | reg1<<9 | reg2<<6 |  imm3);
			case LDW:
				imm3 = whichImmediate(tokens[3],LDW);
				return (0x6000 | reg1<<9 | reg2<<6 |  imm3);
			case LSHF:
				imm3 = whichImmediate(tokens[3],LSHF);
				return (0xD000 | reg1<<9 | reg2<<6 |  imm3);
			case RSHL:
				imm3 = whichImmediate(tokens[3],RSHL);
				return (0x6000 | reg1<<9 | reg2<<6 | 0x0010 | imm3);
			case RSHA:
				imm3 = whichImmediate(tokens[3],RSHA);
				return (0x6000 | reg1<<9 | reg2<<6 | 0x0030 | imm3);
			case STB:
				imm3 = whichImmediate(tokens[3],STB);
				return (0x6000 | reg1<<9 | reg2<<6 | imm3);
			case STW:
				imm3 = whichImmediate(tokens[3],STW);
				return (0x7000 | reg1<<9 | reg2<<6 | imm3);
			}


		}

	}

int
threeTokens()
{
	OPCODE op = whichOp(tokens[0]);
	if(op==NOT)
	{
		int reg1 = whichReg(tokens[1]);
		int reg2 = whichReg(tokens[2]);
		if(reg1==NOT_REG || reg2==NOT_REG)
		{
			printf("%d error: Invalid argument");
			exit(4);
		}
		return (0x9000 || reg1<<9 || reg2<<6);
	}

	if(op==LEA)
	{
		int reg1 = whichReg(tokens[1]);
		int label2 = whichLabel(tokens[2]);
		if(reg1==NOT_REG || label2==NOT_LABEL)
		{
			printf("%d error: Invalid argument");
			exit(4);
		}
		int offset = calcLabel(label2, op);
		if(offset > (1<<9))
		{
			printf("%d error: Offset too large!");
			exit(3);
		}
		int val1 = reg1<<9 ;
		int val2 = 0xE000 | val1;
		int val4 =  val2 | offset;
		return val4;

	}
	printf("%d error: Invalid arguments");
	exit(4);

}
int
twoTokens()
{
	OPCODE op = whichOp(tokens[0]);
	if(op==BR || op==BRn || op==BRnz || op==BRnzp || op==BRz || op==BRzp || op==BRnp || op==BRp || op==JSR)
	{
		int label = whichLabel(tokens[1]);
		if ( label == NOT_LABEL )
		{
			printf("%d error: Undefined label");
			exit(1);
		}
		int offset = calcLabel(label, op);
		switch(op)
		{
		case BR:
		case BRnzp:
			return (0x0 | 0x0E00 | offset);
		case BRn:
			return (0x0 | 0x0800 | offset);
		case BRnz:
			return (0x0 | 0x0C00 | offset);
		case BRz:
			return (0x0 | 0x0400 | offset);
		case BRnp:
			return (0x0 | 0x0A00 | offset);
		case BRzp:
			return (0x0 | 0x0600 | offset);
		case JSR:
			return (0x4800 | offset );
		}
	}

	if( op==JMP|| op==JSRR)
	{
		int reg = whichReg(tokens[1]);
		if( reg == NOT_REG)
		{
			printf("%d error: Undefined operand");
			exit(4);
		}
		switch(op)
		{
		case JMP:
			return ( 0xC000 | reg<<6);
		case JSRR:
			return (0x4000 | reg<<6);
		}

	}

	if(op==TRAP)
	{
		int imm = whichImmediate(tokens[1],TRAP);
		if(imm == NOT_IMMEDIATE)
		{
			printf("%d error: Invalid Operand",line);
			exit(4);
		}
		if(imm<0 || imm>0xFF){
			printf("%d error: Trap vector out of range",line);
			exit(3);
		}
		return (0xF000 | imm);
	}

	if(op==FILL)
	{
		int imm = whichImmediate(tokens[1],FILL);
		return imm;
	}
	printf("%d error: Invalid arguments");
	exit(4);

}
int
oneToken()
{
	OPCODE op = whichOp(tokens[0]);
	if(op==RTI){return 0x8000;}
	if(op==RET){return 0xC1C0;}
	if(op==GETC){return 0xF020 ;}
	if(op==OUT){return 0xF021 ;}
	if(op==PUTS){return 0xF022 ;}
	if(op==IN){return 0xF023 ;}
	if(op==HALT){return 0xF025 ;}

}
char
hexLookup(int n)
{
	if(n<10){return n+'0';}
	else if(n>=10 && n<=15){return 'A'+(n-10);}
	else{
		printf("heeeeeellllp");
	}

}
void
intToHex(int num)
{
	int i = 0;
	int mask = 0xF;
	outfile[ (out_line*6) ] = 'x';
	for(;i<4;i++)
	{
		int mcopy = mask<<(4*i);
		int copy = num<0 ? ((~num)+1) : num;
		copy = copy&mcopy;
		copy = copy>>(4*i);
		outfile[ (out_line*6)+(4-i) ] = hexLookup(copy);
	}
	outfile[ (out_line*6) +5 ] = '\n';
	out_line++;

}

int main(int argc, char* args[]){
	tokens = (snippet*)malloc(sizeof(snippet)*6);
	infile = readFile(args[1]);
	line = 0;
	FP = 0;
	num_tokens = -1;
	codebegin = UNPARSED;


	seekToCode();

	int labels = 0;
	int codend_line = UNPARSED;
	while(1)	/*figure out room for ST*/
	{
		tokenize(" ,\t");
		if(num_tokens==0){
			if( infile[FP]==0){break;}
			continue;

		}
		else if(isOp(tokens[0]) != INVALID_OP){
			if(whichOp(tokens[0])==END)
			{
						codend_line = line;
						break;
			}
			continue;
		}
		else
		{
			labels++;
		}
	}
	if(codend_line == UNPARSED){
		printf("error: No .END found!");
		exit(4);
	}
	num_symbols = labels;
	symbols = malloc(sizeof(symbol)*num_symbols);
	sym_index = 0;
	seekToCode(); /* put space and go back to the beginning to capture the symbols*/
	while(1)
	{
		tokenize(" ,\t");
		if(num_tokens==0){
			if( infile[FP]==0){break;}
			continue;

		}
		else if(isOp(tokens[0]) != INVALID_OP){
			if(whichOp(tokens[0])==END)
			{
						break;
			}
			continue;
		}
		else
		{
			addSymb(tokens[0]);
		}

	}

	outfile = malloc(6*sizeof(char)*codend_line);
	seekToCode();
	bool labeled = false;
	out_line = 0;
	intToHex(beginaddress);
	int addMe = 0xFFFF;
	while(line<codend_line-1)
	{
		tokenize(" ,\t");
		if(num_tokens == 0){continue;}
		if( whichLabel(tokens[0]) != NOT_LABEL)
		{
			labeled=true;
			tokens++;
			num_tokens-=1;
		}
		switch(num_tokens)
		{
		case 1:
			addMe = oneToken();
			break;
		case 2:
			addMe = twoTokens();
			break;
		case 3:
			addMe = threeTokens();
			break;
		case 4:
			addMe = fourTokens();
			break;
		}
		intToHex(addMe);
		if(labeled)
		{
			labeled = false;
			tokens--;
		}
	}
	outfile[ (6*out_line)+6] = 0;
	printf("%s",outfile);
	fflush(stdout);
	free(tokens);
	free(symbols);
	free(outfile);
	return 0;
}

