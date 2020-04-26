// Emu8080.cpp : Defines the entry point for the console application.


#include "stdafx.h"
#include <iostream>
#include <time.h>
#include <windows.h>
#include "Disassemble.h"


int instcount=0;

using namespace std;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	FILE *binFile,*outFile;
	unsigned char *buffer;
	int test = 0;
	int resetOK = 0;
	PWSTR *argList;
	int argc;
	int pos = 0;
	int err = 0;
	int size = 0;
	bool drawInvaders = false;
	
	const wchar_t CLASS_NAME[] = L"Space Invaders";
	WNDCLASS  wc = {};
	MSG msg = { };

	wc.lpfnWndProc   = WindowProc;
	wc.hInstance     = hInstance;
	wc.lpszClassName = CLASS_NAME;

	RegisterClass(&wc);

	clock_t starttime = 0,difference = 0,lastInterrupt = 0;
	
	argList = CommandLineToArgvW(GetCommandLineW(),&argc);

	if(argc > 2)
	{
		err = _wfopen_s(&binFile,argList[1],(const wchar_t *)"r");
		if(err != 0)
		{
			cout << "Coundn't open " << (char *)argList[1] << " for reading.  Error: " << err << endl;
			return 0;
		}
	}
	else
	{
		cout << "Emu8080 <input file> <output file>" << endl; 
		return 0;
	}

	fseek(binFile,0L,SEEK_END);
	size = ftell(binFile);
	fseek(binFile,0L,SEEK_SET);

	buffer = (unsigned char *)malloc(size);

	fread(buffer,size,1,binFile);
	fclose(binFile);
	
	err = _wfopen_s(&outFile,argList[2],(const wchar_t *)"w");
	if(err != 0)
	{
		cout << "The output file couldn't be opened. Error: " << err << endl;
	}

	while(pos < size)
	{
		pos += Disassemble(buffer,pos,outFile);
		
	}
	fclose(outFile);
	
	//want to call the emulator?  go for it 	
	StateMachine *state;	

	//allocate some memory for the state machine, then copy the buffer to it
	state = (StateMachine*)malloc(sizeof(StateMachine));
	state->memory = (uint8_t *)malloc(0xFFFF);
	state->st = (uint16_t*)malloc(sizeof(uint16_t)*256);

	state->sp = -1;

	memcpy(state->memory,buffer,size); //copy in the rom
	
	//memory is set with the rom, reset al of the condition codes and states
	resetOK = Reset8080(state);

	   // Create the window.

    HWND hwnd = CreateWindowEx(
        0,                              // Optional window styles.
        CLASS_NAME,                     // Window class
        L"Space Invaders",    // Window text
        WS_OVERLAPPEDWINDOW,            // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

        NULL,       // Parent window    
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
        );

    if (hwnd == NULL)
    {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    // Run the message loop.

    
   
	//create the timer
	starttime = clock(); 
	while(state->pc < size)
	{
		GetMessage(&msg, NULL, 0, 0);
		difference = clock() - starttime;
		cout<<"time diff: " << difference << endl;
		if(difference >= 16)
		{
			difference = 0;
			//draw, send WM_PAINT
			
			
			starttime = clock();
		}
		if(state->cc.int_enable && (clock() - lastInterrupt) > 1.0/60.0)
		{
			generateInterrupt(state,2,hwnd);
			lastInterrupt = clock();
		}
		Emulate8080(state,true);	
		//TranslateMessage(&msg);
        //DispatchMessage(&msg);
	}
	

	free(state->memory);
	free(state);
	free(buffer);
	

	return 0;
}

void paintInvaders(HWND hwnd,StateMachine* state)
{
	int x = 100;
	int y = 248;
	uint16_t vidMemMin = 0x2400;
	uint16_t vidMemMax = 0x3FFF;
	uint16_t vidPos = 0x2400;
	uint8_t vidByte;

	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd, &ps);

	while(vidPos<0x4000)
	{
		if(state->memory[vidPos]==0x00)
		{
			for(int z=0;z<8;z++)
			{
				SetPixel(hdc,x,y,RGB(0,0,0));
				y--;
				if(y==0)
				{
					x++;
					y=248;
				}
			}
		}
		else
		{
			vidByte = state->memory[vidPos];
			for(int z=0;z<8;z++)
			{
				if((vidByte & 0x01) == 0x01)
					SetPixel(hdc,x,y,RGB(0xff,0xff,0xff));
				else
					SetPixel(hdc,x,y,RGB(0x00,0x00,0x00));
				vidByte = vidByte >> 1;
				y--;
				if(y==0)
				{
					x++;
					y=248;
				}
			}
		}
		vidPos++;
	
	}
	EndPaint(hwnd,&ps);
}

int Disassemble(unsigned char *buffer, int cntr, FILE *fb)
{

	char address[1024];
	string op;

	if(opCode[(int)buffer[cntr]].size == 1)
	{
		op = string(opCode[(int)buffer[cntr]].op);
		sprintf_s(address,"%04X  %s\n",cntr,op.c_str());
	}

	if(opCode[(int)buffer[cntr]].size == 2)
	{
		if(string(opCode[(int)buffer[cntr]].op).find("adr") != string::npos)
			op = string(opCode[(int)buffer[cntr]].op).substr(0,string(opCode[(int)buffer[cntr]].op).find("adr")-1);
		else
			op = string(opCode[(int)buffer[cntr]].op);

		sprintf_s(address,"%04X  %s$%02X\n",cntr,op.c_str(),(int)buffer[cntr+1]);
	}
	
	if( opCode[(int)buffer[cntr]].size == 3)
	{
		if(string(opCode[(int)buffer[cntr]].op).find("adr") != string::npos)
			op = string(opCode[(int)buffer[cntr]].op).substr(0,string(opCode[(int)buffer[cntr]].op).find("adr")-1);
		else
			op = string(opCode[(int)buffer[cntr]].op);

		//adr was found in the instruction, remove it then put the address in there	
		sprintf_s(address,"%04X  %s   $%02X%02X\n",cntr,op.c_str(),(int)buffer[cntr+2],(int)buffer[cntr+1]);
		cout << address << endl; //for testing

	}
	//build string to write 
			
	fwrite(address,sizeof(char),strlen(address),fb);
		
	return opCode[(int)buffer[cntr]].size;
}

void push(StateMachine *state, uint16_t val)
{	
	state->sp++;
	state->st[state->sp] = val;
}

uint16_t pop(StateMachine *state)
{
	uint16_t ret;
	if(state->sp == -1)
		return 0;
	state->sp--;
	ret = state->st[state->sp + 1];
	state->st[state->sp + 1] = 0;	//zero it out!
	return ret;
}

void parity(StateMachine *state, uint8_t reg)
{
	int p=0;
	uint8_t par = reg;
	for(int i=0;i<sizeof(uint8_t);i++)
	{
		if(par & 1)
			p++;
		par = par >> 1;
	}
	if(p % 2 == 0)
		state->cc.p = 1;
	else
		state->cc.p = 0;		
}



void UnimplementedInstruction(StateMachine* state)
{
	//move the program counter back by 1, but we are exiting so who cares
	uint8_t *op = &state->memory[state->pc];
	//state->pc-=1;
	
	printf ("Error: Unimplemented instruction\n %x",*op);  
	exit(1);
}

int Emulate8080(StateMachine* state, bool test)
{
	uint8_t *op = &state->memory[state->pc];
	uint16_t address;
	uint8_t address8;
	uint8_t value = 0;
	uint8_t psw;
	

	if(test)
	{
		printf("Before op %s\n",opCode[*op].op);
		UnitTest8080(state);
	}
	switch(*op)	
	{
		case 0x00:	break;											//op 0x00 NOP
		case 0x01:													//op 0x01 LXI B	
			state->c = state->memory[state->pc + 1];
			state->b = state->memory[state->pc + 2];
			state->pc+=2;			
			break;
		case 0x02:													//op 0x02 STAX B
			address = state->b;
			address = (address << 8)|((uint16_t)state->c);
			state->memory[address] = state->a;			
			break;
		case 0x03:													//op 0x03 INX B
			address = state->b;
			address = (address << 8) | ((uint16_t)state->c);
			state->memory[address] = ((uint16_t)address) + 1;			
			break;
		case 0x04:													//op 0x04 INR V			
			state->b++;
			//set 0 code if the add made 0 (why would it!) ff+1=00 dumbass
			state->cc.z = state->b == 0x00;
			state->cc.s = (state->b >> 7) == 0x01;
			state->cc.ac = (state->b & 0x08) == 0x01;
			//check for parity
			parity(state,state->b);			
			break;
		case 0x05:												//DCR B
			state->b--;
			state->cc.z = state->b == 0x00;
			state->cc.s = (state->b >> 7) == 0x01;
			state->cc.ac = (state->b & 0x08) == 0x01;
			parity(state, state->b);			
			break;
		case 0x06:													//op 0x06 MVI B
			state->b = state->memory[state->pc+1];
			state->pc++;
			break;
		case 0x08:												//op Reserved
			break;
		case 0x09:												//op ADD HL,BC
			address = (state->h << 8)|state->l;
			address += ((state->b << 8)|state->c);
			if((address & 0x80) == 1)
			{
				state->cc.cy = 1;
			}
			else
			{
				state->cc.cy = 0;
			}
			state->h = (address & 0xff00)>>8;
			state->l = (address & 0x00ff);
			break;
		case 0x0d:
			state->c--;
			if(state->c == 0x00)
			{
				state->cc.z = 1;
			}
			else
			{
				state->cc.z = 0;
			}
			state->cc.s = (state->c >> 7) == 0x01;			
			parity(state,state->c);
			state->cc.ac = (state->c & 0x08) == 0x01;
			break;
		case 0x0e:												// op MVI next byte to c
			state->c = state->memory[state->pc + 1];
			state->pc++;
			break;
		case 0x0f:												//op RRCA
			state->cc.cy = (state->a & 0x01);
			state->a = (state->a >> 1)|(state->cc.cy << 7);			
			break;
		case 0x11:												//op 0x11 LXI D
			state->d = state->memory[state->pc+2];
			state->e = state->memory[state->pc+1];
			state->pc+=2;
			break;
		case 0x13:												//INX D increment E, if overflow inc D
			state->e++;
			if(state->e == 0)
				state->d++;
			break;
		case 0x19:												//op DAD D	HL = HL + DE	
			address = (state->h << 8)|state->l;
			address += ((state->d << 8)|state->e);
			if((address & 0x80) == 1)
			{				
				state->cc.cy = 1;
			}
			else
			{
				state->cc.cy = 0;
			}
			state->h = (address & 0xff00)>>8;
			state->l = (address & 0x00ff);
			break;
		case 0x1a:												// op 0x1a LDAX D, load value at 16bit address in accumulator 
			state->a = state->memory[((state->d << 8)|state->e)];
			break;
		case 0x1c:												// op INR e
			state->e++;
			if(state->e == 0x00)
			{
				state->cc.z = 1;
			}
			else
			{
				state->cc.z = 0;
			}
			state->cc.s = (state->b >> 7) == 0x01;
			state->cc.ac = (state->b & 0x08) == 0x01;
			parity(state, state->e);
			break;
		case 0x20:												//op reserved
			break;
		case 0x21:												// op 0x21 LXI H
			state->l = state->memory[state->pc + 1];
			state->h = state->memory[state->pc + 2];
			state->pc+=2;
			break;
		case 0x23:												// op INX H
			state->l++;
			if(state->l == 0x00)
				state->h++;
			break;
		case 0x26:												// op MVI H,D8
			state->h = state->memory[state->pc + 1];
			state->pc++;
			break;
		case 0x29:											//op DAD H			
			address = (state->h << 8)|state->l;
			if((address & 0x80) == 1)
			{
				address = address << 1;
				state->cc.cy = 1;
			}
			else
			{
				address = address << 1;
				state->cc.cy = 0;
			}
			state->h = (address & 0xff00)>>8;
			state->l = (address & 0x00ff);
			break;
		case 0x31:												//op 0x31 LXI SP			
			uint16_t val;
			val = state->memory[state->pc+2];
			val = (val << 8) | state->memory[state->pc+1];
			
			push(state,val);
			
			state->pc+=2;			
			break;
		case 0x32:											//STA ADDR<-A
			state->memory[(state->memory[state->pc + 2])<<8|(state->memory[state->pc + 1])] = state->a;
			state->pc+=2;
			break;
		case 0x35:											//DCR M
			value = (state->memory[(state->h << 8)|state->l]);
			value--;			
			state->cc.z = value == 0x00;
			state->cc.s = (value >> 7) == 0x01;
			state->cc.ac = (value & 0x08) == 0x01;
			parity(state, value);	
			break;
		case 0x36:											//MVI M  MOV Immediate
			state->memory[((state->h<<8)|state->l)] = state->memory[state->pc + 1];
			state->pc++;
			break;
		case 0x3a:											//op LDA <addr>
			state->a = state->memory[(state->memory[state->pc + 2])<<8|(state->memory[state->pc + 1])];
			state->pc+=2;
			break;
		case 0x3d:											//op DCR A
			state->a--;
			parity(state,state->a);
			if(state->a == 0x00)
			{
				state->cc.z = 1;
			}
			else
			{
				state->cc.z = 0;
			}
			state->cc.s = (state->b >> 7) == 0x01;
			state->cc.ac = (state->b & 0x08) == 0x01;
			break;
		case 0x3e:											//op MVI A,D8 8 bit operation
			state->a = state->memory[state->pc + 1];
			state->pc++;
			break;
		case 0x40:											//op MOV B,B
			state->b = state->b;
			break;
		case 0x56:											//op MOV D,M			
			state->d = state->memory[(state->h<<8)|state->l];
			break;
		case 0x5e:											//op MOV E,M memory location in HL to E
			state->e = state->memory[(state->h<<8)|state->l];
			break;
		case 0x66:											//op MOV H,M
			state->h = state->memory[(state->h<<8)|state->l];
			break;
		case 0x67:											// MOV H,A
			state->h = state->a;
			break;
		case 0x6f:											// MOV L,A
			state->l = state->a;
			break;
		case 0x77:											//	MOV M,A	(HL) <- A  moves accumulator into the addres hl
			state->memory[((state->h<<8)|state->l)] = state->a;
			break;
		case 0x7a:										// op  MOV A,D
			state->a = state->d;
			break;
		case 0x7b:										//op MOV A,E
			state->a = state->e;
			break;
		case 0x7c:										// op MOV A,H
			state->a = state->h;
			break;
		case 0x7d:										//op MOV A,L
			state->a = state->l;
			break;
		case 0x7e:										//op MOV A,M
			state->a = state->memory[(state->h<<8)|state->l];
			break;
		case 0xa7:										//op ANA A
			state->a = state->a & state->a;
			if(state->a == 0x00)
			{
				state->cc.z = 1;
			}
			else
			{
				state->cc.z = 0;
			}
			state->cc.s = (state->a >> 7) == 0x01;
			state->cc.ac = (state->a & 0x08) == 0x01;
			parity(state, state->a);			
			break;
		case 0xaf:										//op XRA A
			state->a = state->a ^ state->a;
			state->cc.z = 1;
			state->cc.ac = 0;
			state->cc.cy = 0;
			state->cc.s = 0;
			parity(state,state->a);
			break;
		case 0xc1:										//op POP BC
			address = pop(state);
			state->b = address >> 8;
			state->c = address & 0xff;
			break;
		case 0xc2:										// op JNZ jump if cc.z is 0
			if(state->cc.z == 0)
			{
				state->pc = (state->memory[state->pc+2] << 8) | state->memory[state->pc+1];
				state->pc--;				//must subtract one because we will increment after instruction look
				break;
			}
			state->pc+=2;
			break;
		case 0xc3:											//op 0xC3 JMP			
			state->pc = (state->memory[state->pc+2] << 8) | state->memory[state->pc+1];
			state->pc--;			
			break;
		case 0xc5:											//op PUSH B
			push(state,(state->b<<8)|state->c);
			break;		
		case 0xc6:											// op ADI D8
			state->a = state->a + state->memory[state->pc+1];
			if(state->a==0x00)
			{
				state->cc.z = 1;
			}
			else
			{
				state->cc.z = 0;
			}
			state->cc.s = (state->a >> 7) == 0x01;
			state->cc.ac = (state->a & 0x08) == 0x01;
			parity(state,state->a);
			state->pc++;
			break;
		case 0xc8:											// IF Z RET
			if(state->cc.z)
				state->pc = pop(state) + 2;
			break;
		case 0xc9:											// RET (pop)
			state->pc = pop(state) + 2;
			break;
		case 0xcd:											//op CALL	0xCD										
			push(state,state->pc);
						
			state->pc= (state->memory[state->pc+2] << 8) | state->memory[state->pc+1];
			//(SP-1)=PC.HI,(SP-2)=PC.LO, SP=SP+2, PC=adr
			//store the program counter in SP, inc SP by 2, set the PC to the addrress passed
			state->pc--;
			break;
		case 0xd1:										//op POP DE
			address = pop(state);
			state->d = address >> 8;
			state->e = address & 0xff;
			break;
		case 0xd3:										//op OUT
			//write contents of the accumulator to device # pc + 1
			state->pc++;
			break;
		case 0xd5:										//op PUSH D
			push(state,(state->d<<8)|state->e);			
			break;
		case 0xdb:										//IN D8
			state->cc.int_enable = 0;
			break;
		case 0xe1:										//op POP H
			address = pop(state);
			state->h = address >>8;
			state->l = address & 0xff;
			break;
		case 0xe5:										//op PUSH H
			push(state,(state->h<<8)|state->l);
			break;
		case 0xe6:										// op AND Immediate ANI D8
			state->a = state->a & state->memory[state->pc+1];
			if(state->a==0x00)
			{
				state->cc.z = 1;
			}
			else
			{
				state->cc.z = 0;
			}
			state->cc.s = (state->a >> 7) == 0x01;
			state->cc.ac = (state->a & 0x08) == 0x01;
			parity(state,state->a);
			state->pc++;
			break;
		case 0xeb:										//op XCHG
			address8 = state->e;
			state->e = state->l;
			state->l = address8;
			address8 = state->h;
			state->h = state->d;
			state->d = address8;
			break;
		//case 0xdf:
		//	break;
		case 0xf1:										//op POP PSW
			address = pop(state);
			psw = address &0xff;
			state->cc.s = (psw >> 7) & 0x01;
			state->cc.z = (psw >> 6) & 0x01;
			state->cc.ac = (psw >>4) & 0x01;
			state->cc.p = (psw >> 2) & 0x01;
			state->cc.cy = psw & 0x01;
			state->a = address>>8;
			break;
		case 0xf5:										//op PUSH PSW			
			psw = (state->cc.s<<7)|(state->cc.z<<6)|(0<<5)|(state->cc.ac<<4)|(0<<3)|(state->cc.p<<2)|(1<<1)|(state->cc.cy);
			push(state,(state->a<<8)|psw);			
			break;
		case 0xfb:
			state->cc.int_enable = 0x01;
			break;
		case 0xfe:											// op CPI 
			uint8_t x;
			x = (state->a - state->memory[state->pc + 1]);
			if(x == 0x00)
			{
				state->cc.z = 0x01;
			}
			else
			{
				state->cc.z = 0x00;
			}
			state->cc.s = (x >> 7) == 0x01;
			parity(state,x);
			state->cc.cy = (state->a < state->memory[state->pc + 1]);
			state->pc++;
			break;
		default:
			UnimplementedInstruction(state);

	}

	state->pc++;
	instcount++;
	
	if(test)
		UnitTest8080(state);
	return 0;
}

void generateInterrupt(StateMachine* state, int intNum, HWND hwnd)    
{    
    //perform "PUSH PC"    
       push(state, state->pc);
	   paintInvaders(hwnd,state);
    //Set the PC to the low memory vector.    
    //This is identical to an "RST interrupt_num" instruction.    
    state->pc = 8 * intNum;    
	state->cc.int_enable = 0;
}    

int Reset8080(StateMachine *state)
{
	state->a = 0;
	state->b = 0;
	state->c = 0;
	state->d = 0;
	state->e = 0;
	state->h = 0;
	state->l = 0;
	state->sp = -1;
	state->cc.ac = 0;
	state->cc.cy = 0;
	state->cc.p = 0;
	state->cc.s = 0;
	state->cc.z = 0;
	state->cc.pad = 0;
	state->cc.int_enable = 0;
	state->pc = 0;	
	return 1;
}

int UnitTest8080(StateMachine *testState)
{	
	printf("Current state:\n A: %02X B: %02X C: %02X D: %02X E: %02X H: %02X L: %02X SP: %04X PC: %04X  Condition Codes: AC: %02X CY: %02X P: %02X S: %02X Z: %02X PAD: %02X\n\n",testState->a,testState->b,testState->c,testState->d,testState->e,testState->h,testState->l,testState->sp, testState->pc,testState->cc.ac,testState->cc.cy,testState->cc.p,testState->cc.p,testState->cc.s,testState->cc.z,testState->cc.pad);
	
	return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);



            FillRect(hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));

            EndPaint(hwnd, &ps);
        }
        return 0;

    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}