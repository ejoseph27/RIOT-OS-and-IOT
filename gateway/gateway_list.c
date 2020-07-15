/**
 * @file gateway_list.c
 * @author adarsh.raghoothaman@st.ovgu.de
 * @author elvis.joseph@st.ovgu.de
 * @brief  file containing list related functions
 * @version 0.1
 * @date 2020-07-15
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "gateway_node.h"

struct actuator_entry *head = NULL;
struct actuator_entry *current = NULL;

void insert_node(struct actuator_entry *node)
{
    /*create a link*/
    struct actuator_entry *link = (struct actuator_entry *)malloc(sizeof(struct actuator_entry));
    link = node;
    /*point it to old first node*/
    link->next = head;

    /*point first to new first node*/
    head = link;
}


int delete_node(char *name)
{

    /*start from the first link*/
    struct actuator_entry *current = head;
    struct actuator_entry *previous = NULL;

    /*if list is empty*/
    if (head == NULL)
    {
        return 1;
    }

    /*navigate through list*/
    while (strcmp(current->name ,name)!=0)
    {

        /*if it is last node*/
        if (current->next == NULL)
        {
            return 1;
        }
        else
        {
            /*store reference to current link*/
            previous = current;
            /*move to next link*/
            current = current->next;
        }
    }

    /*found a match, update the link*/
    if (current == head)
    {
        send_deregister_response(&current->remote);
        /*change first to point to next link*/
        head = head->next;
        current = NULL;
        return 0;
    }
    else
    {
        send_deregister_response(&current->remote);
        /*bypass the current link*/
        previous->next = current->next;
        current = NULL;
        return 0;
    }

    /*free(current);*/
    return 1;
}

void traverse_and_fire(void)
{
    struct actuator_entry *ptr = head;
    DEBUG("\ninside traverse and fire\n");

    /*start from the beginning*/
    while (ptr != NULL)
    {

        DEBUG("sending message to actuator\n");
        DEBUG("%s\n", ptr->name);
        coap_put(TURN_ON_ALARM, BEEPER_URI, &ptr->remote);
        ptr = ptr->next;
    }

    return;
}
void traverse_and_off(void)
{
    struct actuator_entry *ptr = head;
    DEBUG("\ninside traverse and fire\n");

    /*start from the beginning*/
    while (ptr != NULL)
    {

        DEBUG("sending message to actuator\n");
        DEBUG("%s\n", ptr->name);
        coap_put(TURN_OFF_ALARM, BEEPER_URI, &ptr->remote);
        //xtimer_usleep(1000);
        ptr = ptr->next;
    }

    return;
}

/*display the list*/
void display_actuators(void)
{
    struct actuator_entry *ptr = head;

    if (head == NULL)
    {
        printf("No actuators authorized \n");
    }

    /*start from the beginning*/
    while (ptr != NULL)
    {
        printf("%s \n ", ptr->name);
        ptr = ptr->next;
    }

    return;
}

struct actuator_entry * get_node(char *name)
{
    struct actuator_entry *ptr = head;
    /*start from the beginning*/
    while (ptr != NULL)
    {
        if (strcmp(ptr->name, name) == 0)
        {
            return ptr;
        }
        ptr = ptr->next;
    }

    return NULL;
}

int node_exists(char *name)
{
    struct actuator_entry *ptr = head;
    /*start from the beginning*/
    while (ptr != NULL)
    {
        if (strcmp(ptr->name, name) == 0)
        {
            return 0;
        }
        ptr = ptr->next;
    }

    return 1;
}