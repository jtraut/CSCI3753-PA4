/*
 * File: pager-predict.c
 * Author:       Andy Sayler
 *               http://www.andysayler.com
 * Adopted From: Dr. Alva Couch
 *               http://www.cs.tufts.edu/~couch/
 *
 * Project: CSCI 3753 Programming Assignment 4
 * Create Date: Unknown
 * Modify Date: 2012/04/03
 * Description:
 * 	This file contains a predictive pageit
 *      implmentation.
 */

//try storing the last 5 or so pages then repeat that same sequence
//inc or reduce number of pages based on hit/misses (thrashing frequency)
//might need to use pc and 100 ticks for predictions 
//allocate active and unactive processes, that way swap in the unactive too
//for loop of predictions after for loop of active, then for loop of pagein predictions

#include <stdio.h> 
#include <stdlib.h>

#include "simulator.h" 
int pagepred;
double upperthres = .5;
double lowerthres = .001;
int originalframes;
int totalpages;
int totalallocated;
static int framecount[MAXPROCESSES];
static int allocationset = 1;

int local_lru(Pentry q[MAXPROCESSES], int timestamps[MAXPROCESSES][MAXPROCPAGES], int proc){
	int evictpage, least, pagetmp;
	int success = 0;
	//find a default page
	for(pagetmp = 0; pagetmp < MAXPROCPAGES; pagetmp++){
		if(q[proc].pages[pagetmp]){
			least = timestamps[proc][pagetmp];
			evictpage = pagetmp;
			//printf("do have a default\n");
			break;
		}
	}
	//now look for better -- LRU
	for(pagetmp = 1; pagetmp < MAXPROCPAGES; pagetmp++){
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
	return success;
}

int global_lru(Pentry q[MAXPROCESSES], int timestamps[MAXPROCESSES][MAXPROCPAGES], int proc, int framecount[MAXPROCESSES], int allocateframes[MAXPROCESSES]){
	int outproc, evictpage, least, pagetmp, proctmp;
	least = 0;
	outproc = -1;
	for(proctmp = 0; proctmp < MAXPROCESSES; proctmp++){
		if(allocationset && allocateframes[proctmp] < 5)
			continue;
		for(pagetmp = 0; pagetmp < MAXPROCPAGES; pagetmp++){
			if(!q[proctmp].pages[pagetmp]) //cant evict page thats not in
				continue;
			if(timestamps[proctmp][pagetmp] > least){
				least = timestamps[proctmp][pagetmp];
				evictpage = pagetmp;
				outproc = proctmp;			
				}
			}
		//printf("current LRU timestamp = %d\n", least);
		//call pageout()
		if(q[outproc].pages[evictpage]){ //can only evict page thats in
			pageout(outproc, evictpage);
			allocateframes[outproc]--;
			if(!q[outproc].pages[evictpage])
				framecount[outproc]--;	
		//	printf("evicting global: process %d page %d\n", outproc, evictpage);
		}
		//else printf("could not find global page to evict\n");
	}
	return outproc;
}

void calc_alloc_status(Pentry q[MAXPROCESSES], int allocateframes[MAXPROCESSES]){
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
	calc_alloc_status(q, allocateframes);
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
			//	printf("allocating page for process %d with PFF: %f\n", proctmp, PFF[proctmp]);
			}
			else if(PFF[proctmp] > upperthres/2 && allocateframes[proctmp] < 10){
				allocateframes[proctmp]++;
				totalallocated++;
			//	printf("allocating page for process %d with PFF: %f\n", proctmp, PFF[proctmp]);	
			}
			else if(allocateframes[proctmp] < 8){
				allocateframes[proctmp]++;
				totalallocated++;
			}
			proctmp++;
		}
	}
}

void swap_page(Pentry q[MAXPROCESSES], int timestamps[MAXPROCESSES][MAXPROCPAGES], int proc, int page, double PFF[MAXPROCESSES], int allocateframes[MAXPROCESSES], int framecount[MAXPROCESSES],
	int faultcount[MAXPROCESSES], int passcount[MAXPROCESSES], int proc_status[MAXPROCESSES], int swapping_in[MAXPROCESSES][MAXPROCPAGES]){ 
	//int proctmp;
	//int outproc = -1;
	//int deallocateproc = -1;
	if(allocationset) page_allocation(q, PFF, framecount, allocateframes);
	if(q[proc].pages[page]){
		swapping_in[proc][page] = 0;
		passcount[proc]++;
		PFF[proc] = faultcount[proc]/passcount[proc];
		return;
	}
	else{
		faultcount[proc]++;
		if(passcount[proc] != 0) PFF[proc] = faultcount[proc]/passcount[proc];
		else PFF[proc] = upperthres;
		//printf("attempting to swap in process %d page %d\n", proc, page);		
		if((allocationset && framecount[proc] < allocateframes[proc] && pagein(proc,page)) 
			|| (!allocationset && pagein(proc,page))){
			//printf("pagein was successful for process %d page %d\n", proc, page);
			proc_status[proc] = 0; //called pagein, set status back to 0
			if(q[proc].pages[page]){
				framecount[proc]++;	
				swapping_in[proc][page] = 0;
				//printf("and page was swapped in, framecount for process %d at %d, PFF is: %f\n",proc,framecount[proc], PFF[proc]);
			}
			else swapping_in[proc][page] = 1;
			return;
		}
	/*
		else if(allocationset && totalpages < PHYSICALPAGES && allocateframes[proc] < MAXPROCPAGES
			&& totalallocated < PHYSICALPAGES && pagein(proc,page)){
			//printf("pagein successful for process %d page %d\n", proc, page);
			proc_status[proc] = 0;
			allocateframes[proc]++;
			printf("raising frame allocation for process %d to %d\n", proc, allocateframes[proc]);
			if(q[proc].pages[page]){
				framecount[proc]++;	
				swapping_in[proc][page] = 0;
				processblock[proc] = 0;
				//printf("and page was swapped in, framecount for process %d at %d, PFF is: %f\n",proc,framecount[proc], PFF[proc]);
			}
			else swapping_in[proc][page] = 1;
			return;
		}
	*/	
		//look for a page to evict - LRU (if not already waiting on a pageout)
		if((allocationset && !proc_status[proc] && framecount[proc] == allocateframes[proc]-1) || (!allocationset && !proc_status[proc])){
			swapping_in[proc][page] = 0;
		/*	
			if(allocationset && PFF[proc] >= upperthres/2 && allocateframes[proc] >= 12 && allocateframes[proc] <= 15 
				&& global_lru(q, timestamps, proc, framecount, allocateframes, swapping_in) != -1){
				 //printf("searching global\n");
				 allocateframes[proc]++;
			 }
			else 
			*/
			if(local_lru(q, timestamps, proc)){
			//printf("page swap successful, framecount for process %d at %d, PFF is: %f\n",proc,framecount[proc],PFF[proc]);
				proc_status[proc] = 1; //process will wait till a pagein before calling another pageout
			//printf("but made it down here\n");
			}
		}
	}		
	
}

int predict_preceeds(int proc, int page, int preceeds[MAXPROCESSES][MAXPROCPAGES][MAXPROCPAGES]){
	int pagetmp;
	int mostfreq = -1;
	pagepred = -1;
	//check if a page frequently follows another
	for(pagetmp = 0; pagetmp < MAXPROCPAGES; pagetmp++){
		if(preceeds[proc][page][pagetmp] > mostfreq){
				mostfreq = preceeds[proc][page][pagetmp];
				pagepred = pagetmp;
				//printf("updating mostfreq to %d for page %d\n", mostfreq, pagetmp);
		}	
	}
	// if(mostfreq > 5) printf("predicted most frequent (%d) page: %d\n", mostfreq, pagepred);
	 return pagepred;
}

void pageit(Pentry q[MAXPROCESSES]) { 
    
    /* This file contains the stub for a predictive pager */
    /* You may need to add/remove/modify any part of this file */

    /* Static vars */
    static int initialized = 0;
    static int tick = 1; // artificial time
    static int timestamps[MAXPROCESSES][MAXPROCPAGES]; //help with LRU
    static int preceeds[MAXPROCESSES][MAXPROCPAGES][MAXPROCPAGES]; //frequency of what comes after what
    static int proc_status[MAXPROCESSES];
    static int goodpredict = 0;
    static int miss = 0;
    static double PFF[MAXPROCESSES];
    static int allocateframes[MAXPROCESSES];
 //   static int framecount[MAXPROCESSES];
    static int faultcount[MAXPROCESSES];
    static int passcount[MAXPROCESSES];
    static int swapping_in[MAXPROCESSES][MAXPROCPAGES];
    static int predictions[MAXPROCESSES];
    
    /* Local vars */
    int proctmp, pagetmp, proc, pc, page, pagepred, nextpg; //lastpg; //lastproc, lastpage;  
    int procactive = 0;
    //btw none of these are actually static rn
    int activeq[MAXPROCESSES];
    static int missrate[MAXPROCESSES]; //reset every 10 or so...
    static int hitrate[MAXPROCESSES]; //same
    int successrate[MAXPROCESSES];
    static int prevpage[MAXPROCESSES];
    static int lastpc[MAXPROCESSES][MAXPROCPAGES] = {{0}};
    static int pchold[MAXPROCESSES][MAXPROCPAGES] = {{0}};
	
    /* initialize static vars on first run */
    if(!initialized){
	/* Init complex static vars here */
	for(proctmp=0; proctmp < MAXPROCESSES; proctmp++){
		missrate[proctmp] = 0;
		hitrate[proctmp] = 0;
		proc_status[proctmp] = 0;
		PFF[proctmp] = 0;
		allocateframes[proctmp] = 5;
		framecount[proctmp] = 0;
		faultcount[proctmp] = 0;
		passcount[proctmp] = 0;
		prevpage[proctmp] = -1;
		predictions[proctmp] = -1;
	//	lastpc2[proctmp] = 0;
	    for(pagetmp=0; pagetmp < MAXPROCPAGES; pagetmp++){
			timestamps[proctmp][pagetmp] = 0; 
			swapping_in[proctmp][pagetmp] = 0;
			for(nextpg=0; nextpg < MAXPROCPAGES; nextpg++){
				preceeds[proctmp][pagetmp][nextpg] = -1;
			}
	    }
	}	
	initialized = 1;
    }
    
    /* TODO: Implement Predictive Paging */
    //init prediction array 
 /* 
   	for(proctmp=0; proctmp < MAXPROCESSES; proctmp++){
	    for(pagetmp=0; pagetmp < MAXPROCPAGES; pagetmp++){	
			//predictions[proctmp][pagetmp] = -1;
	    }
	}	
 */
    //select process
    for(proctmp = 0; proctmp < MAXPROCESSES; proctmp++){
		//find active processes and update 
		if(q[proctmp].active){
			activeq[procactive] = proctmp;
			procactive++;
			page = q[proctmp].pc / PAGESIZE;
	//		timestamps[proctmp][page] = tick;
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
	
	//this killed me, I mean DAYS 
	if(procactive == 0) return;
	
	if(procactive < 7){
		allocationset = 0;
		//printf("down to the last %d processes\n", procactive);
	}	
	//originalframes = (int)PHYSICALPAGES/procactive;
	
	//printf("beginning a new loop\n");	
	//printf("down to the last %d processes\n", procactive);
	
	int i;
	//all active processes
    for(i = 0; i < procactive; i++){
		proc = activeq[i];
		pc = q[proc].pc;
		page = pc / PAGESIZE; //determine current page	
		
		//printf("proc %d pc is %d and page %d\n", proc, pc, page);
		//printf("last pc was %d and page was %d\n", lastpc[proc], prevpage[proc]);
		for(pagetmp = 0; pagetmp < MAXPROCPAGES; pagetmp++){
			//looking for what comes around 100 ticks later 
			if(pchold[proc][pagetmp] && pc >= lastpc[proc][pagetmp]+200){
				pchold[proc][pagetmp] = 0;
				preceeds[proc][pagetmp][page]++;
				if(pc == lastpc[proc][pagetmp])
					preceeds[proc][pagetmp][page]++;
				// if(procactive < 5) printf("current frequency for proc %d page %d to %d ~100ticks later: %d\n", proc, pagetmp, page, preceeds[proc][pagetmp][page]);
			}
		}
		if(!pchold[proc][page]){
			lastpc[proc][page] = pc;
			pchold[proc][page] = 1;
		}
		
/*
		if(predictions[proc] != -1 && page != predictions[proc]){
			printf("MISS: current page: %d, previously predicted: %d\n", page, predictions[proc]);
			miss++;
			missrate[proc]++;
		}
		else if(page == predictions[proc]){
			 printf("HIT: current page: %d, previously predicted: %d\n", page, predictions[proc]);
			 goodpredict++;
			 hitrate[proc]++;
		}
		successrate[proc] = hitrate[proc] - missrate[proc];
		//printf("success rate: %d\n", successrate[proc]);
		
		// run page allocation to see if can allocate better
		if(allocationset) page_allocation(q, PFF, framecount, allocateframes);
		//else calc_alloc_status(q, allocateframes);
				
	//	if(procactive < 10) printf("total pages used: %d, total allocated: %d, process %d using %d and allocated %d\n", totalpages, totalallocated, proc, framecount[proc], allocateframes[proc]);
			
		prevpage[proc] = page;
		
		if(q[proc].pages[page]){
			//if(procactive == 1) printf("page is already in for process %d page %d\n", proc, page);
			swapping_in[proc][page] = 0;	
			passcount[proc]++;
			PFF[proc] = faultcount[proc]/passcount[proc];
			continue;
		}	
		else if(!swapping_in[proc][page]){ 	
			//if(procactive == 1) printf("swapping page in for process %d page %d\n", proc, page);		
			swap_page(q, timestamps, proc, page, PFF, allocateframes, framecount, faultcount, passcount, proc_status, swapping_in);
		}
	*/	
	
			//update other pages timestamp
		for(pagetmp = 0; pagetmp < MAXPROCPAGES; pagetmp++){
			timestamps[proc][pagetmp]++;
		}
		//and reset the counter for most recent 
		timestamps[proc][page] = 0;				
		
		// run page allocation to see if can allocate better
		if(allocationset) page_allocation(q, PFF, framecount, allocateframes);
		//calc_alloc_status(q, allocateframes);		
		//if(procactive < 10) printf("total pages used: %d, total allocated: %d, process %d using %d and allocated %d\n", totalpages, totalallocated, proc, framecount[proc], allocateframes[proc]);
		
		//is page swapped in
		if(q[proc].pages[page]) { //YES page swapped in
			continue;
		}
		else{ //NO page not swapped in - call pagein() 
			if(pagein(proc,page)){
				//	printf("inserting process %d page %d\n", page, proc);
				continue;
			}
			//pagein rejected, need to swap pages... LRU 	
			else{
				local_lru(q, timestamps, proc);
			}
		}				
	}
	
	if(allocationset) page_allocation(q, PFF, framecount, allocateframes);
	int lastpg = -1;
	//time to make some predictions 
	for(i = 0; i < procactive; i++){	
		//calc_alloc_status(q, allocateframes);
		proc = activeq[i];
		pc = q[proc].pc;
		page = pc / PAGESIZE; //determine current page
	//	for(pagetmp = 0; pagetmp < MAXPROCPAGES; pagetmp++){
			predictions[proc] = predict_preceeds(proc, page, preceeds);
			pagepred = predictions[proc];
			if(pagepred == -1) continue; //dont have a prediction yet
			if(q[proc].pages[pagepred]) continue; //already in
			pagein(proc, pagepred);
	//	}
	}
	
	/*
	for(proctmp = 0; proctmp < MAXPROCESSES; proctmp++){
		for(pagetmp=0; pagetmp < MAXPROCPAGES; pagetmp++){
			//if(q[proctmp].pages[pagetmp]) printf("for process %d, page %d is in\n",proctmp,pagetmp);
			//nextpg = predict_preceeds(proctmp, pagetmp, preceeds);
			//printf("proccess %d page %d preceeds %d with frequency: %d\n", proctmp, pagetmp, nextpg, preceeds[proctmp][pagetmp][nextpg]); 
		}
	}
	*/
		
    /* advance time for next pageit iteration */
    tick++;
    //and keep stats updated for next call
	//if(procactive == 1) printf("miss count: %d, good predictions: %d\n", miss, goodpredict);
} 
