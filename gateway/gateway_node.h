/**
 * @file gateway_node.h
 * @author adarsh.raghoothaman@st.ovgu.de
 * @author elvis.joseph@st.ovgu.de
 * @brief  file containing all headers and declarations 
 * @version 0.1
 * @date 2020-07-15
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include <stdio.h>
#include <stdbool.h>
#include "msg.h"
#include "net/ipv6/addr.h"
#include "net/gnrc.h"
#include "net/gnrc/netif.h"
#include "net/gcoap.h"
#include "string.h"
#include "periph/gpio.h"
#include <xtimer.h>
#include "shell.h"
#include "od.h"
#include "mutex.h"
#include "thread.h"
#include "fmt.h"
#define ENABLE_DEBUG  (0)
#include "debug.h"


#define COAP_PORT_S "5683"
#define TAP_URI "/sensor/tap"
#define STATUS_URI "/sensor/status"
#define REGISTER_URI "/sensor/register"
#define REGISTER_RESPONSE_URI "/register"
#define BEEPER_URI  "/actuator/beep"
#define TURN_OFF_ALARM "OFF"
#define TURN_ON_ALARM "ON"
#define LOG_QUEUE_SIZE (2)
#define REGISTER_RESPONSE "AUTH"
#define DEREGISTER_RESPONSE "DEAU"

/*char array to store log information*/ 
extern char log[];
/*mutex object for hashtable access*/
extern mutex_t ht_lock;
/*mutex object for log acces*/
extern mutex_t log_lock;


/*structure fro storing identity of the device*/
struct node_identity;
struct __attribute__((__packed__)) node_identity
{
    char name[10];
    ipv6_addr_t ip_addr;
};

/*structute for storing log mesasge*/
struct log_msg;
struct log_msg
{
    char message[30];
    char name[10];
};

extern gnrc_netif_t *netif ;


/*structure to store  sensor details*/
struct sensor_entry;
struct sensor_entry
{
    char name[10];
    sock_udp_ep_t remote;
    uint8_t wdt;
    struct sensor_entry *next;
};

/*structure to store  actuator details*/
struct actuator_entry;
struct actuator_entry
{
    char name[10];
    sock_udp_ep_t remote;
    struct actuator_entry *next;
};
/*pointers for the actuator list*/
extern struct actuator_entry *head ;
extern struct actuator_entry *current;


/*function to show what happended so far*/
extern int show_log (int argc, char **argv);
/*function to add data in the log*/
extern void add_log(char * message, char * arg);


/*initializes the listner*/
void gcoap_server_init(void);
/*initializes general params and functions*/
void init(void);
/*add sensor entry*/
void add_sensor_entry(struct node_identity *sensor_id, char *ipv6_addr);
/* remove sensor from the table*/
int remove_sensor(char * name);


/*time object for watch dog*/
extern xtimer_ticks32_t prev_time_wdt;
/*watch dog stack size*/
extern char wdt_thread_stack[THREAD_STACKSIZE_LARGE];
/*thread for updating counter values and checking it*/
void *wdt_thread(void *arg);


/*Flag to set when sensor is fired*/
extern volatile bool is_sensor_fired;
/*flag to set when there is no signal from a sensor*/
extern volatile bool is_timeout;
/* alarm thread stack size*/
extern char alarm_thread_stack[THREAD_STACKSIZE_LARGE];
/*Thread for sending signals to actuators*/
void *alarm_thread(void *arg);
/*function to deactivate alarm*/
void deactivate_alarm(void);


/*pid for the log thread*/
extern kernel_pid_t log_rcv_pid;
/*log receive queue size*/
extern msg_t log_rcv_queue[LOG_QUEUE_SIZE];
/*stack size for log thread*/
extern char log_thread_stack[THREAD_STACKSIZE_LARGE + THREAD_EXTRA_STACKSIZE_PRINTF];
/*log thread for updating the log*/
extern void *log_thread(void *arg);


/*flasg is set when received sensor name and the name from user are same*/
extern volatile bool is_registered;
/*register node to the server*/
int register_node(char * name);


/*flag to check if the alarm is engaged */
extern volatile bool is_engaged;
/*params for shell*/
#define MAIN_QUEUE_SIZE (8)
extern msg_t _main_msg_queue[MAIN_QUEUE_SIZE];
/*authorize sensor to the server*/
int auth_sensor_node (int argc, char **argv);
/*deauthorize sensor from the server*/
int de_auth_sensor_node (int argc, char **argv);
/*authorize actuator to the server*/
int auth_actuator_node (int argc, char **argv);
/*deauthorize actuator from the server*/
int de_auth_actuator_node (int argc, char **argv);
/*Engage alarm*/
int engage_alarm(int argc, char **argv);
/*Dis engage alarm*/
int disengage_alarm(int argc, char **argv);
/*show connected actuators*/
int show_actuators(int argc, char **argv);


extern const shell_command_t shell_commands[];
/* string to store user input for actuator name*/
extern char *ACTUATOR_ID;
/* string to store user input for sensor name*/
extern char *SENSOR_ID;
char *strdup(const char *);


/*params for hash table*/
struct ht_t;
typedef  struct
{
    struct sensor_entry **entries;
} ht_t;

/*pointer to hash table*/
extern ht_t *ht_sensor;

/*function to create hash code*/
uint32_t hash(const char *key);
/**
 * @brief function to create hash table
 * 
 * @return ht_t* 
 */
ht_t *ht_create(void);
/**
 * @brief function add sensors to the table
 * 
 * @param hashtable 
 * @param node 
 */
void ht_set(ht_t *hashtable, struct sensor_entry *node);
/**
 * @brief function to get sensor name from the table
 * 
 * @param hashtable 
 * @param name 
 * @return char* 
 */
char *ht_get(ht_t *hashtable, char *name);
/**
 * @brief function to get sensor node with a given key
 * 
 * @param hashtable 
 * @param key 
 * @return struct sensor_entry* 
 */
struct sensor_entry * ht_get_node(ht_t *hashtable, char *key);
/**
 * @brief delete node with  a given name
 * 
 * @param hashtable 
 * @param name 
 * @return int 
 */
int ht_del(ht_t *hashtable, char *name);
/**
 * @brief update time in the table with given name
 * 
 * @param hashtable 
 * @param name 
 */
void ht_update_time(ht_t *hashtable,char *name);
/**
 * @brief update the counter and check its value in table to detect time out
 * 
 * @param hashtable 
 */
void ht_wdt(ht_t *hashtable);
/**
 * @brief table size
 * 
 */
#define TABLE_SIZE 10



/**
 * @brief add actuator entry in server
 * 
 * @param actuator_id 
 * @param ipv6_addr 
 */
void add_actuator_entry(struct node_identity *actuator_id, char *ipv6_addr);
/**
 * @brief insert node to the list
 * 
 * @param node 
 */
void insert_node(struct actuator_entry *node);
/**
 * @brief Get the node object with given name
 * 
 * @param name 
 * @return struct actuator_entry* 
 */
struct actuator_entry * get_node(char *name);
/**
 * @brief check if node exists with given name
 * 
 * @param name 
 * @return int 
 */
int node_exists(char *name);
/**
 * @brief delete ndoe with given name
 * 
 * @param name 
 * @return int 
 */
int delete_node(char *name);
/**
 * @brief remove actuator from the list
 * 
 * @param name 
 * @return int 
 */
int remove_actuator(char *name);


/**
 * @brief sennd message to the remote
 * 
 * @param message 
 * @param uri 
 * @param remote 
 * @return int 
 */
int coap_put(char *message, char *uri, sock_udp_ep_t *remote);
/**
 * @brief activate alarm 
 * 
 */
void activate_alarm(void);
/**
 * @brief traverse through the list and send ON message to actuators
 * 
 */
void traverse_and_fire(void);
/**
 * @brief traverse through the list and send OFF message to actuators
 * 
 */
void traverse_and_off(void);
/**
 * @brief display actuators names
 * 
 */
void display_actuators(void);




/*params for button*/
#define btn_register GPIO_PIN(PORT_B, 13)
#define btn_test GPIO_PIN(PORT_A, 0)
/**
 * @brief interrup to handler to disengage alarm when button is pressed
 * 
 * @param arg 
 */
void dis_engage_intrp(void *arg);  
/**
 * @brief test interrup to activate timeout
 * 
 * @param arg 
 */
void test_intrp(void *arg);

/**
 * @brief send log message to log thread
 * 
 * @param msg 
 * @param name 
 */
void send_log_msg (char * msg, char * name );
/**
 * @brief send response to the node once it is authorized
 * 
 * @param remote 
 */
void send_register_response(sock_udp_ep_t *remote);
/**
 * @brief send response to the node once it is deauthorized
 * 
 * @param remote 
 */
void send_deregister_response(sock_udp_ep_t *remote);