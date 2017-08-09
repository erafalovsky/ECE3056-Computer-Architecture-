/***************************************************************/
/*                                                             */
/* LC-3b Simulator (Adapted from Prof. Yale Patt at UT Austin) */
/*                                                             */
/***************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/***************************************************************/
/*                                                             */
/* Files:  ucode        Microprogram file                      */
/*         isaprogram   LC-3b machine language program file    */
/*                                                             */
/***************************************************************/

/***************************************************************/
/* These are the functions you'll have to write.               */
/***************************************************************/

void eval_micro_sequencer();
void cycle_memory();
void eval_bus_drivers();
void drive_bus();
void latch_datapath_values();

/***************************************************************/
/* A couple of useful definitions.                             */
/***************************************************************/
#define FALSE 0
#define TRUE  1

/***************************************************************/
/* Use this to avoid overflowing 16 bits on the bus.           */
/***************************************************************/
#define Low16bits(x) ((x) & 0xFFFF)

/***************************************************************/
/* Definition of the control store layout.                     */
/***************************************************************/
#define CONTROL_STORE_ROWS 64
#define INITIAL_STATE_NUMBER 18

/***************************************************************/
/* Definition of bit order in control store word.              */
/***************************************************************/
enum CS_BITS {
    IRD,
    COND1, COND0,
    J5, J4, J3, J2, J1, J0,
    LD_MAR,
    LD_MDR,
    LD_IR,
    LD_BEN,
    LD_REG,
    LD_CC,
    LD_PC,
    GATE_PC,
    GATE_MDR,
    GATE_ALU,
    GATE_MARMUX,
    GATE_SHF,
    PCMUX1, PCMUX0,
    DRMUX,
    SR1MUX,
    ADDR1MUX,
    ADDR2MUX1, ADDR2MUX0,
    MARMUX,
    ALUK1, ALUK0,
    MIO_EN,
    R_W,
    DATA_SIZE,
    LSHF1,
    CONTROL_STORE_BITS
} CS_BITS;

/***************************************************************/
/* Functions to get at the control bits.                       */
/***************************************************************/
int GetIRD(int *x)           { return(x[IRD]); }
int GetCOND(int *x)          { return((x[COND1] << 1) + x[COND0]); }
int GetJ(int *x)             { return((x[J5] << 5) + (x[J4] << 4) +
				      (x[J3] << 3) + (x[J2] << 2) +
				      (x[J1] << 1) + x[J0]); }
int GetLD_MAR(int *x)        { return(x[LD_MAR]); }
int GetLD_MDR(int *x)        { return(x[LD_MDR]); }
int GetLD_IR(int *x)         { return(x[LD_IR]); }
int GetLD_BEN(int *x)        { return(x[LD_BEN]); }
int GetLD_REG(int *x)        { return(x[LD_REG]); }
int GetLD_CC(int *x)         { return(x[LD_CC]); }
int GetLD_PC(int *x)         { return(x[LD_PC]); }
int GetGATE_PC(int *x)       { return(x[GATE_PC]); }
int GetGATE_MDR(int *x)      { return(x[GATE_MDR]); }
int GetGATE_ALU(int *x)      { return(x[GATE_ALU]); }
int GetGATE_MARMUX(int *x)   { return(x[GATE_MARMUX]); }
int GetGATE_SHF(int *x)      { return(x[GATE_SHF]); }
int GetPCMUX(int *x)         { return((x[PCMUX1] << 1) + x[PCMUX0]); }
int GetDRMUX(int *x)         { return(x[DRMUX]); }
int GetSR1MUX(int *x)        { return(x[SR1MUX]); }
int GetADDR1MUX(int *x)      { return(x[ADDR1MUX]); }
int GetADDR2MUX(int *x)      { return((x[ADDR2MUX1] << 1) + x[ADDR2MUX0]); }
int GetMARMUX(int *x)        { return(x[MARMUX]); }
int GetALUK(int *x)          { return((x[ALUK1] << 1) + x[ALUK0]); }
int GetMIO_EN(int *x)        { return(x[MIO_EN]); }
int GetR_W(int *x)           { return(x[R_W]); }
int GetDATA_SIZE(int *x)     { return(x[DATA_SIZE]); }
int GetLSHF1(int *x)         { return(x[LSHF1]); }

/***************************************************************/
/* The control store rom.                                      */
/***************************************************************/
int CONTROL_STORE[CONTROL_STORE_ROWS][CONTROL_STORE_BITS];

/***************************************************************/
/* Main memory.                                                */
/***************************************************************/
/* MEMORY[A][0] stores the least significant byte of word at word address A
   MEMORY[A][1] stores the most significant byte of word at word address A
   There are two write enable signals, one for each byte. WE0 is used for
   the least significant byte of a word. WE1 is used for the most significant
   byte of a word. */

#define WORDS_IN_MEM    0x08000
#define MEM_CYCLES      5
int MEMORY[WORDS_IN_MEM][2];

/***************************************************************/

/***************************************************************/

/***************************************************************/
/* LC-3b State info.                                           */
/***************************************************************/
#define LC_3b_REGS 8

int RUN_BIT;	/* run bit */
int BUS;	/* value of the bus */

typedef struct System_Latches_Struct{

int PC,		/* program counter */
    MDR,	/* memory data register */
    MAR,	/* memory address register */
    IR,		/* instruction register */
    N,		/* n condition bit */
    Z,		/* z condition bit */
    P,		/* p condition bit */
    BEN;        /* ben register */

int READY;	/* ready bit */
  /* The ready bit is also latched as you dont want the memory system to assert it
     at a bad point in the cycle*/

int REGS[LC_3b_REGS]; /* register file. */

int MICROINSTRUCTION[CONTROL_STORE_BITS]; /* The microintruction */

int STATE_NUMBER; /* Current State Number - Provided for debugging */
} System_Latches;

/* Data Structure for Latch */

System_Latches CURRENT_LATCHES, NEXT_LATCHES;

/***************************************************************/
/* A cycle counter.                                            */
/***************************************************************/
int CYCLE_COUNT;

/***************************************************************/
/*                                                             */
/* Procedure : help                                            */
/*                                                             */
/* Purpose   : Print out a list of commands.                   */
/*                                                             */
/***************************************************************/
void help() {
    printf("----------------LC-3bSIM Help-------------------------\n");
    printf("go               -  run program to completion       \n");
    printf("run n            -  execute program for n cycles    \n");
    printf("mdump low high   -  dump memory from low to high    \n");
    printf("rdump            -  dump the register & bus values  \n");
    printf("?                -  display this help menu          \n");
    printf("quit             -  exit the program                \n\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : cycle                                           */
/*                                                             */
/* Purpose   : Execute a cycle                                 */
/*                                                             */
/***************************************************************/
void cycle() {

  eval_micro_sequencer();
  cycle_memory();
  eval_bus_drivers();
  drive_bus();
  latch_datapath_values();

  CURRENT_LATCHES = NEXT_LATCHES;

  CYCLE_COUNT++;
}

/***************************************************************/
/*                                                             */
/* Procedure : run n                                           */
/*                                                             */
/* Purpose   : Simulate the LC-3b for n cycles.                 */
/*                                                             */
/***************************************************************/
void run(int num_cycles) {
    int i;

    if (RUN_BIT == FALSE) {
	printf("Can't simulate, Simulator is halted\n\n");
	return;
    }

    printf("Simulating for %d cycles...\n\n", num_cycles);
    for (i = 0; i < num_cycles; i++) {
	if (CURRENT_LATCHES.PC == 0x0000) {
	    RUN_BIT = FALSE;
	    printf("Simulator halted\n\n");
	    break;
	}
	cycle();
    }
}

/***************************************************************/
/*                                                             */
/* Procedure : go                                              */
/*                                                             */
/* Purpose   : Simulate the LC-3b until HALTed.                 */
/*                                                             */
/***************************************************************/
void go() {
    if (RUN_BIT == FALSE) {
	printf("Can't simulate, Simulator is halted\n\n");
	return;
    }

    printf("Simulating...\n\n");
    while (CURRENT_LATCHES.PC != 0x0000)
	cycle();
    RUN_BIT = FALSE;
    printf("Simulator halted\n\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : mdump                                           */
/*                                                             */
/* Purpose   : Dump a word-aligned region of memory to the     */
/*             output file.                                    */
/*                                                             */
/***************************************************************/
void mdump(FILE * dumpsim_file, int start, int stop) {
    int address; /* this is a byte address */

    printf("\nMemory content [0x%04x..0x%04x] :\n", start, stop);
    printf("-------------------------------------\n");
    for (address = (start >> 1); address <= (stop >> 1); address++)
	printf("  0x%04x (%d) : 0x%02x%02x\n", address << 1, address << 1, MEMORY[address][1], MEMORY[address][0]);
    printf("\n");

    /* dump the memory contents into the dumpsim file */
    fprintf(dumpsim_file, "\nMemory content [0x%04x..0x%04x] :\n", start, stop);
    fprintf(dumpsim_file, "-------------------------------------\n");
    for (address = (start >> 1); address <= (stop >> 1); address++)
	fprintf(dumpsim_file, " 0x%04x (%d) : 0x%02x%02x\n", address << 1, address << 1, MEMORY[address][1], MEMORY[address][0]);
    fprintf(dumpsim_file, "\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : rdump                                           */
/*                                                             */
/* Purpose   : Dump current register and bus values to the     */
/*             output file.                                    */
/*                                                             */
/***************************************************************/
void rdump(FILE * dumpsim_file) {
    int k;

    printf("\nCurrent register/bus values :\n");
    printf("-------------------------------------\n");
    printf("Cycle Count  : %d\n", CYCLE_COUNT);
    printf("PC           : 0x%04x\n", CURRENT_LATCHES.PC);
    printf("IR           : 0x%04x\n", CURRENT_LATCHES.IR);
    printf("STATE_NUMBER : 0x%04x\n\n", CURRENT_LATCHES.STATE_NUMBER);
    printf("BUS          : 0x%04x\n", BUS);
    printf("MDR          : 0x%04x\n", CURRENT_LATCHES.MDR);
    printf("MAR          : 0x%04x\n", CURRENT_LATCHES.MAR);
    printf("CCs: N = %d  Z = %d  P = %d\n", CURRENT_LATCHES.N, CURRENT_LATCHES.Z, CURRENT_LATCHES.P);
    printf("Registers:\n");
    for (k = 0; k < LC_3b_REGS; k++)
	printf("%d: 0x%04x\n", k, CURRENT_LATCHES.REGS[k]);
    printf("\n");

    /* dump the state information into the dumpsim file */
    fprintf(dumpsim_file, "\nCurrent register/bus values :\n");
    fprintf(dumpsim_file, "-------------------------------------\n");
    fprintf(dumpsim_file, "Cycle Count  : %d\n", CYCLE_COUNT);
    fprintf(dumpsim_file, "PC           : 0x%04x\n", CURRENT_LATCHES.PC);
    fprintf(dumpsim_file, "IR           : 0x%04x\n", CURRENT_LATCHES.IR);
    fprintf(dumpsim_file, "STATE_NUMBER : 0x%04x\n\n", CURRENT_LATCHES.STATE_NUMBER);
    fprintf(dumpsim_file, "BUS          : 0x%04x\n", BUS);
    fprintf(dumpsim_file, "MDR          : 0x%04x\n", CURRENT_LATCHES.MDR);
    fprintf(dumpsim_file, "MAR          : 0x%04x\n", CURRENT_LATCHES.MAR);
    fprintf(dumpsim_file, "CCs: N = %d  Z = %d  P = %d\n", CURRENT_LATCHES.N, CURRENT_LATCHES.Z, CURRENT_LATCHES.P);
    fprintf(dumpsim_file, "Registers:\n");
    for (k = 0; k < LC_3b_REGS; k++)
	fprintf(dumpsim_file, "%d: 0x%04x\n", k, CURRENT_LATCHES.REGS[k]);
    fprintf(dumpsim_file, "\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : get_command                                     */
/*                                                             */
/* Purpose   : Read a command from standard input.             */
/*                                                             */
/***************************************************************/
void get_command(FILE * dumpsim_file) {
    char buffer[20];
    int start, stop, cycles;

    printf("LC-3b-SIM> ");

    scanf("%s", buffer);
    printf("\n");

    switch(buffer[0]) {
    case 'G':
    case 'g':
	go();
	break;

    case 'M':
    case 'm':
	scanf("%i %i", &start, &stop);
	mdump(dumpsim_file, start, stop);
	break;

    case '?':
	help();
	break;
    case 'Q':
    case 'q':
	printf("Bye.\n");
	exit(0);

    case 'R':
    case 'r':
	if (buffer[1] == 'd' || buffer[1] == 'D')
	    rdump(dumpsim_file);
	else {
	    scanf("%d", &cycles);
	    run(cycles);
	}
	break;

    default:
	printf("Invalid Command\n");
	break;
    }
}

/***************************************************************/
/*                                                             */
/* Procedure : init_control_store                              */
/*                                                             */
/* Purpose   : Load microprogram into control store ROM        */
/*                                                             */
/***************************************************************/
void init_control_store(char *ucode_filename) {
    FILE *ucode;
    int i, j, index;
    char line[200];

    printf("Loading Control Store from file: %s\n", ucode_filename);

    /* Open the micro-code file. */
    if ((ucode = fopen(ucode_filename, "r")) == NULL) {
	printf("Error: Can't open micro-code file %s\n", ucode_filename);
	exit(-1);
    }

    /* Read a line for each row in the control store. */
    for(i = 0; i < CONTROL_STORE_ROWS; i++) {
	if (fscanf(ucode, "%[^\n]\n", line) == EOF) {
	    printf("Error: Too few lines (%d) in micro-code file: %s\n",
		   i, ucode_filename);
	    exit(-1);
	}

	/* Put in bits one at a time. */
	index = 0;

	for (j = 0; j < CONTROL_STORE_BITS; j++) {
	    /* Needs to find enough bits in line. */
	    if (line[index] == '\0') {
		printf("Error: Too few control bits in micro-code file: %s\nLine: %d\n",
		       ucode_filename, i);
		exit(-1);
	    }
	    if (line[index] != '0' && line[index] != '1') {
		printf("Error: Unknown value in micro-code file: %s\nLine: %d, Bit: %d\n",
		       ucode_filename, i, j);
		exit(-1);
	    }

	    /* Set the bit in the Control Store. */
	    CONTROL_STORE[i][j] = (line[index] == '0') ? 0:1;
	    index++;
	}

	/* Warn about extra bits in line. */
	if (line[index] != '\0')
	    printf("Warning: Extra bit(s) in control store file %s. Line: %d\n",
		   ucode_filename, i);
    }
    printf("\n");
}

/************************************************************/
/*                                                          */
/* Procedure : init_memory                                  */
/*                                                          */
/* Purpose   : Zero out the memory array                    */
/*                                                          */
/************************************************************/
void init_memory() {
    int i;

    for (i=0; i < WORDS_IN_MEM; i++) {
	MEMORY[i][0] = 0;
	MEMORY[i][1] = 0;
    }
}

/**************************************************************/
/*                                                            */
/* Procedure : load_program                                   */
/*                                                            */
/* Purpose   : Load program and service routines into mem.    */
/*                                                            */
/**************************************************************/
void load_program(char *program_filename) {
    FILE * prog;
    int ii, word, program_base;

    /* Open program file. */
    prog = fopen(program_filename, "r");
    if (prog == NULL) {
	printf("Error: Can't open program file %s\n", program_filename);
	exit(-1);
    }

    /* Read in the program. */
    if (fscanf(prog, "%x\n", &word) != EOF)
	program_base = word >> 1;
    else {
	printf("Error: Program file is empty\n");
	exit(-1);
    }

    ii = 0;
    while (fscanf(prog, "%x\n", &word) != EOF) {
	/* Make sure it fits. */
	if (program_base + ii >= WORDS_IN_MEM) {
	    printf("Error: Program file %s is too long to fit in memory. %x\n",
		   program_filename, ii);
	    exit(-1);
	}

	/* Write the word to memory array. */
	MEMORY[program_base + ii][0] = word & 0x00FF;
	MEMORY[program_base + ii][1] = (word >> 8) & 0x00FF;
	ii++;
    }

    if (CURRENT_LATCHES.PC == 0) CURRENT_LATCHES.PC = (program_base << 1);

    printf("Read %d words from program into memory.\n\n", ii);
}

/***************************************************************/
/*                                                             */
/* Procedure : initialize                                      */
/*                                                             */
/* Purpose   : Load microprogram and machine language program  */
/*             and set up initial state of the machine.        */
/*                                                             */
/***************************************************************/
void initialize(char *ucode_filename, char *program_filename, int num_prog_files) {
    int i;
    init_control_store(ucode_filename);

    init_memory();
    for ( i = 0; i < num_prog_files; i++ ) {
	load_program(program_filename);
	while(*program_filename++ != '\0');
    }
    CURRENT_LATCHES.Z = 1;
    CURRENT_LATCHES.STATE_NUMBER = INITIAL_STATE_NUMBER;
    memcpy(CURRENT_LATCHES.MICROINSTRUCTION, CONTROL_STORE[INITIAL_STATE_NUMBER], sizeof(int)*CONTROL_STORE_BITS);

    NEXT_LATCHES = CURRENT_LATCHES;

    RUN_BIT = TRUE;
}

/***************************************************************/
/*                                                             */
/* Procedure : main                                            */
/*                                                             */
/***************************************************************/
int main(int argc, char *argv[]) {
    FILE * dumpsim_file;

    /* Error Checking */
    if (argc < 3) {
	printf("Error: usage: %s <micro_code_file> <program_file_1> <program_file_2> ...\n",
	       argv[0]);
	exit(1);
    }

    printf("LC-3b Simulator\n\n");

    initialize(argv[1], argv[2], argc - 2);

    if ( (dumpsim_file = fopen( "dumpsim", "w" )) == NULL ) {
	printf("Error: Can't open dumpsim file\n");
	exit(-1);
    }


    while (1)
	get_command(dumpsim_file);

}

/***************************************************************/
/* --------- DO NOT MODIFY THE CODE ABOVE THIS LINE -----------*/
/***************************************************************/

/***************************************************************/
/* You are allowed to use the following global variables in your
   code. These are defined above.

   CONTROL_STORE
   MEMORY
   BUS

   CURRENT_LATCHES
   NEXT_LATCHES

   You may define your own local/global variables and functions.
   You may use the functions to get at the control bits defined
   above.

   Begin your code here 	  			       */
/***************************************************************/

int mem_cycle_count;
int mem_result;
int PC_result;
int MARMUX_result;
int ALU_result;
int SHF_result;
int MDR_result;
int ADDR2_result;
int ADDR1_result;


int LSHF(int value, int amount){
  return (value << amount) & 0xFFFF;
}

int RSHF(int value, int amount, int topbit ){
  int mask;
  mask = 1 << amount;
  mask -= 1;
  mask = mask << ( 16 -amount );

  return ((value >> amount) & ~mask) | ((topbit)?(mask):0); /* TBD */
}

int SEXT(int value, int topbit){
  int shift = sizeof(int)*8 - topbit;
  return (value << shift )>> shift;
}

int ZEXT(int value, int topbit){
  return value << (32 - topbit);
}

int get_bits(int value, int start, int end){
  int result;

  result = value >> end;
  result = result % ( 1 << ( start - end + 1 ) );
  return result;
}

int read_word(int addr){

  return MEMORY[addr>>1][1]<<8 | MEMORY[addr>>1][0];
}

int read_byte(int addr){
  int bank=addr&1;

  return MEMORY[addr>>1][bank];
}

void write_byte(int addr, int value){
  int bank=addr&1;

  MEMORY[addr>>1][bank]= value & 0xFF;
}

void write_word(int addr, int value){

  MEMORY[addr>>1][1] = (value & 0x0000FF00) >> 8;
  MEMORY[addr>>1][0] = value & 0xFF;
}


void eval_micro_sequencer() {

  /*
   * Evaluate the address of the next state according to the
   * micro sequencer logic. Latch the next microinstruction.
   */

   int * micro_instruction = CURRENT_LATCHES.MICROINSTRUCTION;
   int IR11 = get_bits(CURRENT_LATCHES.IR,11,11);
   int next_state= GetJ(micro_instruction);


   if(GetIRD(micro_instruction))
   {
     next_state = get_bits(CURRENT_LATCHES.IR,15,12);
   }
   else
   {
     if(GetCOND(micro_instruction)==0)
      next_state=next_state;

     if((GetCOND(micro_instruction) == 1) && CURRENT_LATCHES.READY)
      next_state = next_state + 2;

     if((GetCOND(micro_instruction) == 2) && CURRENT_LATCHES.BEN)
      next_state = next_state + 4;

     if((GetCOND(micro_instruction) == 3) && IR11)
      next_state = next_state + 1;
   }


   NEXT_LATCHES.STATE_NUMBER=next_state;
   int i;
   for( i = 0; i<35; i++)
	    {
        NEXT_LATCHES.MICROINSTRUCTION[i] = CONTROL_STORE[next_state][i];
      }

}


void cycle_memory() {

  /*
   * This function emulates memory and the WE logic.
   * Keep track of which cycle of MEMEN we are dealing with.
   * If fourth, we need to latch Ready bit at the end of
   * cycle to prepare microsequencer for the fifth cycle.
   */
  int * micro_instruction = CURRENT_LATCHES.MICROINSTRUCTION;
  if(GetMIO_EN(micro_instruction)){
    mem_cycle_count++;

    if(mem_cycle_count == 4){
     NEXT_LATCHES.READY = 1;
    }

    if(mem_cycle_count == 5){
      if(!GetR_W(micro_instruction)){

        if(GetDATA_SIZE(micro_instruction) == 0)
          mem_result= SEXT(read_byte(CURRENT_LATCHES.MAR),8);
        else
          mem_result= read_word(CURRENT_LATCHES.MAR);
      }

      if(GetR_W(micro_instruction)){

        if(GetDATA_SIZE(micro_instruction) == 0)
          write_byte(CURRENT_LATCHES.MAR,CURRENT_LATCHES.MDR);
        else
          write_word(CURRENT_LATCHES.MAR,CURRENT_LATCHES.MDR);
      }

      NEXT_LATCHES.READY = 0;
      mem_cycle_count=0;
      }
  }

}


void eval_bus_drivers() {

  /*
   * Datapath routine emulating operations before driving the bus.
   * Evaluate the input of tristate drivers
   *     Gate_MARMUX,
   *		 Gate_PC,
   *		 Gate_ALU,
   *		 Gate_SHF,
   *		 Gate_MDR.
   */
  int * micro_instruction = CURRENT_LATCHES.MICROINSTRUCTION;


  PC_result = Low16bits(CURRENT_LATCHES.PC);



    switch(GetADDR2MUX(micro_instruction)){
    	case 0:
    	ADDR2_result = 0;break;

    	case 1:
    	ADDR2_result = SEXT(get_bits(CURRENT_LATCHES.IR,5,0),6);break;

    	case 2:
    	ADDR2_result = SEXT(get_bits(CURRENT_LATCHES.IR,8,0),9);break;

    	case 3:
    	ADDR2_result = SEXT(get_bits(CURRENT_LATCHES.IR,10,0),11);break;
    }

    if(GetLSHF1(micro_instruction))
  	  {ADDR2_result = Low16bits(LSHF(ADDR2_result,1));}


  	if(GetADDR1MUX(micro_instruction) == 0)
  		ADDR1_result = PC_result;
  	else
  		ADDR1_result = CURRENT_LATCHES.REGS[get_bits(CURRENT_LATCHES.IR,8,6)];

    if(GetMARMUX(micro_instruction) == 0)
      MARMUX_result = LSHF(ZEXT(get_bits(CURRENT_LATCHES.IR, 7,0),get_bits(CURRENT_LATCHES.IR,7,7)),1);
    else
  	 MARMUX_result = Low16bits(ADDR1_result+ADDR2_result);



  if(GetGATE_ALU(micro_instruction)){

      int SR1;
  	  int SR2;
      int A = get_bits(CURRENT_LATCHES.IR,5,5);

    	if(A)
    		SR2 = SEXT(get_bits(CURRENT_LATCHES.IR,4,0),5);
    	else
    		SR2 = CURRENT_LATCHES.REGS[get_bits(CURRENT_LATCHES.IR,2,0)];

    	if(GetSR1MUX(micro_instruction))
      	SR1 = CURRENT_LATCHES.REGS[get_bits(CURRENT_LATCHES.IR,8,6)];
    	else
    	  SR1 = CURRENT_LATCHES.REGS[get_bits(CURRENT_LATCHES.IR,11,9)];

    switch (GetALUK(micro_instruction)) {
     case 0: ALU_result = Low16bits(SR1 + SR2);break;

     case 1: ALU_result = Low16bits(SR1 & SR2);break;

     case 2: ALU_result = Low16bits(SR1 ^ SR2);break;

     case 3: ALU_result = Low16bits(SR1); break;
    }
  }

  if(GetGATE_SHF(micro_instruction)){

    int shift_type = get_bits(CURRENT_LATCHES.IR,5, 4);
    int amount4 = get_bits(CURRENT_LATCHES.IR,3,0);
    int SR = get_bits(CURRENT_LATCHES.IR,8,6);

    switch(shift_type)
    {
      case 0:
      SHF_result = Low16bits(LSHF(CURRENT_LATCHES.REGS[SR], amount4));
      break;

      case 1:
      SHF_result = Low16bits(RSHF(CURRENT_LATCHES.REGS[SR], amount4, 0));
      break;

      case 3:
      SHF_result= Low16bits(RSHF(CURRENT_LATCHES.REGS[SR], amount4, get_bits(CURRENT_LATCHES.REGS[SR],15,15)));
    }
  }

  if(GetGATE_MDR(micro_instruction))
  {

  	if(GetDATA_SIZE(micro_instruction) == 0)
  	   MDR_result = SEXT(get_bits(mem_result,7,0),8);
  	else
  	   MDR_result = Low16bits(mem_result);
  }

}


void drive_bus() {

  /*
   * Datapath routine for driving the bus from one of the 5 possible
   * tristate drivers.
   */
  int * micro_instruction = CURRENT_LATCHES.MICROINSTRUCTION;

  if(GetGATE_PC(micro_instruction))
  	BUS = Low16bits(PC_result);

   else if(GetGATE_MARMUX(micro_instruction))
    BUS = Low16bits(MARMUX_result);

  else if(GetGATE_MDR(micro_instruction))
  	BUS = Low16bits(MDR_result);

  else if(GetGATE_ALU(micro_instruction))
    BUS = Low16bits(ALU_result);

  else if(GetGATE_SHF(micro_instruction))
    BUS = Low16bits(SHF_result);
  else
    BUS=0;

}


void latch_datapath_values() {

  /*
   * Datapath routine for computing all functions that need to latch
   * values in the data path at the end of this cycle.  Some values
   * require sourcing the bus; therefore, this routine has to come
   * after drive_bus.
   */
   int * micro_instruction = CURRENT_LATCHES.MICROINSTRUCTION;

   if(GetLD_MAR(micro_instruction))
	  NEXT_LATCHES.MAR = BUS;

   if(GetLD_IR(micro_instruction))
    NEXT_LATCHES.IR = BUS;

   if(GetLD_REG(micro_instruction))
   {
		  if(GetDRMUX(micro_instruction) == 0)
		    NEXT_LATCHES.REGS[Low16bits(get_bits(CURRENT_LATCHES.IR,11,9))] = Low16bits(BUS);
		  if(GetDRMUX(micro_instruction) == 1)
		    NEXT_LATCHES.REGS[7] = Low16bits(BUS);
    }

   if(GetLD_BEN(micro_instruction))
   {
    NEXT_LATCHES.BEN = (get_bits(CURRENT_LATCHES.IR,11,11) && CURRENT_LATCHES.N)
                    || (get_bits(CURRENT_LATCHES.IR,10,10) && CURRENT_LATCHES.Z)
                    || (get_bits(CURRENT_LATCHES.IR,9,9)   && CURRENT_LATCHES.P);
	 }

   if(GetLD_CC(micro_instruction))
   {
     NEXT_LATCHES.N = 0;
     NEXT_LATCHES.Z = 0;
     NEXT_LATCHES.P = 0;

  	if(BUS == 0)
    	NEXT_LATCHES.Z = 1;
  	else if(get_bits(BUS,15,15)==0)
      NEXT_LATCHES.P = 1;
  	else
  	  NEXT_LATCHES.N = 1;

	   }

   if(GetLD_PC(micro_instruction))
   {
     if(GetPCMUX(micro_instruction) == 0)
    	NEXT_LATCHES.PC = Low16bits(CURRENT_LATCHES.PC+2);

     if(GetPCMUX(micro_instruction) == 1)
    	NEXT_LATCHES.PC = Low16bits(BUS);

     if(GetPCMUX(micro_instruction) == 2)
      NEXT_LATCHES.PC = Low16bits(ADDR1_result+ADDR2_result);

    }


   if(GetLD_MDR(micro_instruction))
   {
     if(GetMIO_EN(micro_instruction))
        {NEXT_LATCHES.MDR= Low16bits(mem_result);}
     else
     {

        NEXT_LATCHES.MDR=BUS;
     }

   }

}
