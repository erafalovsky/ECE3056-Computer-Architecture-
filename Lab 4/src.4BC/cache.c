#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "cache.h"
#include <math.h>


extern uns64 cycle_count; // You can use this as timestamp for LRU

////////////////////////////////////////////////////////////////////
// ------------- DO NOT MODIFY THE INIT FUNCTION -----------
////////////////////////////////////////////////////////////////////

Cache  *cache_new(uns64 size, uns64 assoc, uns64 linesize, uns64 repl_policy){

   Cache *c = (Cache *) calloc (1, sizeof (Cache));
   c->num_ways = assoc;
   c->repl_policy = repl_policy;

   if(c->num_ways > MAX_WAYS){
     printf("Change MAX_WAYS in cache.h to support %llu ways\n", c->num_ways);
     exit(-1);
   }

   // determine num sets, and init the cache
   c->num_sets = size/(linesize*assoc);
   c->sets  = (Cache_Set *) calloc (c->num_sets, sizeof(Cache_Set));

   return c;
}

////////////////////////////////////////////////////////////////////
// ------------- DO NOT MODIFY THE PRINT STATS FUNCTION -----------
////////////////////////////////////////////////////////////////////

void    cache_print_stats    (Cache *c, char *header){
  double read_mr =0;
  double write_mr =0;

  if(c->stat_read_access){
    read_mr=(double)(c->stat_read_miss)/(double)(c->stat_read_access);
  }

  if(c->stat_write_access){
    write_mr=(double)(c->stat_write_miss)/(double)(c->stat_write_access);
  }

  printf("\n%s_READ_ACCESS    \t\t : %10llu", header, c->stat_read_access);
  printf("\n%s_WRITE_ACCESS   \t\t : %10llu", header, c->stat_write_access);
  printf("\n%s_READ_MISS      \t\t : %10llu", header, c->stat_read_miss);
  printf("\n%s_WRITE_MISS     \t\t : %10llu", header, c->stat_write_miss);
  printf("\n%s_READ_MISSPERC  \t\t : %10.3f", header, 100*read_mr);
  printf("\n%s_WRITE_MISSPERC \t\t : %10.3f", header, 100*write_mr);
  printf("\n%s_DIRTY_EVICTS   \t\t : %10llu", header, c->stat_dirty_evicts);

  printf("\n");
}



////////////////////////////////////////////////////////////////////
// Note: the system provides the cache with the line address
// Return HIT if access hits in the cache, MISS otherwise
// Also if mark_dirty is TRUE, then mark the resident line as dirty
// Update appropriate stats
////////////////////////////////////////////////////////////////////

int get_bits(Addr value, int start, int end){
  int result;
  assert(start >= end );
  result = value >> end;
  result = result % ( 1 << ( start - end + 1 ) );
  return result;
}



Flag    cache_access(Cache *c, Addr lineaddr, uns mark_dirty){
  Flag outcome=MISS;

  int numberOfSets=log2((c->num_sets));
  int LSBs=get_bits(lineaddr,(numberOfSets-1),0);

  //Addr currentAddress=lineaddr>>numberOfSets;

  int numberOfWays=c->num_ways;
  for(int i=0; i<numberOfWays; i++)
  {
  	if(lineaddr==c->sets[LSBs].line[i].tag && c->sets[LSBs].line[i].valid)
    {
          outcome=HIT;
          c->sets[LSBs].line[i].last_access_time=cycle_count;
          if(mark_dirty)
              {c->sets[LSBs].line[i].dirty=TRUE;}
          break;
  	}
  }


  if(mark_dirty)
  {
    c->stat_write_access++;
    if(outcome==MISS)
      c->stat_write_miss++;
  }
  else
  {
    c->stat_read_access++;
    if(outcome==MISS)
      c->stat_read_miss++;
  }

  return outcome;
}

////////////////////////////////////////////////////////////////////
// Note: the system provides the cache with the line address
// Install the line: determine victim using repl policy (LRU/RAND)
// copy victim into last_evicted_line for tracking writebacks
////////////////////////////////////////////////////////////////////

void    cache_install(Cache *c, Addr lineaddr, uns mark_dirty){

  int numberOfSets=log2((c->num_sets));
  int LSBs=get_bits(lineaddr,(numberOfSets-1),0);

  //Addr currentAddress=lineaddr>>numberOfSets;
  int needToReplace=TRUE;

  int numberOfWays=c->num_ways;
  for(int i=0; i<numberOfWays; i++)
  {
  	if(c->sets[LSBs].line[i].tag==0)
    {
      c->sets[LSBs].line[i].valid=TRUE;
      c->sets[LSBs].line[i].dirty=mark_dirty;
      c->sets[LSBs].line[i].tag=lineaddr;
      c->sets[LSBs].line[i].last_access_time=cycle_count;
      c->last_evicted_line.valid=FALSE;
      needToReplace=FALSE;
      break;
  	}

  }

  if(needToReplace==TRUE)
  {
    int block=0;
    if(c->repl_policy==1)
    {
      block=rand()%(c->num_ways);
    }
    else
    {

      uns minimumCycleCount=c->sets[LSBs].line[0].last_access_time;
      for(int i=0; i<numberOfWays;i++)
      {
          Cache_Line currentLine=c->sets[LSBs].line[i];
          if(currentLine.last_access_time<minimumCycleCount)
          {
              minimumCycleCount=currentLine.last_access_time;
              block=i;
          }
      }


    }
    // check if old is dirty before installing new line
    if(c->sets[LSBs].line[block].dirty)
        c->stat_dirty_evicts++;

    c->last_evicted_line=c->sets[LSBs].line[block];
    c->sets[LSBs].line[block].valid=TRUE;
    c->sets[LSBs].line[block].dirty=mark_dirty;
    c->sets[LSBs].line[block].tag=lineaddr;
    c->sets[LSBs].line[block].last_access_time=cycle_count;


  }
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
