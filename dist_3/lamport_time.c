//
//  lamport_time.c
//  dist_3
//
//  Created by Даниэль Сеидов on 17.12.2021.
//

#include "ipc.h"


timestamp_t ltime = 0;

timestamp_t get_lamport_time (void) {
    return ltime;
}

timestamp_t set_lamport_time(timestamp_t new_ltime) {
    if (ltime < new_ltime){
        ltime = new_ltime;
    }
    return ltime;
}

timestamp_t inc_lamport_time (void) {
    return ltime ++;
}

timestamp_t ltime_receive_inc (Message * msg) {
    set_lamport_time(msg->s_header.s_local_time);
    return inc_lamport_time();
}
