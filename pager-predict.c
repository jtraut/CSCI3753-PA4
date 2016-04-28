/*
 * File: pager-predict.c
 * Author:       Andy Sayler
 *               http://www.andysayler.com
 * Adopted From: Dr. Alva Couch
 *               http://www.cs.tufts.edu/~couch/
 * Modified by:	 Jake Traut
 * 
 * Project: CSCI 3753 Programming Assignment 4
 * Create Date: Unknown
 * Modify Date: 2016/03/31
 * Description:
 * 	This file contains a predictive pageit
 *      implmentation.
 *  70+ hours into this... was hoping to improve 
 *  my score some more, but oh well. 
 */

#include <stdio.h> 
#include <stdlib.h>

#include "simulator.h" 

//global variables for prediction and handling page allocations 
int pagepred; 
double upperthres = .5;
double lowerthres = .00001;
int totalpages;
int totalallocated;
int allocationset = 1;

int local_lru(Pentry q[MAXPROCESSES], int timestamps[MAXPROCESSES][MAXPROCPAGES], int proc, int page, int preceeds[MAXPROCESSES][MAXPROCPAGES][MAXPROCPAGES]){
	int evictpage, least, pagetmp;
	int success = 0;
	
	//find a default page
/*
	for(pagetmp = 0; pagetmp < MAXPROCPAGES; pagetmp++){
		if(q[proc].pages[pagetmp]){
			least = timestamps[proc][pagetmp];
			evictpage = pagetmp;
			//printf("do have a default\n");
			break;
		}
	}
	*/
	 	
	//get rid of unlikely pages
	evictpage = -1;		
	//calc_alloc_status(q, allocateframes);
		least = -10;
		for(pagetmp = 0; pagetmp < MAXPROCPAGES; pagetmp++){
			if(q[proc].pages[pagetmp] && preceeds[proc][page][pagetmp] < least && pagetmp != page){
				//least = preceeds[proc][page][pagetmp];
				evictpage = pagetmp;
				if(pageout(proc, evictpage)) success = 1;
				//printf("least likely is page %d with value %d\n", pagetmp, preceeds[proc][page][pagetmp]);
			}
		}
	//	printf("evictpage %d\n", evictpage);
	//	if(evictpage != -1 && q[proc].pages[evictpage]) pageout(proc, evictpage);
	
	if(evictpage == -1){ 
	//now look for better -- LRU
	for(pagetmp = 0; pagetmp < MAXPROCPAGES; pagetmp++){
		if(!q[proc].pages[pagetmp]) //cant evict page thats not in
			continue;
		if(timestamps[proc][pagetmp] > least){
			least = timestamps[proc][pagetmp];
			evictpage = pagetmp;
			//printf("current LRU timestamp = %d\n", least);	
		}
	}
	//call pageout()
	if(q[proc].pages[evictpage] && pageout(proc, evictpage)){	
		success = 1;
		//printf("evicting local: process %d page %d\n", proc, evictpage);
	}
	}
	
	return success;
}

void calc_alloc_status(Pentry q[MAXPROCESSES], int framecount[MAXPROCESSES], int allocateframes[MAXPROCESSES]){
	int proctmp, pagetmp;
    //keep totalpages and process framecounts updated before doing work on pages 	
	totalpages = 0;
	totalallocated = 0;
	for(proctmp = 0; proctmp < MAXPROCESSES; proctmp++){
		framecount[proctmp] = 0;
		for(pagetmp=0; pagetmp < MAXPROCPAGES; pagetmp++){
			if(q[proctmp].pages[pagetmp])
				framecount[proctmp]++;
		}
		totalpages = totalpages + framecount[proctmp];
		totalallocated = totalallocated + allocateframes[proctmp];
	}
}

void page_allocation(Pentry q[MAXPROCESSES], double PFF[MAXPROCESSES], int framecount[MAXPROCESSES], int allocateframes[MAXPROCESSES]){
	int proctmp;
	calc_alloc_status(q, framecount, allocateframes);
	//check status of page faults 
	for(proctmp = 0; proctmp < MAXPROCESSES; proctmp++){
		if(PFF[proctmp] < lowerthres){ //try and share some pages 
			if(framecount[proctmp] < allocateframes[proctmp] && allocateframes[proctmp] >= 5){
				allocateframes[proctmp]--;
				totalallocated--;
				//printf("deallocating page for process %d\n", proctmp);
			}
		}
	}
	//now try to reallocate to the needy -- charity 
	if(totalallocated < PHYSICALPAGES){
		proctmp = 0;
		while(totalallocated < PHYSICALPAGES && proctmp < MAXPROCESSES){
			if(PFF[proctmp] >= upperthres && allocateframes[proctmp] < 14){
				allocateframes[proctmp]++;
				totalallocated++;
				//printf("allocating page for process %d with PFF: %f\n", proctmp, PFF[proctmp]);
			}
			else if(PFF[proctmp] > upperthres/2 && allocateframes[proctmp] < 10){
				allocateframes[proctmp]++;
				totalallocated++;
				//printf("allocating page for process %d with PFF: %f\n", proctmp, PFF[proctmp]);	
			}
			else if(allocateframes[proctmp] < 8){
				allocateframes[proctmp]++;
				totalallocated++;
			}
			proctmp++;
		}
	}
}

int predict_preceeds(int proc, int page, int preceeds[MAXPROCESSES][MAXPROCPAGES][MAXPROCPAGES]){
	int pagetmp;
	int mostfreq = 0;
	pagepred = -1;
	//check if a page frequently follows another
	for(pagetmp = 0; pagetmp < MAXPROCPAGES; pagetmp++){
		if(preceeds[proc][page][pagetmp] > mostfreq){
				mostfreq = preceeds[proc][page][pagetmp];
				pagepred = pagetmp;
		}	
	}
	 return pagepred;
}

void pageit(Pentry q[MAXPROCESSES]) { 
    
    /* This file contains the stub for a predictive pager */
    /* You may need to add/remove/modify any part of this file */

    /* Static vars */
    static int initialized = 0;
    static int tick = 1; // artificial time
    static int timestamps[MAXPROCESSES][MAXPROCPAGES]; //help with LRU
    static int preceeds[MAXPROCESSES][MAXPROCPAGES][MAXPROCPAGES]; //frequency of what comes before what
    static double PFF[MAXPROCESSES]; //page fault frequency
    static int allocateframes[MAXPROCESSES];
    static int framecount[MAXPROCESSES];
    static int faultcount[MAXPROCESSES];
    static int passcount[MAXPROCESSES];
    static int lastpc[MAXPROCESSES][MAXPROCPAGES];
    static int pchold[MAXPROCESSES][MAXPROCPAGES];
    static int prev_status[MAXPROCESSES][MAXPROCPAGES][MAXPROCPAGES] = {{{0}}}; 
     
    /* Local vars */
    int proctmp, pagetmp, proc, pc, page, pagepred, nextpg; 
    int procactive = 0;
    int activeq[MAXPROCESSES];

    /* initialize static vars on first run */
    if(!initialized){
	/* Init complex static vars here */
	for(proctmp=0; proctmp < MAXPROCESSES; proctmp++){
		PFF[proctmp] = 0;
		allocateframes[proctmp] = 5;
		framecount[proctmp] = 0;
		faultcount[proctmp] = 0;
		passcount[proctmp] = 0;
	    for(pagetmp=0; pagetmp < MAXPROCPAGES; pagetmp++){
			timestamps[proctmp][pagetmp] = 0; 
			lastpc[proctmp][pagetmp] = 0;
			pchold[proctmp][pagetmp] = 0;
			for(nextpg=0; nextpg < MAXPROCPAGES; nextpg++){
				preceeds[proctmp][pagetmp][nextpg] = 0;
			}
	    }
	}	
	initialized = 1;
    }
    
    /* TODO: Implement Predictive Paging */
    
    //select process
    for(proctmp = 0; proctmp < MAXPROCESSES; proctmp++){
		//find active processes and update 
		if(q[proctmp].active){
			activeq[procactive] = proctmp;
			procactive++;
		}
		else{
			allocateframes[proctmp] = 0;
			framecount[proctmp] = 0;
			for(pagetmp = 0; pagetmp < MAXPROCPAGES; pagetmp++){
				if(q[proctmp].pages[pagetmp])
					pageout(proctmp, pagetmp);
			}
		}
	}
	
	if(procactive == 0) return;

	if(procactive <= 7){ //allocating pages no longer helpful at this point 
		allocationset = 0;
	}		
	
	int i;
	//all active processes	
	
    for(i = 0; i < procactive; i++){
		proc = activeq[i];
		pc = q[proc].pc;
		page = pc / PAGESIZE; //determine current page	
			
		if(!pchold[proc][page]){
			lastpc[proc][page] = pc;
			pchold[proc][page] = 1;
			for(pagetmp = 0; pagetmp < MAXPROCPAGES; pagetmp++){
				if(q[proc].pages[page])
					prev_status[proc][page][pagetmp] = 1;
				else
					prev_status[proc][page][pagetmp] = 0;
			}
		}	
			
		for(pagetmp = 0; pagetmp < MAXPROCPAGES; pagetmp++){
			//looking for what comes around 170 ticks later 
			if(pchold[proc][pagetmp] && pc >= lastpc[proc][pagetmp]+170){
				pchold[proc][pagetmp] = 0;
				preceeds[proc][pagetmp][page]++;
				if(pc == lastpc[proc][pagetmp]+170)
					preceeds[proc][pagetmp][page]++;	
				for(nextpg = 0; nextpg < MAXPROCPAGES; nextpg++){	
				if(prev_status[proc][pagetmp][nextpg] && !q[proc].pages[nextpg] && nextpg != page)
					preceeds[proc][pagetmp][nextpg]--;
					if(pc == lastpc[proc][pagetmp]+170)
					preceeds[proc][pagetmp][nextpg]--;
				}
			}
		}
		
		//update other pages timestamp
		for(pagetmp = 0; pagetmp < MAXPROCPAGES; pagetmp++){
			timestamps[proc][pagetmp]++;
		}
		//and reset the counter for most recent 
		timestamps[proc][page] = 0;				
		
		// run page allocation to see if can allocate better
		if(allocationset) page_allocation(q, PFF, framecount, allocateframes); 
		 
		//is page swapped in
		if(q[proc].pages[page]) { //YES page swapped in
			passcount[proc]++;
			PFF[proc] = faultcount[proc]/passcount[proc];	
			continue;
		}
		else{ //NO page not swapped in - call pagein() 
			faultcount[proc]++; 
			if(passcount[proc] != 0) PFF[proc] = faultcount[proc]/passcount[proc];
			else PFF[proc] = upperthres;
			
			if(!allocationset && pagein(proc,page)){
				continue;
			}
			else if(allocationset && framecount[proc] < allocateframes[proc] && pagein(proc, page)){
				continue;
			}		
			else if(allocationset && totalpages < PHYSICALPAGES && totalallocated < PHYSICALPAGES && pagein(proc,page)){
				allocateframes[proc]++;
				continue;
			}
			else{ //pagein rejected, need to swap pages... LRU 
				local_lru(q, timestamps, proc, page, preceeds);
			}
		}				
	}

	//make some predictions for ~170 ticks in advance 
	for(i = 0; i < procactive; i++){	
		//calc_alloc_status(q, allocateframes);
		proc = activeq[i];
		pc = q[proc].pc;
		page = pc / PAGESIZE; //determine current page
		pagepred = predict_preceeds(proc, page, preceeds);
		if(pagepred == -1 || pagepred == page) continue; //dont have a prediction yet or already did work above
		if(q[proc].pages[pagepred]) continue; //already in
		pagein(proc, pagepred);
	}	
	
    /* advance time for next pageit iteration */
    tick++;
} 
