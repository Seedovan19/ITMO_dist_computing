//
//  main.h
//  dist_2
//
//  Created by Даниэль Сеидов
//
#ifndef __IFMO_DISTRIBUTED_CLASS_MAIN__H
#define __IFMO_DISTRIBUTED_CLASS_MAIN__H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "log_messages.h"
#include "pa2345.h"
#include "banking.h"


// Memory structures
typedef struct {
    int fd_read;
    int fd_write;
} pipe_fds;

typedef struct {
    local_id from;
    local_id to;
    BalanceState balance_state;
} self_data;


// Global variables
int fd_events_log;
int fd_pipes_log;
int X;
pipe_fds * pipes = NULL;
BalanceHistory* balance_history_ptr = NULL;


// Functions' initializers
void parent_routine(self_data * IO);
void child_routine(self_data * IO);
void system_start(self_data * IO);
void wait_for_the_message (self_data * IO, Message * msg, MessageType msg_type, local_id from);
void archieve_balance_history (int amount, timestamp_t current_time);
int wait_for_everyone(self_data * IO, MessageType msg_type);
void child_productive_work(self_data * IO);
void message_stop(self_data * IO);
void system_done(self_data * IO);
void aggregate_all_history (self_data * IO);
void send_balance_state(self_data * IO);

// Log functions
void log_errors (int err_num);
void log_events (self_data * IO, int log_num, TransferOrder * transfer_order);
void log_pipes (int log_num, int from, int to);

// Pipes functions
void pipes_init (void * arg);
pipe_fds * find_pipes_index (int from, int to);
void close_excess_pipes(local_id id);
void close_pipes(local_id id);


#endif /* main_h */

