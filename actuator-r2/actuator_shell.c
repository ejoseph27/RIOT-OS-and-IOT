/**
 * @file actuator_shell.c
 * @author adarsh.raghoothaman@st.ovgu.de
 * @author elvis.joseph@st.ovgu.de
 * @brief  file containing shell related functions
 * @version 0.1
 * @date 2020-07-15
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "actuator_node.h"

msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

const shell_command_t shell_commands[] = {
    {"set-server-ip", "configure ipv6 address of the server ", set_server_ip},
    {"set-name", "configure name of the  actuator", set_name},
    {"status", "Displays status of the  actuator", actuator_status},
    {NULL, NULL, NULL}};


 char *SERVER_IP = "fe80::ff:fe00:e";    
int set_server_ip(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("usage: %s \"SERVER_IP_ADDR\" \n", argv[0]);
        return 0;
    }

    else
    {
        SERVER_IP=strdup(argv[1]);
        set_remote(server, strdup(argv[1]));
        printf("server ip set to %s\n",strdup(argv[1]));
        return 0;
    }
}


int set_name(int argc, char **argv)
{

    if (argc != 2)
    {
        printf("usage: %s \"actuator name\" \n", argv[0]);
        return 0;
    }

    else
    {
        set_actuator_name(strdup(argv[1]));
        return 0;
    }
}
int actuator_status(int argc, char **argv)
{
    if (argc != 1)
    {
        printf("usage: %s \n", argv[0]);
        return 0;
    }

    else
    {
        get_status();
        return 0;
    }
}