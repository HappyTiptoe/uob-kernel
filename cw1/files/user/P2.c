/**
 * Handshake test program.
 * Confirms a message was received from P1.
 * Sends message to P2.
 * Terminates.
 */

#include "P2.h"

void main_P2(){
  
  chid_t chan_to_P1;
  int rec_res;
  
  pid_t pid = getpid();
  
  chan_to_P1 = chanend( 1 );
  if( chan_to_P1 == -1 ){ exit( EXIT_FAILURE ); }
  
  rec_res = receive( chan_to_P1 );
  rec_res = receive( chan_to_P1 );
  
  write( STDOUT_FILENO, "hs_done", 7 );
    
  exit( EXIT_SUCCESS );
}