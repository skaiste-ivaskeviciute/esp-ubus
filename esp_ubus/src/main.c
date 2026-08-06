#include "usb_functions.h"
#include "ubus_functions.h"

#include <libubox/blobmsg_json.h>
#include <libubus.h>
#include <libserialport.h>
#include <stdio.h>
#include <string.h>

static int esp_get(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg);

static int esp_devices(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg);

static int esp_on(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg);

static int esp_off(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg);

enum {
	PORT_VALUE,
    PIN_VALUE,
    __PORT_PIN_MAX
};

enum {
	PORT,
    PIN,
    MODEL,
    SENSOR,
    __GET_DATA_MAX
};

static const struct blobmsg_policy on_off_policy[] = {
	[PORT_VALUE] = { .name = "port", .type = BLOBMSG_TYPE_STRING },
	[PIN_VALUE] = { .name = "pin", .type = BLOBMSG_TYPE_INT32 },
};

static const struct blobmsg_policy get_policy[] = {
	[PORT] = { .name = "port", .type = BLOBMSG_TYPE_STRING },
	[PIN] = { .name = "pin", .type = BLOBMSG_TYPE_INT32 },
    [MODEL] = { .name = "model", .type = BLOBMSG_TYPE_STRING }, 
    [SENSOR] = { .name = "sensor", .type = BLOBMSG_TYPE_STRING }, 
};

static const struct ubus_method esp_methods[] = {
	UBUS_METHOD_NOARG("devices", esp_devices),
	UBUS_METHOD("on", esp_on, on_off_policy),
	UBUS_METHOD("off", esp_off, on_off_policy),
	UBUS_METHOD("get", esp_get, get_policy)
};

static struct ubus_object_type esp_object_type =
	UBUS_OBJECT_TYPE("esp", esp_methods);


static struct ubus_object esp_object = {
	.name = "esp",
	.type = &esp_object_type,
	.methods = esp_methods,
	.n_methods = ARRAY_SIZE(esp_methods),
};

// Gets humidity & temperature data from attached DTH sensors via their port, pin, model, sensor
static int esp_get(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	char data[DATA_LENGTH];
	char response[RESPONSE_LENGTH];
	char port_name[STRING_LENGTH];
	char model[STRING_LENGTH];
	char sensor[STRING_LENGTH];
	int pin;
	double humidity = 0;
	double temperature = 0;
	struct blob_attr *tb[__GET_DATA_MAX];
	static struct blob_buf b;
	void *table;
	// Gets data from user inputted JSON string
	blobmsg_parse(get_policy, __GET_DATA_MAX, tb, blob_data(msg), blob_len(msg));
	if (!tb[PORT]) {
		status_message(ctx, req, "Failed to get port from user input.", INVALID_ARGUMENT);
		return UBUS_STATUS_INVALID_ARGUMENT;
	}
	if (!tb[PIN]) {
		status_message(ctx, req, "Failed to get pin from user input.", INVALID_ARGUMENT);
		return UBUS_STATUS_INVALID_ARGUMENT;
	}
	if (!tb[MODEL]) {
		status_message(ctx, req, "Failed to get model from user input.", INVALID_ARGUMENT);
		return UBUS_STATUS_INVALID_ARGUMENT;
	}
	if (!tb[SENSOR]) {
		status_message(ctx, req, "Failed to get sensor from user input.", INVALID_ARGUMENT);
		return UBUS_STATUS_INVALID_ARGUMENT;
	}
		
	pin = blobmsg_get_u32(tb[PIN]);
	strncpy(port_name, blobmsg_get_string(tb[PORT]), STRING_LENGTH);
	strncpy(model, blobmsg_get_string(tb[MODEL]), STRING_LENGTH);
	strncpy(sensor, blobmsg_get_string(tb[SENSOR]), STRING_LENGTH);

	// Sends & receives data from device
	sprintf(data,"{\"action\": \"get\", \"sensor\": \"%s\", \"pin\": %d, \"model\": \"%s\"}", sensor, pin, model);
	if (send_and_receive(ctx, req, port_name, data, response) != 0) {
		return UBUS_STATUS_UNKNOWN_ERROR;
	}

	if (strcmp(response, "") == 0) {
		status_message(ctx, req, "Failed to receive response.", NO_RESPONSE);
		return UBUS_STATUS_UNKNOWN_ERROR;
	}

	// Gets humidity and temperature from device response JSON
	if (get_data_from_response(ctx, req, response, &humidity, &temperature) != 0) {
		return UBUS_STATUS_UNKNOWN_ERROR;
	}

	// Sends humidity and temperature as UBUS reply
	blobmsg_buf_init(&b);
	table = blobmsg_open_table(&b, "data");
	blobmsg_add_double(&b,"temperature",temperature);
	blobmsg_add_double(&b,"humidity",humidity);
	blobmsg_close_table(&b, table);
	blobmsg_add_u32(&b,"rc", SUCCESS);
	blobmsg_add_string(&b,"status_message","Sensor read successfully");
	ubus_send_reply(ctx, req, b.head);
	blob_buf_free(&b);

    return UBUS_STATUS_OK;
}

// Returns an array of devices, containing their ports, vendor ids and product ids.
static int esp_devices(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	static struct blob_buf b;
	blobmsg_buf_init(&b);
	if (get_port_pid_vid(&b) != 0) {
		status_message(ctx, req, "Error getting list of available serial ports via sp_list_ports().", USB_ERROR);
		return UBUS_STATUS_UNKNOWN_ERROR;
	}
	ubus_send_reply(ctx, req, b.head);
	blob_buf_free(&b);
    return UBUS_STATUS_OK;
}

// Turns on a pin, given a port and a pin number
static int esp_on(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	char data[DATA_LENGTH];
	char response[RESPONSE_LENGTH];
	char port_name[STRING_LENGTH];
	char message[DATA_LENGTH];
	struct blob_attr *tb[__PORT_PIN_MAX];
	int pin;
	
	// Gets pin and port from user inputted JSON
	blobmsg_parse(on_off_policy, __PORT_PIN_MAX, tb, blob_data(msg), blob_len(msg));
	if (!tb[PORT_VALUE]) {
		status_message(ctx,req,"Failed to get port from user input.", INVALID_ARGUMENT);
		return UBUS_STATUS_INVALID_ARGUMENT;
	}
	if (!tb[PIN_VALUE]) {
		status_message(ctx,req,"Failed to get pin from user input.", INVALID_ARGUMENT);
		return UBUS_STATUS_INVALID_ARGUMENT;
	}
	pin = blobmsg_get_u32(tb[PIN_VALUE]);
	strncpy(port_name, blobmsg_get_string(tb[PORT]), STRING_LENGTH);

	// Sends data to device
	sprintf(data, "{\"action\": \"on\",\"pin\": %d}", pin);
	
	if (send_and_receive(ctx, req, port_name, data, response) != 0) {
		return UBUS_STATUS_UNKNOWN_ERROR;
	}
	if (strcmp(response, "") == 0) {
		status_message(ctx, req, "Failed to receive response.", NO_RESPONSE);
		return UBUS_STATUS_UNKNOWN_ERROR;
	}
	if (get_data_from_response(ctx, req, response, NULL, NULL) != 0)
		return UBUS_STATUS_UNKNOWN_ERROR;
	
			
	sprintf(message, "Pin %d turned on successfully.", pin);
	status_message(ctx, req, message, SUCCESS);

    return UBUS_STATUS_OK;
}

// Turns off a pin, given a port and a pin number
static int esp_off(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	char data[DATA_LENGTH];
	char response[RESPONSE_LENGTH];
	char port_name[STRING_LENGTH];
	char message[DATA_LENGTH];
	struct blob_attr *tb[__PORT_PIN_MAX];
	int pin;

	// Gets pin and port from user inputted JSON
	blobmsg_parse(on_off_policy, __PORT_PIN_MAX, tb, blob_data(msg), blob_len(msg));
	if (!tb[PORT_VALUE]) {
		status_message(ctx,req,"Failed to get port from user input.", INVALID_ARGUMENT);
		return UBUS_STATUS_INVALID_ARGUMENT;
	}
	if (!tb[PIN_VALUE]) {
		status_message(ctx,req,"Failed to get pin from user input.", INVALID_ARGUMENT);
		return UBUS_STATUS_INVALID_ARGUMENT;
	}
	pin = blobmsg_get_u32(tb[PIN_VALUE]);
	strncpy(port_name, blobmsg_get_string(tb[PORT]), STRING_LENGTH);

	// Sends data to device
	sprintf(data,"{\"action\": \"off\",\"pin\": %d}", pin);
	if (send_and_receive(ctx, req, port_name, data, response) != 0) {
		return UBUS_STATUS_UNKNOWN_ERROR;
	}
	if (strcmp(response, "") == 0) {
		status_message(ctx, req, "Failed to receive response.", NO_RESPONSE);
		return UBUS_STATUS_UNKNOWN_ERROR;
	}
	if (get_data_from_response(ctx, req, response, NULL, NULL) != 0) {
		return UBUS_STATUS_UNKNOWN_ERROR;
	}	
		
	sprintf(message, "Pin %d turned off successfully.", pin);
	status_message(ctx, req, message, SUCCESS);

    return UBUS_STATUS_OK;
}

int main(int argc, char **argv)
{
	struct ubus_context *ctx;
    const char *ubus_socket = NULL;
	int ret;

	uloop_init();

	ctx = ubus_connect(ubus_socket);
	if (!ctx) {
		fprintf(stderr, "Failed to connect to ubus.\n");
		return -1;
	}

	ubus_add_uloop(ctx);
	
	ret = ubus_add_object(ctx, &esp_object);
	
    if (ret) {
        fprintf(stderr, "Failed to add object: %s.\n", ubus_strerror(ret));
    }	
	
	uloop_run();

	ubus_free(ctx);
	uloop_done();

	return 0;
}