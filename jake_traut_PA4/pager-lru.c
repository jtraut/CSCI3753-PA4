/*
 * File: pager-lru.c
 * Author:       Andy Sayler
 *               http://www.andysayler.com
 * Adopted From: Dr. Alva Couch
 *               http://www.cs.tufts.edu/~couch/
 * Modified by:	 Jake Traut
 * 
 * Project: CSCI 3753 Programming Assignment 4
 * Create Date: Unknown
 * Modify Date: 2016/03/30
 * Description:
 * 	This file contains an lru pageit
 *      implmentation.
 */

#include <stdio.h> 
#include <stdlib.h>

#include "simulator.h"

void pageit(Pentry q[MAXPROCESSES]) { 
    
    /* This file contains the stub for an LRU pager */
    /* You may need to add/remove/modify any part of this file */

    /* Static vars */
    static int initialized = 0;
    static int tick = 1; // artificial time
    static int timestamps[MAXPROCESSES][MAXPROCPAGES];
    
    /* Local vars */
    int proctmp, pagetmp, proc, pc, page;      
    int procactive = 0;
    int activeq[MAXPROCESSES];
    int outproc, evictpage, least;

    /* initialize static vars on first run */
    if(!initialized){
	for(proctmp=0; proctmp < MAXPROCESSES; proctmp++){
	    for(pagetmp=0; pagetmp < MAXPROCPAGES; pagetmp++){
			timestamps[proctmp][pagetmp] = 0; 
	    }
	}
	initialized = 1;
    }
    
    /* TODO: Implement LRU Paging */
    
    //select process
    for(proctmp = 0; proctmp < MAXPROCESSES; proctmp++){
		//find active processes and update 
		if(q[proctmp].active){
			activeq[procactive] = proctmp;
			procactive++;
			for(pagetmp = 0; pagetmp < MAXPROCPAGES; pagetmp++)
				timestamps[proctmp][pagetmp] = tick;
		}
		else { //swap out any unactive pages
			for(pagetmp = 0; pagetmp < MAXPROCPAGES; pagetmp++){
				if(q[proctmp].pages[pagetmp])
					pageout(proctmp,pagetmp);
			}
		}
	}

	//all active processes
	int i;
    for(i = 0; i < procactive; i++){
		proc = activeq[i];
		pc = q[proc].pc;
		page = pc / PAGESIZE; //determine current page
		
		//update other pages 
		for(pagetmp = 0; pagetmp < MAXPROCPAGES; pagetmp++){
			timestamps[proc][pagetmp]++;
		}
		//and reset the counter for most recent 
		timestamps[proc][page] = 0;				
		
		//is page swapped in
		if(q[proc].pages[page]) { //YES page swapped in
			continue;
		}
		else{ //NO page not swapped in - call pagein() 
			if(pagein(proc,page)){
					continue;
			}
			else{ //proccess at capacity, need to swap pages		
				//find a default page
				for(pagetmp = 0; pagetmp < MAXPROCPAGES; pagetmp++){
					if(q[proc].pages[pagetmp]){
						least = timestamps[proc][pagetmp];
						evictpage = pagetmp;
						outproc = proc;
						break;
					}
				}

				//look for better - LRU 
				for(pagetmp = 0; pagetmp < MAXPROCPAGES; pagetmp++){
					if(!q[proc].pages[pagetmp]){ //cant evict page thats not in
						continue;
					}
					if(timestamps[proc][pagetmp] > least){
						least = timestamps[proc][pagetmp];
						evictpage = pagetmp;
						outproc = proc;
					//	printf("current LRU timestamp = %d\n", least);
					}
				}
				//call pageout()
				if(q[outproc].pages[evictpage]){ //cant evict page thats not in
					pageout(outproc, evictpage);
				}
			}
		}
	}
	/* advance time for next pageit iteration */
	tick++;	
} 
