/**
 * @file gateway_hashtable.c
 * @author adarsh.raghoothaman@st.ovgu.de
 * @author elvis.joseph@st.ovgu.de
 * @brief  file containing hash table related functions
 * @version 0.1
 * @date 2020-07-15
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "gateway_node.h"

uint32_t hash(const char *key)
{
    uint32_t value = 0;
    uint32_t i = 0;
    uint32_t key_len = strlen(key);

    for (; i < key_len; ++i)
    {
        value = value * 37 + key[i];
    }

    value = value % TABLE_SIZE;

    return value;
}

ht_t *ht_create(void)
{
    /*allocate table*/
    ht_t *hashtable = malloc(sizeof(ht_t) * 1);

    /*allocate table entries*/
    hashtable->entries = malloc(sizeof(struct sensor_entry *) * TABLE_SIZE);

    /*set each to null (needed for proper operation)*/
    uint32_t i = 0;
    for (; i < TABLE_SIZE; ++i)
    {
        hashtable->entries[i] = NULL;
    }

    return hashtable;
}

void ht_set(ht_t *hashtable, struct sensor_entry *node)
{
    mutex_lock(&ht_lock);
    uint32_t slot = hash(node->name);

    /*try to look up an entry set*/
    struct sensor_entry *entry = hashtable->entries[slot];

    /*no entry means slot empty, insert immediately*/
    if (entry == NULL)
    {
        hashtable->entries[slot] = node;
        mutex_unlock(&ht_lock);
        return;
    }

    struct sensor_entry *prev;

    /*walk through each entry until either the end is reached or a matching key is found */
    while (entry != NULL)
    {
        /*check key,what if 2 sensors with same name and different ip wants to connect*/
        if (strcmp(entry->name, node->name) == 0)
        {
            /*match found, key is already in the authorized list*/
            printf("%s is already in the authorized list\n", node->name);
            mutex_unlock(&ht_lock);
            return;
        }

        /*walk to next*/
        prev = entry;
        entry = prev->next;
    }

    /*end of chain reached without a match, add new*/
    prev->next = node;
    mutex_unlock(&ht_lock);
    return;
}

char *ht_get(ht_t *hashtable, char *key)
{
    uint32_t slot = hash(key);
    mutex_lock(&ht_lock);
    /*try to find a valid slot*/
    struct sensor_entry *entry = hashtable->entries[slot];

    /*no slot means no entry*/
    if (entry == NULL)
    {
        mutex_unlock(&ht_lock);
        return NULL;
    }

    /*walk through each entry in the slot, which could just be a single thing*/
    while (entry != NULL)
    {
        /*return value if found*/
        if (strcmp(entry->name, key) == 0)
        {
            mutex_unlock(&ht_lock);
            return entry->name;
        }

        /*proceed to next key if available*/
        entry = entry->next;
    }

    mutex_unlock(&ht_lock);
    /*reaching here means there were >= 1 entries but no key match*/
    return NULL;
}

struct sensor_entry * ht_get_node(ht_t *hashtable, char *key)
{
    uint32_t slot = hash(key);
    mutex_lock(&ht_lock);
    /*try to find a valid slot*/
    struct sensor_entry *entry = hashtable->entries[slot];

    /*no slot means no entry*/
    if (entry == NULL)
    {
        mutex_unlock(&ht_lock);
        return NULL;
    }

    /*walk through each entry in the slot, which could just be a single thing*/
    while (entry != NULL)
    {
        /*return value if found*/
        if (strcmp(entry->name, key) == 0)
        {
            mutex_unlock(&ht_lock);
            return entry;
        }

        /*proceed to next key if available*/
        entry = entry->next;
    }

    mutex_unlock(&ht_lock);
    /*reaching here means there were >= 1 entries but no key match*/
    return NULL;
}



int ht_del(ht_t *hashtable, char *key)
{
    uint32_t bucket = hash(key);
    mutex_lock(&ht_lock);
    /*try to find a valid bucket*/
    struct sensor_entry *entry = hashtable->entries[bucket];

    /*no bucket means no entry*/
    if (entry == NULL)
    {
        mutex_unlock(&ht_lock);
        return 1;
        
    }

    struct sensor_entry *prev;
    int idx = 0;

    /*walk through each entry until either the end is reached or a matching key is found*/
    while (entry != NULL)
    {
        /*check key*/
        if (strcmp(entry->name, key) == 0)
        {

            /*first item and no next entry*/
            if (entry->next == NULL && idx == 0)
            {

                hashtable->entries[bucket] = NULL;
            }

            /*first item with a next entry*/
            if (entry->next != NULL && idx == 0)
            {
                hashtable->entries[bucket] = entry->next;
            }

            /*last item*/
            if (entry->next == NULL && idx != 0)
            {
                prev->next = NULL;
            }

            /*middle item*/
            if (entry->next != NULL && idx != 0)
            {
                prev->next = entry->next;
            }

            /*Send deregister message to sensor node*/
            send_deregister_response(&entry->remote);
            /*free the deleted entry*/
            free(entry->name);
            free(entry);
            mutex_unlock(&ht_lock);
            return 0;
        }

        /*walk to next*/
        prev = entry;
        entry = prev->next;

        ++idx;
    }
    mutex_unlock(&ht_lock);
    return 1;
}

void ht_update_time(ht_t *hashtable, char *key)
{
    uint32_t slot = hash(key);
    mutex_lock(&ht_lock);
    /*try to find a valid slot*/
    struct sensor_entry *entry = hashtable->entries[slot];

    /*no slot means no entry*/
    if (entry == NULL)
    {
        mutex_unlock(&ht_lock);
        return;
    }

    /*walk through each entry in the slot, which could just be a single thing*/
    while (entry != NULL)
    {

        if (strcmp(entry->name, key) == 0)
        {
            entry->wdt = 0;
            DEBUG("status time updated\n");
            mutex_unlock(&ht_lock);
            /*free(entry);*/
            return;
            
        }

        /*proceed to next key if available*/
        entry = entry->next;
    }
    mutex_unlock(&ht_lock);
    /*free(entry);*/
    /*reaching here means there were >= 1 entries but no key match*/
    return;
}

volatile bool is_timeout = false;
void ht_wdt(ht_t *hashtable)
{
    mutex_lock(&ht_lock);
    for (int i = 0; i < TABLE_SIZE; ++i)
    {

        struct sensor_entry *entry = hashtable->entries[i];
        if (entry == NULL)
        {

            continue;
        }

        for (;;)
        {

            if ((entry->wdt) > 2)
            {
                if (is_engaged)
                {
                is_timeout = true;
                
                    DEBUG("Timeout, alarm goes off");
                }
            }
            else
            {
                entry->wdt = (entry->wdt + 1);
            }

            if (entry->next == NULL)
            {
                break;
            }

            entry = entry->next;
        }
        /*free(entry);*/
    }
    mutex_unlock(&ht_lock);
    return;
}