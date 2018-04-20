#include "PH.h"

#define    FORK_WANT  1
#define     FORK_RET  2
#define ORDER_ACCEPT 10
#define ORDER_REJECT 11

void print_num( int x ){
  char* ascii_num;
  itoa( ascii_num, x );
  write( STDOUT_FILENO, ascii_num, 2);
}

void give_forks( bool* fork_l, bool* fork_r, bool* eat_status, chid_t chan ){
  pid_t phil_num = getpid() - 1;
  int waiter_reply;
  send( chan, FORK_RET );
  waiter_reply = receive( chan );
  
  if( waiter_reply == ORDER_ACCEPT ){
    print_num( phil_num ); write( STDOUT_FILENO, ": thinking\n", 12);
    *fork_l = false;
    *fork_r = false;
    *eat_status = false;
  }
  else{
    print_num( phil_num ); write( STDOUT_FILENO, ": eating\n", 10);    
  }
  return;
}

void ask_forks( bool* fork_l, bool* fork_r, bool* eat_status, chid_t chan ){
  
  pid_t phil_num = getpid() - 1; //for print statemnt
  int waiter_reply;
  
  send( chan, FORK_WANT );
  waiter_reply = receive( chan );
  
  if( waiter_reply == ORDER_ACCEPT ){
    *fork_l = true;
    *fork_r = true;
    *eat_status = true;
    print_num( phil_num );   
    write( STDOUT_FILENO, ": eating\n", 10);    
  }
  else{ 
    print_num( phil_num );   
    write( STDOUT_FILENO, ": thinking\n", 12);
  }

  return;
}

void main_PH(){
  chid_t chan_to_waiter;
  pid_t phil_pid;
  pid_t phil_num;
  pid_t waiter_pid;
  bool  eating     = false;
  bool  fork_left  = false;
  bool  fork_right = false;
  
  phil_pid = getpid();
  phil_num = phil_pid - 1;
  
  chan_to_waiter = chanend( phil_pid );
  if( chan_to_waiter == -1){ exit( EXIT_FAILURE ); }
 
  write( STDOUT_FILENO, "Init: ", 6 ); 
  print_num( phil_num );   
  write( STDOUT_FILENO, " connected to waiter\n", 22);  
  
  while( 1 ){
    if( fork_left && fork_right){
      give_forks( &fork_left, &fork_right, &eating, chan_to_waiter );
    }
    else{
      ask_forks( &fork_left, &fork_right, &eating, chan_to_waiter );
    }
  }
}