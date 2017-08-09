
/***************************************************************/
/*                                                             */
/* LC-3b ISA Simulator.  This is a modified version of the     */
/* LC-3b simulator used by Prof. Yale N. Patt at UT Austin     */
/*                                                             */
/***************************************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/***************************************************************/
/*                                                             */
/* Files: isaprogram   LC-3b machine language program file     */
/*                                                             */
/***************************************************************/

/***************************************************************/
/* These are the functions we have already written for you     */
/***************************************************************/

void process_instruction();
void fetch_instruction();
void decode_instruction(); /* only partially written */
void execute_instruction();

/***************************************************************/
/* These are the functions you'll have to write.               */
/***************************************************************/

void execute_BR();
void execute_ADD();
void execute_LDB();
void execute_STB();
void execute_JSR();
void execute_AND();
void execute_LDW();
void execute_STW();
void execute_XOR();
void execute_JMP();
void execute_SHF();
void execute_LEA();
void execute_TRAP();    /* already written as an example */
void execute_unknown(); /* already written */


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
/* Main memory.                                                */
/***************************************************************/
/* MEMORY[A][0] stores the least significant byte of word at word address A
   MEMORY[A][1] stores the most significant byte of word at word address A
*/

#define WORDS_IN_MEM    0x08000
int MEMORY[WORDS_IN_MEM][2];

/***************************************************************/

/***************************************************************/

/***************************************************************/
/* LC-3b State info.                                           */
/***************************************************************/
#define LC_3b_REGS 8

int RUN_BIT;	/* run bit */

/* Data structure for Latch */

typedef struct System_Latches_Struct{

  int PC,	/* program counter */
    N,		/* n condition bit */
    Z,		/* z condition bit */
    P;		/* p condition bit */
  int REGS[LC_3b_REGS]; /* register file. */
} System_Latches;

System_Latches CURRENT_LATCHES, NEXT_LATCHES;


/* Data structure for encapsulating decoded information, not part of ISA */

typedef struct Inst_Info{
  int IR;  /* Instruction Register */
  int OPCODE;
  int DR, SR1, A, imm5, SR2, n, z, p, PCoffset9, BaseR, boffset6, offset6, D, amount4, trapvect8, B, PCoffset11;
} Inst_Info;

Inst_Info INST;


/***************************************************************/
/* A cycle counter.                                            */
/***************************************************************/
int INSTRUCTION_COUNT;

/***************************************************************/
/*                                                             */
/* Procedure : help                                            */
/*                                                             */
/* Purpose   : Print out a list of commands                    */
/*                                                             */
/***************************************************************/
void help() {
  printf("----------------LC-3b ISIM Help-----------------------\n");
  printf("go               -  run program to completion         \n");
  printf("run n            -  execute program for n instructions\n");
  printf("mdump low high   -  dump memory from low to high      \n");
  printf("rdump            -  dump the register & bus values    \n");
  printf("?                -  display this help menu            \n");
  printf("quit             -  exit the program                  \n\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : cycle                                           */
/*                                                             */
/* Purpose   : Execute a cycle                                 */
/*                                                             */
/***************************************************************/
void cycle() {

  process_instruction();
  CURRENT_LATCHES = NEXT_LATCHES;
  INSTRUCTION_COUNT++;
}

/***************************************************************/
/*                                                             */
/* Procedure : run n                                           */
/*                                                             */
/* Purpose   : Simulate the LC-3b for n cycles                 */
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
/* Purpose   : Simulate the LC-3b until HALTed                 */
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

  printf("\nMemory content [0x%0.4x..0x%0.4x] :\n", start, stop);
  printf("-------------------------------------\n");
  for (address = (start >> 1); address <= (stop >> 1); address++)
    printf("  0x%0.4x (%d) : 0x%0.2x%0.2x\n", address << 1, address << 1, MEMORY[address][1], MEMORY[address][0]);
  printf("\n");

  /* dump the memory contents into the dumpsim file */
  fprintf(dumpsim_file, "\nMemory content [0x%0.4x..0x%0.4x] :\n", start, stop);
  fprintf(dumpsim_file, "-------------------------------------\n");
  for (address = (start >> 1); address <= (stop >> 1); address++)
    fprintf(dumpsim_file, " 0x%0.4x (%d) : 0x%0.2x%0.2x\n", address << 1, address << 1, MEMORY[address][1], MEMORY[address][0]);
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
  printf("Instruction Count : %d\n", INSTRUCTION_COUNT);
  printf("PC                : 0x%0.4x\n", CURRENT_LATCHES.PC);
  printf("CCs: N = %d  Z = %d  P = %d\n", CURRENT_LATCHES.N, CURRENT_LATCHES.Z, CURRENT_LATCHES.P);
  printf("Registers:\n");
  for (k = 0; k < LC_3b_REGS; k++)
    printf("%d: 0x%0.4x\n", k, CURRENT_LATCHES.REGS[k]);
  printf("\n");

  /* dump the state information into the dumpsim file */
  fprintf(dumpsim_file, "\nCurrent register/bus values :\n");
  fprintf(dumpsim_file, "-------------------------------------\n");
  fprintf(dumpsim_file, "Instruction Count : %d\n", INSTRUCTION_COUNT);
  fprintf(dumpsim_file, "PC                : 0x%0.4x\n", CURRENT_LATCHES.PC);
  fprintf(dumpsim_file, "CCs: N = %d  Z = %d  P = %d\n", CURRENT_LATCHES.N, CURRENT_LATCHES.Z, CURRENT_LATCHES.P);
  fprintf(dumpsim_file, "Registers:\n");
  for (k = 0; k < LC_3b_REGS; k++)
    fprintf(dumpsim_file, "%d: 0x%0.4x\n", k, CURRENT_LATCHES.REGS[k]);
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
/* Procedure : init_memory                                     */
/*                                                             */
/* Purpose   : Zero out the memory array                       */
/*                                                             */
/***************************************************************/
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

/************************************************************/
/*                                                          */
/* Procedure : initialize                                   */
/*                                                          */
/* Purpose   : Load machine language program                */
/*             and set up initial state of the machine.     */
/*                                                          */
/************************************************************/
void initialize(char *program_filename, int num_prog_files) {
  int i;

  init_memory();
  for ( i = 0; i < num_prog_files; i++ ) {
    load_program(program_filename);
    while(*program_filename++ != '\0');
  }

  CURRENT_LATCHES.Z = 1;

  /***************************************************************/
  /** Some test cases rely on predefined register state **********/
  /** Our test scripts will automatically do INIT_REGS if needed */
  /***************************************************************/
#ifdef INIT_REGS
  CURRENT_LATCHES.REGS[0] = 0;
  CURRENT_LATCHES.REGS[1] = 1;
  CURRENT_LATCHES.REGS[2] = 2;
  CURRENT_LATCHES.REGS[3] = 3;
  CURRENT_LATCHES.REGS[4] = 4;
  CURRENT_LATCHES.REGS[5] = 5;
  CURRENT_LATCHES.REGS[6] = 0x1236;
  CURRENT_LATCHES.REGS[7] = 0xABCE;
#endif

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
  if (argc < 2) {
    printf("Error: usage: %s <program_file_1> <program_file_2> ...\n",
           argv[0]);
    exit(1);
  }

  printf("LC-3b Simulator\n\n");

  initialize(argv[1], argc - 1);

  if ( (dumpsim_file = fopen( "dumpsim", "w" )) == NULL ) {
    printf("Error: Can't open dumpsim file\n");
    exit(-1);
  }

  while (1)
    get_command(dumpsim_file);

}


/***********************************************************/
/* Each instruction goes through fetch, decode and execute */
/***********************************************************/

void process_instruction(){
  fetch_instruction();
  decode_instruction();
  execute_instruction();
}

/*********************************************/
/* Fetch current instruction, update next PC */
/*********************************************/

void fetch_instruction(){
  int pc = CURRENT_LATCHES.PC;
  assert( (pc & 0xFFFF0000) == 0 );
  INST.IR   = (MEMORY[pc>>1][1]<<8) | (MEMORY[pc>>1][0]);
  printf("IR = 0x%0.4X \n", INST.IR);
  NEXT_LATCHES.PC = pc+2;
}


/***************************************************/
/* enum makes code easier to read. Enum starts at 0 */
/***************************************************/

enum OPCODES{
  BR,
  ADD,
  LDB,
  STB,
  JSR,
  AND,
  LDW,
  STW,
  RTI,
  XOR,
  Unknown1,
  Unknown2,
  JMP,
  SHF,
  LEA,
  TRAP
};


/************************************************/
/* Execute the instruction, based on the opcode */
/************************************************/

void execute_instruction(){

  switch ( INST.OPCODE ){

  case BR:
    execute_BR(); break;

  case ADD:
    execute_ADD(); break;

  case LDB:
    execute_LDB(); break;

  case STB:
    execute_STB(); break;

  case JSR:
    execute_JSR(); break;

  case AND:
    execute_AND(); break;

  case LDW:
    execute_LDW(); break;

  case STW:
    execute_STW(); break;

  case RTI: /* not supporting RTI right now */
    execute_unknown(); break;

  case XOR:
    execute_XOR(); break;

  case Unknown1:
    execute_unknown(); break;

  case Unknown2:
    execute_unknown(); break;

  case JMP:
    execute_JMP(); break;

  case SHF:
    execute_SHF(); break;

  case LEA:
    execute_LEA(); break;

  case TRAP:
    execute_TRAP(); break;

  default:
    execute_unknown();

  }

}


/***************************************************************/
/* The following functions are already defined to make your job easier:
   - get_bits
   - read word/byte and write word/byte
   - set condition code
   - Left shift (LSHF) and Right shift (RSHF)
   - Zero Extension (ZEXT) and Sign Extension (SEXT)

*/
/***************************************************************/


int get_bits(int value, int start, int end){
  int result;
  assert(start >= end );
  result = value >> end;
  result = result % ( 1 << ( start - end + 1 ) );
  return result;
}


int read_word(int addr){
  assert( (addr & 0xFFFF0000) == 0 );
  return MEMORY[addr>>1][1]<<8 | MEMORY[addr>>1][0];
}

int read_byte(int addr){
  int bank=addr&1;
  assert( (addr & 0xFFFF0000) == 0 );
  return MEMORY[addr>>1][bank];
}

void write_byte(int addr, int value){
  int bank=addr&1;
  assert( (addr & 0xFFFF0000) == 0 );
  MEMORY[addr>>1][bank]= value & 0xFF;
}

void write_word(int addr, int value){
  assert( (addr & 0xFFFF0000) == 0 );
  MEMORY[addr>>1][1] = (value & 0x0000FF00) >> 8;
  MEMORY[addr>>1][0] = value & 0xFF;
}


void setcc(int value){
  NEXT_LATCHES.N=0;
  NEXT_LATCHES.Z=0;
  NEXT_LATCHES.P=0;
  if ( value == 0 )      NEXT_LATCHES.Z=1;
  else if ( value & 0x8000 )  NEXT_LATCHES.N=1;
  else                   NEXT_LATCHES.P=1;
}

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

/***************************************************************/
/* ------- DO NOT MODIFY THE CODE ABOVE THIS LINE-------------*/
/***************************************************************/
/*  You are allowed to use the following global variables in your
   code. These are defined above.

   MEMORY
   CURRENT_LATCHES
   NEXT_LATCHES
   INST (Decoded information to carry with each instruction)

   You may define your own local/global variables and functions.

   YOUR JOB: to complete decode_instruction() function and
    to implement the execute_OP() functions for each OPCODE.
*/
/***************************************************************/



/************************************************************/
/* Decode the instruction, and set opcode and other fields  */
/************************************************************/
void decode_instruction(){

  /* Setting of Opcode is already done for you as an example */
  INST.OPCODE     = get_bits( INST.IR, 15, 12);


  INST.DR              = get_bits( INST.IR,11,9);
  INST.SR1             = get_bits( INST.IR,8,6);
  INST.A               = get_bits( INST.IR,5,5);
  INST.imm5            = get_bits( INST.IR,4,0);
  INST.SR2             = get_bits( INST.IR,2,0);
  INST.n               = get_bits( INST.IR,11,11);
  INST.z               = get_bits( INST.IR,10,10);
  INST.p               = get_bits( INST.IR,9,9);
  INST.PCoffset9       = get_bits( INST.IR,8,0);
  INST.BaseR           = get_bits( INST.IR,8,6);
  INST.boffset6        = get_bits( INST.IR,5,0);
  INST.offset6         = get_bits( INST.IR,5,0);
  INST.D               = get_bits( INST.IR,4,4);
  INST.amount4         = get_bits( INST.IR,3,0);
  INST.trapvect8       = get_bits( INST.IR,0 ,0);
  INST.B               = get_bits( INST.IR, 11, 11);
  INST.PCoffset11      = get_bits( INST.IR,10,0);


}

/************************************************************/
/* Write the execute function for each instruction          */
/* The function for TRAP is already filled in as an example */
/************************************************************/


 void execute_unknown(){
   printf("Unknown opcode %d \n", INST.OPCODE);
   assert(0);
 }


 void execute_TRAP(){
   NEXT_LATCHES.REGS[7] = NEXT_LATCHES.PC;
   int trap_addr = LSHF(ZEXT(INST.trapvect8, 8),1);
   NEXT_LATCHES.PC = read_word(Low16bits(trap_addr));
 }


 void execute_ADD(){
   if(INST.A == 0)
    NEXT_LATCHES.REGS[INST.DR]=Low16bits(CURRENT_LATCHES.REGS[INST.SR1] + CURRENT_LATCHES.REGS[INST.SR2]);
   else
    NEXT_LATCHES.REGS[INST.DR]= Low16bits(CURRENT_LATCHES.REGS[INST.SR1] + SEXT(INST.imm5,5));
  setcc(NEXT_LATCHES.REGS[INST.DR]);
 }


 void execute_BR(){
   if ((INST.n & CURRENT_LATCHES.N) || (INST.z & CURRENT_LATCHES.Z) || (INST.p & CURRENT_LATCHES.P)) {
     NEXT_LATCHES.PC=(NEXT_LATCHES.PC) + LSHF(SEXT(INST.PCoffset9,9), 1);
   }

 }

 void execute_LDB(){
   int addr=CURRENT_LATCHES.REGS[INST.BaseR] + SEXT(INST.boffset6,6);
   NEXT_LATCHES.REGS[INST.DR] = Low16bits(SEXT(read_byte(addr),8));
   setcc(NEXT_LATCHES.REGS[INST.DR]);
 }


 void execute_STB(){
   int addr=CURRENT_LATCHES.REGS[INST.BaseR]+ SEXT(INST.boffset6,6);
   int value=CURRENT_LATCHES.REGS[INST.DR];
   write_byte(addr,value);
 }


 void execute_JSR(){
    int TEMP = NEXT_LATCHES.PC;
    if (INST.B==0)
        NEXT_LATCHES.PC = CURRENT_LATCHES.REGS[INST.BaseR];
    else
        NEXT_LATCHES.PC = NEXT_LATCHES.PC + LSHF(SEXT(INST.PCoffset11,11), 1);
    NEXT_LATCHES.REGS[7] =  Low16bits(TEMP);
 }


 void execute_AND(){
   if(INST.A == 0)
    NEXT_LATCHES.REGS[INST.DR]=Low16bits(CURRENT_LATCHES.REGS[INST.SR1]&CURRENT_LATCHES.REGS[INST.SR2]);
   else
    NEXT_LATCHES.REGS[INST.DR]= Low16bits(CURRENT_LATCHES.REGS[INST.SR1]&  SEXT(INST.imm5,5));
  setcc(NEXT_LATCHES.REGS[INST.DR]);
 }


 void execute_LDW(){
   int addr1= CURRENT_LATCHES.REGS[INST.BaseR];
   int addr2= LSHF(SEXT(INST.offset6,6), 1);
   NEXT_LATCHES.REGS[INST.DR] = Low16bits(read_word(Low16bits(addr1+addr2)));
   setcc(NEXT_LATCHES.REGS[INST.DR]);
 }


 void execute_STW(){
   int addr=CURRENT_LATCHES.REGS[INST.BaseR] + LSHF(SEXT(INST.offset6,6), 1);
   int value=CURRENT_LATCHES.REGS[INST.DR];

   write_word(Low16bits(addr),value);
 }


 void execute_XOR(){
   if (INST.A == 0)
    NEXT_LATCHES.REGS[INST.DR]= Low16bits(CURRENT_LATCHES.REGS[INST.SR1] ^ CURRENT_LATCHES.REGS[INST.SR2]);
   else
    NEXT_LATCHES.REGS[INST.DR] = Low16bits(CURRENT_LATCHES.REGS[INST.SR1] ^ SEXT(INST.imm5,5));
   setcc(NEXT_LATCHES.REGS[INST.DR]);
 }


 void execute_JMP(){
   NEXT_LATCHES.PC=CURRENT_LATCHES.REGS[INST.BaseR];
 }


 void execute_SHF(){
   if (INST.D == 0)
   {
     NEXT_LATCHES.REGS[INST.DR] = Low16bits(LSHF(CURRENT_LATCHES.REGS[INST.SR1], INST.amount4));
   }
   else
   {
     if (INST.A == 0)
     {NEXT_LATCHES.REGS[INST.DR] = Low16bits(RSHF(CURRENT_LATCHES.REGS[INST.SR1], INST.amount4, 0));}
     else
     {NEXT_LATCHES.REGS[INST.DR] = Low16bits(RSHF(CURRENT_LATCHES.REGS[INST.SR1], INST.amount4, get_bits(CURRENT_LATCHES.REGS[INST.SR1],15,15)));}
   }
   setcc(NEXT_LATCHES.REGS[INST.DR]);
 }


 void execute_LEA(){
   NEXT_LATCHES.REGS[INST.DR] =  Low16bits(NEXT_LATCHES.PC + LSHF(SEXT(INST.PCoffset9,9),1));
 }
