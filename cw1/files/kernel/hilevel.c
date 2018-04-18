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

#define   num_pcbs (32)
#define  num_chans (64)
#define stack_size (0x00001000)

// Running Program Info
int     executing = 0;  // Current executing PCB
pid_t    tail_pid = 0;  // Last PCB element's pid
int      console_last;
int progs_running = 0;

// Declare pcb list and chan array
pcb_t pcb[ num_pcbs ];		//32 PCBs
chan_t chan[ num_chans ]; //64 Chans

// External main funcs (for exec) & stack ptrs
extern void main_P3();       // P3 program
extern void main_P4();       // P4 program
extern void main_P5();       // P5 program
extern void main_console();  // Console program
extern void main_W();
extern void main_PH();
extern uint32_t tos_console; // Base of console's stack
extern uint32_t tos_procs;   // Base of stack for processes



///////////////
// FUNCTIONS //
///////////////

// Works out PCB's priority
uint32_t get_priority(int i){
	return pcb[ i ].age + pcb[ i ].base_priority;
}

/**
  //returns pid of PCB with highest priority
  uint32_t find_next_proc(){
	uint32_t highest_pri = 0;
	uint32_t pri;
	pid_t pid = 0;

	for( int i = 1; i <= tail_pid; i++ ){

		//Program has to be in READY or already EXECUTING 
		if( pcb[ i ].status == STATUS_READY || pcb[ i ].status == STATUS_EXECUTING ){

			pri = get_priority(i);

			if(pri > highest_pri){
				highest_pri = pri;
				pid = i;
			}
		}
	}
	return pid;
  }
*/

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

// Increments all PCB ages except one picked by scheduler
void update_ages(pid_t proc_to_ignore){
	for( int i = 1; i <= tail_pid; i++ ){
    
		if( ( !pcb[ i ].status == STATUS_TERMINATED ) && ( i != proc_to_ignore ) ){
      pcb[ i ].age++; 
    }
    			
	}	
}

 

/*
  //Picks, using priority, next program to execute
  //Alternates between console and programs
  void scheduler(ctx_t* ctx){
  PL011_putc( UART0, 'S', true );

	//If more programs than just the console:
	if( progs_running > 1 ){	
		pid_t next_proc = 0;

		//If console ran last
		if(console_last){
			next_proc = find_next_proc();
			console_last = 0;
		}
		//Otherwise run console (next_proc already 0)
		else{
			console_last = 1;
		}

    update_ages(next_proc);

		//Store old running
   	memcpy( &pcb[ executing ].ctx, ctx, sizeof( ctx_t ) );
   	pcb[ executing ].status = STATUS_READY;  
			
		//Load new running & reset age
		memcpy( ctx, &pcb[ next_proc ].ctx, sizeof( ctx_t ) );
		pcb[ next_proc ].status = STATUS_EXECUTING;
		pcb[ next_proc ].age = 0;
  	executing = next_proc;	
	}
  }
*/

// Basic, round-robin scheduler
void scheduler( ctx_t* ctx ){
  // If not just console running (prevents looping shell$)
  if( progs_running > 1){
    pid_t next_proc = next_alive_prog();
    
    // Store old running
    memcpy( &pcb[ executing ].ctx, ctx, sizeof( ctx_t ) );
    pcb[ executing ].status = STATUS_READY;  
        
    // Load new running
    memcpy( ctx, &pcb[ next_proc ].ctx, sizeof( ctx_t ) );
    pcb[ next_proc ].status = STATUS_EXECUTING;
    executing = next_proc; 
  } 
}


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

	pcb[ 0 ].pid       = 0;
	pcb[ 0 ].status    = STATUS_READY;
	pcb[ 0 ].ctx.cpsr  = 0x50;
	pcb[ 0 ].ctx.pc    = (uint32_t)(&main_console);
	pcb[ 0 ].ctx.sp    = (uint32_t)(&tos_console);
  pcb[ 0 ].stack_top = (uint32_t)(&tos_console);

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
	   	///////////////////////////////////////////////
	   	// 1. Create new child process with unique PID:
	     	
      int free_pid = -1; // Pid of process to become child

      // Search for any existing, free, PCBs:
      for( int i = 1; i <= tail_pid; i++ ){
      	if(pcb[ i ].status == STATUS_TERMINATED){
      		free_pid = i;
      		break;
      	}
      }

      // If none found, create & append new PCB:
      if(free_pid == -1){
	    	free_pid = ++tail_pid;			
      }

      ////////////////////////////////////////
     	// 2. Replicate state of parent in child
     	
      memset( &pcb[ free_pid ], 0, sizeof(pcb_t) );  
	   	memcpy( &pcb[ free_pid ].ctx, ctx, sizeof( ctx_t ) );
    	pcb[ free_pid ].status = STATUS_READY;
    	pcb[ free_pid ].pid = free_pid;
      pcb[ free_pid ].stack_top = (uint32_t)(&tos_procs + (free_pid * stack_size));

      // Copies parent's stack contents into program
      memcpy( pcb[ free_pid ].stack_top - stack_size, pcb[ executing ].stack_top - stack_size, stack_size );
      
      // Sets child proc's sp to same position in parent proc
      uint32_t sp_offset = pcb[ executing ].stack_top - pcb[ executing ].ctx.sp;
      pcb[ free_pid ].ctx.sp = pcb[ free_pid ].stack_top - sp_offset;


    	//////////////////////////////////
     	// 3. Parent and child return vals
     		
     	ctx->gpr[ 0 ] = free_pid;			    // Parent: child's pid
     	pcb[ free_pid ].ctx.gpr[ 0 ] = 0;	// Child: 0


     	//////////////////////////////////////////
     	// 4. Pause parent process & execute child
     		
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
   		//////////////////////////////////////////////////
   		// 1. Replace current process image with new image
     		
   		uint32_t new_prog_img = ctx->gpr[ 0 ];
   		ctx->pc = new_prog_img;

   		/////////////////////////////////////////////////////
   		// 2. Change base priority depending on program image
     		
   		if( strcmp(new_prog_img, &main_P3) == 0 ){pcb[ executing ].base_priority = 1;}
   		if( strcmp(new_prog_img, &main_P4) == 0 ){pcb[ executing ].base_priority = 2;}
      if( strcmp(new_prog_img, &main_P5) == 0 ){pcb[ executing ].base_priority = 4;}
      if( strcmp(new_prog_img, &main_W ) == 0 ){pcb[ executing ].base_priority = 1;}
   		if( strcmp(new_prog_img, &main_PH) == 0 ){pcb[ executing ].base_priority = 1;}

   		////////////////////////////
   		// 3. Reset state (sp & age)
     	
      //Clear stack
      memset( pcb[ executing ].stack_top - stack_size, 0, stack_size);
      
   		ctx->sp = pcb[ executing ].stack_top;
   		pcb[ executing ].age = 0;

   		break;
   	}

    //kill
   	case 0x06 : {
   		PL011_putc( UART0, 'K', true );

   		pid_t pid = ctx->gpr[ 0 ];
      status_t stat = pcb[ pid ].status;
      
      if( stat == STATUS_READY || stat == STATUS_EXECUTING ){
        pcb[ pid ].status = STATUS_TERMINATED;
        //console still running
        if(--progs_running < 1){ progs_running = 1; }
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
    
    //connect to channel
    case 0x10 : {
          
      chid_t chan_id = ctx->gpr[ 0 ];
      
      /* Handle whether channel has:
       * 1) Both ends connected
       *    > fail as requesting to connect 3rd end
       * 2) Just one end connected
       *    > connected the 2nd one & succeed
       * 3) No ends connected.
       *    > create the channel and set pid requesting as p1
       */
      if( chan[ chan_id ].pids_connected == 2 ){
        ctx->gpr[ 0 ] = -1; //fail.
      }
      else if( chan[ chan_id ].pids_connected == 1 ){
        chan[ chan_id ].p2 = pcb[ executing ].pid;
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
    
    //is_connected
    case 0x16: {
      chid_t chid = ctx->gpr[ 0 ];
      chan[ chid ].pids_connected == 2;
      break;
    }
    
    
   	default : {
   		break;
   	}
  }


  return;
}