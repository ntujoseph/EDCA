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
#define AIFS 2
#define BE 3  
#define MAX_NB 10  //maximum backoff slot
#define MAX_FRAME_SIZE 2 
#define N 4 // max number of nodes
#define MAX_ROUND_TEST 1 // average the result 
#define FREEZE 1  //1: enable; 0:disable
int node_q=1; //initial value for the number of node
int game_over=0;
int frame_size=MAX_FRAME_SIZE; //initial data lenth




#define WRITE_FILE 0  //1: enable; 0:disable
#define JOSEPH_DEBUG 1

#if JOSEPH_DEBUG==1
#define dbg_printf(fmt, s...)	do{printf(fmt, ##s);}while(0)  
#else
#define dbg_printf(fmt, s...)
#endif	

enum State {START,BACKOFF,DIFS,TRANSMIT,COMPLETE,FAILURE};
char Str_State[][16]={"Start","Backoff","DIFS","Transmit","Complete","Failure"};
enum Ch_State {IDLE=0,BUSY,COLLISION,IDLE_DIFS,IN_DIFS};
char Str_Ch_State[][16]={"Idle","Busy","Collision","Idle+DIFS","idle-DIFS"};

#define PRINT_FMT "%d\t%6.3lf\n" 


typedef struct {    
enum Ch_State state;     //channel state
enum Ch_State pre_state;     //previous channel state
char users[N];  //active user list
int n_users;    //the number of active  user
unsigned long  complete_users[N]; //record the completion time for each user in this channel
unsigned long last_complete_time;
} Channel;

Channel ch0;
int round_i=1;
FILE *fp;  //for different L

unsigned long long total_time=0; //used for average

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
	//for(t=0;t<100000;t++)
	//   printf("t=%ld\n",t);

  // exit(0);   
    

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
		total_time=0;                             
		for (round_i=1;round_i<=MAX_ROUND_TEST;round_i++) {     
			dbg_printf("Round%d:\n",round_i);    
            init(node,N);
			update_channel_state(node,node_q);  
			 dbg_printf(">>>>>>Channel 0 State:%s\n",Str_Ch_State[ch0.state]);
			for(t=0;t<100;t++)
			{ 
				dbg_printf("t=%ld:\n",t);
			
			    
				 show_state(node,node_q);  
			  
				//dbg_printf("@@@@%c:AIFS=%d,BE=%d,NB=%d,cw=%d,state=%d\n",node[i].id, node[i].ifs, node[i].be, node[i].nb, node[i].cw,node[i].state) ;  
   				if (game_over==1) {   
					ch0.last_complete_time=t;     
					total_time+=ch0.last_complete_time;                          
					break; 
				}   
  
				for(i=0;i<node_q ;i++) {
	//-----------State Machine------------------------------------------------------------------------------			

					if (node[i].state==COMPLETE || node[i].state==FAILURE)
						continue;
			  
           
			   
				   switch (ch0.state) {
  
					   
					   case IDLE:
	                          node[i].var_ifs++;
  	                         if(node[i].var_ifs > node[i].ifs) {
								 printf("1\n");	
                                if (node[i].state==START) {						
							        node[i].state=TRANSMIT; 
									node[i].d_len--;	
								    node[i].var_ifs=0;		
									printf("2\n");										
					   		   }
							 }
							 if (node[i].state==BACKOFF && node[i].cw <=0) { 
									   node[i].cw=0;
                                       node[i].state=TRANSMIT;	
                                       node[i].d_len--;										   
									   
							 } else if (node[i].state==BACKOFF)	{
								   if (ch0.state==IDLE && node[i].var_ifs >= node[i].ifs )
										node[i].cw--;  	
							         
							 }
					
					        break;

							
					   case BUSY: 		
                            node[i].var_ifs=0;					   
							if (node[i].state==TRANSMIT)
								     node[i].d_len--;								 
							if(node[i].d_len==0) {
								node[i].state=COMPLETE;	 		
								printf("%c: complete\n",node[i].id);
								ch0.complete_users[i]=t;
							}								
					
					       break;
					
					   case COLLISION: 
                           node[i].var_ifs=0;			
						
						  if(node[i].d_len==0) {
							 reset_node(&node[i]);
							 do_backoff(&node[i]);
							  node[i].cw--;  				

						}		
                         
						 if (node[i].state==TRANSMIT)  
								 node[i].d_len--;								 
													
					 
					      break;					   
					   
				 } //end switch 	   
			 printf("%c:,D=%d,state=%s\n", node[i].id,node[i].d_len,Str_State[node[i].state]) ; 
	 
			
			
                }
    
            


			
  //----------------------------------------------------------------------------------------------------------------------
			
				//after this time slot, we check the channel state 
				update_channel_state(node,node_q);  
				 dbg_printf(">>>>>>Channel 0 State:%s\n",Str_Ch_State[ch0.state]);
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
     reset_node(&p[i]);            
     }  
   
   //init channel
   memset(&ch0,0,sizeof(Channel));
   ch0.last_complete_time=0xFFFFFFFF;
             
   //for each round
   game_over=0;   
   
   

   
}    

void reset_node (pNode p)
{

   
     p->ifs=AIFS;
     p->cw_min=3;  
     p->cw_max=1024;  
     p->be=BE;
     p->cw=0;
     p->d_len=frame_size;
     p->state=START; //init state     
   	 
               
     
} 

void show_report(pNode p,int size)
{
  int i;
  double T=0;
  double overall_T; //   //overall throughtput 
  static int line=0;  
  char wbuf[64];
 
  if (line++==0) {
  printf("Round\tL\tN\ttime\tthroughput\n");
  printf("-----------------------------------\n");
  }
 
  overall_T=(double)frame_size*size/ch0.last_complete_time;
  printf("%d\t%d\t%d\t%ld\t%6.2lf\n",round_i,frame_size,size,ch0.last_complete_time,overall_T);  


#if 0    //list throughput for each node

  for(i=0;i<size;i++) {
     T=(double)frame_size/ch0.complete_users[i];
      printf("%c complete at t=%5ld, throughput=%6.2lf\n", p[i].id,ch0.complete_users[i],T);      
   }  
   printf("last complete time:%ld\n",ch0.last_complete_time);  
#endif         
  dbg_printf("\n--------------------\n");    
}      




void show_round_report(pNode p,int size)
{
  int i;
  double T=0;
  double avg_overall_T;
  static int line=0;  
  char wbuf[64];

  //average MAX_ROUND_TEST
  avg_overall_T=(double)((frame_size*size)*MAX_ROUND_TEST)/total_time;

  printf("Round=%d\tL=%d\tN=%d\ttime=%lld  avgT=%6.2lf\n",MAX_ROUND_TEST,frame_size,size,total_time,avg_overall_T);  
 
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
	  
	//     dbg_printf("%c:AIFS=%d,BE=%d,NB=%d,cw=%d,var_ifs=%d\n",
     //  p[i].id,p[i].ifs,p[i].be,p[i].nb,p[i].cw,p[i].var_ifs) ;           
    // } 
   dbg_printf("%c:AIFS=%d,BE=%d,NB=%d,cw=%d,D=%d,var_ifs=%d\n",
       p[i].id,p[i].ifs,p[i].be,p[i].nb,p[i].cw,p[i].d_len,p[i].var_ifs) ;           
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
  if (j==node_q) game_over=1;   
  
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
      return ;
    }
   mod =(int)pow(2,p->be);  
  
   p->cw=rand()%mod;  //get 0~(2^be-1)

 
   p->nb++;
   p->be++; 
   p->ifs=AIFS;
   dbg_printf("%s: %c:AIFS=%d,BE=%d,NB=%d,cw=%d,state=%s\n",__func__, p->id, p->ifs, p->be, p->nb, p->cw,"Backoff") ;           
   
  
}





      

