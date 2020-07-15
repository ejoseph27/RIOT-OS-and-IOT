/**
 * @file sensor_shell.c
 * @author adarsh.raghoothaman@st.ovgu.de
 * @author elvis.joseph@st.ovgu.de
 * @brief  file containing shell related functions
 * @version 0.1
 * @date 2020-07-15
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "sensor_node.h"

msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

const shell_command_t shell_commands[] = {
    {"set-server-ip", "Configure ipv6 address of the server ", set_server_ip},
    {"set-name", "Set the name of the sensor", set_name},
    {"status", "Displays status of the sensor", sensor_status},
    {NULL, NULL, NULL}};

int set_server_ip(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("usage: %s \"SERVER_IP_ADDR\" \n", argv[0]);
        return 0;
    }

    else
    {
        ipv6_addr_t *result = malloc(sizeof(ipv6_addr_t));
        if (ipv6_addr_from_str(result, strdup(argv[1])) == NULL)
        {
            puts("Address format is wrong...");
            return 1;
        }
        free(result);
        set_remote(server, strdup(argv[1]));
        strcpy(server_ip,strdup(argv[1]));
        printf("server ip set to %s\n", server_ip);
        return 0;
    }
}

int set_name(int argc, char **argv)
{

    if (argc != 2)
    {
        printf("usage: %s \"sensor name\" \n", argv[0]);
        return 0;
    }

    else
    {
        set_sensor_name(strdup(argv[1]));
        return 0;
    }
}
int sensor_status(int argc, char **argv)
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
