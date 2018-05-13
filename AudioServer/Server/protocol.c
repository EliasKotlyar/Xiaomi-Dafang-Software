/*
 * ws protocol handler plugin for "lws-minimal-server-echo"
 *
 * Copyright (C) 2010-2018 Andy Green <andy@warmcat.com>
 *
 * This file is made available under the Creative Commons CC0 1.0
 * Universal Public Domain Dedication.
 *
 * The protocol shows how to send and receive bulk messages over a ws connection
 * that optionally may have the permessage-deflate extension negotiated on it.
 */

#if !defined (LWS_PLUGIN_STATIC)
#define LWS_DLL
#define LWS_INTERNAL
#include <libwebsockets.h>
#endif
#include <stdio.h>
#include <string.h>
#include "server.h"
#include "play.h"
/* destroys the message when everyone has had a copy of it */

static void __minimal_destroy_message(void *_msg)
{
	struct msg *msg = _msg;

	free(msg->payload);
	msg->payload = NULL;
	msg->len = 0;
}
/* binary search in memory */
int memsearch(const char *hay, int haysize, const char *needle, int needlesize) {
    int haypos, needlepos;
    haysize -= needlesize;
    for (haypos = 0; haypos <= haysize; haypos++) {
        for (needlepos = 0; needlepos < needlesize; needlepos++) {
            if (hay[haypos + needlepos] != needle[needlepos]) {
                // Next character in haystack.
                break;
            }
        }
        if (needlepos == needlesize) {
            return haypos;
        }
    }
    return -1;
}

#define MAGIC "ServerSetValues "
#define MIN(a,b) (((a)<(b))?(a):(b))

int callback_minimal(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
	struct per_session_data__minimal *pss =	(struct per_session_data__minimal *)user;
	struct per_vhost_data__minimal *vhd =	(struct per_vhost_data__minimal *)
			lws_protocol_vh_priv_get(lws_get_vhost(wsi), lws_get_protocol(wsi));
	static int fd = -1;
    static int inSampleRate = 48000;
    static int inVolume = 100;


	switch (reason) {
	case LWS_CALLBACK_PROTOCOL_INIT:
		vhd = lws_protocol_vh_priv_zalloc(lws_get_vhost(wsi),
				lws_get_protocol(wsi),
				sizeof(struct per_vhost_data__minimal));
		vhd->context = lws_get_context(wsi);
		vhd->protocol = lws_get_protocol(wsi);
		vhd->vhost = lws_get_vhost(wsi);

		break;

	case LWS_CALLBACK_ESTABLISHED:

		/* add ourselves to the list of live pss held in the vhd */
		lws_ll_fwd_insert(pss, pss_list, vhd->pss_list);
		pss->wsi = wsi;
		pss->last = vhd->current;
		break;

	case LWS_CALLBACK_CLOSED:
    	lwsl_user("LWS_CALLBACK_CLOSE");

		/* remove our closing pss from the list of live pss */
		lws_ll_fwd_remove(struct per_session_data__minimal, pss_list,
				  pss, vhd->pss_list);
	    if (fd != -1)
        {
            close_device(&fd);
        }

		break;

	case LWS_CALLBACK_SERVER_WRITEABLE:
	/*	if (!vhd->amsg.payload)
			break;

		if (pss->last == vhd->current)
			break;

		// notice we allowed for LWS_PRE in the payload already
		m = lws_write(wsi, vhd->amsg.payload + LWS_PRE, vhd->amsg.len,
			      LWS_WRITE_TEXT);
		if (m < (int)vhd->amsg.len) {
			lwsl_err("ERROR %d writing to ws\n", m);
			return -1;
		}

		pss->last = vhd->current;*/
		break;
	case LWS_CALLBACK_RECEIVE:
        lwsl_user("LWS_CALLBACK_RECEIVE: %d", (int)len);


        if (len) {
           int vol = 0;
           int retval = sscanf(in, MAGIC"[%d,%d]", &inSampleRate, &vol);
           if (retval == 2)
           {
                lwsl_user("Sample Rate=%d, Volume=%d oldvol=%d fd=%d", inSampleRate, vol,inVolume, fd);
                if ((fd > 0) &&
                    (vol != inVolume))
                {
                    controlVolume(&fd,vol );
                    lwsl_user("Changing volume to %d", vol);
                }
                inVolume = vol;
           } else {
                if (fd == -1)
                {
                    open_device(&fd, inSampleRate);
                    controlVolume(&fd,inVolume );
                }

                play(&fd, len, in);
                    //fwrite(in, len, 1, stdout);
                    //puts((const char *)in);
                    //lwsl_hexdump_notice(in, len);
            }
        }

		if (vhd->amsg.payload)
			__minimal_destroy_message(&vhd->amsg);

		vhd->amsg.len = len;
		/* notice we over-allocate by LWS_PRE */
		vhd->amsg.payload = malloc(LWS_PRE + len);
		if (!vhd->amsg.payload) {
			lwsl_user("OOM: dropping\n");
			break;
		}

		memcpy((char *)vhd->amsg.payload + LWS_PRE, in, len);
		vhd->current++;

		/*
		 * let everybody know we want to write something on them
		 * as soon as they are ready
		 */
		lws_start_foreach_llp(struct per_session_data__minimal **,
				      ppss, vhd->pss_list) {
			lws_callback_on_writable((*ppss)->wsi);
		} lws_end_foreach_llp(ppss, pss_list);
		break;

	default:
		break;
	}

	return 0;
}
