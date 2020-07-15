/**
 * @file main.c
 * @author adarsh.raghoothaman@st.ovgu.de
 * @author elvis.joseph@st.ovgu.de
 * @brief application for sensor node
 * @version 0.1
 * @date 2020-07-15
 * 
 * @copyright Copyright (c) 2020
 * 
 */


#include "sensor_node.h"

int main(void)
{
    init();
    thread_create(register_thread_stack, sizeof(register_thread_stack),
                  THREAD_PRIORITY_MAIN + 4, THREAD_CREATE_STACKTEST,
                  register_thread, NULL, "register_thread");

    thread_create(status_thread_stack, sizeof(status_thread_stack),
                  THREAD_PRIORITY_MAIN + 3, THREAD_CREATE_STACKTEST,
                  status_thread, NULL, "status_thread");

    thread_create(detection_thread_stack, sizeof(detection_thread_stack),
                  THREAD_PRIORITY_MAIN + 2, THREAD_CREATE_STACKTEST,
                  detection_thread, NULL, "detection_thread");

    sensor_run_pid = thread_create(sensor_run_stack, sizeof(sensor_run_stack),
                                   THREAD_PRIORITY_MAIN + 1, THREAD_CREATE_SLEEPING,
                                   sensor_run_thread, NULL, "sensor_run_thread");

    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    puts("All up, running sensor the shell now");
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}