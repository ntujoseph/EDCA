/*
 For WiFi EDCA Simulation
 Joseph (Yi-Fang,Chen)
 Date: Dec 29, 2016
 
 http://www.invocom.et.put.poznan.pl/~invocom/C/P1-4/p1-4_en/p1-4_7_4.htm
*/
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <string.h>
#define max(a,b) (((a)>(b))?(a):(b)) 
#define min(a,b) (((a)<(b))?(a):(b))  
#define BE 3  
#define MAX_NB 10  //maximum backoff slot
#define MAX_FRAME_SIZE 10 
#define N 4  // max number of nodes
int node_q=2; //initial value for the number of node
#define MAX_ROUND_TEST 10 // average the result 
#define FREEZE 1  //1: enable; 0:disable

#define MAX_TIME 10000
int frame_size=MAX_FRAME_SIZE; //initial data lenth




#define WRITE_FILE 0  //1: enable; 0:disable
#define JOSEPH_DEBUG 0

#if JOSEPH_DEBUG==1
#define dbg_printf(fmt, s...)	do{printf(fmt, ##s);}while(0)  
#else
#define dbg_printf(fmt, s...)
#endif	

enum State {START,BACKOFF,DIFS,TRANSMIT,COMPLETE,FAILURE};
char Str_State[][16]={"Start","Backoff","DIFS","Transmit","Complete","Failure"};
enum Ch_State {IDLE=0,BUSY,COLLISION};
char Str_Ch_State[][16]={"Idle","Busy","Collision"};

#define PRINT_FMT "%d\t%6.3lf\n" 


typedef struct {    
enum Ch_State state;     //channel state
enum Ch_State pre_state;     //previous channel state
char users[N];  //active user list
int n_users;    //the number of active  user
unsigned long end_of_time;
} Channel;

Channel ch0;
int round_i=1;
FILE *fp;  //for different L


unsigned long long total_time=0; //used for average

typedef struct
{
  int num_collisions;
  int num_success;
  int num_failure;  //> MAX_NB
  unsigned long occupied_time;
  
  
} Statistic;

typedef struct
{
  char id;
  int ifs;  
  int cw_min;  
  int cw_max;  
  int var_ifs;
  int be; //backoff exponent
  int nb; //the number of backoff
  int cw; //backoff period (//contention window)
  int d_len;  //frame length
  enum Ch_State ch_state;
  enum State state;
  Statistic info;
}Node;





typedef Node * pNode;

//----function list----------
void init(pNode p,int size);
void reset_node (pNode p);
void show_state (pNode p,int size);
int update_channel_state(pNode p,int size);
void do_backoff(pNode p);
void show_report(pNode p,int size);
void show_round_report(pNode p,int size);
int main()
{
 
 int i;
 unsigned long t=0;
 Node node[N];
 char filename[32];

   //for random number , call only once  
   srand(time(0)); 
                                 
for (;frame_size<=MAX_FRAME_SIZE;frame_size++) {   
	dbg_printf("Frame size=%d==>\n",frame_size);
	#if WRITE_FILE==1 
	sprintf(filename,"L%d.txt",frame_size);
	if ((fp = fopen(filename, "w+")) == NULL)
	{
		 fprintf(stderr,"Error! Can't create %s\n",filename);
		 exit(1);
	} 
	#endif  
	for(;node_q<=N;node_q++) { 
	
		dbg_printf("Number of node=%d:\n",node_q);                               
                           
		for (round_i=1;round_i<=MAX_ROUND_TEST;round_i++) {     
			dbg_printf("Round%d:\n",round_i);    
            init(node,N);
			update_channel_state(node,node_q);  
			
			 dbg_printf(">>>>>>Channel 0 State:%s\n",Str_Ch_State[ch0.state]);
			for(t=0;t<MAX_TIME;t++)
			{ 
				dbg_printf("t=%ld:\n",t);
			
			    
				 show_state(node,node_q);  
			  
				
   		 
  
				for(i=0;i<node_q ;i++) {
	//-----------State Machine------------------------------------------------------------------------------			

					if (node[i].state==COMPLETE || node[i].state==FAILURE)
						continue;
			  
           
			   
				   switch (ch0.state) {
  
					    //wait for Idle long than DIFS, and do random backoff(contention window), if cw=0 enter transmit state 
						
					   case IDLE:
	                         
  	                         if(node[i].var_ifs >= node[i].ifs) {
								  if (node[i].state==START) {//new transmit or re-transmit	
                                     dbg_printf("%c sensing is longer than DIFS, so wait random backoff time\n",node[i].id);								  
							         do_backoff(&node[i]);  
        					        								 
							         
								 }
							 }
							  //in backoff state and cw count to 0
							 if (node[i].state==BACKOFF && node[i].cw <=0) { 
									   node[i].cw=0;
									   node[i].state=TRANSMIT;	
                                       node[i].d_len--;	
									   node[i].info.occupied_time++;
									   node[i].nb=0;
												   
							 } else if (node[i].state==BACKOFF)	{
								   if (ch0.state==IDLE && node[i].var_ifs >= node[i].ifs ) {
										node[i].cw--;  	
										if(node[i].cw<0) node[i].cw=0;		
											
										
								   }
							 }
							 
							  node[i].var_ifs++;
					
					        break;

							
					   case BUSY: 		
                            node[i].var_ifs=0;					   
							if (node[i].state==TRANSMIT) {
								     node[i].d_len--;	
									 node[i].info.occupied_time++;
									 node[i].nb=0;
							}
							if(node[i].d_len==0) {
								node[i].state=COMPLETE;	 
                                dbg_printf("%c: complete\n",node[i].id);
								node[i].info.num_success++;
								//never stop, transmit agiain and again & forever
								node[i].state=START;	
                                reset_node(&node[i]);								
								
							}								
					
					       break;
					
					   case COLLISION: 
                           node[i].var_ifs=0;		
						   
						 if (node[i].state==TRANSMIT) { 
								 node[i].d_len--;		
								 node[i].info.occupied_time++;
						 }							   
						
						  if(node[i].d_len==0) {						 
							 reset_node(&node[i]);
							 do_backoff(&node[i]);					 
                             if(node[i].cw==0) { 
							    node[i].state=START;	
                             }else 
								 node[i].cw--; 
                             							 
                            
							node[i].info.num_collisions++;
						}	                                
                         
												 
													
					 
					      break;					   
					   
				 } //end switch 	   
		
                     				
			 dbg_printf("%c:,D=%d,time %ld,state=%s,cw=%d\n", node[i].id,node[i].d_len,node[i].info.occupied_time,Str_State[node[i].state],node[i].cw) ; 
		    
			
                }

           
		
  //----------------------------------------------------------------------------------------------------------------------
			
				//after this time slot, we check the channel state 
				update_channel_state(node,node_q);  
				 dbg_printf(">>>>>>Channel 0 State:%s\n",Str_Ch_State[ch0.state]);
				 ch0.end_of_time=t;
  			} // end for loop t
			show_report(node,node_q);  
		} //end for loop each round 
       //end of round test, output report 
		show_round_report(node,node_q);  
	} //end for loop node size
  
	#if WRITE_FILE==1   
    if (fclose(fp) != 0)
		fprintf(stderr,"Error closing file\n");
    #endif
 }  //end for frame_size increament  
 

return 0;
}


///========================================================
void init(pNode p,int size)
{
   
     
    int i;   
   //init node 
   
   for(i=0;i<size;i++) {
	 memset(&p[i],0,sizeof(Node));
     p[i].id='A'+i;
 
	 p[i].ifs=1*(i+1);	
	 p[i].cw_min=(p[i].ifs)*4;
     p[i].cw_max=8;
	 
	 
     reset_node(&p[i]);    
    	 
     }  
	 
	//for 1:1
	#if N>=4
    p[2].ifs=p[3].ifs; 
	p[2].cw_max=p[3].cw_max; 
	p[2].cw_min=p[3].cw_min; 

   #endif
   
   //init channel
   memset(&ch0,0,sizeof(Channel));

		 
	/* 若channel一開始為有人在傳送, 且每個人一開始就想傳,
     所以一開始就要做do_backoff()	
	*/	 
	/*
	 for(i=0;i<size;i++) {
          do_backoff(&p[i]);
     }	 
			 
    */
    
}    

void reset_node (pNode p)
{
   //reset to constant value  
     p->be=BE;
     p->cw=0;
	 p->var_ifs=0;
     p->d_len=frame_size;
     p->state=START; //init state     
   	 
               
     
} 

void show_report(pNode p,int size)
{
  int i;
  double T=0;
  static int line=0;  
   float ratio;

  //list throughput for each node
	   
  if (line++==0) {
   printf("-----------------------------------\n");
  }

  for(i=0;i<size;i++) {
     //T=(double)frame_size*p[i].info.num_success/p[i].info.occupied_time;
	  ratio=(float)p[i].info.occupied_time/MAX_TIME;
	  printf("%c (AIFS=%d,cwmax=%d,cwmin=%d), channel access time=%-3ld,success=%d,collsion=%d,failure=%d,ratio=%6.2lf\n",
	        p[i].id,p[i].ifs,p[i].cw_max,p[i].cw_min,p[i].info.occupied_time,p[i].info.num_success,p[i].info.num_collisions, p[i].info.num_failure,ratio);      
   }  
   
     for(i=0;i<size;i++) {
   	  printf("ratio=%6.2lf\n",(float) p[i].info.occupied_time/p[0].info.occupied_time);      
   } 
    printf("\n===========================\n");    
       
 
}      




void show_round_report(pNode p,int size)
{
  int i;

  static int line=0;  
  char wbuf[64];

  printf("Round=%d\tL=%d\tN=%d\n",MAX_ROUND_TEST,frame_size,size);  
 
  #if WRITE_FILE==1   
  sprintf(wbuf,PRINT_FMT,size,avg_overall_T);      
  fprintf(fp,"%s",wbuf);    
  #endif  

}      


void show_state (pNode p,int size)
{
    int i;   
   for(i=0;i<size;i++) {
      if(p[i].state==COMPLETE) continue; 
	  

   dbg_printf("%c:AIFS=%d,cwmax=%d,cwmin=%d,BE=%d,NB=%d,cw=%d,D=%d,var_ifs=%d\n",
       p[i].id,p[i].ifs,p[i].cw_max,p[i].cw_min,p[i].be,p[i].nb,p[i].cw,p[i].d_len,p[i].var_ifs) ;           
    }

  	
   
}           


//@return :  number of users is in transmit 
// c=0 , channel idle
// c=1 , someone is transmiting (maybe the yourself)
// c=2 , must be collision
// if c==1, but you are not in TRANSMIT state, then you should backoff 
int update_channel_state(pNode p,int size)
{
                
   int i; 
   int j=0;  
   int c=0;
   for(i=0;i<size;i++) {
       if (p[i].state==TRANSMIT) {
           ch0.users[c++]=p[i].id;                      
        } else if (p[i].state==COMPLETE || p[i].state==FAILURE) {
              j++;                     
        }
        
               
       }   
	   
   ch0.pre_state=ch0.state;	   
  if (c==0) ch0.state=IDLE;
  else if (c==1) ch0.state=BUSY;
  else if (c>1) ch0.state=COLLISION;

  
  ch0.n_users=c;
  
  
   
     for(i=0;i<ch0.n_users;i++) {
       dbg_printf("  %c  ", ch0.users[i]) ;           
     }       
          
       dbg_printf("\n") ;         
  
  return c; 

}


void do_backoff(pNode p)
{
   int mod;
   p->state=BACKOFF;
   
   if(p->nb==MAX_NB) {
      p->state=FAILURE;  // give up
	  p->info.num_failure++;
	  //-------
	  //restart play again
	  reset_node(p);
	  p->nb=0;
      return ;
    }
   mod =(int)pow(2,p->be);  
  
   p->cw=rand()%mod;  //get 0~(2^be-1)

   p->cw=max(p->cw_min,p->cw); //min cw is cwmin	
   p->cw=min(p->cw_max,p->cw); //max cw is cwmax
   
   p->nb++;
   p->be++; 
   dbg_printf("%s: %c:AIFS=%d,BE=%d,NB=%d,cw=%d,state=%s\n",__func__, p->id, p->ifs, p->be, p->nb, p->cw,"Backoff") ;           
   
  
}





      

