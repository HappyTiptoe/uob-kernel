/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of 
 * which can be found via http://creativecommons.org (and should be included as 
 * LICENSE.txt within the associated archive or repository).
 */

#include "hilevel.h"

/////////////////
// GLOBAL VARS //
/////////////////


// #define    NUM_FDS (32)
#define   NUM_PCBS (32)
#define  NUM_CHANS (64)
#define STACK_SIZE (0x00001000)
#define ROUND_ROBIN (1)
#define PRIORITY    (2)

// Running Program Info
pid_t     tail_pid = 0;  // Last PCB element's pid
int      executing = 0;  // Current executing PCB
int   console_last = 0;
int  progs_running = 0;
int scheduler_mode = PRIORITY;

// Declare pcb list and chan array
pcb_t         pcb[ NUM_PCBS ];   //32 PCBs
chan_t       chan[ NUM_CHANS ]; //64 Chans
// pipeend_t filedes[ NUM_FDS ];

// External main funcs (for exec) & stack ptrs
extern void main_P1();       // P1 program
extern void main_P2();       // P2 program
extern void main_P3();       // P3 program
extern void main_P4();       // P4 program
extern void main_P5();       // P5 program
extern void main_console();  // Console program
extern void main_W();        // Waiter program
extern void main_PH();       // Philosopher program
extern uint32_t tos_console; // Base of console's stack
extern uint32_t tos_procs;   // Base of stack for processes



////////////////////////
// PRIORITY SCHEDULER //
////////////////////////

// Works out PCB's priority
uint32_t get_priority( pid_t pid ){
	return pcb[ pid ].age + pcb[ pid ].base_priority;
}

//returns pid of PCB with highest priority
pid_t find_next_proc(){
	pid_t         pid =  0;
  int           pri = -1;
  int   highest_pri = -1;

	for( int i = 0; i <= tail_pid; i++ ){
    if( pcb[ i ].status == STATUS_READY || pcb[ i ].status == STATUS_EXECUTING ){
			pri = get_priority( i );
		}
    if(pri > highest_pri){
      highest_pri = pri;
      pid = i;
    }
	}
	return pid;
}


// Increments all PCB ages except one picked by scheduler
void update_ages(){
	for( int i = 0; i <= tail_pid; i++ ){
      pcb[ i ].age++; 
	}	
}

///////////////////////////
// ROUND ROBIN SCHEDULER //
///////////////////////////

// For use of Round-robin scheduler
pid_t next_alive_prog(){
  pid_t next_val = executing + 1;
  for( next_val; next_val <= tail_pid; next_val++ ){
    if( pcb[ next_val ].status == STATUS_READY || pcb[ next_val ].status == STATUS_EXECUTING ){
      return next_val;
    }
  }
  return 0;
}

 
//Picks, using priority, next program to execute
//Alternates between console and programs
void scheduler( ctx_t* ctx ){

	//If more programs than just the console:
	if( progs_running > 1 ){	
    
    pid_t next_proc = 0;
    
    if( scheduler_mode == ROUND_ROBIN ){
      pid_t next_proc = next_alive_prog();
    }
    else if( scheduler_mode == PRIORITY ){
      next_proc = find_next_proc();
      update_ages();
    }

		//Store old running
   	memcpy( &pcb[ executing ].ctx, ctx, sizeof( ctx_t ) );
   	if( pcb[ executing ].status != STATUS_TERMINATED ){
      pcb[ executing ].status = STATUS_READY; 
    }
			
		//Load new running & reset age
		memcpy( ctx, &pcb[ next_proc ].ctx, sizeof( ctx_t ) );
		pcb[ next_proc ].status = STATUS_EXECUTING;
		pcb[ next_proc ].age = 0;
  	executing = next_proc;	
	}
}


////////////////////////
// INTERRUPT HANDLERS //
////////////////////////

void hilevel_handler_rst( ctx_t* ctx ) {
	TIMER0->Timer1Load  = 0x00100000; // select period = 2^2 ticks ~= 0.1 sec
	TIMER0->Timer1Ctrl  = 0x00000002; // select 32-bit   timer
 	TIMER0->Timer1Ctrl |= 0x00000040; // select periodic timer
 	TIMER0->Timer1Ctrl |= 0x00000020; // enable          timer interrupt
 	TIMER0->Timer1Ctrl |= 0x00000080; // enable          timer

	GICC0->PMR          = 0x000000F0; // unmask all            interrupts
	GICD0->ISENABLER1  |= 0x00000010; // enable timer          interrupt
	GICC0->CTLR         = 0x00000001; // enable GIC interface
	GICD0->CTLR         = 0x00000001; // enable GIC distributor

	// Initialise console PCB:
	memset( &pcb[ 0 ], 0, sizeof(pcb_t) );

	pcb[ 0 ].pid           = 0;
	pcb[ 0 ].status        = STATUS_READY;
	pcb[ 0 ].ctx.cpsr      = 0x50;
  pcb[ 0 ].ctx.pc        = (uint32_t)(&main_console);
  pcb[ 0 ].ctx.sp        = (uint32_t)(&tos_console);
  pcb[ 0 ].base_priority = 1;
  pcb[ 0 ].stack_top     = (uint32_t)(&tos_console);

	int_enable_irq();

	// Execute console
  memcpy( ctx, &pcb[ 0 ].ctx, sizeof( ctx_t ) );
  pcb[ 0 ].status = STATUS_EXECUTING;
  executing       = 0;
  console_last    = 1;
  progs_running   = 1;
  
	return;
}


void hilevel_handler_irq( ctx_t* ctx ) {
	// PL011_putc( UART0, 'I', true );
	// Step 2: read  the interrupt identifier so we know the source.
 	uint32_t id = GICC0->IAR;

 	// Step 4: handle the interrupt, then clear (or reset) the source.
  	if( id == GIC_SOURCE_TIMER0 ) {
    	scheduler(ctx);
    	TIMER0->Timer1IntClr = 0x01;
  	}

  	// Step 5: write the interrupt identifier to signal we're done.
  	GICC0->EOIR = id;

 	return;
}

pid_t find_empty_pcb(){
  pid_t free_pid = 0;
  for( int i = 0; i <= tail_pid; i++ ){
    if(pcb[ i ].status == STATUS_TERMINATED){
      free_pid = i;
      return free_pid;
    }
  }
  return -1;
}

void hilevel_handler_svc( ctx_t* ctx, uint32_t id ) {

	switch( id ){ 

    // yield() 
    case 0x00 : {
      scheduler( ctx );
      break;
    }

		// write()
		case 0x01 : {
			int fd = (int)(ctx->gpr[0]);  
      char* x = (char*)(ctx->gpr[1]);  
      int n = (int)(ctx->gpr[2]); 

      for(int i = 0; i < n; i++) {
        PL011_putc(UART0, *x++, true);
     	 }
      
     	 ctx->gpr[0] = n;
     	 break;
     	}

    // fork()
    case 0x03 : {

      // Locate PCB to become child:
      pid_t free_pid = find_empty_pcb();

      // If none found, create & append new PCB:
      if(free_pid == -1){
	    	free_pid = ++tail_pid;			
      }

     	// Replicate state of parent in child
      memset( &pcb[ free_pid ], 0, sizeof(pcb_t) );  
	   	memcpy( &pcb[ free_pid ].ctx, ctx, sizeof( ctx_t ) );
      memcpy( pcb[ free_pid ].stack_top - STACK_SIZE, pcb[ executing ].stack_top - STACK_SIZE, STACK_SIZE );
      uint32_t sp_offset = pcb[ executing ].stack_top - pcb[ executing ].ctx.sp;

      pcb[ free_pid ].status = STATUS_READY;
      pcb[ free_pid ].pid = free_pid;
      pcb[ free_pid ].stack_top = (uint32_t)(&tos_procs + (free_pid * STACK_SIZE));
      pcb[ free_pid ].ctx.sp = pcb[ free_pid ].stack_top - sp_offset;

     	ctx->gpr[ 0 ] = free_pid;			    // Parent: child's pid
     	pcb[ free_pid ].ctx.gpr[ 0 ] = 0;	// Child: 0

     	// Pause parent process & execute child
     	memcpy( &pcb[ executing ].ctx, ctx, sizeof( ctx_t ) );	 // Preserve current running
    	pcb[ executing ].status = STATUS_READY;					         // Update status

  		memcpy( ctx, &pcb[ free_pid ].ctx, sizeof( ctx_t ) );	   // Load child into current
  		pcb[ free_pid ].status = STATUS_EXECUTING;				       // Update status
  		executing = free_pid;
      
      progs_running++;
     		
  		break;		
		}

		//exit
		case 0x04 : {
      pcb[ executing ].status = STATUS_TERMINATED;
      if( --progs_running < 1 ){ 
        progs_running = 1;
      }    
      scheduler(ctx);
			break;
		}

    //exec
    case 0x05 : {
   		uint32_t new_prog_img = ctx->gpr[ 0 ];
   		ctx->pc = new_prog_img;

   		// Change base priority depending on program image     		
      if( strcmp(new_prog_img, &main_P1) == 0 ){ pcb[ executing ].base_priority = 1; }
      if( strcmp(new_prog_img, &main_P2) == 0 ){ pcb[ executing ].base_priority = 1; }
   		if( strcmp(new_prog_img, &main_P3) == 0 ){ pcb[ executing ].base_priority = 1; }
   		if( strcmp(new_prog_img, &main_P4) == 0 ){ pcb[ executing ].base_priority = 2; }
      if( strcmp(new_prog_img, &main_P5) == 0 ){ pcb[ executing ].base_priority = 4; }
      if( strcmp(new_prog_img, &main_W ) == 0 ){ pcb[ executing ].base_priority = 1; }
   		if( strcmp(new_prog_img, &main_PH) == 0 ){ pcb[ executing ].base_priority = 1; }

   		// Reset state (sp & age)
      memset( pcb[ executing ].stack_top - STACK_SIZE, 0, STACK_SIZE);
   		ctx->sp = pcb[ executing ].stack_top;
   		pcb[ executing ].age = 0;

   		break;
   	}

    //kill
   	case 0x06 : {
   		pid_t     pid = ctx->gpr[ 0 ];
      status_t stat = pcb[ pid ].status;
      
      if( stat == STATUS_READY || stat == STATUS_EXECUTING ){
        pcb[ pid ].status = STATUS_TERMINATED;
        if(--progs_running < 1){
          progs_running = 1;
        }
        ctx->gpr[ 0 ] = 1;
        scheduler(ctx);
      }
      else{ 
        ctx->gpr[ 0 ] = -1;
      }  
   		break;
   	}
    
    //getpid
    case 0x08 : {
      ctx->gpr[ 0 ] = pcb[ executing ].pid;
      break;
    }
    
    //switch_sched
    case 0x09 : {
      int mode = ctx->gpr[ 0 ];
      if( mode == 1 | mode == 2){
        scheduler_mode = mode;
      }
      break;
    }
    
    //connect to channel
    case 0x10 : {
          
      chid_t chan_id = ctx->gpr[ 0 ];

      if( chan[ chan_id ].pids_connected == 2 ){
        ctx->gpr[ 0 ] = -1; //fail.
      }
      else if( chan[ chan_id ].pids_connected == 1 ){
        chan[ chan_id ].p2 = pcb[ executing ].pid; //connect second end
        chan[ chan_id ].pids_connected = 2;
        ctx->gpr[ 0 ] = chan_id; //success
      }
      else{
        memset( &chan[ chan_id ], 0, sizeof(chan_t) ); //creating new channel
        
        chan[ chan_id ].p1 = pcb[ executing ].pid;
        chan[ chan_id ].chan_id = chan_id;
        chan[ chan_id ].pids_connected = 1;
        chan[ chan_id ].data_on_chan_to_p1 = false; 
        chan[ chan_id ].data_on_chan_to_p2 = false; 
        
        ctx->gpr[ 0 ] = chan_id; //success
      }
      
      break;  
    }
    
    //send
    case 0x11 : {
      //PL011_putc( UART0, 'S', true );
      chid_t chid = ctx->gpr[ 0 ];
      int data = ctx->gpr[ 1 ];
      pid_t pid = executing;
      
      //if process = p1, set to_p2 flag
      if( pid == chan[ chid ].p1 ){
        chan[ chid ].data_on_chan_to_p2 = true;
        chan[ chid ].data_for_p2 = data;
      }
      else if( pid == chan[ chid ].p2 ){
        chan[ chid ].data_on_chan_to_p1 = true;
        chan[ chid ].data_for_p1 = data;
      }
      else{
        //fail
        ctx->gpr[ 0 ] = -1;
      }
        
      break;
    }
    
    //receive
    case 0x12 : {
      chid_t chid = ctx->gpr[ 0 ];
      pid_t pid = executing;

      //if process = p1, clear to_p1 flag
      if( pid == chan[ chid ].p1 ){
        chan[ chid ].data_on_chan_to_p1 = false;
        ctx->gpr[ 0 ] = chan[ chid ].data_for_p1;
      }
      //if process = p2, clear to_p2 flag
      else if( pid == chan[ chid ].p2 ){
        chan[ chid ].data_on_chan_to_p2 = false;
        ctx->gpr[ 0 ] = chan[ chid ].data_for_p2;
      }
      else{
        //fail
        ctx->gpr[ 0 ] = -1;
      }
      
      break;
    }
    
    //check
    case 0x14 : {
      chid_t chid = ctx->gpr[ 0 ];
      bool d_to_p1 = chan[ chid ].data_on_chan_to_p1;
      bool d_to_p2 = chan[ chid ].data_on_chan_to_p2;
      
      if( !d_to_p1 && !d_to_p2 ){
        //Neither channel has data:
        ctx->gpr[ 0 ] = 0;  
      }
      else if( d_to_p1 && !d_to_p2 ){
        //P2 has put data on for P1
        ctx->gpr[ 0 ] = 1;
      }
      else if ( !d_to_p1 && d_to_p2 ){
        //P1 has put data on for P2
        ctx->gpr[ 0 ] = 2;
      }
      else{
        //Both have data on (deadlock, so -1)
        ctx->gpr[ 0 ] = -1; 
      }
      break;
    }
    
    //which_end
    case 0x15 : {
      chid_t chid = ctx->gpr[ 0 ];
      pid_t   pid = ctx->gpr[ 1 ];
      
      if( pid == chan[ chid ].p1 ){
        ctx->gpr[ 0 ] = 1;
      }
      else if( pid == chan[ chid ].p2 ){
        ctx->gpr[ 0 ] = 2;
      }
      else{
        ctx->gpr[ 0 ] = -1;
      }
      break;
    }
    
    /*
    //pipe
    case 0x20: {
      int* fds = (ctx->gpr[ 0 ]);
      
      int last_fd = find_last_fd();

      memset( &filedes[ 5 ], 0, sizeof( pipeend_t ) );
      memset( &filedes[ 6 ], 0, sizeof( pipeend_t ) );
      fds[0] = 7;
      fds[1] = 6;
      ctx->gpr[ 0 ] = 0;
    }*/
   
   	default : {
   		break;
   	}
  }


  return;
}