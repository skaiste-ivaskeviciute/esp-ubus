#ifndef USBFUNCTIONS_H
#define USBFUNCTIONS_H

struct sp_port;
enum sp_return;
struct blob_buf;
struct ubus_request_data;
struct ubus_context;

int get_port_pid_vid(struct blob_buf *buf);
int send_and_receive(struct ubus_context *ctx, struct ubus_request_data *req, char* port_name, char* data, char* response);
int open_port(struct sp_port **port, char* port_name);
int close_port(struct sp_port **port);
int send_data(struct sp_port **port, char* data);
int receive_data(struct sp_port **port, char* data);
int wait_for_rx_ready(struct sp_port *port);
void usb_errors(struct ubus_context *ctx, struct ubus_request_data *req, int error_code);

#endif