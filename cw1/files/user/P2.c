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
  while( rec_res == -1 || rec_res != 12 ){
    yield();
    rec_res = receive( chan_to_P1 );
  }
  
  write( STDOUT_FILENO, "hs1", 3 );
  
  rec_res = receive( chan_to_P1 );
  while( rec_res == -1 || rec_res != 20 ){
    yield();
    rec_res = receive( chan_to_P1 );
  }
  
  write( STDOUT_FILENO, "hs2", 3 );
  
  exit( EXIT_SUCCESS );
}