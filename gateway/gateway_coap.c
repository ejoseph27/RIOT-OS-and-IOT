/**
 * @file gateway_coap.c
 * @author adarsh.raghoothaman@st.ovgu.de
 * @author elvis.joseph@st.ovgu.de
 * @brief  file handling CoAP related activities 
 * @version 0.1
 * @date 2020-07-15
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "gateway_node.h"

/*first block for handling messages from sensors and actuators */

static ssize_t _encode_link(const coap_resource_t *resource, char *buf,
                            size_t maxlen, coap_link_encoder_ctx_t *context);

static ssize_t _sensor_tap_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, void *ctx);
static ssize_t _sensor_register_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, void *ctx);
static ssize_t _sensor_status_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, void *ctx);
static ssize_t _actuator_register_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, void *ctx);

/* CoAP resources. Must be sorted by path (ASCII order). */
static const coap_resource_t _resources[] = {

    {"/actuator/register", COAP_PUT, _actuator_register_handler, NULL},
    {"/sensor/register", COAP_PUT, _sensor_register_handler, NULL},
    {"/sensor/status", COAP_PUT, _sensor_status_handler, NULL},
    {"/sensor/tap", COAP_PUT, _sensor_tap_handler, NULL},

};

static const char *_link_params[] = {
    ";ct=0;rt=\"count\";obs",
    NULL};

static gcoap_listener_t _listener = {
    &_resources[0],
    ARRAY_SIZE(_resources),
    _encode_link,
    NULL};

/* Adds link format params to resource list */
static ssize_t _encode_link(const coap_resource_t *resource, char *buf,
                            size_t maxlen, coap_link_encoder_ctx_t *context)
{
    ssize_t res = gcoap_encode_link(resource, buf, maxlen, context);
    if (res > 0)
    {
        if (_link_params[context->link_pos] && (strlen(_link_params[context->link_pos]) < (maxlen - res)))
        {
            if (buf)
            {
                memcpy(buf + res, _link_params[context->link_pos],
                       strlen(_link_params[context->link_pos]));
            }
            return res + strlen(_link_params[context->link_pos]);
        }
    }

    return res;
}


/**
 * @brief handler for sensor register request
 * 
 * @param pdu 
 * @param buf 
 * @param len 
 * @param ctx 
 * @return ssize_t 
 */
static ssize_t _sensor_register_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, void *ctx)
{
    (void)ctx;

    if(SENSOR_ID==NULL)
    {
        return gcoap_response(pdu, buf, len, COAP_CODE_BAD_REQUEST);
    }

    if (pdu->payload_len)
    {
        struct node_identity *sensor_id = malloc(sizeof(struct node_identity));
        memcpy(sensor_id, pdu->payload, sizeof(struct node_identity));
        DEBUG("received node name :%s\n", sensor_id->name);

        char ipv6_addr[IPV6_ADDR_MAX_STR_LEN];
        ipv6_addr_to_str(ipv6_addr, &sensor_id->ip_addr, IPV6_ADDR_MAX_STR_LEN);
        DEBUG("received  address is %s\n", ipv6_addr);

        if (strcmp((ht_get(ht_sensor, sensor_id->name)), sensor_id->name) == 0)
        {
            puts("Node already authorized!");
            return gcoap_response(pdu, buf, len, COAP_CODE_BAD_REQUEST);
        }

        /*if name from shell and received sensor name are same register the sensor*/
        if (strcmp(SENSOR_ID, sensor_id->name) == 0)
        {
         
            /*SENSOR_ID = NULL;*/
            is_registered = true;
            add_sensor_entry(sensor_id,ipv6_addr);
            /*free(sensor_id);*/
            return gcoap_response(pdu, buf, len, COAP_CODE_CHANGED);
        }
        else
        {

            free(sensor_id);
            SENSOR_ID = NULL;
            return gcoap_response(pdu, buf, len, COAP_CODE_BAD_REQUEST);
        }
    }
    else
    {

        //SENSOR_ID = NULL;
        printf("Payload not found\n");
        return gcoap_response(pdu, buf, len, COAP_CODE_BAD_REQUEST);
    }
    return 0;
}

/**
 * @brief handler for sensor status messages
 * 
 * @param pdu 
 * @param buf 
 * @param len 
 * @param ctx 
 * @return ssize_t 
 */
static ssize_t _sensor_status_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, void *ctx)
{
    (void)ctx;
    //puts("status");
    if (pdu->payload_len)
    {

        struct node_identity *sensor_id = malloc(sizeof(struct node_identity));
        memcpy(sensor_id, pdu->payload, sizeof(struct node_identity));
        DEBUG("%s",sensor_id->name);
        DEBUG("Inside status handler");
        if (strcmp((ht_get(ht_sensor, sensor_id->name)), sensor_id->name) == 0)
        {
            
            //DEBUG("updating wdt in sensor structure");
            ht_update_time(ht_sensor, sensor_id->name);
            return gcoap_response(pdu, buf, len, COAP_CODE_CHANGED);
        }
        else
        {
            return gcoap_response(pdu, buf, len, COAP_CODE_BAD_REQUEST);
        }
    }
    else
    {
        puts("Payload not found");
        return gcoap_response(pdu, buf, len, COAP_CODE_BAD_REQUEST);
    }

    return 0;
}

/**
 * @brief handler fro messages from sensor when a event is detected
 * 
 * @param pdu 
 * @param buf 
 * @param len 
 * @param ctx 
 * @return ssize_t 
 */
static ssize_t _sensor_tap_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, void *ctx)
{
    (void)ctx;

    if (pdu->payload_len)
    {
        struct node_identity *sensor_id = malloc(sizeof(struct node_identity));
        memcpy(sensor_id, pdu->payload, sizeof(struct node_identity));

        if (strcmp((ht_get(ht_sensor, sensor_id->name)), sensor_id->name) == 0)
        {
            if (is_engaged)
            {
                puts("Sensor fired, activating alarm!");
                activate_alarm();
                send_log_msg("Sensor fired: ", sensor_id->name);
            }
            else
            {
                send_log_msg("Sensor fired: ", sensor_id->name);
                puts("Sensor fired, alarm not engaged!");
            }

            return gcoap_response(pdu, buf, len, COAP_CODE_CHANGED);
        }
        else
        {
            return gcoap_response(pdu, buf, len, COAP_CODE_BAD_REQUEST);
        }
    }
    else
    {
        printf("Payload not found\n");
        return gcoap_response(pdu, buf, len, COAP_CODE_BAD_REQUEST);
    }

    return 0;
}

/**
 * @brief handler for register request from actuator
 * 
 * @param pdu 
 * @param buf 
 * @param len 
 * @param ctx 
 * @return ssize_t 
 */
static ssize_t _actuator_register_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, void *ctx)
{
    (void)ctx;

    if(ACTUATOR_ID==NULL)
    {
        return gcoap_response(pdu, buf, len, COAP_CODE_BAD_REQUEST);
    }

    if (pdu->payload_len)
    {
        struct node_identity *actuator_id = malloc(sizeof(struct node_identity));
        memcpy(actuator_id, pdu->payload, sizeof(struct node_identity));
        DEBUG("received actuator node name :%s\n", actuator_id->name);

        char ipv6_addr[IPV6_ADDR_MAX_STR_LEN];
        ipv6_addr_to_str(ipv6_addr, &actuator_id->ip_addr, IPV6_ADDR_MAX_STR_LEN);
        DEBUG("received  address is %s\n", ipv6_addr);

        if (!node_exists(ACTUATOR_ID))
        {
            puts("Node already authorized!");
            return gcoap_response(pdu, buf, len, COAP_CODE_BAD_REQUEST);
        }

        /*if name from shell and received sensor name are same register the sensor*/
        if (strcmp(ACTUATOR_ID, actuator_id->name) == 0)
        {
            is_registered = true;
            add_actuator_entry(actuator_id, ipv6_addr);
            return gcoap_response(pdu, buf, len, COAP_CODE_CHANGED);
        }
        else
        {
            free(actuator_id);
            ACTUATOR_ID = NULL;
            return gcoap_response(pdu, buf, len, COAP_CODE_BAD_REQUEST);
        }
    }
    else
    {

        ACTUATOR_ID = NULL;
        printf("Payload not found\n");
        return gcoap_response(pdu, buf, len, COAP_CODE_BAD_REQUEST);
    }

    return 0;
}

void gcoap_server_init(void)
{
    gcoap_register_listener(&_listener);
}

/*second block for sending messages to actuators  */

static void _resp_handler(const gcoap_request_memo_t *memo, coap_pkt_t *pdu,
                          const sock_udp_ep_t *remote);

#define _LAST_REQ_PATH_MAX (64)
static char _last_req_path[_LAST_REQ_PATH_MAX];

bool _proxied = false;

/* Counts requests sent by CLI. */
static uint16_t req_count = 0;

/*
 * Response callback.
 */
static void _resp_handler(const gcoap_request_memo_t *memo, coap_pkt_t *pdu,
                          const sock_udp_ep_t *remote)
{
    (void)remote; /* not interested in the source currently */
    (void)req_count;

    if (memo->state == GCOAP_MEMO_TIMEOUT)
    {
        DEBUG("gcoap: timeout for msg ID %02u\n", coap_get_id(pdu));
        return;
    }
    else if (memo->state == GCOAP_MEMO_ERR)
    {
        DEBUG("gcoap: error in response\n");
        return;
    }

    coap_block1_t block;
    if (coap_get_block2(pdu, &block) && block.blknum == 0)
    {
        DEBUG("--- blockwise start ---");
    }

    char *class_str = (coap_get_code_class(pdu) == COAP_CLASS_SUCCESS)
                          ? "Success"
                          : "Error";
    DEBUG("gcoap: response %s, code %1u.%02u", class_str,
           coap_get_code_class(pdu),
           coap_get_code_detail(pdu));
    if (pdu->payload_len)
    {
        unsigned content_type = coap_get_content_type(pdu);
        if (content_type == COAP_FORMAT_TEXT || content_type == COAP_FORMAT_LINK || coap_get_code_class(pdu) == COAP_CLASS_CLIENT_FAILURE || coap_get_code_class(pdu) == COAP_CLASS_SERVER_FAILURE)
        {
            /* Expecting diagnostic payload in failure cases */
            DEBUG(", %u bytes\n%.*s\n", pdu->payload_len, pdu->payload_len,
                   (char *)pdu->payload);
        }
        else
        {
            DEBUG(", %u bytes\n", pdu->payload_len);
            od_hex_dump(pdu->payload, pdu->payload_len, OD_WIDTH_DEFAULT);
        }
    }
    else
    {
        DEBUG(", empty payload\n");
    }

    /* ask for next block if present */
    if (coap_get_block2(pdu, &block))
    {
        if (block.more)
        {
            unsigned msg_type = coap_get_type(pdu);
            if (block.blknum == 0 && !strlen(_last_req_path))
            {
                DEBUG("Path too long; can't complete blockwise");
                return;
            }

            if (_proxied)
            {
                gcoap_req_init(pdu, (uint8_t *)pdu->hdr, CONFIG_GCOAP_PDU_BUF_SIZE,
                               COAP_METHOD_GET, NULL);
            }
            else
            {
                gcoap_req_init(pdu, (uint8_t *)pdu->hdr, CONFIG_GCOAP_PDU_BUF_SIZE,
                               COAP_METHOD_GET, _last_req_path);
            }

            if (msg_type == COAP_TYPE_ACK)
            {
                coap_hdr_set_type(pdu->hdr, COAP_TYPE_CON);
            }
            block.blknum++;
            coap_opt_add_block2_control(pdu, &block);

            if (_proxied)
            {
                coap_opt_add_proxy_uri(pdu, _last_req_path);
            }

            int len = coap_opt_finish(pdu, COAP_OPT_FINISH_NONE);
            gcoap_req_send((uint8_t *)pdu->hdr, len, remote,
                           _resp_handler, memo->context);
        }
        else
        {
            DEBUG("--- blockwise complete ---");
        }
    }
}

/**
 * @brief send messages to the remote
 * 
 * @param buf 
 * @param len 
 * @param remote 
 * @return size_t 
 */
static size_t _send(uint8_t *buf, size_t len, sock_udp_ep_t *remote)
{
    size_t bytes_sent;

    bytes_sent = gcoap_req_send(buf, len, remote, _resp_handler, NULL);

    return bytes_sent;
}

int coap_put(char *message, char *uri, sock_udp_ep_t *remote)
{

    uint8_t buf[CONFIG_GCOAP_PDU_BUF_SIZE];
    coap_pkt_t pdu;
    size_t len;

    unsigned msg_type = COAP_TYPE_NON;

    int uri_len = strlen(uri);
    gcoap_req_init(&pdu, &buf[0], CONFIG_GCOAP_PDU_BUF_SIZE, COAP_METHOD_PUT, uri);
    coap_hdr_set_type(pdu.hdr, msg_type);
    memset(_last_req_path, 0, _LAST_REQ_PATH_MAX);
    if (uri_len < _LAST_REQ_PATH_MAX)
    {
        memcpy(_last_req_path, uri, uri_len);
    }
    coap_opt_add_format(&pdu, COAP_FORMAT_TEXT);
    len = coap_opt_finish(&pdu, COAP_OPT_FINISH_PAYLOAD);
    memcpy(pdu.payload, message, sizeof(message));
    len += sizeof(message);

    if (!_send(&buf[0], len, remote))
    {
        DEBUG("coap_client: msg send failed");
        return 1;
    }
    else
    {
        DEBUG("Message sent");
    }

    return 0;
}