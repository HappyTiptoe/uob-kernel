/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of 
 * which can be found via http://creativecommons.org (and should be included as 
 * LICENSE.txt within the associated archive or repository).
 */

#include "hilevel.h"


//Which program running
int executing = 0;
int console_next = 0;
int last_pcb_id = 0;

//Program mains & console main
pcb_t pcb[ 32 ];
extern void main_P3(); 
extern void main_P4(); 
extern void main_P5(); 
extern void main_console();
extern uint32_t tos_procs;
extern uint32_t tos_procs;

void scheduler(ctx_t* ctx){

	int next_prog;

	//Store current program
	memcpy( &pcb[ executing ].ctx, ctx, sizeof( ctx_t ) );
	pcb[ executing ].status = STATUS_READY; 

	//Choose next program
	
	if(console_next){
		//Set next program to be console
		next_prog = 0;
		console_next = 0;
	}else{
		//Choose next program based on priority
		//Hard-coded to just last added 
		
		next_prog = last_pcb_id;
		console_next = 1;
	}

	//Load next_to_run program
	memcpy( ctx, &pcb[ next_prog ].ctx, sizeof( ctx_t ) );
	pcb[ next_prog ].status = STATUS_EXECUTING;
	executing = next_prog;	



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
	pcb[0].status = STATUS_EXECUTING;
	pcb[0].ctx.cpsr = 0x50;
	pcb[0].ctx.pc = (uint32_t)(&main_console);
	pcb[0].ctx.sp = (uint32_t)(&tos_procs);

	int_enable_irq();

  	memcpy( ctx, &pcb[ 0 ].ctx, sizeof( ctx_t ) );
  	pcb[ 0 ].status = STATUS_EXECUTING;
  	executing = 0;


	return;
}

void hilevel_handler_irq(ctx_t* ctx) {
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
		case 0x01 :

			int   fd = ( int   )( ctx->gpr[ 0 ] ); 
      		char* x = (char*)(ctx->gpr[1]);  
      		int n = (int)(ctx->gpr[2]); 

      		for(int i = 0; i < n; i++) {
      		  PL011_putc(UART0, *x++, true);
     		 }
      
     		 ctx->gpr[0] = n;
     		 break;

     	//fork
     	case 0x03 :
     		last_pcb_index++; //adding a new PCB
     		memset( &pcb[last_pcb_index], 0, sizeof(pcb_t) ); //Allocate space for the new PCB
			pcb[last_pcb_index] = pcb[executing]; //PCB is same as current executing
			pcb[last_pcb_index].pid = last_pcb_index; //new process, gets new id (-1 because 0 init)
			
			//Store parent
			memcpy( &pcb[executing].ctx, ctx, sizeof( ctx_t ) ); // preserve current running (parent)
    		pcb[executing].status = STATUS_READY;                // update status

			//set to executing
  			memcpy( ctx, &pcb[last_pcb_index].ctx, sizeof( ctx_t ) );
  			pcb[last_pcb_index].status = STATUS_EXECUTING;
  			executing = last_pcb_index;
			break;

     	//exit
     	case 0x04 :
     		coding_and_algorithms();
     		break;

     	//ExeC
     	case 0x05 :
     		pcb[executing].ctx = ctx;
     		pcb[executing].ctx.pc = ctx->gpr[0];
     		pcb[executing].ctx.sp = &tos_procs + (0x00001000 * pcb[executing].pid);
     		break;

	}


  return;
}
