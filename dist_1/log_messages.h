//
//  log_messages.h
//  dist_1
//
//  Created by Даниэль Сеидов
//

#ifndef __IFMO_DISTRIBUTED_CLASS_LOG_MESSAGES__H
#define __IFMO_DISTRIBUTED_CLASS_LOG_MESSAGES__H

static const char * const process_num_key =
  "-p";

// Error messages
static const char * const args_error =
    "Some problem with arguments!\n";
static const char * const fork_error =
    "Oops..fork error has occured\n";
static const char * const open_events_log_file_error =
    "Sorry, could not open the events log file\n";
static const char * const open_pipes_log_file_error =
    "Sorry, could not open the pipes log file\n";
static const char * const open_pipe_error =
    "Could not create pipe!\n";


// Pipe log messages
static const char * const log_opened_write_pipe =
    "Write pipe opened! From process with id %d to process with id %d\n";
static const char * const log_opened_read_pipe =
    "Read pipe opened! Reading from process with id %d in process with id %d\n";
static const char * const log_closed_write_pipe =
    "Closed write pipe. Was writing from %d to %d, \n";
static const char * const log_closed_read_pipe =
    "Closed read pipe. Was reading in %d from %d, \n";

#endif /* log_messages_h */

