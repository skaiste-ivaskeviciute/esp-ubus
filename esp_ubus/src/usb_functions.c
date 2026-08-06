#include "usb_functions.h"
#include "ubus_functions.h"

#include <libubox/blobmsg_json.h>
#include <libubus.h>
#include <libserialport.h>

// Adds all available port names, their product ids and vendor ids to a buffer
int get_port_pid_vid(struct blob_buf *buf)
{
	struct sp_port **port_list;
	enum sp_return result = sp_list_ports(&port_list);
	int vid, pid;
	char vid_formatted[5];
	char pid_formatted[5];
	void *table;

	if (result != SP_OK) {
		return FAILURE;
	}

	for (int i = 0; port_list[i] != NULL; i++) {
		struct sp_port *port = port_list[i];
		char *port_name = sp_get_port_name(port);
		enum sp_transport transport = sp_get_port_transport(port_list[i]);
		
		// Skips non-usb ports
		if (transport != SP_TRANSPORT_USB) {
			continue;
		}

		sp_get_port_usb_vid_pid(port_list[i], &vid, &pid);

		table = blobmsg_open_table(buf, NULL);
		blobmsg_add_string(buf, "port", port_name);

		sprintf(vid_formatted, "%04X", vid);
		blobmsg_add_string(buf, "vendorid", vid_formatted);

		sprintf(pid_formatted, "%04X", pid);
		blobmsg_add_string(buf, "productid", pid_formatted);
		blobmsg_close_table(buf, table);
	}

	sp_free_port_list(port_list);
	return SUCCESS;
}

// Opens a port, sends data to it, receives data from it, and closes the port 
int send_and_receive(struct ubus_context *ctx, struct ubus_request_data *req, char* port_name, char* data, char* response)
{
	struct sp_port *port;
	int rc = 0;

	rc = open_port(&port, port_name);
	if(rc != 0) {
		usb_errors(ctx, req, rc);
		return FAILURE;
	}
	
	rc = send_data(&port, data);
	if(rc != 0) {
		usb_errors(ctx, req, rc);
		return FAILURE;
	}

	rc = wait_for_rx_ready(port);
	if(rc != 0) {
		usb_errors(ctx, req, rc);
		return FAILURE;
	}

	rc = receive_data(&port, response);
	if(rc != 0) {
		usb_errors(ctx, req, rc);
		return FAILURE;
	}

	rc = close_port(&port);
	if(rc != 0) {
		usb_errors(ctx, req, rc);
		return FAILURE;
	}
	return SUCCESS;
}

// Opens a port via port name
int open_port(struct sp_port **port, char* port_name)
{
	int rc = 0;

	rc = sp_get_port_by_name(port_name, port);
	if (rc != 0) return rc;

	rc = sp_open(*port, SP_MODE_READ_WRITE);
	if (rc != 0) return rc;

	rc = sp_set_baudrate(*port, 9600);
	if (rc != 0) return rc;

	rc = sp_set_bits(*port, 8);
	if (rc != 0) return rc;

	rc = sp_set_parity(*port, SP_PARITY_NONE);
	if (rc != 0) return rc;

	rc = sp_set_stopbits(*port, 1);
	if (rc != 0) return rc;

	rc = sp_set_flowcontrol(*port, SP_FLOWCONTROL_NONE);
	return rc;
}

// Closes a given port
int close_port(struct sp_port **port)
{	
	int rc = 0;
	rc = sp_close(*port);
	if (rc != 0) return rc;
	sp_free_port(*port);
	return rc;
}

// Sends a string to a port
int send_data(struct sp_port **port, char* data)
{
	int size = strlen(data);
	int result = 0;
	unsigned int timeout = 1000;

	result = sp_blocking_write(*port, data, size, timeout);
	if (result < 0) {
		return result;
	}
	return 0;
}

// Receives a string from a port
int receive_data(struct sp_port **port, char* data)
{
	unsigned int timeout = 1000;
	int result = 0;
	char *buffer = malloc(RESPONSE_LENGTH + 1);

	result = sp_blocking_read(*port, buffer, RESPONSE_LENGTH, timeout);
	if (result < 0) {
		goto cleanup;
	}
	
	buffer[result] = '\0';
	strcpy(data, buffer);
	result = 0;
cleanup:
	free(buffer);
	return result;
}

// Waits until data is ready to be read 
int wait_for_rx_ready(struct sp_port *port)
{
	int rc = 0;
	unsigned int timeout = 5000;
	struct sp_event_set *event_set;

	rc = sp_new_event_set(&event_set);
	if(rc != 0) return rc;

	rc = sp_add_port_events(event_set, port, SP_EVENT_RX_READY); 
	if(rc != 0) return rc;

	rc = sp_wait(event_set, timeout); 
	if(rc != 0) return rc;

	sp_free_event_set(event_set);
	return rc;
}

// Sends an error message to ubus for a given libserialports error code
void usb_errors(struct ubus_context *ctx, struct ubus_request_data *req, int error_code)
{
	char full_message[DATA_LENGTH];
	char *failure_message;
	switch (error_code) {
	case SP_ERR_ARG:
		status_message(ctx, req, "USB error: Invalid argument.", USB_ERROR);
		break;
	case SP_ERR_FAIL:
		failure_message = sp_last_error_message();
		sprintf(full_message, "USB error: Failed: %s.", failure_message);
		status_message(ctx, req, full_message, USB_ERROR);
		sp_free_error_message(failure_message);
		break;
	case SP_ERR_MEM:
		status_message(ctx, req, "USB error: Couldn't allocate memory.", USB_ERROR);
		break;
	case SP_ERR_SUPP:
		status_message(ctx, req, "USB error: Operation is not supported.", USB_ERROR);
		break;
	default:
		break;
	}
}
