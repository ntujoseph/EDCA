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
#define CW 1
#define BE 3  
#define MAX_NB 10  //maximum backoff slot
#define MAX_FRAME_SIZE 2 
#define N 2 // max number of nodes
#define MAX_ROUND_TEST 1 // average the result 
#define FREEZE 1  //1: enable; 0:disable
int node_q=N; //initial value for the number of node
int game_over=0;
int frame_size=MAX_FRAME_SIZE; //initial data lenth




#define WRITE_FILE 0  //1: enable; 0:disable
#define JOSEPH_DEBUG 1

#if JOSEPH_DEBUG==1
#define dbg_printf(fmt, s...)	do{printf(fmt, ##s);}while(0)  
#else
#define dbg_printf(fmt, s...)
#endif	

enum State {START,BACKOFF,CCA,TRANSMIT,COMPLETE,FAILURE};
char Str_State[][16]={"Start","Backoff","CCA","Transmit","Complete","Failure"};
enum Ch_State {IDLE=0,ACTIVE,COLLISION};
char Str_Ch_State[][16]={"Idle","Active","Collision"};

#define PRINT_FMT "%d\t%6.3lf\n" 


typedef struct {    
enum Ch_State state;     //channel state
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
  int cw;  //contention window
  int be; //backoff exponent
  int nb; //the number of backoff
  int bp; //backoff period
  int d_len;  //frame length
  enum State state;
}Node;

typedef Node * pNode;

//----function list----------
void init(pNode p,int size);
void reset_node (pNode p);
void show_state (pNode p,int size);
int get_channel_state(pNode p,int size);
void do_backoff(pNode p);
void do_cca(pNode p);
void show_report(pNode p,int size);
void show_round_report(pNode p,int size);

int main()
{

 int i;
 unsigned long t;
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
		total_time=0;                             
		for (round_i=1;round_i<=MAX_ROUND_TEST;round_i++) {     
			dbg_printf("Round%d:\n",round_i);    
            init(node,N);
			for(t=0;t<10000000;t++)
			{ 
				dbg_printf("t=%ld==>\n",t);
				show_state(node,node_q);   
   				if (game_over==1) {   
					ch0.last_complete_time=t;     
					total_time+=ch0.last_complete_time;                          
					break; 
				}   
  
				for(i=0;i<node_q ;i++) {
					if (node[i].state==COMPLETE || node[i].state==FAILURE)
						continue;
                 	if (node[i].state==START)  //do backoff for each attempt                     
						do_backoff(&node[i]);
       
  
					if(node[i].state==BACKOFF) {
 
                        if (ch0.state==IDLE){ 
                         node[i].bp--; 
                         
                        } else {
                          ; // do-nothing , because we "Freeze" in backoff period  
                                
                        }                         
                      
                    
                                                    
						node[i].bp=max(0,node[i].bp); //min of bp is 0
       
					}
    
					if(node[i].state==BACKOFF && node[i].bp==0)
						node[i].state=CCA;        
					else if (node[i].state==CCA)
						do_cca(&node[i]);
        
        
					 if(node[i].state==CCA && node[i].cw==0) {
						if (ch0.state==IDLE) 
							node[i].state=TRANSMIT;
						else 
							 do_backoff(&node[i]);
					 } else if (node[i].state==TRANSMIT) {
						node[i].d_len--;
						if (node[i].d_len==0){
							if (ch0.state==COLLISION){ 
								//wc check channel for each time slot, so collision only occurs here                       
								reset_node(&node[i]);
							} else { 
								node[i].state=COMPLETE;
								ch0.complete_users[i]=t+1;
							}   
						}  
					 }       
     
 
  
				}
				//after this time slot, we check the channel state 
				get_channel_state(node,node_q);  
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
 

  printf("Press any KEY to exit");   
 getchar();
return 0;
}


///========================================================
void init(pNode p,int size)
{
   
     
    int i;   
   //init node 
   for(i=0;i<size;i++) {
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

   
     p->cw=CW;
     p->be=BE;
     p->nb=0; 
     p->bp=0;
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
    dbg_printf("%c:CW=%d,BE=%d,NB=%d,bp=%d,D=%d,state=%s\n",
       p[i].id,p[i].cw,p[i].be,p[i].nb,p[i].bp,p[i].d_len,Str_State[p[i].state]) ;           
     }     
     dbg_printf("Channel 0 State:%s",Str_Ch_State[ch0.state]);
     for(i=0;i<ch0.n_users;i++) {
       dbg_printf("  %c  ", ch0.users[i]) ;           
     }       
          
       dbg_printf("\n") ;         
}           


//@return :  number of users is in transmit 
// c=0 , channel idle
// c=1 , someone is transmiting (maybe the yourself)
// c=2 , must be collision
// if c==1, but you are not in TRANSMIT state, then you should backoff 
int get_channel_state(pNode p,int size)
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
  
  
  if (c==0) ch0.state=IDLE;
  else if (c==1) ch0.state=ACTIVE;
  else if (c>1) ch0.state=COLLISION;
  
  if (j==node_q) game_over=1;   
  
  ch0.n_users=c;
  return c; 

}

void do_backoff(pNode p)
{
   int mod;
   if(p->nb==MAX_NB) {
      p->state=FAILURE;  // give up
      return ;
    }
   mod =(int)pow(2,p->be);  
  
   p->bp=rand()%mod;  //get 0~2^be-1

   p->state=(p->bp>0)?BACKOFF:CCA;
   p->nb++;
   p->be++; 
   p->cw=CW;
   dbg_printf("%s: %c:CW=%d,BE=%d,NB=%d,bp=%d,state=%s\n",__func__, p->id, p->cw, p->be, p->nb, p->bp,"Backoff") ;           
         
   

   
  
}

void do_cca(pNode p)
{
   p->state=CCA;  
   p->cw--;
   p->be=BE;
   p->nb=0; 
   p->bp=0;
   dbg_printf("%s: %c:CW=%d,BE=%d,NB=%d,bp=%d,state=%s\n",__func__, p->id, p->cw, p->be, p->nb, p->bp,"CCA") ;  
}







      

