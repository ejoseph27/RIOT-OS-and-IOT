/**
 * @file sensor_node.h
* @author adarsh.raghoothaman@st.ovgu.de
 * @author elvis.joseph@st.ovgu.de
 * @brief header file containing all headers and declarations
 * @version 0.1
 * @date 2020-07-15
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include "msg.h"
#include "shell.h"
#include "net/ipv6/addr.h"
#include "net/gnrc.h"
#include "net/gnrc/netif.h"
#include "net/gcoap.h"
#include "string.h"
#include "periph/gpio.h"
#include <xtimer.h>
#include "od.h"
#include "fmt.h"
#include "thread.h"
#define ENABLE_DEBUG  (0)
#include "debug.h"

#ifndef COAP_PORT_S
#define COAP_PORT_S "5683"
#endif
#ifndef FIRE_URI
#define FIRE_URI "/sensor/tap"
#endif
#ifndef STATUS_URI
#define STATUS_URI "/sensor/status"
#endif
#ifndef REGISTER_URI
#define REGISTER_URI "/sensor/register"
#endif
#define REGISTER_RESPONSE "AUTH"
#define DEREGISTER_RESPONSE "DEAU"


/*structure fro storing identity of the device*/
struct node_identity;
struct __attribute__((__packed__)) node_identity
{
    char name[10];
    ipv6_addr_t ip_addr;
};


/*UDP sock object for storing server ip*/
extern sock_udp_ep_t  * server;
/*string for storing server ip from user input*/
extern char * server_ip;
/*object to store sensor details*/
extern struct node_identity * sensor_id;


/* msbiot left button for register interrupt*/
#define btn_register GPIO_PIN(PORT_B, 13)
/* msbiot right button for test interrupt*/
#define btn_test GPIO_PIN(PORT_A, 0)
/**
 * @brief register interrupt handler
 * 
 * @param arg 
 */
void register_intrp(void *arg); 
/**
 * @brief test interrupt handler
 * 
 * @param arg 
 */
void test_intrp(void *arg);


/* flag that  triggers sending  node identity to gateway/server*/
extern volatile bool send_id;
/*register thread stack size*/
extern char register_thread_stack[THREAD_STACKSIZE_MAIN];
/**
 * @brief thread for registering the device to server
 * 
 * @param arg 
 * @return void* 
 */
void *register_thread(void *arg);


/* flag that  triggers sending  status to gateway/server*/
extern volatile bool send_status;
/*time object */
extern xtimer_ticks32_t prev_time;
/*status thread stack size*/
extern char status_thread_stack[THREAD_STACKSIZE_MAIN];
/**
 * @brief thread for sending status messages to gateway
 * 
 * @param arg 
 * @return void* 
 */
void *status_thread(void *arg);


/*flag for checking if sensor has detected movement sofar*/
extern volatile bool fire_status;
/*flag for checking if sensor has detected movement*/
extern volatile bool sensor_fired;
/* variable  to store lat time of detection*/
extern uint32_t last_fire ;
/*time object*/
extern xtimer_ticks32_t prev_time_detection;
/*detection thread stack size*/
extern char detection_thread_stack[THREAD_STACKSIZE_MAIN];
/**
 * @brief thread for sending messages to gateway once detected movement 
 * 
 * @param arg 
 * @return void* 
 */
void *detection_thread(void *arg);
//extern char *tap_time;



/*params for shell*/
#define MAIN_QUEUE_SIZE (8)
extern msg_t _main_msg_queue[MAIN_QUEUE_SIZE];
/**
 * @brief Set the server ip object
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int set_server_ip(int argc, char **argv);
/**
 * @brief Set the name object
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int set_name(int argc, char **argv);
/**
 * @brief get status of the device
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int sensor_status(int argc, char **argv);
extern const shell_command_t shell_commands[];


/**
 * @brief Set the remote object
 * 
 * @param remote 
 * @param addr_str 
 * @return true 
 * @return false 
 */
bool set_remote(sock_udp_ep_t *remote, char * addr_str);
/**
 * @brief Get the status of he device
 * 
 */
void get_status(void);
char *strdup(const char *);
/**
 * @brief set the shell input name to sensor id
 * 
 * @return int 
 */
int set_sensor_name(char *);
/**
 * @brief Initializes whole functions

 * 
 */
void init(void);


/*flag which indicates if the node is connected to gateway*/
extern volatile bool connection_status;
/**
 * @brief initialize the listener
 * 
 */
void gcoap_sensor_init(void);


/*pid value of the sensor run thread*/
extern kernel_pid_t sensor_run_pid;
/*stack size for the sensor run thread*/
extern char sensor_run_stack[THREAD_STACKSIZE_MAIN];
/**
 * @brief sensor run thread where the events are detected
 * 
 * @param arg 
 * @return void* 
 */
void *sensor_run_thread(void *arg);


/**
 * @brief function to detect events in sensor node,
 *  do detection and make  sensor_fired flag true
 * 
 */
void sensor_detection_loop(void);
/**
 * @brief intialize required sensor specific configurations in this function
 * 
 */
void sensor_init(void);






