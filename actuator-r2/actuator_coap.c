/**
 * @file actuator_coap.c
 * @author adarsh.raghoothaman@st.ovgu.de
 * @author elvis.joseph@st.ovgu.de
 * @brief  file handling CoAP related activities
 * @version 0.1
 * @date 2020-07-15
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "actuator_node.h"

/* fist block for sending message to gateway */
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
        printf("gcoap: timeout for msg ID %02u\n", coap_get_id(pdu));
        return;
    }
    else if (memo->state == GCOAP_MEMO_ERR)
    {
        printf("gcoap: error in response\n");
        return;
    }

    coap_block1_t block;
    if (coap_get_block2(pdu, &block) && block.blknum == 0)
    {
        puts("--- blockwise start ---");
    }

    char *class_str = (coap_get_code_class(pdu) == COAP_CLASS_SUCCESS)
                          ? "Success"
                          : "Error";
    printf("gcoap: response %s, code %1u.%02u", class_str,
           coap_get_code_class(pdu),
           coap_get_code_detail(pdu));
    if (pdu->payload_len)
    {
        unsigned content_type = coap_get_content_type(pdu);
        if (content_type == COAP_FORMAT_TEXT || content_type == COAP_FORMAT_LINK || coap_get_code_class(pdu) == COAP_CLASS_CLIENT_FAILURE || coap_get_code_class(pdu) == COAP_CLASS_SERVER_FAILURE)
        {
            /* Expecting diagnostic payload in failure cases */
            printf(", %u bytes\n%.*s\n", pdu->payload_len, pdu->payload_len,
                   (char *)pdu->payload);
        }
        else
        {
            printf(", %u bytes\n", pdu->payload_len);
            od_hex_dump(pdu->payload, pdu->payload_len, OD_WIDTH_DEFAULT);
        }
    }
    else
    {
        printf(", empty payload\n");
    }

    /* ask for next block if present */
    if (coap_get_block2(pdu, &block))
    {
        if (block.more)
        {
            unsigned msg_type = coap_get_type(pdu);
            if (block.blknum == 0 && !strlen(_last_req_path))
            {
                puts("Path too long; can't complete blockwise");
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
            puts("--- blockwise complete ---");
        }
    }
}

static size_t _send(uint8_t *buf, size_t len, sock_udp_ep_t *remote)
{
    size_t bytes_sent;

    bytes_sent = gcoap_req_send(buf, len, remote, _resp_handler, NULL);

    return bytes_sent;
}

int coap_register_put(struct node_identity *sensor_id, char *uri)
{
    uint8_t buf[CONFIG_GCOAP_PDU_BUF_SIZE];
    coap_pkt_t pdu;
    size_t len;

    unsigned msg_type = COAP_TYPE_CON;

    int uri_len = strlen(uri);
    gcoap_req_init(&pdu, &buf[0], CONFIG_GCOAP_PDU_BUF_SIZE, COAP_METHOD_PUT, uri);
    coap_hdr_set_type(pdu.hdr, msg_type);
    memset(_last_req_path, 0, _LAST_REQ_PATH_MAX);
    if (uri_len < _LAST_REQ_PATH_MAX)
    {
        memcpy(_last_req_path, uri, uri_len);
    }
    coap_opt_add_format(&pdu, COAP_FORMAT_OCTET);
    len = coap_opt_finish(&pdu, COAP_OPT_FINISH_PAYLOAD);
    memcpy(pdu.payload, sensor_id, sizeof(struct node_identity));
    len += sizeof(struct node_identity);

    if (!_send(&buf[0], len, server))
    {
        puts("coap_client: msg send failed");
        return 1;
    }
    else
    {
        puts("Message sent");
    }

    return 0;
}

/*second block to receive messages from gateway */

static ssize_t _encode_link(const coap_resource_t *resource, char *buf,
                            size_t maxlen, coap_link_encoder_ctx_t *context);

static ssize_t _gateway_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, void *ctx);
static ssize_t _gateway_connection_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, void *ctx);

static const coap_resource_t _resources[] = {

    {"/actuator/beep", COAP_PUT, _gateway_handler, NULL},
    {"/register", COAP_PUT, _gateway_connection_handler, NULL},

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

static ssize_t _gateway_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, void *ctx)
{
    (void)ctx;

    if (pdu->payload_len)
    {

        //memcpy(rcv_msg, pdu->payload, sizeof(pdu->payload));
        DEBUG("received message: %s \n", pdu->payload);
        if (strcmp((char *)pdu->payload, TURN_ON_ALARM) == 0)
        {
            DEBUG("Beep\n");
            actuator_fire();
            return gcoap_response(pdu, buf, len, COAP_CODE_CHANGED);
        }

    }
    else
    {
        printf("Payload not found\n");
        return gcoap_response(pdu, buf, len, COAP_CODE_BAD_REQUEST);
    }

    return 0;
}

volatile bool connection_status = false;
static ssize_t _gateway_connection_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, void *ctx)
{
    (void)ctx;

    if (pdu->payload_len)
    {

        DEBUG("received status message from gateway :%s\n", pdu->payload);
        if (strcmp((char *)pdu->payload, REGISTER_RESPONSE) == 0)
        {
            connection_status = true;
            return gcoap_response(pdu, buf, len, COAP_CODE_CHANGED);
        }
        if (strcmp((char *)pdu->payload, DEREGISTER_RESPONSE) == 0)
        {
            connection_status = false;
            return gcoap_response(pdu, buf, len, COAP_CODE_CHANGED);
        }
    }
    else
    {

        printf("Payload not found\n");
        return gcoap_response(pdu, buf, len, COAP_CODE_BAD_REQUEST);
    }
    return 1;
}

void gcoap_actuator_init(void)
{
    gcoap_register_listener(&_listener);
}