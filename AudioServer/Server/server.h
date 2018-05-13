
#ifndef SERVER_H
#define SERVER_H
#ifdef toto
/* one of these created for each message */

struct msg {
	void *payload; /* is malloc'd */
	size_t len;
	char binary;
	char first;
	char final;
};

struct per_session_data__minimal_server_echo {
	struct lws_ring *ring;
	uint32_t msglen;
	uint32_t tail;
	uint8_t completed:1;
	uint8_t flow_controlled:1;
};

struct vhd_minimal_server_echo {
	struct lws_context *context;
	struct lws_vhost *vhost;

	int *interrupted;
	int *options;
};

static void
__minimal_destroy_message(void *_msg)
{
	struct msg *msg = _msg;

	free(msg->payload);
	msg->payload = NULL;
	msg->len = 0;
}
int callback_minimal_server_echo(struct lws *wsi, enum lws_callback_reasons reason,  void *user, void *in, size_t len);

#define LWS_PLUGIN_PROTOCOL_MINIMAL_SERVER_ECHO \
	{ \
		"lws-minimal-server-echo", \
		callback_minimal_server_echo, \
		sizeof(struct per_session_data__minimal_server_echo), \
		1024, \
		0, NULL, 0 \
	}

#endif
#endif
/* one of these created for each message */

struct msg {
	void *payload; /* is malloc'd */
	size_t len;
};

/* one of these is created for each client connecting to us */

struct per_session_data__minimal {
	struct per_session_data__minimal *pss_list;
	struct lws *wsi;
	int last; /* the last message number we sent */
};

/* one of these is created for each vhost our protocol is used with */

struct per_vhost_data__minimal {
	struct lws_context *context;
	struct lws_vhost *vhost;
	const struct lws_protocols *protocol;

	struct per_session_data__minimal *pss_list; /* linked-list of live pss*/

	struct msg amsg; /* the one pending message... */
	int current; /* the current message number we are caching */
};

int callback_minimal(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);

#define LWS_PLUGIN_PROTOCOL_MINIMAL \
	{ \
		"lws-minimal", \
		callback_minimal, \
		sizeof(struct per_session_data__minimal), \
		1024, \
		0, NULL, 0 \
	}

