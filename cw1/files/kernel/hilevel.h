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
  STATUS_CREATED,
  STATUS_READY,
  STATUS_EXECUTING,
  STATUS_WAITING,
  STATUS_TERMINATED
} status_t;

typedef enum {
  STATUS_NONE,
  STATUS_ONE,
  STATUS_BOTH
} chan_status_t;

typedef struct {
  uint32_t cpsr, pc, gpr[ 13 ], sp, lr;
} ctx_t;

typedef struct {
  pid_t    pid;
  status_t status;
  ctx_t    ctx;
  uint32_t age;
  uint32_t base_priority;
  chid_t   chan_end;
} pcb_t;

typedef struct {
  pid_t  p1;              //process DOING THE SENDING
  pid_t  p2;              //process to send to
  chid_t chan_id;         //for identifying
  int    pids_connected;  //to prevent too many connections
  int    data_on_chan;    //flag for if data ready to be read
  int    data;            //value of data to be sent across
} chan_t;

#endif




