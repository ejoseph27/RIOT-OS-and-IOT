/**
 * @file main.c
 * @author adarsh.raghoothaman@st.ovgu.de
 * @author elvis.joseph@st.ovgu.de
 * @brief  application for gateway/controller for alarm system   
 * @version 0.1
 * @date 2020-07-15
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "gateway_node.h"


int main(void)
{
    log_rcv_pid = thread_create(log_thread_stack, sizeof(log_thread_stack),
                        THREAD_PRIORITY_MAIN -1, THREAD_CREATE_STACKTEST, log_thread, NULL, "log");   
    init();
    thread_create(wdt_thread_stack, sizeof(wdt_thread_stack),
                  THREAD_PRIORITY_MAIN + 2, THREAD_CREATE_STACKTEST,
                  wdt_thread, NULL, "wdt_thread");

    thread_create(alarm_thread_stack, sizeof(alarm_thread_stack),
                  THREAD_PRIORITY_MAIN + 1, THREAD_CREATE_STACKTEST,
                  alarm_thread, NULL, "alarm_thread");  
            

    
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    puts("All up, running the shell now");
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}