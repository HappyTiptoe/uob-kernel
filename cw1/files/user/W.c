#include "W.h"

#define    NUM_PHILS  4
#define    FORK_WANT  1
#define    FORK_GIVE  2
#define ORDER_ACCEPT 10
#define ORDER_REJECT 11

extern void main_PH();

chid_t chan_to_phil[ NUM_PHILS ];
pid_t     phil_pids[ NUM_PHILS ];
pid_t                 waiter_pid;
bool          forks[ NUM_PHILS ];


void toggle_forks( int phil_num ){
  if( phil_num == NUM_PHILS - 1){
    //Last philosopher in list
    forks[ 0 ]        = !forks[ 0 ];
    forks[ phil_num ] = !forks[ phil_num ];
  }
  else{
    forks[ phil_num ]     = !forks[ phil_num ];
    forks[ phil_num + 1 ] = !forks[ phil_num + 1 ];
  }
}


//Checks if both forks are present for given phil
bool can_eat( int phil_num ){
  if( phil_num == NUM_PHILS - 1 ){
    //Last philosopher in list
    return ( forks[ phil_num ] && forks[ 0 ] );
  }
  else{
    return ( forks[ phil_num ] && forks[ phil_num + 1 ] );
  }
}


void create_phils(){
  for( int i = 0; i < NUM_PHILS; i++ ){
    pid_t pid = fork();
    
    if( pid == 0 ){
      //if child process, turn into phil program
      exec( &main_PH );
    }
    else{
      //still waiter, put pid in list
      phil_pids[ i ] = pid;
    }
  }
}


void serve_phils(){
  
  for( int i = 0; i < NUM_PHILS; i++ ){
    int    phil_order = -1;               // to hold give/want order
    chid_t phil_chan = chan_to_phil[ i ]; // select phil channel
    phil_order = receive( phil_chan );    // get order

    switch( phil_order ){
      
      // phil wants to eat
      case FORK_WANT : {
        if( can_eat( i ) ){
          // both forks present, begin to eat
          send( phil_chan, ORDER_ACCEPT );
          toggle_forks( i );
        }
        else{
          // requires both forks
          send( phil_chan, ORDER_REJECT );
        }
        break;
      } 
      
      // phil wants to return forks
      case FORK_GIVE : {
        if( !can_eat( i ) ){
          // neither fork present (due to implimentation)
          send( phil_chan, ORDER_ACCEPT );
          toggle_forks( i );
        }
        else{
          // can't give back forks doesn't have
          send( phil_chan, ORDER_REJECT );
        }
        break;
      }
      
      default: {
        break;
      }
    }
  }             
}


void main_W(){
  write( STDOUT_FILENO, "Program started...\n", 20);
  waiter_pid = getpid();                   // Obtain waiter's PID  
  
  write( STDOUT_FILENO, "Creating phil progs...\n", 24);
  create_phils();                          // Create 16 PH programs
  write( STDOUT_FILENO, "All phils created...\n", 22);
  
  for( int i = 0; i < NUM_PHILS; i++ ){    // For each one:
    forks[ i ] = true;                     // Reset forks
    
    pid_t pid = phil_pids[ i ];
    chan_to_phil[ i ] = chanend( pid );    // Create channel to it
    send( chan_to_phil[ i ], waiter_pid ); // Send waiter's pid
  }

  while( 1 ){
    serve_phils();
  }
   
}