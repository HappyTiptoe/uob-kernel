#include "P1.h"

void main_P1(){
  
  chid_t    chan_to_P2;
  int    send_res = -1;
  
  int     data_send = 12;
  int   data_send_2 = 20;
  int  data_receive = -1;
  
  pid_t pid = getpid();
  
  chan_to_P2 = chanend( 1 );
  if( chan_to_P2 == -1 ){
    exit( EXIT_FAILURE );
  }
  
  send( chan_to_P2, data_send );
  
  send( chan_to_P2, data_send_2 );
  
  exit( EXIT_SUCCESS );  
  
  write( STDOUT_FILENO, "exit", 4 );
}