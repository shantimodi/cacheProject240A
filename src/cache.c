//========================================================//
//  cache.c                                               //
//  Source file for the Cache Simulator                   //
//                                                        //
//  Implement the I-cache, D-Cache and L2-cache as        //
//  described in the README                               //
//========================================================//

#include "cache.h"

//
// TODO:Student Information
//
const char *studentName = "Shanti Modi";
const char *studentID   = "A53305577";
const char *email       = "shmodi@eng.ucsd.edu";

//------------------------------------//
//        Cache Configuration         //
//------------------------------------//

uint32_t icacheSets;     // Number of sets in the I$
uint32_t icacheAssoc;    // Associativity of the I$
uint32_t icacheHitTime;  // Hit Time of the I$

uint32_t dcacheSets;     // Number of sets in the D$
uint32_t dcacheAssoc;    // Associativity of the D$
uint32_t dcacheHitTime;  // Hit Time of the D$

uint32_t l2cacheSets;    // Number of sets in the L2$
uint32_t l2cacheAssoc;   // Associativity of the L2$
uint32_t l2cacheHitTime; // Hit Time of the L2$
uint32_t inclusive;      // Indicates if the L2 is inclusive

uint32_t blocksize;      // Block/Line size
uint32_t memspeed;       // Latency of Main Memory

//------------------------------------//
//          Cache Statistics          //
//------------------------------------//

uint64_t icacheRefs;       // I$ references
uint64_t icacheMisses;     // I$ misses
uint64_t icachePenalties;  // I$ penalties

uint64_t dcacheRefs;       // D$ references
uint64_t dcacheMisses;     // D$ misses
uint64_t dcachePenalties;  // D$ penalties

uint64_t l2cacheRefs;      // L2$ references
uint64_t l2cacheMisses;    // L2$ misses
uint64_t l2cachePenalties; // L2$ penalties

//------------------------------------//
//        Cache Data Structures       //
//------------------------------------//

typedef struct set_queue
{
	block *set_front_block, *set_rear_block
}set_queue;


typedef struct block
{
	block *prev_block, *next_block;
	uint32_t tag;
}block;


void add_block_to_front(set_queue* current_set, block *hit_block)
{
	if(current_set->set_front_block!=NULL && current_set->set_rear_block!=NULL)
	{
		current_set->set_front_block = hit_block;
		current_set->set_rear_block = hit_block;
	}
	else
	{
		hit_block->next_block = current_set->set_front_block;
		hit_block->prev_block = hit_block;
		current_set->set_front_block = hit_block;
	}
}

void bring_block_to_front(set_queue* current_set, block *hit_block)
{
	if(hit_block == current_set->set_front_block)
		return;
	if(hit_block == current_set->set_rear_block)
	{
		current_set->set_rear_block = current_set->set_rear_block->prev_block;
		current_set->set_rear_block->next = NULL;
	}
	else
	{
		hit_block->prev_block->next_block = hit_block->next_block;
		hit_block->next_block->prev_block = hit_block->prev_block;
	}
	hit_block->next_block = current_set->set_front_block;
	hit_block->prev_block = NULL;
	current_set->set_front_block->prev_block = hit_block;
	current_set->set_front_block = hit_block;
}

void free_rear_block()
{
	if(current_set->set_rear_block==NULL)
		return;
	if(current_set->set_front_block == current_set->set_rear_block)
	{
		free(current_set->set_rear_block);
		current_set->set_front_block = NULL;
		current_set->set_rare_block = NULL;
	}
	else
	{
		block *temp = current_set->set_rare_block;
		current_set->set_rare_block = current_set->set_rare_block->prev_block;
		current_set->set_rare_block->next_block = NULL;
		free(temp);
	}
}

//
//TODO: Add your Cache data structures here
//

set_queue* icache;
set_queue* dcache;
set_queue* l2cache;

uint32_t block_offset_bits = log2(blocksize);
uint32_t icache_index_bits = log2(icacheSets);
uint32_t dcache_index_bits = log2(dcacheSets);
uint32_t l2cache_index_bits = log2(l2cacheSets);

uint32_t mask_block_offset = (1 << blocksize_offset) - 1;
uint32_t mask_icache_set= ( (1 << icache_index) - 1) << block_size_offset;
uint32_t mask_dcache_set= ( (1 << dcache_index) - 1) << block_size_offset;
uint32_t mask_l2cache_set = ( (1 << l2cache_index) - 1) << block_size_offset;

uint32_t icache_current_index;
uint32_t dcache_current_index;
uint32_t l2cache_current_index;

//------------------------------------//
//          Cache Functions           //
//------------------------------------//

// Initialize the Cache Hierarchy
//
void
init_cache()
{
  // Initialize cache stats
  icacheRefs        = 0;
  icacheMisses      = 0;
  icachePenalties   = 0;
  dcacheRefs        = 0;
  dcacheMisses      = 0;
  dcachePenalties   = 0;
  l2cacheRefs       = 0;
  l2cacheMisses     = 0;
  l2cachePenalties  = 0;
  
  //
  //TODO: Initialize Cache Simulator Data Structures
  //
  icache = (set_queue*)malloc(icacheSets*sizeof(set_queue));
  dcache = (set_queue*)malloc(dcacheSets*sizeof(set_queue));
  l2cache = (set_queue*)malloc(l2cacheSets*sizeof(set_queue));
}

// Perform a memory access through the icache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
icache_access(uint32_t addr)
{
  //
  //TODO: Implement I$
  //
  uint32_t tag;
  uint32_t l2_access_penalty;
  
  icacheRefs++;
  
  
  tag = addr >> (blocksize_offset + icache_index);
  icache_current_index = (addr & mask_icache_set) >> blocksize_offset;
  block *temp = icache[icache_current_index].set_front_block;
  
  for(i=0; (i<icacheAssoc) && (temp!= NULL); i++)
  {
	if(temp->tag == tag)
	{
		icachehit = 1;
		break;
	}
	temp = temp->next_block;
  }
  
  switch(icachehit):
  {
	  case 0://icache_miss
		l2_access_penalty = l2cache_access(addr);
        if(i == icacheAssoc)
			free_rear_block(icache[icache_current_index]);
		add_block_to_front(icache[icache_current_index], l2_access_block);
		
	  
	  case 1://hit
		bring_block_to_front(icache[icache_current_index], icache[i]);
		return icacheHitTime;

  }
  
  
  

  icacheMisses++;
  
  
  
  
  
  return memspeed;
}

// Perform a memory access through the dcache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
dcache_access(uint32_t addr)
{
  //
  //TODO: Implement D$
  //
  return memspeed;
}

// Perform a memory access to the l2cache for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
l2cache_access(uint32_t addr)
{
  //
  //TODO: Implement L2$
  //
  return memspeed;
}
