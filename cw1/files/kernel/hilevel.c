/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of 
 * which can be found via http://creativecommons.org (and should be included as 
 * LICENSE.txt within the associated archive or repository).
 */

#include "hilevel.h"


//Which program running
int executing = 0;
int tail_pid = 0;	//Last PCB element's pid
int console_last;

//PCB Definitions
pcb_t pcb[ 32 ];			//Init space for 32 PCBs
extern void main_P3(); 		//P3 program
extern void main_P4(); 		//P4 program
extern void main_P5(); 		//P5 program
extern void main_console();	//Console program
extern uint32_t tos_procs;	//Base of stack for processes


//works out PCB's priority
uint32_t get_priority(int i){
	return pcb[ i ].age + pcb[ i ].base_priority;
}


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


//increments all PCB ages except one picked by scheduler
void update_ages(pid_t proc_to_ignore){
	for( int i = 1; i <= tail_pid; i++ ){
		if( pcb[ i ].status == STATUS_READY || pcb[ i ].status != STATUS_EXECUTING ){
			if( i != proc_to_ignore ){
				pcb[ i ].age++;
			}
		}			
	}	
}


//Picks, using priority, next program to execute
//Alternates between console and programs
void scheduler(ctx_t* ctx){

	//If more programs than just the console:
	if( tail_pid > 0 ){	
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


void hilevel_handler_rst(ctx_t* ctx) {
	TIMER0->Timer1Load  = 0x00100000; // select period = 2^20 ticks ~= 1 sec
	TIMER0->Timer1Ctrl  = 0x00000002; // select 32-bit   timer
 	TIMER0->Timer1Ctrl |= 0x00000040; // select periodic timer
 	TIMER0->Timer1Ctrl |= 0x00000020; // enable          timer interrupt
 	TIMER0->Timer1Ctrl |= 0x00000080; // enable          timer

	GICC0->PMR          = 0x000000F0; // unmask all            interrupts
	GICD0->ISENABLER1  |= 0x00000010; // enable timer          interrupt
	GICC0->CTLR         = 0x00000001; // enable GIC interface
	GICD0->CTLR         = 0x00000001; // enable GIC distributor

	//Initialise console PCB:
	memset( &pcb[0], 0, sizeof(pcb_t) );

	pcb[0].pid = 0;
	pcb[0].status = STATUS_READY;
	pcb[0].ctx.cpsr = 0x50;
	pcb[0].ctx.pc = (uint32_t)(&main_console);
	pcb[0].ctx.sp = (uint32_t)(&tos_procs);

	int_enable_irq();

	//Execute console
  memcpy( ctx, &pcb[ 0 ].ctx, sizeof( ctx_t ) );
  pcb[ 0 ].status = STATUS_EXECUTING;
  executing = 0;
  	console_last = 1;

	return;
}


void hilevel_handler_irq(ctx_t* ctx) {
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


void hilevel_handler_svc(ctx_t* ctx, uint32_t id) {

	switch(id){

		//write
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

    //fork
    case 0x03 : {
	     	
	   	//////////////////////////////////////////////////
	   	// 1. Create new child process with unique PID: //
	   	//////////////////////////////////////////////////
	     	
      int free_pid = 0; //pid of process to become child

      //Search for any existing, free, PCBs:
      for( int i = 1; i <= tail_pid; i++ ){
      	if(pcb[ i ].status == STATUS_TERMINATED){
      		free_pid = i;
      		break;
      	}
      }

      //If none found, create & append new PCB:
      if(free_pid == 0){
      	PL011_putc( UART0, 'N', true );
	    	tail_pid++; 									//Creating new PCB
	    	free_pid = tail_pid; 							//PCB to become child = new PCB
	    	memset( &pcb[ free_pid ], 0, sizeof(pcb_t) );	//Allocate phys memory for child     			
      }

      ///////////////////////////////////////////
     	// 2. Replicate state of parent in child //
     	///////////////////////////////////////////
     	
	   	memcpy( &pcb[ free_pid ].ctx, ctx, sizeof( ctx_t ) );	//Copy parent prog into child
    	pcb[ free_pid ].status = STATUS_CREATED; 				//Child process created
    	pcb[ free_pid ].pid = free_pid;							//Set pid equal to its position in PCB list

    	/////////////////////////////////////
     	// 3. Parent and child return vals //
     	/////////////////////////////////////
     		
     	ctx->gpr[ 0 ] = free_pid;			//PARENT: child.pid
     	pcb[ free_pid ].ctx.gpr[ 0 ] = 0;	//CHILD: 0

     	////////////////////////////////////////////
     	//4. Pause parent process & execute child //
     	////////////////////////////////////////////
     		
     	memcpy( &pcb[ executing ].ctx, ctx, sizeof( ctx_t ) );	//Preserve current running
    	pcb[ executing ].status = STATUS_READY;					//Update status

  		memcpy( ctx, &pcb[ free_pid ].ctx, sizeof( ctx_t ) );	//Load child into current
  		pcb[ free_pid ].status = STATUS_EXECUTING;				//Update status
  		executing = free_pid;
     		
  		break;		
		}

		//exit
		case 0x04 : {

      pcb[ executing ].status = STATUS_TERMINATED;
      //scheduler(ctx);
      executing = 0; //Jump to console
			break;
		}

    //exec
    case 0x05 : {

   		/////////////////////////////////////////////////////
   		// 1. Replace current process image with new image //
   		/////////////////////////////////////////////////////
     		
   		uint32_t new_prog_img = ctx->gpr[ 0 ];
   		ctx->pc = new_prog_img;

   		////////////////////////////////////////////////////////
   		// 2. Change base priority depending on program image //
   		////////////////////////////////////////////////////////
     		
   		if( strcmp(new_prog_img, &main_P3) == 0 ){pcb[ executing ].base_priority = 1;}
   		if( strcmp(new_prog_img, &main_P4) == 0 ){pcb[ executing ].base_priority = 2;}
   		if( strcmp(new_prog_img, &main_P5) == 0 ){pcb[ executing ].base_priority = 4;}

   		///////////////////////////////
   		// 3. Reset state (sp & age) //
   		///////////////////////////////
     		
   		ctx->sp = (uint32_t)(&tos_procs + 4096*pcb[ executing ].pid );
   		pcb[ executing ].age = 0;

   		break;
   	}

   	case 0x06 : {
   		PL011_putc( UART0, 'K', true );

   		pid_t pid = ctx->gpr[ 0 ];
   		pcb[ pid ].status = STATUS_TERMINATED;
      scheduler(ctx);
   		break;
   	}

   	default : {
   		break;
   	}
  }


  return;
}
