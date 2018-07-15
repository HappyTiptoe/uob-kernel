/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of 
 * which can be found via http://creativecommons.org (and should be included as 
 * LICENSE.txt within the associated archive or repository).
 */

#include "libc.h"

int  atoi( char* x        ) {
  char* p = x; bool s = false; int r = 0;

  if     ( *p == '-' ) {
    s =  true; p++;
  }
  else if( *p == '+' ) {
    s = false; p++;
  }

  for( int i = 0; *p != '\x00'; i++, p++ ) {
    r = s ? ( r * 10 ) - ( *p - '0' ) :
            ( r * 10 ) + ( *p - '0' ) ;
  }

  return r;
}

void itoa( char* r, int x ) {
  char* p = r; int t, n;

  if( x < 0 ) {
     p++; t = -x; n = t;
  }
  else {
          t = +x; n = t;
  }

  do {
     p++;                    n /= 10;
  } while( n );

    *p-- = '\x00';

  do {
    *p-- = '0' + ( t % 10 ); t /= 10;
  } while( t );

  if( x < 0 ) {
    *p-- = '-';
  }

  return;
}

void yield() {
  asm volatile( "svc %0     \n" // make system call SYS_YIELD
              :
              : "I" (SYS_YIELD)
              : );

  return;
}

int write( int fd, const void* x, size_t n ) {
  int r;

  asm volatile( "mov r0, %2 \n" // assign r0 = fd
                "mov r1, %3 \n" // assign r1 =  x
                "mov r2, %4 \n" // assign r2 =  n
                "svc %1     \n" // make system call SYS_WRITE
                "mov %0, r0 \n" // assign r  = r0
              : "=r" (r) 
              : "I" (SYS_WRITE), "r" (fd), "r" (x), "r" (n)
              : "r0", "r1", "r2" );

  return r;
}

int  read( int fd, void* x, size_t n ) {
  int r;

  asm volatile( "mov r0, %2 \n" // assign r0 = fd
                "mov r1, %3 \n" // assign r1 =  x
                "mov r2, %4 \n" // assign r2 =  n
                "svc %1     \n" // make system call SYS_READ
                "mov %0, r0 \n" // assign r  = r0
              : "=r" (r) 
              : "I" (SYS_READ),  "r" (fd), "r" (x), "r" (n) 
              : "r0", "r1", "r2" );

  return r;
}

int  fork() {
  int r;

  asm volatile( "svc %1     \n" // make system call SYS_FORK
                "mov %0, r0 \n" // assign r  = r0 
              : "=r" (r) 
              : "I" (SYS_FORK)
              : "r0" );

  return r;
}

void exit( int x ) {
  asm volatile( "mov r0, %1 \n" // assign r0 =  x
                "svc %0     \n" // make system call SYS_EXIT
              :
              : "I" (SYS_EXIT), "r" (x)
              : "r0" );

  return;
}

void exec( const void* x ) {
  asm volatile( "mov r0, %1 \n" // assign r0 = x
                "svc %0     \n" // make system call SYS_EXEC
              :
              : "I" (SYS_EXEC), "r" (x)
              : "r0" );

  return;
}

int  kill( int pid, int x ) {
  int r;

  asm volatile( "mov r0, %2 \n" // assign r0 =  pid
                "mov r1, %3 \n" // assign r1 =    x
                "svc %1     \n" // make system call SYS_KILL
                "mov %0, r0 \n" // assign r0 =    r
              : "=r" (r) 
              : "I" (SYS_KILL), "r" (pid), "r" (x)
              : "r0", "r1" );

  return r;
}

void nice( int pid, int x ) {
  asm volatile( "mov r0, %1 \n" // assign r0 =  pid
                "mov r1, %2 \n" // assign r1 =    x
                "svc %0     \n" // make system call SYS_NICE
              : 
              : "I" (SYS_NICE), "r" (pid), "r" (x)
              : "r0", "r1" );

  return;
}

void switch_sched( int x ){
  asm volatile( "mov r0, %1 \n" // assign r0 =  pid
                "svc %0     \n" // make system call SYS_NICE
              : 
              : "I" (SYS_SCHED), "r" (x)
              : "r0" );
  
}

pid_t getpid(){
  int r;
  
  asm volatile( "svc %1     \n" // make system call GET_PID
                "mov %0, r0 \n" // assign r  = r0 
              : "=r" (r) 
              : "I" (SYS_GET_PID)
              : "r0" );
  return r;
}

chid_t chanend( chid_t chid ){
  int r;
  
  asm volatile( "mov r0, %2 \n" // assign r0 = chid
                "svc %1     \n" // make system call IPC_CHANEND
                "mov %0, r0 \n" // assign r  = r0 
              : "=r" (r) 
              : "I" (IPC_CHANEND), "r" (chid)
              : "r0" );
  
  return r;
}

void send( chid_t chid, int data ){

  asm volatile( "mov r0, %1 \n" // assign r0 = chan's id
                "mov r1, %2 \n" // assign r1 = data to send
                "svc %0     \n" // make system call IPD_SEND
              :
              : "I" (IPC_SEND), "r" (chid), "r" (data)
              : "r0", "r1" );  
  
  pid_t pid = getpid();
  
  switch( which_end( chid, pid ) ){
    //if p1 is sending:
    case 1 : {
      if( check( chid ) != 2 ){
        write( STDOUT_FILENO, "Nothing set on channel.\n", 25);
        return;
      }
      // if: data_on_chan_to_p2, yield
      while( check( chid ) == 2 ){
        // write( STDOUT_FILENO, "\nSEND: ", 8 );  
        // print_num( pid ); 
        // write( STDOUT_FILENO, " blocking p1\n", 14);
        yield();
      }
      break;
    }
    
    //if p2 is sending:
    case 2 : {
      if( check( chid ) != 1 ){
        write( STDOUT_FILENO, "Nothing set on channel.\n", 25);
        return;
      }
      // if: data_on_chan_to_p2, yield
      while( check( chid ) == 1 ){
        // write( STDOUT_FILENO, "\nSEND: ", 8 );  
        // print_num( pid ); 
        // write( STDOUT_FILENO, " blocking p2\n", 14);        
        yield();
      }
      break;
    }
    
    default : {
      break;
    }
  }
  
  return;
}

int receive( chid_t chid ){
  int r;
  
  pid_t pid = getpid();
  
  switch( which_end( chid, pid ) ){
    //if p1 is receiving:
    case 1 : {
      // if: no data_on_chan_to_p1, yield
      while( check( chid ) != 1 ){
        // write( STDOUT_FILENO, "\nREC: ", 7 );  
        // print_num( pid ); 
        // write( STDOUT_FILENO, " blocking p1\n", 14);   
        yield();
      }
      break;
    }
    
    //if p2 is receiving:
    case 2 : {
      // if: no data_on_chan_to_p2, yield
      while( check( chid ) != 2 ){
        // write( STDOUT_FILENO, "\nREC: ", 7 );  
        // print_num( pid ); 
        // write( STDOUT_FILENO, " blocking p2\n", 14);   
        yield();
      }
      break;
    }
    
    default : {
      break;
    }
  }
  
  asm volatile( "mov r0, %2 \n" // assign r0 = chan's id
                "svc %1     \n" // make system call IPC_REC
                "mov %0, r0 \n" // assign r  = r0 
              : "=r" (r) 
              : "I" (IPC_REC), "r" (chid)
              : "r0" );
                  
  return r;
}


int check( chid_t chid ){
  int r;
  
  asm volatile( "mov r0, %2 \n" // assign r0 = chid of channel to check
                "svc %1     \n" // make system call IPC_CHECK
                "mov %0, r0 \n" // assign r  = r0 
              : "=r" (r) 
              : "I" (IPC_CHECK), "r" (chid) 
              : "r0" );  
  
  return r;
}

int which_end( chid_t chid, pid_t pid ){
  int r;
  
  asm volatile( "mov r0, %2 \n" // assign r0 = chid
                "mov r1, %3 \n" // assign r1 = pid
                "svc %1     \n" // make system call IPC_CHECK
                "mov %0, r0 \n" // assign r  = r0 
              : "=r" (r) 
              : "I" (IPC_WHICH), "r" (chid), "r" (pid)
              : "r0", "r1" );  
  
  return r;
}

int pipe( int filedes[2] ){
  int r;
  int* fds = filedes;
  asm volatile( "mov r0, %2 \n" // assign r0 = chid
                "svc %1     \n" // make system call IPC_CHECK
                "mov %0, r0 \n" // assign r  = r0 
              : "=r" (r) 
              : "I" (SYS_PIPE), "r" (fds)
              : "r0" );  
  
  return r;  
}