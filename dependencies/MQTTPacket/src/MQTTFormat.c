/*******************************************************************************
 * Copyright (c) 2014 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *******************************************************************************/

#include "StackTrace.h"
#include "MQTTPacket.h"

#include <string.h>


const char* MQTTPacket_names[] =
{
        "RESERVED", "CONNECT", "CONNACK", "PUBLISH", "PUBACK", "PUBREC", "PUBREL",
        "PUBCOMP", "SUBSCRIBE", "SUBACK", "UNSUBSCRIBE", "UNSUBACK",
        "PINGREQ", "PINGRESP", "DISCONNECT"
};


const char* MQTTPacket_getName(unsigned short packetid)
{
        return MQTTPacket_names[packetid];
}


int MQTTStringFormat_connect(char* strbuf, int strbuflen, MQTTPacket_connectData* connectData)
{
        int strindex = 0;

        strindex = sprintf(strbuf,
                        "CONNECT MQTT version %d, client id %.*s, clean session %d, keep alive %d",
                        (int)connectData->MQTTVersion, connectData->clientID.lenstring.len, connectData->clientID.lenstring.str,
                        (int)connectData->cleansession, connectData->keepAliveInterval);
        if (connectData->willFlag)
                strindex += sprintf(&strbuf[strindex],
                                ", will QoS %d, will retain %d, will topic %.*s, will message %.*s",
                                connectData->will.qos, connectData->will.retained,
                                connectData->will.topicName.lenstring.len, connectData->will.topicName.lenstring.str,
                                connectData->will.message.lenstring.len, connectData->will.message.lenstring.str);
        if (connectData->username.lenstring.str && connectData->username.lenstring.len > 0)
                strindex += sprintf(&strbuf[strindex],
                                ", user name %.*s", connectData->username.lenstring.len, connectData->username.lenstring.str);
        if (connectData->password.lenstring.str && connectData->password.lenstring.len > 0)
                strindex += sprintf(&strbuf[strindex],
                                ", password %.*s", connectData->password.lenstring.len, connectData->password.lenstring.str);
        return strindex;
}


int MQTTStringFormat_connack(char* strbuf, int strbuflen, unsigned char connack_rc, unsigned char sessionPresent)
{
        int strindex = sprintf(strbuf, strbuflen, "CONNACK session present %d, rc %d", sessionPresent, connack_rc);
        return strindex;
}


int MQTTStringFormat_publish(char* strbuf, int strbuflen, unsigned char dup, int qos, unsigned char retained,
                unsigned short packetid, MQTTString topicName, unsigned char* payload, int payloadlen)
{
        int strindex = sprintf(strbuf,
                                "PUBLISH dup %d, QoS %d, retained %d, packet id %d, topic %.*s, payload length %d, payload %.*s",
                                dup, qos, retained, packetid,
                                (topicName.lenstring.len < 20) ? topicName.lenstring.len : 20, topicName.lenstring.str,
                                payloadlen, (payloadlen < 20) ? payloadlen : 20, payload);
        return strindex;
}


int MQTTStringFormat_ack(char* strbuf, int strbuflen, unsigned char packettype, unsigned char dup, unsigned short packetid)
{
        int strindex = sprintf(strbuf, "%s, packet id %d", MQTTPacket_names[packettype], packetid);
        if (dup)
                strindex += sprintf(strbuf + strindex, ", dup %d", dup);
        return strindex;
}


int MQTTStringFormat_subscribe(char* strbuf, int strbuflen, unsigned char dup, unsigned short packetid, int count,
                MQTTString topicFilters[], int requestedQoSs[])
{
        return sprintf(strbuf,
                "SUBSCRIBE dup %d, packet id %d count %d topic %.*s qos %d",
                dup, packetid, count,
                topicFilters[0].lenstring.len, topicFilters[0].lenstring.str,
                requestedQoSs[0]);
}


int MQTTStringFormat_suback(char* strbuf, int strbuflen, unsigned short packetid, int count, int* grantedQoSs)
{
        return sprintf(strbuf, strbuflen,
                "SUBACK packet id %d count %d granted qos %d", packetid, count, grantedQoSs[0]);
}


int MQTTStringFormat_unsubscribe(char* strbuf, int strbuflen, unsigned char dup, unsigned short packetid,
                int count, MQTTString topicFilters[])
{
        return sprintf(strbuf, strbuflen,
                                        "UNSUBSCRIBE dup %d, packet id %d count %d topic %.*s",
                                        dup, packetid, count,
                                        topicFilters[0].lenstring.len, topicFilters[0].lenstring.str);
}


#if defined(MQTT_CLIENT)
char* MQTTFormat_toClientString(char* strbuf, int strbuflen, unsigned char* buf, int buflen)
{
        int index = 0;
        int rem_length = 0;
        MQTTHeader header = {0};
        int strindex = 0;

        header.byte = buf[index++];
        index += MQTTPacket_decodeBuf(&buf[index], &rem_length);

        switch (header.bits.type)
        {

        case CONNACK:
        {
                unsigned char sessionPresent, connack_rc;
                if (MQTTDeserialize_connack(&sessionPresent, &connack_rc, buf, buflen) == 1)
                        strindex = MQTTStringFormat_connack(strbuf, strbuflen, connack_rc, sessionPresent);
        }
        break;
        case PUBLISH:
        {
                unsigned char dup, retained, *payload;
                unsigned short packetid;
                int qos, payloadlen;
                MQTTString topicName = MQTTString_initializer;
                if (MQTTDeserialize_publish(&dup, &qos, &retained, &packetid, &topicName,
                                &payload, &payloadlen, buf, buflen) == 1)
                        strindex = MQTTStringFormat_publish(strbuf, strbuflen, dup, qos, retained, packetid,
                                        topicName, payload, payloadlen);
        }
        break;
        case PUBACK:
        case PUBREC:
        case PUBREL:
        case PUBCOMP:
        {
                unsigned char packettype, dup;
                unsigned short packetid;
                if (MQTTDeserialize_ack(&packettype, &dup, &packetid, buf, buflen) == 1)
                        strindex = MQTTStringFormat_ack(strbuf, strbuflen, packettype, dup, packetid);
        }
        break;
        case SUBACK:
        {
                unsigned short packetid;
                int maxcount = 1, count = 0;
                int grantedQoSs[1];
                if (MQTTDeserialize_suback(&packetid, maxcount, &count, grantedQoSs, buf, buflen) == 1)
                        strindex = MQTTStringFormat_suback(strbuf, strbuflen, packetid, count, grantedQoSs);
        }
        break;
        case UNSUBACK:
        {
                unsigned short packetid;
                if (MQTTDeserialize_unsuback(&packetid, buf, buflen) == 1)
                        strindex = MQTTStringFormat_ack(strbuf, strbuflen, UNSUBACK, 0, packetid);
        }
        break;
        case PINGREQ:
        case PINGRESP:
        case DISCONNECT:
                strindex = snprintf(strbuf, strbuflen, "%s", MQTTPacket_names[header.bits.type]);
                break;
        }
        return strbuf;
}
#endif

#if defined(MQTT_SERVER)
char* MQTTFormat_toServerString(char* strbuf, int strbuflen, unsigned char* buf, int buflen)
{
        int index = 0;
        int rem_length = 0;
        MQTTHeader header = {0};
        int strindex = 0;

        header.byte = buf[index++];
        index += MQTTPacket_decodeBuf(&buf[index], &rem_length);

        switch (header.bits.type)
        {
        case CONNECT:
        {
                MQTTPacket_connectData data;
                int rc;
                if ((rc = MQTTDeserialize_connect(&data, buf, buflen)) == 1)
                        strindex = MQTTStringFormat_connect(strbuf, strbuflen, &data);
        }
        break;
        case PUBLISH:
        {
                unsigned char dup, retained, *payload;
                unsigned short packetid;
                int qos, payloadlen;
                MQTTString topicName = MQTTString_initializer;
                if (MQTTDeserialize_publish(&dup, &qos, &retained, &packetid, &topicName,
                                &payload, &payloadlen, buf, buflen) == 1)
                        strindex = MQTTStringFormat_publish(strbuf, strbuflen, dup, qos, retained, packetid,
                                        topicName, payload, payloadlen);
        }
        break;
        case PUBACK:
        case PUBREC:
        case PUBREL:
        case PUBCOMP:
        {
                unsigned char packettype, dup;
                unsigned short packetid;
                if (MQTTDeserialize_ack(&packettype, &dup, &packetid, buf, buflen) == 1)
                        strindex = MQTTStringFormat_ack(strbuf, strbuflen, packettype, dup, packetid);
        }
        break;
        case SUBSCRIBE:
        {
                unsigned char dup;
                unsigned short packetid;
                int maxcount = 1, count = 0;
                MQTTString topicFilters[1];
                int requestedQoSs[1];
                if (MQTTDeserialize_subscribe(&dup, &packetid, maxcount, &count,
                                topicFilters, requestedQoSs, buf, buflen) == 1)
                        strindex = MQTTStringFormat_subscribe(strbuf, strbuflen, dup, packetid, count, topicFilters, requestedQoSs);;
        }
        break;
        case UNSUBSCRIBE:
        {
                unsigned char dup;
                unsigned short packetid;
                int maxcount = 1, count = 0;
                MQTTString topicFilters[1];
                if (MQTTDeserialize_unsubscribe(&dup, &packetid, maxcount, &count, topicFilters, buf, buflen) == 1)
                        strindex =  MQTTStringFormat_unsubscribe(strbuf, strbuflen, dup, packetid, count, topicFilters);
        }
        break;
        case PINGREQ:
        case PINGRESP:
        case DISCONNECT:
                strindex = snprintf(strbuf, strbuflen, "%s", MQTTPacket_names[header.bits.type]);
                break;
        }
        strbuf[strbuflen] = '\0';
        return strbuf;
}
#endif
