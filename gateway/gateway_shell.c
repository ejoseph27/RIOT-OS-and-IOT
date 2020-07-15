/**
 * @file gateway_shell.c
 * @author adarsh.raghoothaman@st.ovgu.de
 * @author elvis.joseph@st.ovgu.de
 * @brief  file containing shell related  functions
 * @version 0.1
 * @date 2020-07-15
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "gateway_node.h"

msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

const shell_command_t shell_commands[] = {
    {"auth_sensor", "Authenticate sensor node to the server ", auth_sensor_node},
    {"deauth_sensor", "Deauthenticate sensor node from the server ", de_auth_sensor_node},
    {"auth_actuator", "Authenticate actuator node to the server ", auth_actuator_node},
    {"deauth_actuator", "Deauthenticate actuator node from the server ", de_auth_actuator_node},
    {"engage", "Engage alarm", engage_alarm},
    {"disengage", "Disengage alarm", disengage_alarm},
    {"show-actuators", "Displays the authorized actuator nodes ", show_actuators},
    {"log", "Display the Alarm server log", show_log},
    {NULL, NULL, NULL}};

char *SENSOR_ID;
int auth_sensor_node(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("usage: %s \"NAME\" \n", argv[0]);
        return 0;
    }

    else
    {
        SENSOR_ID = strdup(argv[1]);
        if (strcmp((ht_get(ht_sensor, SENSOR_ID)),SENSOR_ID) == 0)
        {
            puts("Node already authorized!");
            SENSOR_ID = NULL;
            return 1;
        }
        if (!register_node(SENSOR_ID))
        {
            send_log_msg("Sensor authorized: ", SENSOR_ID);
            
            send_register_response(&(ht_get_node(ht_sensor, SENSOR_ID)->remote));
            SENSOR_ID = NULL;
            return 0;
        }
        SENSOR_ID = NULL;
        return 1;
    }
}



int de_auth_sensor_node(int argc, char **argv)
{

    if (argc != 2)
    {
        printf("usage: %s \"SENSOR_NAME\" \n", argv[0]);
        return 0;
    }

    else
    {
        SENSOR_ID = strdup(argv[1]);
        if (!remove_sensor(SENSOR_ID))
        {
            printf("sensor %s is de-authorized \n", SENSOR_ID);
            send_log_msg("Sensor deauthorized", SENSOR_ID);
            SENSOR_ID = NULL;
            return 0;
        }
        else
        {
            puts("Sensor not found!");
            SENSOR_ID = NULL;
            return 1;
        }
    }
}

char *ACTUATOR_ID;
int auth_actuator_node(int argc, char **argv)
{

    if (argc != 2)
    {
        printf("usage: %s \"NAME\" \n", argv[0]);
        return 0;
    }

    else
    {
        ACTUATOR_ID = strdup(argv[1]);
        if (!node_exists(ACTUATOR_ID))
        {
            puts("Node already authorized!");
            ACTUATOR_ID = NULL;
            return 1;
        }
       
        if (!register_node(ACTUATOR_ID))
        {
            send_log_msg("Actuator authorized: ", ACTUATOR_ID);
            send_register_response(&get_node(ACTUATOR_ID)->remote);
            ACTUATOR_ID = NULL;
            return 0;
        }
        ACTUATOR_ID = NULL;
        return 1;
    }
}

int de_auth_actuator_node(int argc, char **argv)
{

    if (argc != 2)
    {
        printf("usage: %s \"ACTUATOR_NAME\" \n", argv[0]);
        return 0;
    }

    else
    {
        ACTUATOR_ID = strdup(argv[1]);
        if (!remove_actuator(ACTUATOR_ID))
        {
            printf("actuator %s is de-authorized \n", ACTUATOR_ID);
            send_log_msg("Actuator deauthorized: ", ACTUATOR_ID);
            ACTUATOR_ID = NULL;
            return 0;
        }
        puts("Actuator not found!");
        ACTUATOR_ID = NULL;
        return 1;
    }
}

volatile bool is_engaged = false;
int engage_alarm(int argc, char **argv)
{
    if (argc != 1)
    {
        printf("usage: %s \n", argv[0]);
        return 0;
    }

    else
    {
        is_engaged = true;
        puts("Alarm Engaged!");
        send_log_msg("Alarm Engaged!",NULL);
        return 0;
    }
}
int disengage_alarm(int argc, char **argv)
{

    if (argc != 1)
    {
        printf("usage: %s \n", argv[0]);
        return 0;
    }

    else
    {
        deactivate_alarm();
        puts("Alarm Disengaged!");
        
        return 0;
    }
}

int show_actuators(int argc, char **argv)
{
    if (argc != 1)
    {
        printf("usage: %s \n", argv[0]);
        return 0;
    }

    else
    {
        display_actuators();
        return 0;
    }
}

int show_log(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    mutex_lock(&log_lock);
    puts(log);
    mutex_unlock(&log_lock);
    return 0;
}