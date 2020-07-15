/**
 * @file sensor_node.c
 * @author adarsh.raghoothaman@st.ovgu.de
 * @author elvis.joseph@st.ovgu.de
 * @brief  file containing helper functions
 * @version 0.1
 * @date 2020-07-15
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "sensor_node.h"


char * server_ip = NULL;
sock_udp_ep_t *server;
struct node_identity *sensor_id;

/**
 * @brief Funnction to get the ip address of the device 
 * 
 * @param ip_addr 
 */
void _get_ip(ipv6_addr_t *ip_addr)
{
    gnrc_netif_t *netif = NULL;
    while ((netif = gnrc_netif_iter(netif)))
    {
        ipv6_addr_t ipv6_addrs[CONFIG_GNRC_NETIF_IPV6_ADDRS_NUMOF];
        int res = gnrc_netapi_get(netif->pid, NETOPT_IPV6_ADDR, 0, ipv6_addrs,
                                  sizeof(ipv6_addrs));
        if (res < 0)
        {
            continue;
        }
        for (unsigned i = 0; i < (unsigned)(res / sizeof(ipv6_addr_t)); i++)
        {
            (void)ip_addr;
            memcpy(ip_addr, &ipv6_addrs[i], sizeof(ipv6_addr_t));
            return;
        }
    }
}


/**
 * @brief Set the ip of the remote object
 * 
 * @param remote 
 * @param addr_str 
 * @return true 
 * @return false 
 */
bool set_remote(sock_udp_ep_t *remote, char *addr_str)
{
    ipv6_addr_t addr;
    remote->family = AF_INET6;

    /* parse for interface */
    char *iface = ipv6_addr_split_iface(addr_str);
    if (!iface)
    {
        if (gnrc_netif_numof() == 1)
        {
            /* assign the single interface found in gnrc_netif_numof() */
            remote->netif = (uint16_t)gnrc_netif_iter(NULL)->pid;
        }
        else
        {
            remote->netif = SOCK_ADDR_ANY_NETIF;
        }
    }
    else
    {
        int pid = atoi(iface);
        if (gnrc_netif_get_by_pid(pid) == NULL)
        {
            puts("coap_client: interface not valid");
            return false;
        }
        remote->netif = pid;
    }
    /* parse destination address */
    if (ipv6_addr_from_str(&addr, addr_str) == NULL)
    {
        puts("coap_client: unable to parse destination address");
        return false;
    }
    if ((remote->netif == SOCK_ADDR_ANY_NETIF) && ipv6_addr_is_link_local(&addr))
    {
        puts("coap_client: must specify interface for link local target");
        return false;
    }
    memcpy(&remote->addr.ipv6[0], &addr.u8[0], sizeof(addr.u8));

    /* parse port */
    remote->port = atoi(COAP_PORT_S);
    if (remote->port == 0)
    {
        puts("coap_client: unable to parse destination port");
        return false;
    }

    return true;
}


void init(void)
{
    sensor_id = malloc(sizeof(struct node_identity));
    server = malloc(sizeof(sock_udp_ep_t));
    _get_ip(&sensor_id->ip_addr);
    strcpy(sensor_id->name, "tapsense");
    server_ip = malloc(IPV6_ADDR_MAX_STR_LEN);
    strcpy(server_ip,"fe80::ff:fe00:e");
    set_remote(server, server_ip);

    gpio_init_int(btn_register, GPIO_IN, GPIO_FALLING, register_intrp, NULL);
    gpio_init_int(btn_test, GPIO_IN, GPIO_FALLING, test_intrp, NULL);
    gpio_irq_enable(btn_register);
    gpio_irq_enable(btn_test);
    gcoap_sensor_init();
    sensor_init();
}


int set_sensor_name(char *name)
{
    if (strlen(name) > 10)
    {
        puts("Name larger than 10 characters");
        return 1;
    }
    else
    {
        printf("sensor name set to %s\n", name);
        strcpy(sensor_id->name, name);
    }

    return 0;
}



volatile bool send_id = false;
void register_intrp(void *arg)
{
    (void)arg;
    /* manage interrupt */
    puts("Register pressed");
    send_id = true;
}


volatile bool sensor_fired = false;
void test_intrp(void *arg)
{
    (void)arg;
    /* manage interrupt */
    puts("test pressed");
    sensor_fired = true;
}


char register_thread_stack[THREAD_STACKSIZE_MAIN];
extern int coap_register_put(struct node_identity *sensor_id, char *uri);
void *register_thread(void *arg)
{
    (void)arg;
    while (1)
    {

        if (send_id) 
        {
            DEBUG("sending id to server");
            coap_register_put(sensor_id, REGISTER_URI);
            send_id = false;
        }
        else
        {
            thread_yield();
        }
        
    }
    return NULL;
}


volatile bool send_status = true;
xtimer_ticks32_t prev_time;
char status_thread_stack[THREAD_STACKSIZE_MAIN];
extern int coap_status_put(struct node_identity *sensor_id, char *uri);
void *status_thread(void *arg)
{
    (void)arg;
    prev_time = xtimer_now();
    while (1)
    {
        if (send_status && connection_status)
        {
            DEBUG("sending status id to server\n");
            coap_status_put(sensor_id, STATUS_URI);
        }
        xtimer_periodic_wakeup(&prev_time, 1000000);
        prev_time = xtimer_now(); 
    }
    return NULL;
}



volatile bool fire_status = false;
uint32_t last_fire; 
xtimer_ticks32_t prev_time_detection;
char detection_thread_stack[THREAD_STACKSIZE_MAIN];
void *detection_thread(void *arg)
{
    (void)arg;
    prev_time_detection = xtimer_now();
    while (1)
    {
        if (sensor_fired && connection_status)
        {
            last_fire = xtimer_now_usec();
            fire_status = true;
            coap_status_put(sensor_id, FIRE_URI);
            sensor_fired = false;
        }
        xtimer_periodic_wakeup(&prev_time, 100000);
        prev_time_detection = xtimer_now();
    }
    return NULL;
}


void get_status(void)
{
    printf("Name: %s\n", sensor_id->name);
    if (connection_status)
    {
        printf("Connected to gateway: %s\n",server_ip);
    }
    else if (!connection_status)
    {
        puts("Not connected to gateway");
    }
    if (fire_status)
    {
        printf("Last fired at :%ld sec\n", (last_fire / 1000000));
    }
    else if (!fire_status)
    {
        puts("Not fired yet");
    }
    
}

kernel_pid_t sensor_run_pid;
char sensor_run_stack[THREAD_STACKSIZE_MAIN];
void *sensor_run_thread(void *arg)
{
    (void)arg;
    sensor_detection_loop();
    return NULL;
}
