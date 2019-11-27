//========================================================//
//  cache.c                                               //
//  Source file for the Cache Simulator                   //
//                                                        //
//  Implement the I-cache, D-Cache and L2-cache as        //
//  described in the README                               //
//========================================================//

#include "cache.h"
#include <math.h>
#include <stdio.h>

//
// Student Information
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


typedef struct block
{
	struct block *prev_block, *next_block;
	uint32_t tag;
}block;

typedef struct set_queue
{
	block *set_front_block, *set_rear_block;
}set_queue;

void add_block_to_front(set_queue* current_set, block *new_block)
{
	if(current_set->set_front_block==NULL && current_set->set_rear_block==NULL)
	{
		new_block->prev_block = NULL;
		new_block->next_block = NULL;
		current_set->set_front_block = new_block;
		current_set->set_rear_block = new_block;
	}
	else
	{
		new_block->next_block = current_set->set_front_block;
		new_block->prev_block = NULL;
		new_block->next_block->prev_block = new_block;
		current_set->set_front_block = new_block;
		
	}
}

void bring_block_to_front(set_queue* current_set, block *hit_block)
{
	if(hit_block == current_set->set_front_block)
		return;
	if(hit_block == current_set->set_rear_block)
	{
		current_set->set_rear_block = current_set->set_rear_block->prev_block;
		current_set->set_rear_block->next_block = NULL;
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

void free_rear_block(set_queue* current_set)
{
	if(current_set->set_rear_block==NULL)
		return;
	if(current_set->set_front_block == current_set->set_rear_block)
	{
		free(current_set->set_front_block);
		current_set->set_front_block = NULL;
		current_set->set_rear_block = NULL;
	}
	else
	{
		block *temp = current_set->set_rear_block;
		current_set->set_rear_block = current_set->set_rear_block->prev_block;
		current_set->set_rear_block->next_block = NULL;
		free(temp);
	}
}

//
// Add your Cache data structures here
//

set_queue* icache;
set_queue* dcache;
set_queue* l2cache;

uint32_t block_offset_bits; 
uint32_t icache_index_bits ;
uint32_t dcache_index_bits ;
uint32_t l2cache_index_bits;

uint32_t mask_block_offset;
uint32_t mask_icache_set  ;
uint32_t mask_dcache_set  ;
uint32_t mask_l2cache_set ;

uint32_t icache_current_index;
uint32_t dcache_current_index;
uint32_t l2cache_current_index;

void invalidate(uint32_t addr)
{
	uint32_t tag;
	block *temp = NULL;

    // Invalidating i cache  
    tag = addr >> (icache_index_bits + block_offset_bits);
    icache_current_index = (addr & mask_icache_set) >> block_offset_bits;	
    temp = icache[icache_current_index].set_front_block;
	
	for(int i=0; (i<icacheAssoc) && (temp!= NULL); i++)
	{
		if(temp->tag == tag)
		{
			if(temp->prev_block == NULL)
			{
				free(temp);
				icache[icache_current_index].set_front_block = NULL;
				icache[icache_current_index].set_rear_block = NULL;
			}
			else if(temp->next_block == NULL) 
			{

        		icache[icache_current_index].set_rear_block = temp->prev_block;
        		icache[icache_current_index].set_rear_block->next_block = NULL;
		        free(temp);
			}
			else
			{
				temp->prev_block->next_block = temp->next_block;
				temp->next_block->prev_block = temp->prev_block;
				free(temp);
			}
			break;
		}
		temp = temp->next_block;
	}
	
	    // Invalidating d cache  
	
	tag = addr >> (dcache_index_bits + block_offset_bits);  
	dcache_current_index = (addr & mask_dcache_set) >> block_offset_bits;	
    temp = dcache[dcache_current_index].set_front_block;
	
	for(int i=0; (i<dcacheAssoc) && (temp!= NULL); i++)
	{
		if(temp->tag == tag)
		{
			if(temp->prev_block == NULL)
			{
				free(temp);
				dcache[dcache_current_index].set_front_block = NULL;
				dcache[dcache_current_index].set_rear_block = NULL;
			}
			else if(temp->next_block == NULL) 
			{

        		dcache[dcache_current_index].set_rear_block = temp->prev_block;
        		dcache[dcache_current_index].set_rear_block->next_block = NULL;
		        free(temp);
			}
			else
			{
				temp->prev_block->next_block = temp->next_block;
				temp->next_block->prev_block = temp->prev_block;
				free(temp);
			}
			break;
		}
		temp = temp->next_block;
	}
}

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
  //Initialize Cache Simulator Data Structures
  //
  icache = (set_queue*)malloc(icacheSets*sizeof(set_queue));
  dcache = (set_queue*)malloc(dcacheSets*sizeof(set_queue));
  l2cache = (set_queue*)malloc(l2cacheSets*sizeof(set_queue));
  
  for(int i=0; i<icacheSets; i++)
  {
    icache[i].set_front_block = NULL;
    icache[i].set_rear_block = NULL;
  }

  for(int i=0; i<dcacheSets; i++)
  {
    dcache[i].set_front_block = NULL;
    dcache[i].set_rear_block = NULL;
  }

  for(int i=0; i<l2cacheSets; i++)
  {
    l2cache[i].set_front_block = NULL;
    l2cache[i].set_rear_block = NULL;
  }
  
  block_offset_bits = (uint32_t)log2(blocksize);
  block_offset_bits+= ((1<<block_offset_bits)==blocksize)?0:1;
  icache_index_bits = (uint32_t)log2(icacheSets);
  icache_index_bits+=((1<<icache_index_bits)==icacheSets)?0:1;
  dcache_index_bits = (uint32_t)log2(dcacheSets);
  dcache_index_bits+=((1<<dcache_index_bits)==dcacheSets)?0:1;
  l2cache_index_bits =(uint32_t)log2(l2cacheSets);
  l2cache_index_bits+=((1<<l2cache_index_bits)==l2cacheSets)?0:1;
  
  mask_block_offset = (1 << block_offset_bits) - 1;
  mask_icache_set = ((1 << icache_index_bits) - 1) << block_offset_bits;
  mask_dcache_set = ((1 << dcache_index_bits) - 1) << block_offset_bits;
  mask_l2cache_set = ((1 << l2cache_index_bits) - 1) << block_offset_bits; 
  
}

// Perform a memory access through the icache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
icache_access(uint32_t addr)
{
  //
  // I$
  //
  uint32_t tag;
  uint8_t icachehit = 0;
  uint8_t i = 0x0;
  uint32_t l2_access_penalty = 0;
  
  if(icacheSets==0)
  {
	return l2cache_access(addr);
  }
  icacheRefs++;
  
  
  tag = addr >> (block_offset_bits + icache_index_bits);
  icache_current_index = (addr & mask_icache_set) >> block_offset_bits;
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

  switch(icachehit)
  {
	  case 0://icache_miss
		l2_access_penalty = l2cache_access(addr);
        if(i == (icacheAssoc))
		{
			free_rear_block(&icache[icache_current_index]);
		}
		block *new_icache_block = malloc(sizeof(block));
		new_icache_block->tag = tag;
		add_block_to_front(&icache[icache_current_index], new_icache_block);
		icachePenalties+=l2_access_penalty;
		icacheMisses++;       
		return icacheHitTime + l2_access_penalty;
		break;
	  
	  case 1://icache_hit
		bring_block_to_front(&icache[icache_current_index], temp);
		return icacheHitTime;
		break;
  }  
  return memspeed;
}

// Perform a memory access through the dcache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
dcache_access(uint32_t addr)
{
  //
  //Implement D$
  //
  uint32_t tag;
  uint8_t dcachehit = 0;
  uint8_t i = 0;
  uint32_t l2_access_penalty = 0;
  
  if(dcacheSets==0)
  {
		return l2cache_access(addr);
  }
  
  dcacheRefs++;
 
  tag = addr >> (block_offset_bits + dcache_index_bits);
  dcache_current_index = (addr & mask_dcache_set) >> block_offset_bits;
  block *temp = dcache[dcache_current_index].set_front_block;
  
  for(i=0; (i<dcacheAssoc) && (temp!= NULL); i++)
  {
	if(temp->tag == tag)
	{
		dcachehit = 1;
		break;
	}
	temp = temp->next_block;
  }
  
  switch(dcachehit)
  {
	  case 0://dcache_miss
		l2_access_penalty = l2cache_access(addr);
        if(i == (dcacheAssoc))
			free_rear_block(&dcache[dcache_current_index]);
		block *new_dcache_block = malloc(sizeof(block));
		new_dcache_block->tag = tag;
		add_block_to_front(&dcache[dcache_current_index], new_dcache_block);
		dcachePenalties+=l2_access_penalty;
		dcacheMisses++;
		return dcacheHitTime + l2_access_penalty;
		break;
	  
	  case 1://dcache_hit
		bring_block_to_front(&dcache[dcache_current_index], temp);
		return dcacheHitTime;
		break;

  }  
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
  
  uint32_t tag;
  uint8_t i =0;
  uint8_t l2cachehit =0;
  
  if(l2cacheSets==0)
  {
		return memspeed;
  }
  
  l2cacheRefs++;
  
  
  tag = addr >> (block_offset_bits + l2cache_index_bits);
  l2cache_current_index = (addr & mask_l2cache_set) >> block_offset_bits;
  block *temp = l2cache[l2cache_current_index].set_front_block;
  
  for(i=0; (i<l2cacheAssoc) && (temp!= NULL); i++)
  {
	if(temp->tag == tag)
	{
		l2cachehit = 1;
		break;
	}
	temp = temp->next_block;
  }
    
  switch(l2cachehit)
  {
	  case 0://l2cache_miss
        if(i == (l2cacheAssoc))
		{
			free_rear_block(&l2cache[l2cache_current_index]);
			if(inclusive == TRUE)
			{
				invalidate(addr);
			}
		}
		block *new_l2cache_block = malloc(sizeof(block));
		new_l2cache_block->tag = tag;
		add_block_to_front(&l2cache[l2cache_current_index], new_l2cache_block);
		l2cacheMisses++;
		l2cachePenalties+= memspeed;
		return l2cacheHitTime + memspeed;
		break;
	  
	  case 1://l2cache_hit       
		bring_block_to_front(&l2cache[l2cache_current_index], temp);
		return l2cacheHitTime;
		break;
  }  
  return memspeed;
}
