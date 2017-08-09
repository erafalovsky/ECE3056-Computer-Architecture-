 /*************************************************************************
 * File         : sim.c
 * Author       : Moinuddin K. Qureshi 
 * Date         : 18th October 2014
 * Description  : Memory system simulator for Lab 4 of ECE3056
 *************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>

#include "types.h"
#include "memsys.h"

#define PRINT_DOTS   1
#define DOT_INTERVAL 100000

/***************************************************************************
 * Globals 
 **************************************************************************/

MODE        SIM_MODE        = SIM_MODE_A;
uns64       CACHE_LINESIZE  = 64;
uns64       REPL_POLICY     = 0; // 0:LRU 1:RAND

uns64       DCACHE_SIZE     = 32*1024; 
uns64       DCACHE_ASSOC    = 8; 

uns64       ICACHE_SIZE     = 32*1024; 
uns64       ICACHE_ASSOC    = 8; 

uns64       L2CACHE_SIZE    = 512*1024; 
uns64       L2CACHE_ASSOC   = 16; 


/***************************************************************************************
 * Functions
 ***************************************************************************************/
void print_dots(void);
void die_usage();
void die_message(const char * msg);
void get_params(int argc, char** argv);
void print_stats();

/***************************************************************************************
 * Globals
 ***************************************************************************************/
FILE        *trfile;
Memsys      *memsys; 
uns64       cycle_count;
uns64       inst_count; 
uns64       last_printdot_inst;


/***************************************************************************************
 * Main
 ***************************************************************************************/
int main(int argc, char** argv)
{
    Flag tmp, done=0;

    srand(42);
    get_params(argc, argv);
    memsys = memsys_new();
    print_dots();

    //--------------------------------------------------------------------
    // -- Iterate through the traces until done
    //--------------------------------------------------------------------

    while( !done ){
      Addr inst_addr=0, ldst_addr=0;
      Inst_Type inst_type=0; 
      uns ifetch_delay=0, ld_delay=0, st_delay=0;

      //------ read the trace record for each instruction ----------------      

      tmp = fread (&inst_addr, 4, 1, trfile);
      tmp = fread (&inst_type, 1, 1, trfile);
      tmp = fread (&ldst_addr, 4, 1, trfile);

      if(feof(trfile)){
	done=TRUE;
	break;
      }

      //------ access the memory system ----------------------------------

      ifetch_delay = memsys_access(memsys, inst_addr, ACCESS_TYPE_IFETCH);

      if(inst_type==INST_TYPE_LOAD){
	ld_delay = memsys_access(memsys, ldst_addr, ACCESS_TYPE_LOAD);
      }

      if(inst_type==INST_TYPE_STORE){
	st_delay = memsys_access(memsys, ldst_addr, ACCESS_TYPE_STORE);
      }
     

      //------ update the stats  ------------------------------------------

      inst_count++;
      cycle_count++; //assume 1 IPC for perfect pipeline

      if(ifetch_delay>1){
	cycle_count += (ifetch_delay-1);
      }

      if(ld_delay>1){
	cycle_count += (ld_delay-1);
      }

      if(st_delay>1){
	// with store buffers, store misses do not stall the pipeline
      }


      //------ check for heartbeat -------------------------
      if (inst_count - last_printdot_inst >= DOT_INTERVAL){
	    print_dots();
      }
      
    }

    print_stats();
    return 0;

}

//--------------------------------------------------------------------
// -- Print statistics
//--------------------------------------------------------------------

void print_stats(){
    printf("\n");
    printf("\nINST        \t\t\t : %10llu", inst_count);
    printf("\nCYCLES      \t\t\t : %10llu", cycle_count);
    printf("\nCPI         \t\t\t : %10.3f", (double)cycle_count/(double)inst_count);

    memsys_print_stats(memsys);

    printf("\n\n");
}

//--------------------------------------------------------------------
// -- Print Hearbeats 
//--------------------------------------------------------------------


void print_dots(){
  uns LINE_INTERVAL = 50 *  DOT_INTERVAL;

  last_printdot_inst = inst_count;

  if(!PRINT_DOTS){
      return;
  }

  if (inst_count % LINE_INTERVAL ==0){
	printf("\n%4llu M\t", inst_count/1000000);
	fflush(stdout);
    }
    else{
	printf(".");
	fflush(stdout);
    }
    
}


//--------------------------------------------------------------------
// -- Usage Menu
//--------------------------------------------------------------------

void die_usage() {
    printf("Usage : sim [-option <value>] trace_file \n");
    printf("   Options\n");
    printf("      -mode            <num>    Set mode of the simulator[1:PartA, 2:PartB, 3:PartC]  (Default: 1)\n");
    printf("      -linesize        <num>    Set cache linesize for all caches (Default:64)\n");
    printf("      -repl            <num>    Set replacement policy for all caches [0:LRU,1:RND] (Default:0)\n");
    printf("      -DsizeKB         <num>    Set capacity in KB of the the Level 1 DCACHE (Default:32 KB)\n");
    printf("      -Dassoc          <num>    Set associativity of the the Level 1 DCACHE (Default:8)\n");
    printf("      -L2sizeKB        <num>    Set capacity in KB of the unified Level 2 cache (Default: 512 KB)\n");

    exit(0);
}

//--------------------------------------------------------------------
// -- Print Error Message and Die
//--------------------------------------------------------------------

												          
void die_message(const char * msg) {
    printf("Error! %s. Exiting...\n", msg);
    exit(1);
}

//--------------------------------------------------------------------
// -- Read Parameters from Command Line
//--------------------------------------------------------------------

void get_params(int argc, char** argv){
  char  trace_filename[1024];
  int   ii;
  Flag  got_trace_filename=FALSE;

  if (argc < 2) {
    die_usage();
  }

    //--------------------------------------------------------------------
    // -- Get command line options
    //--------------------------------------------------------------------    
    for ( ii = 1; ii < argc; ii++) {
	if (argv[ii][0] == '-') {	    
	    if (!strcmp(argv[ii], "-h") || !strcmp(argv[ii], "-help")) {
		die_usage();
	    }	    

	    else if (!strcmp(argv[ii], "-mode")) {
		if (ii < argc - 1) {		  
     		  SIM_MODE = atoi(argv[ii+1]);
		  ii += 1;
		}
	    }

	    else if (!strcmp(argv[ii], "-linesize")) {
		if (ii < argc - 1) {		  
		    CACHE_LINESIZE = atoi(argv[ii+1]);
		    ii += 1;
		}
	    }

	    else if (!strcmp(argv[ii], "-repl")) {
		if (ii < argc - 1) {		  
		    REPL_POLICY = atoi(argv[ii+1]);
		    ii += 1;
		}
	    }

	    else if (!strcmp(argv[ii], "-DsizeKB")) {
		if (ii < argc - 1) {		  
		    DCACHE_SIZE = atoi(argv[ii+1])*1024;
		    ii += 1;
		}
	    }

	    else if (!strcmp(argv[ii], "-Dassoc")) {
		if (ii < argc - 1) {		  
		    DCACHE_ASSOC = atoi(argv[ii+1]);
		    ii += 1;
		}
	    }

	    else if (!strcmp(argv[ii], "-L2sizeKB")) {
		if (ii < argc - 1) {		  
		    L2CACHE_SIZE = atoi(argv[ii+1])*1024;
		    ii += 1;
		}
	    }

	    else {
		char msg[256];
		sprintf(msg, "Invalid option %s", argv[ii]);
		die_message(msg);
	    }
	}
	else if (!got_trace_filename) {
	    strcpy(trace_filename, argv[ii]);
	    got_trace_filename=TRUE;
	}
	else {
	    char msg[256];
	    sprintf(msg, "Invalid option %s, got filename %s", argv[ii], trace_filename);
	    die_message(msg);
	}    
    }
	    
    //--------------------------------------------------------------------
    // Error checking
    //--------------------------------------------------------------------
    if (!got_trace_filename) {
	die_message("Must provide at least one trace file");
    }


    //--------------------------------------------------------------------
    // -- Open the trace file
    //--------------------------------------------------------------------

    char  command_string[256];
    sprintf(command_string,"gunzip -c %s", trace_filename);
    if ((trfile = popen(command_string, "r")) == NULL){
      printf("Command string is %s\n", command_string);
      die_message("Unable to open the trace file with gzip option \n");
    }else{
      printf("Opened file with command: %s \n", command_string);
    }

}
