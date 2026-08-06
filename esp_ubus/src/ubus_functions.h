#ifndef UBUSFUNCTIONS_H
#define UBUSFUNCTIONS_H

#define DATA_LENGTH 128
#define RESPONSE_LENGTH 256
#define STRING_LENGTH 32

enum {
    SUCCESS,
    FAILURE,
    INVALID_ARGUMENT,
    NO_RESPONSE,
    USB_ERROR
};

struct ubus_context;
struct ubus_request_data;
struct blob_attr;

int get_data_from_response(struct ubus_context *ctx, struct ubus_request_data *req,char* response, double *humidity, double *temperature);
int parse_response_attribute(struct ubus_context *ctx, struct ubus_request_data *req, struct blob_attr *data, char* message, double* humidity, double* temperature, int *rc);
void get_humidity_temperature(double *humidity, double *temperature, struct blob_attr *data);
void status_message(struct ubus_context *ctx, struct ubus_request_data *req, char* message, int rc);

#endif