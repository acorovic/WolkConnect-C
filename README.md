# WolkConnect-C
WolkAbout C Connector library for connecting devices to WolkAbout IoT platform.

WolkConnect-C is transportation layer agnostic which means it is up to the user of the library to
open socket to WolkAbout IoT platform, configure SSL if desired, and forward read/write implementation to WolkConnect-C.

This allows WolkConnect-C to work on wide variety of systems, bare metal to OS based ones.

Prerequisite
------
Following tools/libraries are required in order to build WolkAbout C connector

 * cmake

Former can be installed on Debian based system from terminal by invoking
`apt-get install cmake`

Afterwards dependencies are installed, Unix Makefiles build system can generated by invoking
`./configure`

Generated build system is located inside 'build' directory

WolkAbout C Connector library, and example are built from 'build' directory by invoking
`make` in terminal

Binary of example app is located in out/bin folder.

## Library usage

Edit device information

```
static const char *device_key = "device_key";
static const char *password = "password";

```

Set protocol which will be used

```
wolk_set_protocol (wolk_ctx_t *ctx, protocol_type_t protocol);
```

Connect to server

```
wolk_connect (wolk_ctx_t *ctx, send_func snd_func, recv_func rcv_func, const char *device_key, const char *password);
    send_buffer - function that will serve as callback when payload needs to be sent
    receive_buffer  - function that will serve as callback when data needs to be received from cloud
```

Set actuator references

```
wolk_set_actuator_references (wolk_ctx_t *ctx, int num_of_items, const char *item, ...);
```

If actuators are present, send initial actuator status to WolkAbout IoT platform
Depending on the actuator type you can use:

```
wolk_publish_num_actuator_status (wolk_ctx_t *ctx,const char *reference,double value, actuator_status_t state, uint32_t utc_time);
```
or
```
wolk_publish_bool_actuator_status (wolk_ctx_t *ctx,const char *reference,bool value, actuator_status_t state, uint32_t utc_time);
```

Publishing data can be done either through:

```
wolk_publish_single (wolk_ctx_t *ctx,const char *reference,const char *value, data_type_t type, uint32_t utc_time)
```
or you can aggregate multiple readings and then publish them with use of:

```
wolk_add_string_reading(wolk_ctx_t *ctx,const char *reference,const char *value, uint32_t utc_time);
wolk_add_numeric_reading(wolk_ctx_t *ctx,const char *reference,double value, uint32_t utc_time);
wolk_add_bool_reading(wolk_ctx_t *ctx,const char *reference,bool value, uint32_t utc_time);
wolk_publish (wolk_ctx_t *ctx);
```

Receiving actuation:

First you read all incoming traffic and the read actuators

```
wolk_receive (wolk_ctx_t *ctx, unsigned int timeout);
wolk_read_actuator (wolk_ctx_t *ctx, char *command, char *reference, char *value);
```

## Example
Send readings example

```
////////////////////////
Callback functions example:

static int send_buffer(unsigned char* buffer, unsigned int len)
{
    int n = write(sockfd, buffer, len);
    if (n < 0)
        return -1;

    return n;
}

static int receive_buffer(unsigned char* buffer, unsigned int max_bytes)
{
    bzero(buffer, max_bytes);
    int n = read(sockfd, buffer, max_bytes);
    if (n < 0)
        return -1;

    return n;
}
///////////////////////
const char *device_key = "device_key";
const char *password = "password";
wolk_ctx_t wolk;
wolk_set_protocol(&wolk, PROTOCOL_TYPE_JSON);
wolk_connect(&wolk, &send_buffer, &receive_buffer, device_key, password);
wolk_publish_single (&wolk, "reference", "23.2", DATA_TYPE_NUMERIC, 0);
```

Receive actuation example

```
const char *device_key = "device_key";
const char *password = "password";
char reference[32];
char command [32];
char value[64];
unsigned current_time = (unsigned)time(NULL);
wolk_ctx_t wolk;
wolk_set_protocol(&wolk, PROTOCOL_TYPE_JSON);
wolk_connect(&wolk, &send_buffer, &receive_buffer, device_key, password);
wolk_set_actuator_references (&wolk, 1, "Actuator reference");
wolk_publish_num_actuator_status (&wolk, "Actuator reference", value, ACTUATOR_STATUS_READY, current_time);
wolk_receive (&wolk, timeout);
wolk_read_actuator (&wolk, command, reference, value);
```
