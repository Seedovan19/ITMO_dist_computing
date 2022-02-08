//
//  main.h
//  distributive_1
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
#include "common.h"
#include "ipc.h"
#include "pa1.h"
#include "log_messages.h"


// Memory structures
typedef struct {
    int fd_read;
    int fd_write;
} pipe_fds;


// Global variables
int fd_events_log;
int fd_pipes_log;
int X;
pipe_fds * pipes = NULL;


// Functions' initializers
void parent_routine(local_id id);
void child_routine(local_id id);
void system_start(local_id id);
int wait_for_everyone(local_id id, Message * msg, MessageType msg_type);
void system_done(local_id id);

// Log functions
void log_errors (int err_num);
void log_events (local_id id, int log_num);
void log_pipes (int log_num, int from, int to);

// Pipes functions
void pipes_init (void * arg);
pipe_fds * find_pipes_index (int from, int to);
void close_excess_pipes(local_id id);
void close_pipes(local_id id);


#endif /* main_h */

