/**
 * @file actuator_node.h
 * @author adarsh.raghoothaman@st.ovgu.de
 * @author elvis.joseph@st.ovgu.de
 * @brief  header containing all headers and declarations
 * @version 0.1
 * @date 2020-07-15
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include <stdio.h>
#include <stdbool.h>
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
#include "periph/pwm.h"
#define ENABLE_DEBUG (1)
#include "debug.h"

#ifndef COAP_PORT_S
#define COAP_PORT_S "5683"
#endif
#ifndef REGISTER_URI
#define REGISTER_URI "/actuator/register"
#endif

/*structure to store node details*/
struct node_identity;
struct __attribute__((__packed__)) node_identity
{
    char name[10];
    ipv6_addr_t ip_addr;
};

/*UDP sock object to store server ip*/
extern sock_udp_ep_t *server;
/* object for identity*/
extern struct node_identity *actuator_id;

#define btn_register GPIO_PIN(PORT_B, 13)
#define btn_test GPIO_PIN(PORT_A, 0)
/**
 * @brief interrupt handler for register
 * 
 * @param arg 
 */
void register_intrp(void *arg);
/**
 * @brief test interrupt
 * 
 * @param arg 
 */
void test_intrp(void *arg);

/*flag that triggers sending sending node identity to server*/
extern volatile bool send_id;
/*register thread stack size*/
extern char register_thread_stack[THREAD_STACKSIZE_MAIN];
/**
 * @brief threding for registering the node to server
 * 
 * @param arg 
 * @return void* 
 */
void *register_thread(void *arg);

/*params for shell*/
#define MAIN_QUEUE_SIZE (8)
extern msg_t _main_msg_queue[MAIN_QUEUE_SIZE];
/*set server  ip*/
int set_server_ip(int argc, char **argv);
/*set name of the node*/
int set_name(int argc, char **argv);
/*display actuator status*/
int actuator_status(int argc, char **argv);
extern const shell_command_t shell_commands[];

/**
 * @brief Set the remote object
 * 
 * @param remote 
 * @param addr_str 
 * @return true 
 * @return false 
 */
bool set_remote(sock_udp_ep_t *remote, char *addr_str);
/**
 * @brief Get the status object
 * 
 */
void get_status(void);
char *strdup(const char *);
/**
 * @brief Set the actuator name object
 * 
 * @return int 
 */
int set_actuator_name(char *);
/**
 * @brief initilize general params and functions
 * 
 */
void init(void);

/**
 * @brief initialize the actuators listener
 * 
 */
void gcoap_actuator_init(void);

/*flasg to indicate if node is connected to server*/
extern volatile bool connection_status;
#define REGISTER_RESPONSE "AUTH"
#define DEREGISTER_RESPONSE "DEAU"
/*string fro storing server ip */
extern char *SERVER_IP;

#define TURN_OFF_ALARM "OFF"
#define TURN_ON_ALARM "ON"

/*PWM pin*/
#define BEEP PWM_DEV(0)
/*PWM mode*/
#define OSC_MODE PWM_LEFT
/*PWM frequency*/
#define OSC_FREQ (1000U)
/*PWM steps*/
#define OSC_STEPS (1000U)
void _beep_long(void);
/**
 * @brief Initialize the actuator
 * 
 */
void actuator_init(void);
/**
 * @brief do athe actuator function
 * 
 */
void actuator_fire(void);
