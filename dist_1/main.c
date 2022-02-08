//
//  main.c
//  distributive_1
//
//  Created by Даниэль Сеидов P34211
//

#include "main.h"


int main(int argc, const char * argv[]) {
    if (argc == 3 && !strcmp(argv[1], process_num_key)) {
        X = atoi(argv[2]);
    } else {
        log_errors(0);
        exit(0);
    }
    
    fd_events_log = open(events_log, O_WRONLY | O_APPEND | O_CREAT | O_TRUNC, 0777);
    if (fd_events_log < 0) log_errors(2);
    
    fd_pipes_log = open(pipes_log, O_WRONLY | O_APPEND | O_CREAT | O_TRUNC, 0777);
    if (fd_pipes_log < 0) log_errors(3);

    pipes_init(NULL);
    local_id id = 0;
    for (int i = 0; i < X; i++) {
        id ++;
        int tmp = fork();
        switch (tmp) {
            case 0:
                child_routine(id);
                exit(0);
                break;
            case -1:
                log_errors(1);
                break;
            default: //родительский процесс
                break;
        }
    }
    
    parent_routine(PARENT_ID);
    
    close(fd_events_log);
    close(fd_pipes_log);
    
    while (wait(NULL) > 0);
    
    return 0;
}


void pipes_init (void * arg){
    pipes = (pipe_fds *)malloc(X * (X + 1) * sizeof(pipe_fds));
    int pipe_id[2];
    pipe_fds * temp;
    for (int i = 0; i < X + 1; i++) {
        for (int j = 0; j < X + 1; j++) {
            if (i != j) {
                if (pipe(pipe_id) == -1) {
                    log_errors(4);
                };
                temp = find_pipes_index(i, j);
                temp -> fd_write = pipe_id[1];
                log_pipes(1, i, j);

                
                temp = find_pipes_index(j, i);
                temp -> fd_read = pipe_id[0];
                log_pipes(2, i, j);
            }
        }
    }
}


pipe_fds * find_pipes_index (int from, int to) {
    int num = from * X + (to >= from  ? (to-1) : to);
    pipe_fds * index = pipes + (size_t)num;
    return index;
}


void close_excess_pipes(local_id id) {
    pipe_fds * temp;

    for (local_id i = 0; i < X + 1; i++) {
        if (i == id)
            continue;
        for (local_id j = 0; j < X + 1; j++) {
            if (i != j) {
                temp = find_pipes_index(i, j);
                close(temp -> fd_write);
                log_pipes(3, i, j);
                close(temp -> fd_read);
                log_pipes(4, i, j);
            }
        }
    }
}


void parent_routine(local_id id) {
    close_excess_pipes(id);
    Message msg;
    wait_for_everyone(id, &msg, STARTED);
    wait_for_everyone(id, &msg, DONE);
    while (wait(NULL) > 0);
    close_pipes(id);
    free(pipes);
}


void child_routine(local_id id) {
    close_excess_pipes(id);
    system_start(id);
    
    int work = 0;
    while(work < 191919) work++; // "полезная работа"
    
    system_done(id);
    close_pipes(id);
}


int wait_for_everyone(local_id id, Message * msg, MessageType msg_type) {
    for (int i = 1; i < X + 1; i++) {
        if (i != id) {
            do {
                receive(&id, i, msg);
            } while (msg -> s_header.s_type != msg_type);
        }
    }
    return 0;
}


void system_start(local_id id){
    Message msg;
    int msg_length = sprintf(msg.s_payload, log_started_fmt, id, getpid(), getppid());
    
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_payload_len = msg_length;
    msg.s_header.s_type = STARTED;
    msg.s_header.s_local_time = time(NULL);
    
    if (send_multicast(&id, &msg) != 0) {
        exit(1);
    }
    log_events(id, 1);

    wait_for_everyone(id, &msg, STARTED);
    log_events(id, 2);
}


void system_done(local_id id) {
    Message msg;
    int msg_length = sprintf(msg.s_payload, log_done_fmt, id);
    
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_payload_len = msg_length;
    msg.s_header.s_type = DONE;
    msg.s_header.s_local_time = time(NULL);
    
    if (send_multicast(&id, &msg) != 0) {
        exit(1);
    }
    log_events(id, 3);
    
    wait_for_everyone(id, &msg, DONE);
    log_events(id, 4);
}


void close_pipes(local_id id) {
    pipe_fds * temp;

    for (local_id i = 0; i < X + 1; i++) {
        if (i != id) {
            temp = find_pipes_index(id, i);
            close(temp -> fd_read);
            log_pipes(4, id, i);
            close(temp -> fd_write);
            log_pipes(3, id, i);
        }
    }
}


void log_errors (int err_num) {
    switch (err_num) {
        case 0:
            printf("%s", args_error);
            break;
        case 1:
            printf("%s", fork_error);
            break;
        case 2:
            printf("%s", open_events_log_file_error);
            break;
        case 3:
            printf("%s", open_pipes_log_file_error);
        case 4:
            printf("%s", open_pipe_error);
        default:
            break;
    }
}


void log_events (local_id id, int log_num) {
    char buffer[200];
    switch (log_num) {
        case 1:
            sprintf(buffer, log_started_fmt, id, getpid(), getppid());
            break;
        case 2:
            sprintf(buffer, log_received_all_started_fmt, id);
            break;
        case 3:
            sprintf(buffer, log_done_fmt, id);
            break;
        case 4:
            sprintf(buffer, log_received_all_done_fmt, id);
            break;
        default:
            break;
    }
    printf(buffer, 0);
    write(fd_events_log, buffer, strlen(buffer));
}


void log_pipes (int log_num, int from, int to) {
    char buffer[200];
    switch (log_num) {
        case 1:
            sprintf(buffer, log_opened_write_pipe, from, to);
            break;
        case 2:
            sprintf(buffer, log_opened_read_pipe, from, to);
            break;
        case 3:
            sprintf(buffer, log_closed_write_pipe, from, to);
            break;
        case 4:
            sprintf(buffer, log_closed_read_pipe, from, to);
            break;
        default:
            break;
    }
    printf(buffer,0);
    write(fd_pipes_log, buffer, strlen(buffer));
}


/** Send a message to the process specified by id.
 *
 * @param self    Any data structure implemented by students to perform I/O
 * @param dst     ID of recepient
 * @param msg     Message to send
 *
 * @return 0 on success, any non-zero value on error
 *  */
int send(void * self, local_id dst, const Message * msg) {
    local_id from = *(local_id *) self;
    pipe_fds * temp = find_pipes_index (from, dst);
    long int result = write(temp -> fd_write, msg, msg -> s_header.s_payload_len + sizeof(MessageHeader));

    if (result > 0) {
        return 0;
    } else {
        return -1;
    }
}
/** Send multicast message.
 *
 * Send msg to all other processes including parrent.
 * Should stop on the first error.
 *
 * @param self    Any data structure implemented by students to perform I/O
 * @param msg     Message to multicast.
 *
 * @return 0 on success, any non-zero value on error
 */
int send_multicast(void * self, const Message * msg) {
    local_id from = *(local_id *) self;
    
    for (local_id i = 0; i < X + 1; i++) {
        if (i != from) {
            if (send(self, i, msg) != 0) {
                return -1;
            }
        }
    }
    return 0;
}
/** Receive a message from the process specified by id.
 *
 * Might block depending on IPC settings.
 *
 * @param self    Any data structure implemented by students to perform I/O
 * @param from    ID of the process to receive message from
 * @param msg     Message structure allocated by the caller
 *
 * @return 0 on success, any non-zero value on error
 */
int receive(void * self, local_id from, Message * msg) {
    local_id current_proc_id = *(local_id *)self;
    pipe_fds *temp = find_pipes_index(current_proc_id, from);
    
    if (read(temp->fd_read, &msg->s_header, sizeof(MessageHeader)) != 0)
        return -1;
    if (msg->s_header.s_payload_len > 0) {
        if (read(temp->fd_read, msg->s_payload, msg->s_header.s_payload_len) != 0)
            return -1;
    }
    return 0;
}

/** Receive a message from any process.
 *
 * Receive a message from any process, in case of blocking I/O should be used
 * with extra care to avoid deadlocks.
 *
 * @param self    Any data structure implemented by students to perform I/O
 * @param msg     Message structure allocated by the caller
 *
 * @return 0 on success, any non-zero value on error
 */
int receive_any(void * self, Message * msg) {
    local_id current_proc_id = *(local_id *)self;

    for (local_id i = 1; i < X + 1; i++) {
        if (i != current_proc_id) {
            if (receive(&current_proc_id, i, msg) != 0)
                return -1;
        }
    }
    return 0;
}
