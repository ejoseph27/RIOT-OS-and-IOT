/**
 * @file gateway_node.c
 * @author adarsh.raghoothaman@st.ovgu.de
 * @author elvis.joseph@st.ovgu.de
 * @brief  file containing helper functions
 * @version 0.1
 * @date 2020-07-15
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "gateway_node.h"

char log[4096];

mutex_t ht_lock = MUTEX_INIT;
mutex_t log_lock = MUTEX_INIT;
gnrc_netif_t *netif = NULL;

ht_t *ht_sensor;
void init(void)
{

    gcoap_server_init();
    ht_sensor = ht_create();

    gpio_init_int(btn_register, GPIO_IN, GPIO_FALLING, dis_engage_intrp, NULL);
    gpio_init_int(btn_test, GPIO_IN, GPIO_FALLING, test_intrp, NULL);
    gpio_irq_enable(btn_register);
    gpio_irq_enable(btn_test);
    log[0] = '\0';
    send_log_msg("Server Started", NULL);
}

void dis_engage_intrp(void *arg)
{

    (void)arg;
    /* manage interrupt */
    puts("Disengaging alarm!");
    deactivate_alarm();
    return;
}

void test_intrp(void *arg)
{
    (void)arg;
    /* manage interrupt */
    if (is_engaged)
    {
        puts("Test Alarm!");
        activate_alarm();
        send_log_msg("Test Alarm", NULL);
    }
    else
    {
        puts("Alarm disengaged, can't test!");
    }

    return;
}

void add_sensor_entry(struct node_identity *sensor_id, char *ipv6_addr)
{
    struct sensor_entry *sensor = malloc(sizeof(struct sensor_entry));
    strcpy(sensor->name, sensor_id->name);

    sensor->remote.family = AF_INET6;
    /* parse for interface */
    char *iface = ipv6_addr_split_iface(ipv6_addr);
    if (!iface)
    {
        if (gnrc_netif_numof() == 1)
        {
            /* assign the single interface found in gnrc_netif_numof() */
            sensor->remote.netif = (uint16_t)gnrc_netif_iter(NULL)->pid;
        }
        else
        {
            sensor->remote.netif = SOCK_ADDR_ANY_NETIF;
        }
    }
    else
    {
        int pid = atoi(iface);
        if (gnrc_netif_get_by_pid(pid) == NULL)
        {
            DEBUG("coap_client: interface not valid");
            //return false;
        }
        sensor->remote.netif = pid;
    }

    memcpy(&sensor->remote.addr.ipv6[0], &sensor_id->ip_addr.u8[0], sizeof(sensor_id->ip_addr.u8));

    /* parse port */
    sensor->remote.port = atoi(COAP_PORT_S);
    if (sensor->remote.port == 0)
    {
        DEBUG("coap_client: unable to parse destination port");
        return;
    }
    sensor->wdt = 0;
    sensor->next = NULL;

    free(sensor_id);
    /* call hashtable add with sensor */
    ht_set(ht_sensor, sensor);
    return;
}

int remove_sensor(char *name)
{
    return ht_del(ht_sensor, name);
}

xtimer_ticks32_t prev_time_wdt;
char wdt_thread_stack[THREAD_STACKSIZE_LARGE];
void *wdt_thread(void *arg)
{

    (void)arg;
    prev_time_wdt = xtimer_now();
    while (1)
    {
        //DEBUG("10");
        ht_wdt(ht_sensor);
        xtimer_periodic_wakeup(&prev_time_wdt, 1000000);
        prev_time_wdt = xtimer_now();
    }

    return NULL;
}

volatile bool is_sensor_fired = false;
char alarm_thread_stack[THREAD_STACKSIZE_LARGE];
void *alarm_thread(void *arg)
{

    (void)arg;
    prev_time_wdt = xtimer_now();
    while (1)
    {
        if ((is_timeout || is_sensor_fired) && is_engaged)
        {
            if (is_timeout)
                DEBUG("No signal from sensor,turning on alarms");
            traverse_and_fire();
            is_timeout = false;
        }
        xtimer_periodic_wakeup(&prev_time_wdt, 1000000);
        prev_time_wdt = xtimer_now();
    }

    return NULL;
}

volatile bool is_registered = false;
int register_node(char *name)
{

    xtimer_ticks32_t prev_time_wdt = xtimer_now();
    uint8_t count = 0;
    puts("Press the register button on the node within 5 Seconds");
    while (1)
    {
        if (count > 50)
        {
            printf("No message recieved from node %s\n", name);
            return 1;
        }
        if (is_registered)
        {
            printf("Node %s authorized\n", name);

            is_registered = false;
            //name = NULL;
            return 0;
        }

        xtimer_periodic_wakeup(&prev_time_wdt, 100000);
        prev_time_wdt = xtimer_now();
        count++;
    }
    return 1;
}

void add_actuator_entry(struct node_identity *actuator_id, char *ipv6_addr)
{
    struct actuator_entry *actuator = malloc(sizeof(struct actuator_entry));
    strcpy(actuator->name, actuator_id->name);

    actuator->remote.family = AF_INET6;
    /* parse for interface */
    char *iface = ipv6_addr_split_iface(ipv6_addr);
    if (!iface)
    {
        if (gnrc_netif_numof() == 1)
        {
            /* assign the single interface found in gnrc_netif_numof() */
            actuator->remote.netif = (uint16_t)gnrc_netif_iter(NULL)->pid;
        }
        else
        {
            actuator->remote.netif = SOCK_ADDR_ANY_NETIF;
        }
    }
    else
    {
        int pid = atoi(iface);
        if (gnrc_netif_get_by_pid(pid) == NULL)
        {
            DEBUG("coap_client: interface not valid");
            //return false;
        }
        actuator->remote.netif = pid;
    }

    memcpy(&actuator->remote.addr.ipv6[0], &actuator_id->ip_addr.u8[0], sizeof(actuator_id->ip_addr.u8));

    /* parse port */
    actuator->remote.port = atoi(COAP_PORT_S);
    if (actuator->remote.port == 0)
    {
        DEBUG("coap_client: unable to parse destination port");
        return;
    }
    actuator->next = NULL;

    free(actuator_id);

    insert_node(actuator);
    return;
}

int remove_actuator(char *name)
{
    return delete_node(name);
}

inline void activate_alarm(void)
{
    is_sensor_fired = true;
    return;
}

//call inside interrupt
inline void deactivate_alarm(void)
{
    is_engaged = false;
    is_sensor_fired = false;
    is_timeout = false;
    send_log_msg("Alarm Disengaged!", NULL);
    return;
}

kernel_pid_t log_rcv_pid;
msg_t log_rcv_queue[LOG_QUEUE_SIZE];
char log_thread_stack[THREAD_STACKSIZE_LARGE + THREAD_EXTRA_STACKSIZE_PRINTF];
void *log_thread(void *arg)
{
    msg_t msg;
    (void)arg;
    msg_init_queue(log_rcv_queue, LOG_QUEUE_SIZE);
    while (1)
    {
        msg_receive(&msg);
        uint32_t time = xtimer_now_usec();
        time /= 1000000;
        char entry[30];
        struct log_msg *logmsg = (struct log_msg *)msg.content.ptr;
        if (logmsg->name != NULL)
        {
            sprintf(entry, "%ldS: %s %s\n", time, (char *)logmsg->message, (char *)logmsg->name);
        }
        else
            sprintf(entry, "%ld: %s\n", time, logmsg->message);
        mutex_lock(&log_lock);
        strcat(log, entry);
        mutex_unlock(&log_lock);
        free(msg.content.ptr);
    }

    return NULL;
}

void send_log_msg(char *message, char *name)
{
    msg_t msg;
    struct log_msg *logmsg = malloc(sizeof(struct log_msg));
    strncpy(logmsg->message, message, sizeof(logmsg->message));
    if (name != NULL)
        strncpy(logmsg->name, name, sizeof(logmsg->name));
    else
        logmsg->name[0] = '\0';
    msg.content.ptr = logmsg;
    msg_send(&msg, log_rcv_pid);
    name = NULL;
    return;
}

void send_register_response(sock_udp_ep_t *remote)
{
    coap_put(REGISTER_RESPONSE, REGISTER_RESPONSE_URI, remote);
}

void send_deregister_response(sock_udp_ep_t *remote)
{
    coap_put(DEREGISTER_RESPONSE, REGISTER_RESPONSE_URI, remote);
}