
#include <libwebsockets.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#define LWS_PLUGIN_STATIC
#include "server.h"
#include "play.h"

static struct lws_protocols protocols[] = {
	LWS_PLUGIN_PROTOCOL_MINIMAL,
	{ NULL, NULL, 0, 0 } /* terminator */
};

static int interrupted;
static int portSecure = 7681, portUnsecure = 7682;
static int options;

/* pass pointers to shared vars to the protocol */

static const struct lws_protocol_vhost_options pvo_options = {
	NULL,
	NULL,
	"options",		/* pvo name */
	(void *)&options	/* pvo value */
};

static const struct lws_protocol_vhost_options pvo_interrupted = {
	&pvo_options,
	NULL,
	"interrupted",		/* pvo name */
	(void *)&interrupted	/* pvo value */
};

static const struct lws_protocol_vhost_options pvo = {
	NULL,				/* "next" pvo linked-list */
	&pvo_interrupted,		/* "child" pvo linked-list */
	"AudioSever",	/* protocol name we belong to on this vhost */
	""				/* ignored */
};
static const struct lws_extension extensions[] = {
	{
		"permessage-deflate",
		lws_extension_callback_pm_deflate,
		"permessage-deflate"
		 "; client_no_context_takeover"
		 "; client_max_window_bits"
	},
	{ NULL, NULL, NULL /* terminator */ }
};

void sigint_handler(int sig)
{
	interrupted = 1;
}





int main(int argc, const char **argv)
{
	struct lws_context_creation_info info;
	struct lws_context *context;
	const char *p;
	int n = 0, logs = LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE
			/* for LLL_ verbosity above NOTICE to be built into lws,
			 * lws must have been configured and built with
			 * -DCMAKE_BUILD_TYPE=DEBUG instead of =RELEASE */
			/* | LLL_INFO */ /* | LLL_PARSER */ /* | LLL_HEADER */
			/* | LLL_EXT */ /* | LLL_CLIENT */ /* | LLL_LATENCY */
			/* | LLL_DEBUG */;


	signal(SIGINT, sigint_handler);

	if ((p = lws_cmdline_option(argc, argv, "-d")))
		logs = atoi(p);

	lws_set_log_level(logs, NULL);
	lwsl_user("%s [-u unsecurePort(ws)] [-s securePort(wss)]\n", argv[0]);


	if ((p = lws_cmdline_option(argc, argv, "-u")))
	    portUnsecure = atoi(p);

	if ((p = lws_cmdline_option(argc, argv, "-p")))
		portSecure = atoi(p);

/*	if (lws_cmdline_option(argc, argv, "-o"))
	{
		options |= 1;
	}
*/

	memset(&info, 0, sizeof info); /* otherwise uninitialized garbage */

	info.pvo = &pvo;
	if (!lws_cmdline_option(argc, argv, "-n"))
		info.extensions = extensions;
	info.pt_serv_buf_size = 8 * 1024;
	info.options = LWS_SERVER_OPTION_VALIDATE_UTF8;

    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    info.options |= LWS_SERVER_OPTION_EXPLICIT_VHOSTS;
    info.ssl_cert_filepath = "/system/sdcard/config/dafang.com.crt";
    info.ssl_private_key_filepath = "/system/sdcard/config/dafang.com.key";

	context = lws_create_context(&info);
	if (!context) {
		lwsl_err("lws init failed\n");
		return 1;
	}

    // As I didn't succeed to listen ws and wss on the same port, let's create vhosts
    // One for ws and one for wss

	memset(&info, 0, sizeof info);
	info.port = portSecure;
    info.protocols = protocols;
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    info.options |= LWS_SERVER_OPTION_EXPLICIT_VHOSTS;
    info.ssl_cert_filepath = "/system/sdcard/config/lighttpd.crt";
    info.ssl_private_key_filepath = "/system/sdcard/config/lighttpd.key";

    struct lws_vhost* vhost = lws_create_vhost(context, &info);
    if (vhost == NULL)
    {
    	lwsl_err("error lws_create_vhost\n");
    }

	memset(&info, 0, sizeof info);
	info.port = portUnsecure;
    info.protocols = protocols;
    vhost = lws_create_vhost(context, &info);
    if (vhost == NULL)
    {
    	lwsl_err("error lws_create_vhost\n");
    }

	while (n >= 0 && !interrupted)
		n = lws_service(context, 1000);

	lws_context_destroy(context);

	lwsl_user("Completed %s\n", interrupted == 2 ? "OK" : "failed");

	return interrupted != 2;
}
