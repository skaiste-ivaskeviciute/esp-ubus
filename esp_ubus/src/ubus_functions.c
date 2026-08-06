#include "ubus_functions.h"

#include <libubox/blobmsg_json.h>
#include <libubus.h>
#include <libserialport.h>
#include <stdio.h>
#include <string.h>

// Gets data from ESP's response JSON
int get_data_from_response(struct ubus_context *ctx, struct ubus_request_data *req, char* response, double *humidity, double *temperature)
{
	static struct blob_buf buf;
	struct blob_attr *cur;
	size_t rem;
	int rc = SUCCESS;
	int response_rc = SUCCESS;
	char message[RESPONSE_LENGTH];
	
	blobmsg_buf_init(&buf);
	blobmsg_add_json_from_string(&buf, response);

	blobmsg_for_each_attr(cur, buf.head, rem) {
		rc = parse_response_attribute(ctx, req, cur, message, humidity, temperature, &response_rc);
		if (rc != SUCCESS) {
			goto cleanup;
		}
	}
cleanup:
	blob_buf_free(&buf);
	return rc;
}

// Parses an attribute of the response JSON
int parse_response_attribute(struct ubus_context *ctx, struct ubus_request_data *req, struct blob_attr *data, char* message, double* humidity, double* temperature, int *rc)
{
	if (strcmp(blobmsg_name(data), "rc") == 0) {
		*rc = blobmsg_get_u32(data);
	}
	if (strcmp(blobmsg_name(data), "msg") == 0) {
		strcpy(message, blobmsg_get_string(data));
		if ((*rc != SUCCESS) || (strcmp(message, "DHT returned no data") == 0)) {
			status_message(ctx, req, message, FAILURE);
			return FAILURE;
		}
	}
	if (strcmp(blobmsg_name(data), "data") == 0) {
		if ((humidity == NULL) || (temperature == NULL)) {
			return SUCCESS;
		}
		if (*rc == SUCCESS) {
			get_humidity_temperature(humidity, temperature, data);
		}
	}
	return SUCCESS;
}

// Gets humidity and temperature from the response's data attribute
void get_humidity_temperature(double *humidity, double *temperature, struct blob_attr *data)
{
	struct blob_attr *cur;
	size_t rem;

	blobmsg_for_each_attr(cur, data, rem) {
		if (strcmp(blobmsg_name(cur), "humidity") == 0) {
			*humidity = blobmsg_get_double(cur);
		}
		if (strcmp(blobmsg_name(cur), "temperature") == 0) {
			*temperature = blobmsg_get_double(cur);
		}
	}
}

// Sends an error message via the blobmsg library
void status_message(struct ubus_context *ctx, struct ubus_request_data *req, char* message, int rc)
{
	static struct blob_buf b;
	blobmsg_buf_init(&b);
	blobmsg_add_u32(&b, "rc", rc);
	blobmsg_add_string(&b,"status_message", message);
	ubus_send_reply(ctx, req, b.head);
	blob_buf_free(&b);
}