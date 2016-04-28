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
//inc or reduce number of pages based on hit/misses

#include <stdio.h> 
#include <stdlib.h>

#include "simulator.h" 
int pagepred = 0;
int serieslength = 6;

void swap_page(Pentry q[MAXPROCESSES], int timestamps[MAXPROCESSES][MAXPROCPAGES], int proc, int page, int current, int hotpage[MAXPROCESSES][MAXPROCPAGES], int prediction[MAXPROCESSES][MAXPROCPAGES]){ 
		int pagetmp, outproc, evictpage, least, hotness;
		//is page swapped in
		current++;
		if(!q[proc].pages[page]){ //page not swapped in
			if(!pagein(proc,page)){ //failure - select a page to evict		
				//find a default page	
				for(pagetmp = 0; pagetmp < MAXPROCPAGES; pagetmp++){
					if(q[proc].pages[pagetmp] && timestamps[proc][pagetmp] > 0){
						least = timestamps[proc][pagetmp];
						evictpage = pagetmp;
						outproc = proc;
						hotness = hotpage[proc][page];
						//printf("do have a default\n");
						break;
					}
				}
		
				//look for better - LRU
				for(pagetmp = 1; pagetmp < MAXPROCPAGES; pagetmp++){
					if(!q[proc].pages[pagetmp]) //cant evict page thats not in
						continue;
					if(timestamps[proc][pagetmp] > least && !prediction[proc][page]){
						least = timestamps[proc][pagetmp];
						evictpage = pagetmp;
						outproc = proc;
					//	hotness = hotpage[proctmp][pagetmp];
					//	printf("current LRU timestamp = %d\n", least);
					}
					if(hotpage[proc][pagetmp] < hotness){
						hotness = hotpage[proc][pagetmp];
				//		evictpage = pagetmp;
				//		outproc = proc;
					}
				}
				//call pageout()
				if(q[outproc].pages[evictpage]){ //can only evict page thats in
					pageout(outproc, evictpage);				
					//and if have prediction put it in so its there before needed
					pagein(proc, page);
				}
			}
		}		
}

int predict_hot(int proc, int hotpage[MAXPROCESSES][MAXPROCPAGES]){
	int pagetmp;
	int hotness = -1;
		//or predict next page from hotness
	for(pagetmp = 0; pagetmp < MAXPROCPAGES; pagetmp++){
		if(hotpage[proc][pagetmp] > hotness){
			hotness = hotpage[proc][pagetmp];
			pagepred = pagetmp; //hottest page rn
		}
	}	
	//printf("predicted hot page: %d\n", pagepred); 
	return pagepred;
}

int predict_preceeds(int proc, int page, int preceeds[MAXPROCESSES][MAXPROCPAGES][MAXPROCPAGES]){
	int pagetmp;
	int mostfreq = -1;
	//check if a page frequently follows another
	for(pagetmp = 0; pagetmp < MAXPROCPAGES; pagetmp++){
		if(preceeds[proc][page][pagetmp] > mostfreq){
				mostfreq = preceeds[proc][page][pagetmp];
				pagepred = pagetmp;
		}	
	}
	// printf("predicted frequent page: %d\n", pagepred);
	 return pagepred;
}

int predict_repeats(int proc, int page, int repeatcount[MAXPROCESSES], int repeats[MAXPROCESSES][MAXPROCPAGES]){
	//is it a repetitive page, that hasn't hit its expected #
	if((repeats[proc][page] >= 1 && repeatcount[proc] < repeats[proc][page]) || repeats[proc][page] > 8){
	//	printf("predicted repeat page: %d\n", page); 
		pagepred = page;
	}
	//printf("repeats value: %d, repeat counter: %d, for page: %d\n", repeats[proc][page], repeatcount, page);
	return pagepred;
}
	
int predict_series(int proc, int series[MAXPROCESSES][serieslength], int seriescount[MAXPROCESSES]){
	if(seriescount[proc] < serieslength)
		pagepred = series[proc][seriescount[proc]];
	//printf("next in series: %d\n", pagepred);
	return pagepred;
}

void pageit(Pentry q[MAXPROCESSES]) { 
    
    /* This file contains the stub for a predictive pager */
    /* You may need to add/remove/modify any part of this file */

    /* Static vars */
    static int initialized = 0;
    static int tick = 1; // artificial time
    static int timestamps[MAXPROCESSES][MAXPROCPAGES]; //help with LRU
    static int hotpage[MAXPROCESSES][MAXPROCPAGES]; //help choosing new page
    static int repeats[MAXPROCESSES][MAXPROCPAGES]; //predict repeating pages
    static int repeatset[MAXPROCESSES][MAXPROCPAGES]; //when to update repeats    
    static int preceeds[MAXPROCESSES][MAXPROCPAGES][MAXPROCPAGES]; //frequency of what comes after what
    static int proc_status[MAXPROCESSES];
    static int predictions[MAXPROCESSES][MAXPROCPAGES];
    static int goodpredict = 0;
    static int miss = 0;
    
    /* Local vars */
 //   int hotpage[MAXPROCESSES][MAXPROCPAGES]; //help choosing new page
 //   int repeatset[MAXPROCESSES][MAXPROCPAGES]; //when to update repeats   
 //   int repeats[MAXPROCESSES][MAXPROCPAGES]; //predict repeating pages
 //   int preceeds[MAXPROCESSES][MAXPROCPAGES][MAXPROCPAGES]; //frequency of what comes after what
  //  int serieslength = 5;
    int series[MAXPROCESSES][serieslength];
    int seriescount[MAXPROCESSES] = { 0 };
    int seriesfull[MAXPROCESSES] = { 0 };
    int proctmp, pagetmp, proc, pc, page, pagepred, nextpg, lastproc, lastpage;  
    int procactive = 0;
    //btw none of these are actually static rn
    int activeq[MAXPROCESSES];
    int goodpredictor[MAXPROCESSES][4] = { { 0 } }; //maybe add a proc dimension
    int bestoption = 0;
    static int missrate[MAXPROCESSES]; //reset every 10 or so...
    static int hitrate[MAXPROCESSES]; //same
    int successrate[MAXPROCESSES];
    static int repeatcount[MAXPROCESSES];
    static int samepage[MAXPROCESSES];
    static int currentpredictor[MAXPROCESSES] = { 1 };
    static int prevpredictor[MAXPROCESSES] = { 1 };
    int prevpage[MAXPROCESSES] = { -1 };
    int prevpred[MAXPROCESSES] = { -1 };
  //  static int pagelock[MAXPROCESSES][MAXPROCPAGES];
	int current;
	static int prediction[MAXPROCESSES][MAXPROCPAGES];
	
    /* initialize static vars on first run */
    if(!initialized){
	/* Init complex static vars here */
	for(proctmp=0; proctmp < MAXPROCESSES; proctmp++){
		missrate[proctmp] = 0;
		hitrate[proctmp] = 0;
		repeatcount[proctmp] = 0;
		samepage[proctmp] = 0;
		proc_status[proctmp] = 0;
	    for(pagetmp=0; pagetmp < MAXPROCPAGES; pagetmp++){
			prediction[proctmp][pagetmp] = 0;
			timestamps[proctmp][pagetmp] = 0; 
			hotpage[proctmp][pagetmp] = 0;
			repeats[proctmp][pagetmp] = 0;
			repeatset[proctmp][pagetmp] = 0;
		//	pagelock[proctmp][pagetmp] = 0;
			for(nextpg=0; nextpg < MAXPROCPAGES; nextpg++){
				preceeds[proctmp][pagetmp][nextpg] = 0;
			}
	    }
	}	
	initialized = 1;
    }
    
    /* TODO: Implement Predictive Paging */
    //init prediction helpers
   
   	for(proctmp=0; proctmp < MAXPROCESSES; proctmp++){
	    for(pagetmp=0; pagetmp < MAXPROCPAGES; pagetmp++){
		//	hotpage[proctmp][pagetmp] = 0;
		//	repeatset[proctmp][pagetmp] = 0;
		//	repeats[proctmp][pagetmp] = 0;
		//	for(nextpg=0; nextpg < MAXPROCPAGES; nextpg++){
		//		preceeds[proctmp][pagetmp][nextpg] = 0;
		//	}		
			if(pagetmp < serieslength)
				series[proctmp][pagetmp] = -1;
	    }
	}	
   
    //select process
    for(proctmp = 0; proctmp < MAXPROCESSES; proctmp++){
		//find active processes and update 
		if(q[proctmp].active){
			activeq[procactive] = proctmp;
			procactive++;
			page = q[proctmp].pc / PAGESIZE;
			timestamps[proctmp][page] = tick;
		}
		else { //swap out any unactive pages
			for(pagetmp = 0; pagetmp < MAXPROCPAGES; pagetmp++){
				if(q[proctmp].pages[pagetmp])
					pageout(proctmp,pagetmp);
			}
		}
	}
	lastproc = 0;
	lastpage = 0;
	int i, j;
	//all active processes
    for(i = 0; i < procactive; i++){
		proc = activeq[i];
		pc = q[proc].pc;
		page = pc / PAGESIZE; //determine current page
		
		//printf("previous proccess %d and previous page %d\n", lastproc, lastpage);
		//printf("current process %d and current page %d\n", proc, page);
		prediction[proc][page] = 0;
		
		if(prevpred[proc] != -1 && page != prevpred[proc]){
			//printf("MISS: current page: %d, previously predicted: %d\n", page, prevpred[proc]);
			miss++;
			missrate[proc]++;
		//	pagelock[proc][prevpred[proc]] = 0;
			goodpredictor[proc][prevpredictor[proc]-1]--;
		}
		else if(page == prevpred[proc]){
			// printf("HIT: current page: %d, previously predicted: %d\n", page, prevpred[proc]);
			 goodpredict++;
			 hitrate[proc]++;
			 goodpredictor[proc][prevpredictor[proc]-1]++; 
		//	 pagelock[proc][page] = 1;
		}
		successrate[proc] = hitrate[proc] - missrate[proc];
	//	printf("success rate: %d\n", successrate[proc]);
		
		//update other pages for LRU
		for(pagetmp = 0; pagetmp < MAXPROCPAGES; pagetmp++){
			timestamps[proc][pagetmp]++;
		}
		//and reset the counter for most recent 
		timestamps[proc][page] = 0;		
		
		//update hotness for choosing a new page
		/*
		if(hotpage[proc][page] > 150){ //hot cap
			for(pagetmp = 0; pagetmp < MAXPROCPAGES; pagetmp++)
				hotpage[proc][pagetmp] = 0;
			hotpage[proc][page] = 5;
		}	
		else hotpage[proc][page]++;
		*/
		hotpage[proc][page]++;
		//update repeats for predicting sequential calls
		if(page == prevpage[proc]){
			if(!repeatset[proc][page]){
				samepage[proc]++;
				repeats[proc][page] = samepage[proc];
			}
			repeatcount[proc]++;
		}
		else if(repeats[proc][prevpage[proc]] > 0){
			repeatset[proc][prevpage[proc]] = 1;
			if(repeats[proc][prevpage[proc]] > 6){ 
				repeats[proc][prevpage[proc]] = 0;
				repeatset[proc][prevpage[proc]] = 0;	
			}
			samepage[proc] = 0;
			repeatcount[proc] = 0;
		}
		//printf("current page: %d repeats %d times\n", page, repeats[proc][page]);	
	
		//update preceeds
		if(prevpage[proc] != -1){
			preceeds[proc][prevpage[proc]][page]++;
			/*
			if(preceeds[proc][prevpage[proc]][page] > 100){
				//reset the frequencies 
				for(pagetmp=0; pagetmp < MAXPROCPAGES; pagetmp++){
					for(nextpg=0; nextpg < MAXPROCPAGES; nextpg++){
						preceeds[proc][pagetmp][nextpg] = 0;
					}	
				preceeds[proc][prevpage[proc]][page] = 5;
				}	
			}
			*/
		//	printf("page %d preceeds %d with frequency of: %d\n", prevpage[proc], page, preceeds[proc][prevpage[proc]][page]); 
		}
	
		//update the series
		if(seriescount[proc] < serieslength){
			series[proc][seriescount[proc]] = page;
			seriescount[proc]++;
			if(seriescount[proc] == serieslength){
				seriescount[proc] = 0;
				seriesfull[proc] = 1;
			}
		}
		else{ //series if full, is it working or not...
			if(successrate[proc] < -1){ //nope - start a new one
				for(pagetmp = 0; pagetmp < MAXPROCPAGES; pagetmp++){
					for(j = 0; j < serieslength; j++){
						series[proc][j] = -1;
					}
				}
				seriescount[proc] = 0;
				seriesfull[proc] = 0;
			}
			//or don't change it
		}
		
		//how's the current method doing
		if(successrate[proc] >= 0){ //good enough	
			//maybe prevpredictor == currentpredictor...
			if(currentpredictor[proc] == 1)
				pagepred = predict_hot(proc, hotpage);	
			else if(currentpredictor[proc] == 2)
				pagepred = predict_repeats(proc, page, repeatcount, repeats);
			else if(currentpredictor[proc] == 3 && seriescount[proc] < serieslength)
				pagepred = predict_series(proc, series, seriescount);
			else
				pagepred = predict_preceeds(proc, page, preceeds);
		}
		if(successrate[proc] < 0){
			//make prediction method decisions here
			if(bestoption <= 5){
				int stop = 0;
				for(pagetmp = 0; (pagetmp < MAXPROCPAGES) && !stop; pagetmp++){
					if(preceeds[proc][page][pagetmp] > 30){
						currentpredictor[proc] = 4; //better chances here
						stop = 1; 
					}
				}
				for(pagetmp = 0; (pagetmp < MAXPROCPAGES) && !stop; pagetmp++){
					if(hotpage[proc][pagetmp] > 50){
						currentpredictor[proc] = 1; //maybe there's a hot page to swap
						stop = 1;
					}
				}
				if(repeats[proc][page] > 1 && !stop){
					 currentpredictor[proc] = 2;
					 stop = 1;
				}
				else if(seriesfull[proc] == 1 && !stop){
					currentpredictor[proc] = 3;
					stop = 1;
				}
				bestoption = goodpredictor[proc][j];
			}
			else{
				for(j = 0; j < 4; j++){
					if(goodpredictor[proc][j] > bestoption){
						bestoption = goodpredictor[proc][j];
						currentpredictor[proc] = j+1;
					}
				}
				if(bestoption > 20){
					for(j = 0; j < 4; j++)
						goodpredictor[proc][j] = 0;
					goodpredictor[proc][j] = 2;
					bestoption = 0;
				}
			}
		//	printf("current best predictor is %d with score %d\n", currentpredictor[proc], bestoption);
			currentpredictor[proc] = 4;
			if(currentpredictor[proc] == 1)
				pagepred = predict_hot(proc, hotpage);
			else if(currentpredictor[proc] == 2)
				pagepred = predict_repeats(proc, page, repeatcount, repeats);
			else if(currentpredictor[proc] == 3 && seriescount[proc] < serieslength)
				pagepred = predict_series(proc, series, seriescount);		
			else
				pagepred = predict_preceeds(proc, page, preceeds);
		}
		//printf("current method: %d\n", currentpredictor[proc]);
		prevpredictor[proc] = currentpredictor[proc];
		timestamps[proc][pagepred] = 0;
		
		if(!q[proc].pages[page]){
			current = 1;		
			swap_page(q, timestamps, proc, page, current, hotpage, prediction);
		}
		if(successrate[proc] >= 20){
			int evictpage, outproc, least;
			nextpg = page;
			for(j = 0; j < 5; j++){
				least = 0;
				for(pagetmp = 0; pagetmp < MAXPROCPAGES; pagetmp++){
					if(!q[proc].pages[pagetmp]) //cant evict page thats not in
						continue;
					if(timestamps[proc][pagetmp] > least){
						least = timestamps[proc][pagetmp];
						evictpage = pagetmp;
						outproc = proc;
					//	hotness = hotpage[proctmp][pagetmp];
					//	printf("current LRU timestamp = %d\n", least);
					}

				}
				//call pageout()
				if(q[outproc].pages[evictpage]){ //can only evict page thats in
					pageout(outproc, evictpage);				
				}
				
				nextpg = predict_preceeds(proc, nextpg, preceeds);
				swap_page(q, timestamps, proc, nextpg, current, hotpage, prediction);
			}
		}
		lastproc = proc;
		lastpage = page;
		prevpage[proc] = page;
		prevpred[proc] = pagepred;
	}
    /* advance time for next pageit iteration */
    tick++;
	//printf("miss count: %d, good predictions: %d\n", miss, goodpredict);
} 
