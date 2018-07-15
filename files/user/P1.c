#include "P1.h"

void main_P1(){
  
  chid_t chan_to_P2;
  int    rec_res;
  int    data_send = 12;
  
  pid_t pid = getpid();
  
  chan_to_P2 = chanend( 1 );
  if( chan_to_P2 == -1 ){ exit( EXIT_FAILURE ); }
  
  send( chan_to_P2, data_send );
  
  rec_res = receive( chan_to_P2 );
  write( STDOUT_FILENO, "P2 spoke to P1\n", 16 );
  
  exit( EXIT_SUCCESS );  
}

// void main_P1(){
//   int fd[2] = {3, 4};
//   int status;
  
//   status = pipe( fd );
//   if( status == -1 ){
//     //error;
//   }
//   print_num( fd[ 0 ] );
//   print_num( fd[ 1 ] );
  
//   exit( EXIT_SUCCESS );
// }
