// #include "messages.pb.h"

#include <libwebsockets.h>
#include <string.h>
#include <stdio.h>



static int callback_http( struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len )
{
	switch( reason )
	{
		case LWS_CALLBACK_HTTP:
			lws_serve_http_file( wsi, "example.html", "text/html", NULL, 0 );
			break;
		default:
			break;
	}

	return 0;
}

#define EXAMPLE_RX_BUFFER_BYTES (10)
struct payload
{
	unsigned char data[LWS_SEND_BUFFER_PRE_PADDING + EXAMPLE_RX_BUFFER_BYTES + LWS_SEND_BUFFER_POST_PADDING];
	size_t len;
} received_payload;

static int callback_example( struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len )
{
	switch( reason )
	{
		case LWS_CALLBACK_ESTABLISHED: // just log message that someone is connecting
            printf("Connection established\n");
            break;

		case LWS_CALLBACK_RECEIVE:
			// create a buffer to hold our response
            // it has to have some pre and post padding. You don't need to care
            // what comes there, lwss will do everything for you. For more info see
            // http://git.warmcat.com/cgi-bin/cgit/lwss/tree/lib/lwss.h#n597
			for (int i = 0; i < len; i++) {
				received_payload.data[LWS_SEND_BUFFER_PRE_PADDING + (len - 1) - i ] = 
					((char *) in)[i];
			}
			// memcpy( &received_payload.data[LWS_SEND_BUFFER_PRE_PADDING], in, len );
			received_payload.len = len;
			// log what we recieved and what we're going to send as a response.
            // that disco syntax `%.*s` is used to print just a part of our buffer
            // http://stackoverflow.com/questions/5189071/print-part-of-char-array
			printf("received data: %.*s, replying: %.*s, len: %d\n", 
			(int) len, 
			(char *) in, 
			(int) len, 
			received_payload.data + LWS_SEND_BUFFER_PRE_PADDING,
			(int) len);
			lws_callback_on_writable_all_protocol( lws_get_context( wsi ), lws_get_protocol( wsi ) );
			break;

		case LWS_CALLBACK_SERVER_WRITEABLE:
			// send response
            // just notice that we have to tell where exactly our response starts. That's
            // why there's `buf[LWS_SEND_BUFFER_PRE_PADDING]` and how long it is.
            // we know that our response has the same length as request because
            // it's the same message in reverse order.
			lws_write( wsi, &received_payload.data[LWS_SEND_BUFFER_PRE_PADDING], received_payload.len, LWS_WRITE_TEXT );
			break;

		default:
			break;
	}

	return 0;
}

enum protocols
{
	PROTOCOL_HTTP = 0,
	PROTOCOL_EXAMPLE,
	PROTOCOL_COUNT
};

static struct lws_protocols protocols[] =
{
	/* The first protocol must always be the HTTP handler */
	{
		"http-only",   /* name */
		callback_http, /* callback */
		0,             /* No per session data. */
		0,             /* max frame size / rx buffer */
	},
	{
		"example-protocol", // protocol name - very important!
		callback_example, // callback
		0, // we don't use any per session data
		EXAMPLE_RX_BUFFER_BYTES,
	},
	{ NULL, NULL, 0, 0 } /* terminator of list*/
};

int main( int argc, char *argv[] )
{
	int port = 8000;
	struct lws_context *context;
	struct lws_context_creation_info context_info;
	memset( &context_info, 0, sizeof(context_info) );

	/*struct lws_context_creation_info context_info =
    {
        .port = port, .iface = NULL, .protocols = protocols, .extensions = NULL,
        .ssl_cert_filepath = NULL, .ssl_private_key_filepath = NULL, .ssl_ca_filepath = NULL,
        .gid = -1, .uid = -1, .options = 0, NULL, .ka_time = 0, .ka_probes = 0, .ka_interval = 0
    };*/

	context_info.port = port;
	context_info.protocols = protocols;
	context_info.gid = -1;
	context_info.uid = -1;

	// create lws context representing this server
	context = lws_create_context( &context_info );

	if (context == NULL) {
        fprintf(stderr, "lws init failed\n");
        return -1;
    }

    printf("starting server...\n");


	while( 1 )
	{ 	// to end this server send SIGTERM. (CTRL+C)
		lws_service( context, /* timeout_ms = */ 1000000 );

		// lws_service will process all waiting events with their
        // callback functions and then wait for timeout ms.
        // (this is a single threaded webserver and this will keep our server
        // from generating load while there are not requests to process)
	}

	lws_context_destroy( context );

	return 0;
}
