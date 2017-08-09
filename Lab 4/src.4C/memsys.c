#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "memsys.h"


//---- Cache Latencies  ------

#define DCACHE_HIT_LATENCY   1
#define ICACHE_HIT_LATENCY   1
#define L2CACHE_HIT_LATENCY  10

extern MODE   SIM_MODE;
extern uns64  CACHE_LINESIZE;
extern uns64  REPL_POLICY;

extern uns64  DCACHE_SIZE;
extern uns64  DCACHE_ASSOC;
extern uns64  ICACHE_SIZE;
extern uns64  ICACHE_ASSOC;
extern uns64  L2CACHE_SIZE;
extern uns64  L2CACHE_ASSOC;

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////


Memsys *memsys_new(void)
{
  Memsys *sys = (Memsys *) calloc (1, sizeof (Memsys));

  sys->dcache = cache_new(DCACHE_SIZE, DCACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);

  if(SIM_MODE!=SIM_MODE_A){
    sys->icache = cache_new(ICACHE_SIZE, ICACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
    sys->l2cache = cache_new(L2CACHE_SIZE, L2CACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
    sys->dram    = dram_new();
  }

  return sys;

}


////////////////////////////////////////////////////////////////////
// This function takes an ifetch/ldst access and returns the delay
////////////////////////////////////////////////////////////////////

uns64 memsys_access(Memsys *sys, Addr addr, Access_Type type)
{
  uns delay=0;


  // all cache transactions happen at line granularity, so get lineaddr
  Addr lineaddr=addr/CACHE_LINESIZE;


  if(SIM_MODE==SIM_MODE_A){
    delay = memsys_access_modeA(sys,lineaddr,type);
  }else{
    delay = memsys_access_modeBC(sys,lineaddr,type);
  }


  //update the stats
  if(type==ACCESS_TYPE_IFETCH){
    sys->stat_ifetch_access++;
    sys->stat_ifetch_delay+=delay;
  }

  if(type==ACCESS_TYPE_LOAD){
    sys->stat_load_access++;
    sys->stat_load_delay+=delay;
  }

  if(type==ACCESS_TYPE_STORE){
    sys->stat_store_access++;
    sys->stat_store_delay+=delay;
  }


  return delay;
}



////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

void memsys_print_stats(Memsys *sys)
{
  char header[256];
  sprintf(header, "MEMSYS");

  double ifetch_delay_avg=0;
  double load_delay_avg=0;
  double store_delay_avg=0;

  if(sys->stat_ifetch_access){
    ifetch_delay_avg = (double)(sys->stat_ifetch_delay)/(double)(sys->stat_ifetch_access);
  }

  if(sys->stat_load_access){
    load_delay_avg = (double)(sys->stat_load_delay)/(double)(sys->stat_load_access);
  }

  if(sys->stat_store_access){
    store_delay_avg = (double)(sys->stat_store_delay)/(double)(sys->stat_store_access);
  }


  printf("\n");
  printf("\n%s_IFETCH_ACCESS  \t\t : %10llu",  header, sys->stat_ifetch_access);
  printf("\n%s_LOAD_ACCESS    \t\t : %10llu",  header, sys->stat_load_access);
  printf("\n%s_STORE_ACCESS   \t\t : %10llu",  header, sys->stat_store_access);
  printf("\n%s_IFETCH_AVGDELAY\t\t : %10.3f",  header, ifetch_delay_avg);
  printf("\n%s_LOAD_AVGDELAY  \t\t : %10.3f",  header, load_delay_avg);
  printf("\n%s_STORE_AVGDELAY \t\t : %10.3f",  header, store_delay_avg);
  printf("\n");

  cache_print_stats(sys->dcache, "DCACHE");

  if(SIM_MODE!=SIM_MODE_A){
    cache_print_stats(sys->icache, "ICACHE");
    cache_print_stats(sys->l2cache, "L2CACHE");
    dram_print_stats(sys->dram);
  }

}


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

uns64 memsys_access_modeA(Memsys *sys, Addr lineaddr, Access_Type type){
  Flag access_dcache=FALSE;
  Flag mark_dirty=FALSE;

  if(type == ACCESS_TYPE_IFETCH){
    // no icache in this mode
  }

  if(type == ACCESS_TYPE_LOAD){
    access_dcache=TRUE;
    mark_dirty=FALSE;
  }

  if(type == ACCESS_TYPE_STORE){
    access_dcache=TRUE;
    mark_dirty=TRUE;
  }

  if(access_dcache){
    Flag hit=cache_access(sys->dcache, lineaddr, mark_dirty);
    if(hit==MISS){
      cache_install(sys->dcache, lineaddr, mark_dirty);
    }
  }

  // timing is not simulated in Part A
  return 0;
}

////////////////////////////////////////////////////////////////////
// --------------- DO NOT CHANGE THE CODE ABOVE THIS LINE ----------
////////////////////////////////////////////////////////////////////
uns64 memsys_access_modeBC(Memsys *sys, Addr lineaddr, Access_Type type){
  uns64 delay=0;
  Flag access_dcache=FALSE;
  Flag access_icache=FALSE;
  uns mark_dirty=FALSE;

  if(type == ACCESS_TYPE_IFETCH){
    access_icache=TRUE;
    mark_dirty=FALSE;
  }
  else if(type == ACCESS_TYPE_LOAD){
    access_dcache=TRUE;
    mark_dirty=FALSE;
  }
  else if(type == ACCESS_TYPE_STORE){
    access_dcache=TRUE;
    mark_dirty=TRUE;
  }

  if(access_icache)
  {
      delay=ICACHE_HIT_LATENCY;
      Flag hit=cache_access(sys->icache, lineaddr, FALSE);
      if(hit==MISS)
      {
          delay+=memsys_L2_access(sys,lineaddr,FALSE);
          cache_install(sys->icache, lineaddr, mark_dirty);
      }
  }
  else if(access_dcache)
  {
      delay=DCACHE_HIT_LATENCY;
      Flag hit=cache_access(sys->dcache, lineaddr, mark_dirty);
      if(hit==MISS)
      {
          delay+=memsys_L2_access(sys,lineaddr,FALSE);
          cache_install(sys->dcache, lineaddr, mark_dirty);
          if(sys->dcache->last_evicted_line.dirty==TRUE && sys->dcache->last_evicted_line.valid==TRUE)
          {
              //int numberOfSets=log2(sys->dcache->num_sets);
              //int MSBs=get_bits(sys->dcache->last_evicted_line.tag, 64,(numberOfSets));

              int dontCareDelay=memsys_L2_access(sys,sys->dcache->last_evicted_line.tag, TRUE);
              sys->dcache->last_evicted_line.dirty=FALSE;
              //sys->dcache->last_evicted_line.valid=FALSE;
          }
      }
  }
  return delay;
}


/////////////////////////////////////////////////////////////////////
// This function is called on ICACHE miss, DCACHE miss, DCACHE writeback
// ----- YOU NEED TO WRITE THIS FUNCTION AND UPDATE DELAY ----------
/////////////////////////////////////////////////////////////////////

uns64   memsys_L2_access(Memsys *sys, Addr lineaddr, Flag is_writeback){
  uns64 delay = L2CACHE_HIT_LATENCY;

  //To get the delay of L2 MISS, you must use the dram_access() function
  //To perform writebacks to memory, you must use the dram_access() function
  //This will help us track your memory reads and memory writes
  Flag hit=cache_access(sys->l2cache, lineaddr, is_writeback);
  if(hit==MISS)
  {
      delay+=dram_access(sys->dram,lineaddr, FALSE);
      cache_install(sys->l2cache, lineaddr, is_writeback);
      if(sys->l2cache->last_evicted_line.dirty==TRUE)
      {
        //int numberOfSets=log2(sys->dcache->num_sets);
        //int MSBs=get_bits(sys->dcache->last_evicted_line.tag, 64,(numberOfSets));
        sys->l2cache->last_evicted_line.dirty=FALSE;
        //sys->l2cache->last_evicted_line.valid=FALSE;
        int dontCareDelay=dram_access(sys->dram,sys->l2cache->last_evicted_line.tag, TRUE);

      }
  }

  return delay;
}
