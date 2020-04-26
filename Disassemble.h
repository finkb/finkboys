#include <iostream>
#include <string>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <malloc.h>

#ifndef UNICODE
#define UNICODE
#endif 

//include GLEW
//#include <GL/glew.h>

// Include GLFW
//#include <GLFW/glfw3.h>

//#pragma comment(lib,"glew32.lib")
//#pragma comment(lib,"glfw3.lib")
//#pragma comment(lib,"opengl32.lib")

							//will put glew32.dll in game exe
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
//#define WINDOW_TITLE "OpenGL Project"
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
//#define GLEW_STATIC
using namespace std;


typedef struct ConditionCodes {    
    uint8_t    z:1;    
    uint8_t    s:1;    
    uint8_t    p:1;    
    uint8_t    cy:1;    
    uint8_t    ac:1;    
    uint8_t    pad:3;  
	uint8_t    int_enable:1;
} ConditionCodes;

typedef struct StateMachine {    
    uint8_t    a;    
    uint8_t    b;    
    uint8_t    c;    
    uint8_t    d;    
    uint8_t    e;    
    uint8_t    h;    
    uint8_t    l;    
    int16_t    sp;    
    uint16_t    pc;    
    uint8_t    *memory; //1FFF is the last of the ROM, ram is at $2000 and working ram is $4000 and vid is $6000  
	uint16_t   *st; //(max 256)
    struct      ConditionCodes      cc;   
} StateMachine;    

typedef struct{
	char* op;
	int size;
} OPCODE;

//Certain instructions are "non-instructions" set the bytes to 1 so that if for some reason that instructionis encountered we can skip it
OPCODE opCode[] = {{"NOP",1},{"LXI   B,#",3},{"STAX  B",1},{"INX   B",1},{"INR   B",1},{"DCR   B",1},{"MVI   B,#",2},{"RLC",1},{"",1},{"DAD   B",1},{"LDAX  B",1},{"DCX   B",1},
                   {"INR   C",1},{"DCR   C",1},{"MVI   C,#",2},{"RRC",1},{"",1},{"LXI   D,#",3},{"STAX  D",1},{"INX   D",1},{"INR   D",1},{"DCR   D",1},{"MVI   D,#",2},{"RAL",1},
                   {"",1},{"DAD   D",1},{"LDAX  D",1},{"DCX   D",1},{"INR   E",1},{"DCR   E",1},{"MVI   E,#",2},{"RAR",1},{"RIM",1},{"LXI   H,#",3},{"SHLD adr",3},{"INX   H",1},
                   {"INR   H",1},{"DCR   H",1},{"MVI   H,#",2},{"DAA",1},{"",1},{"DAD   H",1},{"LHLD adr",3},{"DCX   H",1},{"INR   L",1},{"DCR   L",1},{"MVI   L,#",2},{"CMA",1},
                   {"SIM",1},{"LXI   SP,#",3},{"STA adr",3},{"INX   SP",1},{"INR   M",1},{"DCR   M",1},{"MVI   M,#",2},{"STC",1},{"",1},{"DAD",1},{"LDA adr",3},{"DCX   SP",1},
                   {"INR   A",1},{"DCR   A",1},{"MVI   A,#",2},{"CMC",1},{"MOV   B,B",1},{"MOV   B,C",1},{"MOV   B,D",1},{"MOV   B,E",1},{"MOV   B,H",1},{"MOV   B,L",1},{"MOV   B,M",1},
                   {"MOV   B,A",1},{"MOV   C,B",1},{"MOV   C,C",1},{"MOV   C,D",1},{"MOV   C,E",1},{"MOV   C,H",1},{"MOV   C,L",1},{"MOV   C,M",1},{"MOV   C,A",1},{"MOV   D,B",1},
                   {"MOV   D,C",1},{"MOV   D,D",1},{"MOV   D,E",1},{"MOV   D,H",1},{"MOV   D,L",1},{"MOV   D,M",1},{"MOV   D,A",1},{"MOV   E,B",1},{"MOV E,C",1},{"MOV   E,D",1},
                   {"MOV   E,E",1},{"MOV   E,H",1},{"MOV   E,L",1},{"MOV   E,M",1},{"MOV   E,A",1},{"MOV   H,B",1},{"MOV   H,C",1},{"MOV   H,D",1},{"MOV   H,E",1},{"MOV   H,H",1},
                   {"MOV   H,L",1},{"MOV   H,M",1},{"MOV   H,A",1},{"MOV   L,B",1},{"MOV   L,C",1},{"MOV   L,D",1},{"MOV   L,E",1},{"MOV   L,H",1},{"MOV   L,L",1},{"MOV   L,M",1},
                   {"MOV   L,A",1},{"MOV   M,B",1},{"MOV M,C",1},{"MOV   M,D",1},{"MOV   M,E",1},{"MOV   M,H",1},{"MOV   M,L",1},{"HLT",1},{"MOV   M,A",1},{"MOV   A,B",1},{"MOV   A,C",1},
                   {"MOV   A,D",1},{"MOV   A,E",1},{"MOV   A,H",1},{"MOV   A,L",1},{"MOV   A,M",1},{"MOV   A,A",1},{"ADD   B",1},{"ADD   C",1},{"ADD   D",1},{"ADD   E",1},{"ADD   H",1},
                   {"ADD   L",1},{"ADD   M",1},{"ADD   A",1},{"ADC   B",1},{"ADC   C",1},{"ADC   D",1},{"ADC   E",1},{"ADC   H",1},{"ADC   L",1},{"ADC   M",1},{"ADC   A",1},{"SUB   B",1},
                   {"SUB   C",1},{"SUB   D",1},{"SUB   E",1},{"SUB   H",1},{"SUB L",1},{"SUB   M",1},{"SUB   A",1},{"SBB B",1},{"SBB   C",1},{"SBB   D",1},{"SBB   E",1},{"SBB   H",1},
                   {"SBB   L",1},{"SBB   M",1},{"SBB   A",1},{"ANA   B",1},{"ANA   C",1},{"ANA   D",1},{"ANA E",1},{"ANA   H",1},{"ANA   L",1},{"ANA M",1},{"ANA   A",1},{"XRA   B",1},
                   {"XRA   C",1},{"XRA   D",1},{"XRA   E",1},{"XRA   H",1},{"XRA   L",1},{"XRA   M",1},{"XRA   A",1},{"ORA   B",1},{"ORA   C",1},{"ORA   D",1},{"ORA   E",1},{"ORA   H",1},
                   {"ORA   L",1},{"ORA   M",1},{"ORA   A",1},{"CMP   B",1},{"CMP   C",1},{"CMP   D",1},{"CMP   E",1},{"CMP   H",1},{"CMP   L",1},{"CMP   M",1},{"CMP A",1},{"RNZ",1},
                   {"POP   B",1},{"JNZ adr",3},{"JMP adr",3},{"CNZ adr",3},{"PUSH  B",1},{"ADI   #",2},{"RST   0",1},{"RZ",1},{"RET",1},{"JZ adr",3},{"",1},{"CZ adr",3},{"CALL adr",3},
                   {"ACI   #",2},{"RST   1",1},{"RNC",1},{"POP   D",1},{"JNC adr",3},{"OUT   #",2},{"CNC adr",3},{"PUSH  D",1},{"SUI   #",2},{"RST 2",1},{"RC",1},{"",1},{"JC adr",3},
                   {"IN    #",2},{"CC adr",3},{"",1},{"SBI   #",2},{"RST   3",1},{"RPO",1},{"POP   H",1},{"JPO adr",3},{"XTHL",1},{"CPO adr",1},{"PUSH  H",1},{"ANI   #",2},{"RST   4",1},
                   {"RPE",1},{"PCHL",1},{"JPE adr",3},{"XCHG",1},{"CPE adr",3},{"",1},{"XRI   #",2},{"RST   5",1},{"RP",1},{"POP   PSW",1},{"JP adr",3},{"DI",1},{"CP adr",3},
                   {"PUSH  PSW",1},{"ORI   #",1},{"RST 6",1},{"RM",1},{"SPHL",1},{"JM adr",3},{"EI",1},{"CM adr",2},{"",1},{"CPI   #",2},{"RST   7",1}};


void UnimplementedInstruction(StateMachine*);
int Emulate8080(StateMachine*,bool);
int Reset8080(StateMachine*);
void push(StateMachine*,uint16_t);
uint16_t pop(StateMachine*);
void parity(StateMachine *, uint8_t);
void drawInvaders(StateMachine *);
void generateInterrupt(StateMachine* state, int intNum, HWND hwnd);
int Disassemble(unsigned char *, int, FILE *);
int UnitTest8080(StateMachine *);
void paintInvaders(HWND, StateMachine*);
