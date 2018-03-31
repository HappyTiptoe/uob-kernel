/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of 
 * which can be found via http://creativecommons.org (and should be included as 
 * LICENSE.txt within the associated archive or repository).
 */

.global lolevel_handler_rst
.global test

lolevel_handler_rst: sub   lr, lr, #0   
					 msr   cpsr, #0xD3             @ enter SVC mode with IRQ and FIQ interrupts disabled
                     ldr   sp, =tos_svc            @ initialise SVC mode stack

                     bl    hilevel_handler_rst     @ invoke high-level C function
                     bl	   hilevel_handler_rst	   @ invoke again
                     

test: msr   cpsr, #0xD3             @ enter SVC mode with IRQ and FIQ interrupts disabled
      ldr   sp, =tos_svc            @ initialise SVC mode stack
      bl    myfunc     				@ invoke high-level C function
      bl	myfunc	   				@ invoke again
            bl	myfunc	   				@ invoke again
      b     .          				@ halt
