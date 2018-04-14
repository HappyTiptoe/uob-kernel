#include "P1.h"

void main_P1(){
  
  chid_t    chan_to_P2;
  int    send_res = -1;
  
  int   data_send = 12;
  int data_send_2 = 20;
  int    data_rec = -1;
  
  chan_to_P2 = create_chan( 2 );
  while( chan_to_P2 == -1){
    yield();
    chan_to_P2 = create_chan( 2 );
  }

  send_res = send( chan_to_P2, data_send );
  while( send_res == -1 ){
    yield();
    send_res = send( chan_to_P2, data_send );
  }
  
  send_res = send( chan_to_P2, data_send_2 );
  while( send_res == -1 ){
    yield();
    send_res = send( chan_to_P2, data_send_2 );
  }
  exit( EXIT_SUCCESS );  
}