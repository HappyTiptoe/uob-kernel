/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of 
 * which can be found via http://creativecommons.org (and should be included as 
 * LICENSE.txt within the associated archive or repository).
 */

#ifndef __HILEVEL_H
#define __HILEVEL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <string.h>

// Include functionality relating to the platform.

#include   "GIC.h"
#include "PL011.h"
#include "SP804.h"


// Include functionality relating to the kernel.

#include "lolevel.h"
#include     "int.h"

typedef int pid_t;
typedef int chid_t;

typedef enum { 
  STATUS_READY,
  STATUS_CREATED,
  STATUS_EXECUTING,
  STATUS_TERMINATED
} status_t;

typedef struct {
  uint32_t cpsr, pc, gpr[ 13 ], sp, lr;
} ctx_t;

typedef struct {
  pid_t    pid;
  status_t status;
  ctx_t    ctx;
  uint32_t age;
  uint32_t base_priority;
  uint32_t stack_top;
  chid_t   chan_end;
} pcb_t;

typedef struct {
  pid_t  p1;              
  pid_t  p2;              
  chid_t chan_id;            //for identifying
  int    pids_connected;     //to prevent too many connections
  bool   data_on_chan_to_p1; //flag for if data ready for p1 to read
  bool   data_on_chan_to_p2; //flag for if data ready for p2 to read
  int    data_for_p1;        //value of data to be sent across
  int    data_for_p2;        //value of data to be sent across
} chan_t;

typedef struct {
  bool     flag; //1 = data inside
  bool     open;
  char*    data;
  size_t   data_size;
  status_t status;
} pipeend_t;

#endif




