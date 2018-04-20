#include "W.h"

#define    NUM_PHILS  4
#define    FORK_WANT  1
#define     FORK_RET  2
#define ORDER_ACCEPT 10
#define ORDER_REJECT 11
#define     PHIL_FIN 12

extern void main_PH();

int     phils_eating = NUM_PHILS;
bool          forks[ NUM_PHILS ];
pid_t     phil_pids[ NUM_PHILS ];
chid_t chan_to_phil[ NUM_PHILS ];


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


void init_phils(){
  for( int i = 0; i < NUM_PHILS; i++ ){
    pid_t pid = fork();
    
    if( pid == 0 ){
      //if child process, turn into phil program
      exec( &main_PH );
    }
    else{
      //waiter
      forks[ i ] = true;                     // Reset forks
      phil_pids[ i ] = pid;
      chan_to_phil[ i ] = chanend( pid );    // Create channel to it
    }
  }
}


void serve_phils(){
  for( int i = 0; i < NUM_PHILS; i++ ){
    chid_t phil_chan = chan_to_phil[ i ];  // select phil channel
    int phil_order = receive( phil_chan ); // get order

    if(      phil_order == FORK_WANT &&  can_eat( i ) ){
      send( phil_chan, ORDER_ACCEPT ); 
      toggle_forks( i );
    }
    else if( phil_order == FORK_RET && !can_eat( i ) ){
      send( phil_chan, ORDER_ACCEPT );
      toggle_forks( i );
    }
    // else if( phil_order == PHIL_FIN ){
    //   toggle_forks( i );
    //   phils_eating--;
    // }
    else{
      send( phil_chan, ORDER_REJECT );
    }
  }
  write( STDOUT_FILENO, "\n", 2);             
}

void main_W(){
  // int counter = 0; 
  
  init_phils();

  while( 1 && phils_eating > 0){
    serve_phils();
    // counter++;
  }
  
  exit( EXIT_SUCCESS );
   
}