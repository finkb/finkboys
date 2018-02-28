// 8080Disassembler.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include "Disassemble.h"
using namespace std;

int _tmain(int argc, _TCHAR* argv[])
{
	FILE *binFile,*outFile;
	unsigned char *buffer;
	int pos = 0;

	if(argc>2)
	{
		if((binFile = fopen(argv[1],"r"))==NULL)
		{
			cout << "Coundn't open " << argv[1] << " for reading." << endl;
			return 0;
		}
	}
	else
	{
		cout << "8080Disassembler <input file> <output file>" << endl; 
		return 0;
	}

	fseek(binFile,0L,SEEK_END);
	int size = ftell(binFile);
	fseek(binFile,0L,SEEK_SET);

	buffer = (unsigned char *)malloc(size);

	fread(buffer,size,1,binFile);
	fclose(binFile);
	
	if((outFile = fopen(argv[2],"w"))==NULL)
	{
		cout << "The output file couldn't be opened" << endl;
	}

	while(pos < size)
	{
		pos+=Disassemble(buffer,pos,outFile);
		
	}
	fclose(outFile);
	
	//want to call the emulator?  go for it 
	ConditionCodes *cCode;
	StateMachine *state;
	

	int test = 0;
	int resetOK = 0;
	//allocate some memory for the state machine, then copy the buffer to it
	state = (StateMachine*)malloc(sizeof(StateMachine));
	state->memory = (uint8_t *)malloc(0xFFFF);
	
	memcpy(state->memory,buffer,size); //copy in the rom
	
	//memory is set with the rom, reset al of the condition codes and states
	resetOK = Reset8080(state);

	while(state->pc < size)
	{
		Emulate8080(state,true);
	}
	

	free(state->memory);
	free(state);
	free(buffer);

	return 0;
}



