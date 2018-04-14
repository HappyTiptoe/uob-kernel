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
  
  chan_to_P1 = create_chan( 1 );
  while(chan_to_P1 == -1 ){
    yield();
    chan_to_P1 = create_chan( 1 );
  }
  
  rec_res = listen( chan_to_P1 );
  while( rec_res == -1 || rec_res != 12 ){
    yield();
    rec_res = listen( chan_to_P1 );
  }
  
  write( STDOUT_FILENO, "hs1", 3 );
  
  rec_res = listen( chan_to_P1 );
  while( rec_res == -1 || rec_res != 20 ){
    yield();
    rec_res = listen( chan_to_P1 );
  }
  
  write( STDOUT_FILENO, "hs2", 3 );
  
  exit( EXIT_SUCCESS );
}