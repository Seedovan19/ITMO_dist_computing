#include "main.h"


int main(int argc, const char * argv[]) {
    self_data IO;
    timestamp_t beggining_time = 0;
    
    if (argc < 3 && strcmp(argv[1], process_num_key)) {
        log_errors(0);
        exit(0);
    }
    X = atoi(argv[2]);

    
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
                IO.balance_state.s_balance = atoi(argv[2 + id]);
                IO.balance_state.s_balance_pending_in = 0;
                IO.balance_state.s_time = beggining_time;
                IO.from = id;
                IO.to = id;
                balance_history_ptr = (BalanceHistory*)malloc(sizeof(BalanceHistory));
                balance_history_ptr -> s_id = IO.to;
                balance_history_ptr -> s_history_len = 0;
                archieve_balance_history(IO.balance_state.s_balance, IO.balance_state.s_balance_pending_in, beggining_time);
                child_routine(&IO);
                return 0;
            case -1:
                log_errors(1);
                break;
            default: //родительский процесс
                break;
        }
    }
    IO.to = PARENT_ID;
    IO.from = PARENT_ID;
    parent_routine(&IO);
    
    close(fd_events_log);
    close(fd_pipes_log);
    
    while (wait(NULL) > 0);
    free(balance_history_ptr);
    free(pipes);
    
    return 0;
}


void pipes_init (void * arg){
    int open_flag;
    pipes = (pipe_fds *)malloc(X * (X + 1) * sizeof(pipe_fds));
    int pipe_id[2];
    pipe_fds * temp;
    for (int i = 0; i < X + 1; i++) {
        for (int j = 0; j < X + 1; j++) {
            if (i != j) {
                if (pipe(pipe_id) == -1) {
                    log_errors(4);
                };
                open_flag = fcntl(pipe_id[1], F_GETFL);
                fcntl(pipe_id[1], F_SETFL, open_flag | O_NONBLOCK);
                temp = find_pipes_index(i, j);
                temp -> fd_write = pipe_id[1];
                log_pipes(1, i, j);
                
                open_flag = fcntl(pipe_id[0], F_GETFL);
                fcntl(pipe_id[0], F_SETFL, open_flag | O_NONBLOCK);
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
                temp -> fd_write = -1;
                log_pipes(3, i, j);
                
                close(temp -> fd_read);
                temp -> fd_read = -1;
                log_pipes(4, i, j);
            }
        }
    }
}


void parent_routine(self_data * IO) {
    close_excess_pipes(PARENT_ID);
    wait_for_everyone(IO, STARTED);
    log_events(IO, 2, NULL);

    bank_robbery(IO, X);
    message_stop(IO);
    wait_for_everyone(IO, DONE);
    log_events(IO, 4, NULL);

    aggregate_all_history(IO);

    close_pipes(PARENT_ID);
}


void child_routine(self_data * IO) {
    local_id id = IO -> from;
    close_excess_pipes(id);
    system_start(IO);
    child_productive_work(IO);
    sleep(1);
    send_balance_state(IO);

    close_pipes(id);
}


int wait_for_everyone(self_data * IO, MessageType msg_type) {
    Message msg;
    for (local_id i = 1; i < X + 1; i++){
        if (i == IO -> to){
            continue;
        }
        while (receive(IO, i, &msg) < 0);
    }
    return 0;
}


void message_stop(self_data * IO) {
    inc_lamport_time();
    Message msg;
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_local_time = get_lamport_time();
    msg.s_header.s_type = STOP;
    msg.s_header.s_payload_len = 0;
    
    send_multicast(IO, &msg);
}


void system_start(self_data * IO){
    inc_lamport_time();
    Message msg;
    int msg_length = sprintf(msg.s_payload, log_started_fmt, get_lamport_time(), IO -> to, getpid(), getppid(), IO -> balance_state.s_balance);
    
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_payload_len = msg_length;
    msg.s_header.s_type = STARTED;
    msg.s_header.s_local_time = get_lamport_time();

    if (send_multicast(IO, &msg) != 0) {
        printf("Error in multicasting the START messages!\n");
    }
    log_events(IO, 1, NULL);
    
    wait_for_everyone(IO, STARTED);
    log_events(IO, 2, NULL);

}


void wait_for_the_message (self_data * IO, Message * message, MessageType msg_type, local_id from) {
    Message msg;
    while (receive(IO, from, &msg) < 0 || msg.s_header.s_type != msg_type);
    *message = msg;
}


void archieve_balance_history (int amount, int amount_pending_in, timestamp_t current_time) {
    timestamp_t t = get_lamport_time();
    timestamp_t my_time = balance_history_ptr -> s_history[balance_history_ptr -> s_history_len - 1].s_time + 1;
    
    for (int i = 0; i < balance_history_ptr -> s_history_len; i++) {
        if (balance_history_ptr -> s_history[i].s_time == current_time) {
            if (i > 0 && i <= balance_history_ptr -> s_history_len) {
                balance_history_ptr -> s_history[i].s_time = current_time;
                balance_history_ptr -> s_history[i].s_balance = amount;
                balance_history_ptr -> s_history[i].s_balance_pending_in = amount_pending_in;
                return;
            }
        }
    }
    
    if (my_time < t) {
        while (my_time <= t) {
            balance_history_ptr -> s_history[balance_history_ptr -> s_history_len].s_time = my_time;
            balance_history_ptr -> s_history[balance_history_ptr -> s_history_len].s_balance = amount;
            balance_history_ptr -> s_history[balance_history_ptr -> s_history_len].s_balance_pending_in = 0;
            balance_history_ptr -> s_history_len++;
            my_time ++;
        }
        return;
    }
    balance_history_ptr -> s_history[balance_history_ptr -> s_history_len].s_time = current_time;
    balance_history_ptr -> s_history[balance_history_ptr -> s_history_len].s_balance = amount;
    balance_history_ptr -> s_history[balance_history_ptr -> s_history_len].s_balance_pending_in = amount_pending_in;
        
    balance_history_ptr -> s_history_len ++;
}


void transfer(void * parent_data, local_id src, local_id dst, balance_t amount) {
    inc_lamport_time();

    Message msg;
    TransferOrder *transfer_order;
    transfer_order = (TransferOrder *)msg.s_payload;
    transfer_order -> s_src = src;
    transfer_order -> s_dst = dst;
    transfer_order -> s_amount = amount;
    
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_type = TRANSFER;
    msg.s_header.s_local_time = get_lamport_time();
    msg.s_header.s_payload_len = sizeof(TransferOrder);
    
    if (send(parent_data,src,&msg) != 0) {
        printf("Error in transfer!\n");
    }
    
    wait_for_the_message(parent_data, &msg, ACK, dst);
}


void child_productive_work(self_data * IO) {
    int done_received = X - 1;
    int stop_flag = 0;
    
    
    while(done_received || !stop_flag) {
        Message msg;
        if (receive_any(IO, &msg) == 0) {
            if (msg.s_header.s_type == TRANSFER) {
                TransferOrder * transfer_order = (TransferOrder *)msg.s_payload;
                if(IO -> from == PARENT_ID) {
                    archieve_balance_history(IO -> balance_state.s_balance, 0, msg.s_header.s_local_time); //пишем сколько у нас денег было все это время при первом получении трансфера
                    inc_lamport_time();
                    IO -> balance_state.s_balance -= transfer_order -> s_amount;
                    IO -> balance_state.s_time = get_lamport_time();
                    IO -> balance_state.s_balance_pending_in += transfer_order -> s_amount;
                    
                    archieve_balance_history(IO->balance_state.s_balance, IO -> balance_state.s_balance_pending_in, IO -> balance_state.s_time);
                    archieve_balance_history(IO->balance_state.s_balance, IO -> balance_state.s_balance_pending_in, IO -> balance_state.s_time+1); //записываем вручную второй раз, чтобы включить поле pending_in

                    log_events(IO, 5, transfer_order);

                    IO -> from = IO -> to;
                    msg.s_header.s_local_time = get_lamport_time();
                    send(IO, transfer_order -> s_dst, &msg);
                }
                else {
                    archieve_balance_history(IO -> balance_state.s_balance, 0, msg.s_header.s_local_time);
                    
                    inc_lamport_time();
                    IO -> balance_state.s_balance += transfer_order -> s_amount;
                    IO -> balance_state.s_time = get_lamport_time();
                    IO -> balance_state.s_balance_pending_in = 0;

                    archieve_balance_history(IO->balance_state.s_balance, IO -> balance_state.s_balance_pending_in, IO -> balance_state.s_time);
                    log_events(IO, 6, transfer_order);
                    
                    
                    Message msg;
                    msg.s_header.s_magic = MESSAGE_MAGIC;
                    msg.s_header.s_local_time = IO -> balance_state.s_time;
                    msg.s_header.s_type = ACK;
                    msg.s_header.s_payload_len = 0;
                    IO -> from = IO -> to;
                    send(IO, PARENT_ID, &msg);
                }
            } else if (msg.s_header.s_type == STOP) {
                archieve_balance_history(IO->balance_state.s_balance, 0, msg.s_header.s_local_time);
                IO -> from = IO -> to;
                printf("Child process %d got STOP message\n", IO -> to);;
                system_done(IO);
                stop_flag = 1;
            } else if (msg.s_header.s_type == DONE) {
                IO -> from = IO -> to;
                done_received --;
            } else {
                printf("Oh nooo...  I should not get messages with such header >>>>> %x\n", msg.s_header.s_type);
                IO -> from = IO -> to;
            }
        }
    }
    printf("Child process %d finished the work\n", IO -> to);
}


void system_done(self_data * IO) {
    inc_lamport_time();
    Message msg;
    int msg_length = sprintf(msg.s_payload, log_done_fmt, get_lamport_time(), IO -> to, IO -> balance_state.s_balance);
    
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_payload_len = msg_length;
    msg.s_header.s_type = DONE;
    msg.s_header.s_local_time = get_lamport_time();
    
    
    if (send_multicast(IO, &msg) != 0) {
        printf("Error in multicasting the DONE messages!\n");
    }
    log_events(IO, 3, NULL);
}


void aggregate_all_history (self_data * IO) {
    Message msg;
    AllHistory history;
    history.s_history_len = X;
    for (int i = 1; i <= X; i++) {
        wait_for_the_message(IO, &msg, BALANCE_HISTORY, i);
        history.s_history[i-1] = *(BalanceHistory*)msg.s_payload;
    }
    
    print_history(&history);
}


void send_balance_state(self_data * IO) {
    inc_lamport_time();
    Message msg;
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_local_time = get_lamport_time();
    msg.s_header.s_type = BALANCE_HISTORY;
    msg.s_header.s_payload_len = sizeof(BalanceState)*(balance_history_ptr->s_history_len) + sizeof(balance_history_ptr->s_history_len) + sizeof(balance_history_ptr->s_id);

    *(BalanceHistory*)msg.s_payload = *balance_history_ptr;
    while (send(IO, PARENT_ID, &msg) < 0);
}


void close_pipes(local_id id) {
    pipe_fds * temp;
    
    for (local_id i = 0; i < X; i++) {
        if (i != id) {
            temp = find_pipes_index(id, i);
            close(temp -> fd_read);
            temp -> fd_read = -1;
            log_pipes(4, id, i);
            
            close(temp -> fd_write);
            temp -> fd_write = -1;
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


void log_events (self_data * IO, int log_num, TransferOrder * transfer_order) {
    char buffer[MAX_PAYLOAD_LEN];
    switch (log_num) {
        case 1:
            sprintf(buffer, log_started_fmt, get_lamport_time(), IO -> to, getpid(), getppid(), IO -> balance_state.s_balance);
            break;
        case 2:
            sprintf(buffer, log_received_all_started_fmt, get_lamport_time(),  IO -> to);
            break;
        case 3:
            sprintf(buffer, log_done_fmt, get_lamport_time(), IO -> to, IO -> balance_state.s_balance);
            break;
        case 4:
            sprintf(buffer, log_received_all_done_fmt, get_lamport_time(), IO -> to);
            break;
        case 5:
            sprintf(buffer, log_transfer_out_fmt, get_lamport_time(), transfer_order -> s_src, transfer_order -> s_amount,transfer_order -> s_dst);
            break;
        case 6:
            sprintf(buffer, log_transfer_in_fmt, get_lamport_time(), transfer_order -> s_dst, transfer_order -> s_amount, transfer_order -> s_src);
        default:
            break;
    }
    printf(buffer, 0);
    if (write(fd_events_log, buffer, strlen(buffer)) == -1) {
        printf("Не смог записать в файл!\n");
    }
}


void log_pipes (int log_num, int from, int to) {
    char buffer[MAX_PAYLOAD_LEN];
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
    if (write(fd_pipes_log, buffer, strlen(buffer)) == -1) {
        printf("Не смог записать в файл!\n");
    }
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
    self_data* IO = (self_data*)self;
    pipe_fds * temp = find_pipes_index (IO -> to, dst);
    if (dst == IO -> to) {
        return -1;
    }
    
    ssize_t result = write(temp -> fd_write, msg, msg -> s_header.s_payload_len + sizeof(MessageHeader));
    
    if (result < 0) {
        return -1;
    }
    return 0;
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
    self_data* IO = (self_data*)self;

    for (local_id i = 0; i < X + 1; i++) {
        if (i != IO -> from) {
            send(IO, i, msg);
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
    self_data* IO = (self_data*)self;
    pipe_fds *temp = find_pipes_index(IO -> to, from);
    
    if (from == IO -> to) {
        return -1;
    }

    if (read(temp->fd_read, &msg->s_header, sizeof(MessageHeader)) < 0) {
        return -1;
    }
    
    if (msg->s_header.s_payload_len > 0) {
        if (read(temp->fd_read, msg->s_payload, msg->s_header.s_payload_len) < 0) {
            return -1;
        }
    }
    
    ltime_receive_inc(msg);
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
    self_data* IO = (self_data*)self;
    
    for (local_id i = 0; i < X + 1; i++) {
        
        if (i != IO -> to) {
            int check = receive(self, i, msg);
            
            if (check == -1) {
                continue;
            } else {
                IO -> from = i;
                return 0;
            }
        }
    }
    return -1;
}
