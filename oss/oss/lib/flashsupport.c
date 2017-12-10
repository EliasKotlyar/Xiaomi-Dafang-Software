/*
Adobe Systems Incorporated(r) Source Code License Agreement
Copyright(c) 2006 Adobe Systems Incorporated. All rights reserved.

Please read this Source Code License Agreement carefully before using
the source code.

Adobe Systems Incorporated grants to you a perpetual, worldwide, non-exclusive,
no-charge, royalty-free, irrevocable copyright license, to reproduce,
prepare derivative works of, publicly display, publicly perform, and
distribute this source code and such derivative works in source or
object code form without any attribution requirements.

The name "Adobe Systems Incorporated" must not be used to endorse or promote products
derived from the source code without prior written permission.

You agree to indemnify, hold harmless and defend Adobe Systems Incorporated from and
against any loss, damage, claims or lawsuits, including attorney's
fees that arise or result from your use or distribution of the source
code.

THIS SOURCE CODE IS PROVIDED "AS IS" AND "WITH ALL FAULTS", WITHOUT
ANY TECHNICAL SUPPORT OR ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED. ALSO, THERE IS NO WARRANTY OF
NON-INFRINGEMENT, TITLE OR QUIET ENJOYMENT. IN NO EVENT SHALL MACROMEDIA
OR ITS SUPPLIERS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOURCE CODE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// File name: flashsupport.c
// Targets: Adobe Flash Player 9.1 beta 2 for Linux (9.0.21.78)
// Last Revision Date: 11/20/2006
//

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// These are the feature which can be turned on and off. They are all
// optional. The ALSA and Video4Linux support in this file is somewhat
// redundant and reduced in functionality as the Flash Player already has
// ALSA and Video4Linux support built-in. It is provided here for reference only.
// Also, if your system has ALSA support in the kernel there is no need to
// enable OSS here as it will override it.
//

//#define OPENSSL
//#define GNUTLS
//#define ALSA
#define OSS
//#define V4L1

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// To compile and install flashsupport.c the following components are required:
//
//  - a C compiler (gcc 4.03 is known to be working)
//  - OpenSSL developer package and working user libraries (OpenSSL 0.9.8 is known to be working)
//  - OSS (or ALSA) developer package and working users libraries (Linux 2.6.15 is known to be working)
//  - sudo or root access to install the generated library to /usr/lib
//
// We suggest these steps in a terminal:
//
// > cc -shared -O2 -Wall -Werror -lssl flashsupport.c -o libflashsupport.so
// > ldd libflashplayer.so
// > sudo cp libflashplayer.so /usr/lib
//
// Make sure that 'ldd' can resolve all dynamic libraries. Otherwise the Flash Player
// will silently fail to load and use libflashsupport.so.
//

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// SHARED SECTION, DO NOT CHANGE!
//
#ifdef cplusplus
extern "C"
{
#endif				// cplusplus

//
// Imported functions
//

  typedef void *(*T_FPI_Mem_Alloc) (int size);	// This function is not thread safe
  typedef void (*T_FPI_Mem_Free) (void *ptr);	// This function is not thread safe

#if defined(ALSA) || defined(OSS)
  typedef void (*T_FPI_SoundOutput_FillBuffer) (void *ptr, char *buffer, int n_bytes);	// This function is thread safe
#endif				// defined(ALSA) || defined(OSS)

  struct FPI_Functions
  {
    int fpi_count;
    void *fpi_mem_alloc;	// 1
    void *fpi_mem_free;		// 2
    void *fpi_soundoutput_fillbuffer;	// 3
  };

//
// Exported functions
//

  void *FPX_Init (void *ptr);

  static void FPX_Shutdown (void);

#if defined(OPENSSL) || defined(GNUTLS)
  static void *FPX_SSLSocket_Create (int socket_fd);
  static int FPX_SSLSocket_Destroy (void *ptr);
  static int FPX_SSLSocket_Connect (void *ptr);
  static int FPX_SSLSocket_Receive (void *ptr, char *buffer, int n_bytes);
  static int FPX_SSLSocket_Send (void *ptr, const char *buffer, int n_bytes);
#endif				// defined(OPENSSL) || defined(GNUTLS)

#if defined(ALSA) || defined(OSS)
  static void *FPX_SoundOutput_Open (void);
  static int FPX_SoundOutput_Close (void *ptr);
  static int FPX_SoundOutput_Latency (void *ptr);
#endif				// defined(ALSA) || defined(OSS)

#ifdef V4L1
  static void *FPX_VideoInput_Open ();
  static int FPX_VideoInput_Close (void *ptr);
  static int FPX_VideoInput_GetFrame (void *ptr, char *data, int width,
				      int height, int pitch_n_bytes);
#endif				// V4L1

  struct FPX_Functions
  {
    int fpx_count;
    void *fpx_shutdown;		// 1
    void *fpx_sslsocket_create;	// 2
    void *fpx_sslsocket_destroy;	// 3
    void *fpx_sslsocket_connect;	// 4
    void *fpx_sslsocket_receive;	// 5
    void *fpx_sslsocket_send;	// 6
    void *fpx_soundoutput_open;	// 7
    void *fpx_soundoutput_close;	// 8
    void *fpx_soundoutput_latency;	// 9
    void *fpx_videoinput_open;	// 10
    void *fpx_videoinput_close;	// 11
    void *fpx_videoinput_getframe;	// 12
  };

#ifdef cplusplus
};
#endif // cplusplus
//
// END OF SHARED SECTION
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <memory.h>

#ifdef OPENSSL
#include <openssl/ssl.h>
#elif defined(GNUTLS)
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <gnutls/gnutls.h>
#endif // GNUTLS

#ifdef ALSA
#include <pthread.h>
#include <semaphore.h>
#include <alsa/asoundlib.h>
#elif defined(OSS)
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#endif // OSS

#ifdef V4L1
#include <unistd.h>
#include <pthread.h>
#include <linux/videodev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#endif // V4L1


static struct FPX_Functions fpx_functions;

static T_FPI_Mem_Alloc FPI_Mem_Alloc = 0;
static T_FPI_Mem_Free FPI_Mem_Free = 0;

#if defined(ALSA) || defined(OSS)
static T_FPI_SoundOutput_FillBuffer FPI_SoundOutput_FillBuffer = 0;
#endif // defined(ALSA) || defined(OSS)

void *
FPX_Init (void *ptr)
{
  if (!ptr)
    return 0;

  //
  // Setup imported functions
  //

  struct FPI_Functions *fpi_functions = (struct FPI_Functions *) ptr;

  if (fpi_functions->fpi_count >= 1)
    FPI_Mem_Alloc = fpi_functions->fpi_mem_alloc;	// 1
  if (fpi_functions->fpi_count >= 2)
    FPI_Mem_Free = fpi_functions->fpi_mem_free;	// 2

#if defined(ALSA) || defined(OSS)
  if (fpi_functions->fpi_count >= 3)
    FPI_SoundOutput_FillBuffer = fpi_functions->fpi_soundoutput_fillbuffer;	// 3
#endif // defined(ALSA) || defined(OSS)

  //
  // Setup exported functions
  //

  memset (&fpx_functions, 0, sizeof (fpx_functions));

  fpx_functions.fpx_shutdown = FPX_Shutdown;	// 1

#if defined(OPENSSL) || defined(GNUTLS)
  fpx_functions.fpx_sslsocket_create = FPX_SSLSocket_Create;	// 2
  fpx_functions.fpx_sslsocket_destroy = FPX_SSLSocket_Destroy;	// 3
  fpx_functions.fpx_sslsocket_connect = FPX_SSLSocket_Connect;	// 4
  fpx_functions.fpx_sslsocket_receive = FPX_SSLSocket_Receive;	// 5
  fpx_functions.fpx_sslsocket_send = FPX_SSLSocket_Send;	// 6
#endif // defined(OPENSSL) || defined(GNUTLS)

#if defined(ALSA) || defined(OSS)
  fpx_functions.fpx_soundoutput_open = FPX_SoundOutput_Open;	// 7
  fpx_functions.fpx_soundoutput_close = FPX_SoundOutput_Close;	// 8
  fpx_functions.fpx_soundoutput_latency = FPX_SoundOutput_Latency;	// 9
#endif // defined(ALSA) || defined(OSS)

#ifdef V4L1
  fpx_functions.fpx_videoinput_open = FPX_VideoInput_Open;	// 10
  fpx_functions.fpx_videoinput_close = FPX_VideoInput_Close;	// 11
  fpx_functions.fpx_videoinput_getframe = FPX_VideoInput_GetFrame;	// 12
#endif // V4L1

  fpx_functions.fpx_count = 14;

#ifdef OPENSSL
  SSL_library_init ();
#elif defined(GNUTLS)
  gnutls_global_init ();
#endif // GNUTLS

  return (void *) &fpx_functions;
}

static void
FPX_Shutdown ()
{
#ifdef OPENSSL

#elif defined(GNUTLS)
  gnutls_global_deinit ();
#endif // GNUTLS
}

//
// SSL support functions
//

#ifdef OPENSSL
struct SSL_Instance
{
  SSL *ssl;
  SSL_CTX *sslCtx;
};

static void *
FPX_SSLSocket_Create (int socket_fd)
	// return = instance pointer
{
  struct SSL_Instance *instance =
    (struct SSL_Instance *) FPI_Mem_Alloc (sizeof (struct SSL_Instance));
  memset (instance, 0, sizeof (struct SSL_Instance));

  if ((instance->sslCtx = SSL_CTX_new (TLSv1_client_method ())) == 0)
    goto fail;

  if ((instance->ssl = SSL_new (instance->sslCtx)) == 0)
    goto fail;

  if (SSL_set_fd (instance->ssl, socket_fd) < 0)
    goto fail;

  return (void *) instance;
fail:
  if (instance->ssl)
    {
      SSL_shutdown (instance->ssl);
    }

  if (instance->sslCtx)
    {
      SSL_CTX_free (instance->sslCtx);
    }

  if (FPI_Mem_Free)
    FPI_Mem_Free (instance);

  return 0;
}

static int
FPX_SSLSocket_Destroy (void *ptr)
	// ptr = instance pointer
	// return = 0 on sucess, < 0 on error
{
  struct SSL_Instance *instance = (struct SSL_Instance *) ptr;

  if (instance->ssl)
    {
      SSL_shutdown (instance->ssl);
    }

  if (instance->sslCtx)
    {
      SSL_CTX_free (instance->sslCtx);
    }

  if (FPI_Mem_Free)
    FPI_Mem_Free (instance);

  return 0;
}

static int
FPX_SSLSocket_Connect (void *ptr)
	// ptr = instance pointer
	// socket_fd = BSD socket fd to be associated with SSL connection
	// return = 0 on sucess, < 0 on error
	//
	// Flash Player will use errno to obtain current status
{
  struct SSL_Instance *instance = (struct SSL_Instance *) ptr;
  return SSL_connect (instance->ssl);
}

static int
FPX_SSLSocket_Receive (void *ptr, char *buffer, int n_bytes)
	// ptr = instance pointer
	// buffer = raw buffer to place received data into
	// n_bytes = length of buffer in bytes
	// return = actual bytes received, < 0 on error
	//
	// Flash Player will use errno to obtain current status
{
  struct SSL_Instance *instance = (struct SSL_Instance *) ptr;
  return SSL_read (instance->ssl, buffer, n_bytes);
}

static int
FPX_SSLSocket_Send (void *ptr, const char *buffer, int n_bytes)
	// ptr = instance pointer
	// buffer = raw buffer to be sent
	// n_bytes = length of input buffer in bytes
	// return = actual bytes sent, < 0 on error
	//
	// Flash Player will use errno to obtain current status
{
  struct SSL_Instance *instance = (struct SSL_Instance *) ptr;
  return SSL_write (instance->ssl, buffer, n_bytes);
}
#elif defined(GNUTLS)
struct SSL_Instance
{
  gnutls_session_t session;
  gnutls_anon_client_credentials_t anoncred;
};

static void *
FPX_SSLSocket_Create (int socket_fd)
	// return = instance pointer
{
  const int kx_prio[] = { GNUTLS_KX_ANON_DH, 0 };

  struct SSL_Instance *instance =
    (struct SSL_Instance *) FPI_Mem_Alloc (sizeof (struct SSL_Instance));
  memset (instance, 0, sizeof (struct SSL_Instance));

  if (gnutls_anon_allocate_client_credentials (&instance->anoncred) < 0)
    goto fail;

  if (gnutls_init (&instance->session, GNUTLS_CLIENT) < 0)
    goto fail;

  if (gnutls_set_default_priority (instance->session) < 0)
    goto fail;

  if (gnutls_kx_set_priority (instance->session, kx_prio) < 0)
    goto fail;

  if (gnutls_credentials_set
      (instance->session, GNUTLS_CRD_ANON, instance->anoncred) < 0)
    goto fail;

  gnutls_transport_set_ptr (instance->session,
			    (gnutls_transport_ptr_t) socket_fd);

  return (void *) instance;
fail:

  if (instance->session)
    {
      gnutls_deinit (instance->session);
    }

  if (instance->anoncred)
    {
      gnutls_anon_free_client_credentials (instance->anoncred);
    }

  if (FPI_Mem_Free)
    FPI_Mem_Free (instance);

  return 0;
}

static int
FPX_SSLSocket_Destroy (void *ptr)
	// ptr = instance pointer
	// return = 0 on sucess, < 0 on error
{
  struct SSL_Instance *instance =
    (struct SSL_Instance *) FPI_Mem_Alloc (sizeof (struct SSL_Instance));

  gnutls_bye (instance->session, GNUTLS_SHUT_RDWR);

  gnutls_deinit (instance->session);

  gnutls_anon_free_client_credentials (instance->anoncred);

  if (FPI_Mem_Free)
    FPI_Mem_Free (instance);

  return 0;
}

static int
FPX_SSLSocket_Connect (void *ptr)
	// ptr = instance pointer
	// socket_fd = BSD socket fd to be associated with SSL connection
	// return = 0 on sucess, < 0 on error
	//
	// Flash Player will use errno to obtain current status
{
  struct SSL_Instance *instance = (struct SSL_Instance *) ptr;
  return gnutls_handshake (instance->session);
}

static int
FPX_SSLSocket_Receive (void *ptr, char *buffer, int n_bytes)
	// ptr = instance pointer
	// buffer = raw buffer to place received data into
	// n_bytes = length of buffer in bytes
	// return = actual bytes received, < 0 on error
	//
	// Flash Player will use errno to obtain current status
{
  struct SSL_Instance *instance = (struct SSL_Instance *) ptr;
  return gnutls_record_recv (instance->session, buffer, n_bytes);
}

static int
FPX_SSLSocket_Send (void *ptr, const char *buffer, int n_bytes)
	// ptr = instance pointer
	// buffer = raw buffer to be sent
	// n_bytes = length of input buffer in bytes
	// return = actual bytes sent, < 0 on error
	//
	// Flash Player will use errno to obtain current status
{
  struct SSL_Instance *instance = (struct SSL_Instance *) ptr;
  return gnutls_record_send (instance->session, buffer, n_bytes);
}
#endif // GNUTLS

//
// Sound support functions
//
#ifdef ALSA
struct SoundOutput_Instance
{
  snd_pcm_t *handle;
  snd_async_handler_t *async_handler;
  sem_t semaphore;
  pthread_t thread;
  char *buffer;
  snd_pcm_sframes_t buffer_size;
  snd_pcm_sframes_t period_size;
  snd_pcm_sframes_t buffer_pos;
  char *buffer_ptr;
};

static void *
alsa_thread (void *ptr)
{
  struct SoundOutput_Instance *instance = (struct SoundOutput_Instance *) ptr;
  snd_pcm_sframes_t avail = 0;
  int result = 0;
  int err = 0;
  int state = 0;

  for (;;)
    {

      err = sem_wait (&instance->semaphore);
      if (!instance->handle)
	{
	  pthread_exit (0);
	  return 0;
	}

      if (err < 0)
	{
	  usleep (1);
	  continue;
	}

      do
	{
	  if (instance->buffer_pos <= 0)
	    {
	      FPI_SoundOutput_FillBuffer (ptr, instance->buffer,
					  snd_pcm_frames_to_bytes (instance->
								   handle,
								   instance->
								   period_size));
	      instance->buffer_pos = instance->period_size;
	      instance->buffer_ptr = instance->buffer;
	    }
	  do
	    {
	      state = snd_pcm_state (instance->handle);
	      if (state != SND_PCM_STATE_RUNNING
		  && state != SND_PCM_STATE_PREPARED)
		{
		  snd_pcm_prepare (instance->handle);
		}
	      result =
		snd_pcm_writei (instance->handle, instance->buffer_ptr,
				instance->buffer_pos);
	      if (result <= 0)
		{
		  switch (result)
		    {
		    case -EPIPE:
		      {
			snd_pcm_prepare (instance->handle);
		      }
		      break;
		    case -ESTRPIPE:
		      {
			err = snd_pcm_resume (instance->handle);
			if (err < 0)
			  {
			    snd_pcm_prepare (instance->handle);
			  }
		      }
		      break;
		    }
		  break;
		}
	      else
		{
		  instance->buffer_pos -= result;
		  instance->buffer_ptr +=
		    snd_pcm_frames_to_bytes (instance->handle, result);
		}
	    }
	  while (instance->buffer_pos);
	  avail = snd_pcm_avail_update (instance->handle);
	  if (avail < 0)
	    {
	      switch (avail)
		{
		case -EPIPE:
		  {
		    snd_pcm_prepare (instance->handle);
		  }
		  break;
		case -ESTRPIPE:
		  {
		    err = snd_pcm_resume (instance->handle);
		    if (err < 0)
		      {
			snd_pcm_prepare (instance->handle);
		      }
		  }
		  break;
		}
	      break;
	    }

	}
      while (avail >= instance->period_size);
    }
}

static void
alsa_callback (snd_async_handler_t * ahandler)
{
  struct SoundOutput_Instance *instance =
    (struct SoundOutput_Instance *)
    snd_async_handler_get_callback_private (ahandler);
  // signal mixer thread
  sem_post (&instance->semaphore);
}

static void *
FPX_SoundOutput_Open ()
	// return = instance pointer
{
  struct SoundOutput_Instance *instance = 0;
  snd_pcm_hw_params_t *hwparams = 0;
  snd_pcm_sw_params_t *swparams = 0;
  snd_pcm_format_t pcm_format;
  unsigned int buffer_time = 500000;
  unsigned int period_time = 20000;
  unsigned int actual_rate;
  snd_pcm_uframes_t size;
  int direction;
  void *retVal = 0;
  int err = 0;

  if (!FPI_SoundOutput_FillBuffer)
    goto fail;
  if (!FPI_Mem_Alloc)
    goto fail;

  instance = FPI_Mem_Alloc (sizeof (struct SoundOutput_Instance));
  memset (instance, 0, sizeof (struct SoundOutput_Instance));

  if ((err =
       snd_pcm_open (&instance->handle, "default", SND_PCM_STREAM_PLAYBACK,
		     SND_PCM_NONBLOCK)) < 0)
    {
      if ((err =
	   snd_pcm_open (&instance->handle, "plughw:0,0",
			 SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK)) < 0)
	{
	  goto fail;
	}
    }

  snd_pcm_hw_params_alloca (&hwparams);
  snd_pcm_sw_params_alloca (&swparams);

  if (snd_pcm_hw_params_any (instance->handle, hwparams) < 0)
    goto fail;

  if (snd_pcm_hw_params_set_access
      (instance->handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
    goto fail;

  pcm_format = SND_PCM_FORMAT_S16_LE;

  if (snd_pcm_hw_params_set_format (instance->handle, hwparams, pcm_format) <
      0)
    goto fail;

  if (snd_pcm_hw_params_set_channels (instance->handle, hwparams, 2) < 0)
    goto fail;

  actual_rate = 44100;

  if (snd_pcm_hw_params_set_rate_near
      (instance->handle, hwparams, &actual_rate, 0) < 0)
    goto fail;

  if (actual_rate != 44100)
    goto fail;

  if (snd_pcm_hw_params_set_buffer_time_near
      (instance->handle, hwparams, &buffer_time, &direction) < 0)
    goto fail;

  if (snd_pcm_hw_params_get_buffer_size (hwparams, &size) < 0)
    goto fail;

  instance->buffer_size = (snd_pcm_sframes_t) size;

  if (snd_pcm_hw_params_set_period_time_near
      (instance->handle, hwparams, &period_time, &direction) < 0)
    goto fail;

  if (snd_pcm_hw_params_get_period_size (hwparams, &size, &direction) < 0)
    goto fail;

  instance->period_size = (snd_pcm_sframes_t) size;

  if (snd_pcm_hw_params (instance->handle, hwparams) < 0)
    goto fail;

  if (snd_pcm_sw_params_current (instance->handle, swparams) < 0)
    goto fail;

  if (snd_pcm_sw_params_set_start_threshold
      (instance->handle, swparams,
       ((instance->buffer_size -
	 1) / instance->period_size) * instance->period_size) < 0)
    goto fail;

  if (snd_pcm_sw_params_set_stop_threshold (instance->handle, swparams, ~0U) <
      0)
    goto fail;

  if (snd_pcm_sw_params_set_avail_min
      (instance->handle, swparams, instance->period_size) < 0)
    goto fail;

  if (snd_pcm_sw_params_set_xfer_align (instance->handle, swparams, 1) < 0)
    goto fail;

  if (snd_pcm_sw_params (instance->handle, swparams) < 0)
    goto fail;

  if (snd_async_add_pcm_handler
      (&instance->async_handler, instance->handle, &alsa_callback,
       instance) < 0)
    goto fail;

  if ((instance->buffer =
       FPI_Mem_Alloc (instance->buffer_size * 2 * 2 * 2)) == 0)
    goto fail;

  if (pthread_create (&instance->thread, 0, alsa_thread, instance) < 0)
    goto fail;

  sem_post (&instance->semaphore);

  return instance;

fail:
  if (instance)
    {
      if (instance->handle)
	{
	  snd_pcm_drop (instance->handle);
	  snd_pcm_close (instance->handle);
	  instance->handle = 0;
	}
      if (instance->thread)
	{
	  sem_post (&instance->semaphore);
	  sem_destroy (&instance->semaphore);
	  pthread_join (instance->thread, &retVal);
	}
      if (instance->buffer)
	{
	  if (FPI_Mem_Free)
	    FPI_Mem_Free (instance->buffer);
	}
      if (FPI_Mem_Free)
	FPI_Mem_Free (instance);
    }
  return 0;
}

static int
FPX_SoundOutput_Close (void *ptr)
	// ptr = instance pointer
	// return = 0 on success, < 0 on error
{
  struct SoundOutput_Instance *instance = (struct SoundOutput_Instance *) ptr;
  void *retVal = 0;
  if (instance->handle)
    {
      snd_pcm_drop (instance->handle);
      snd_pcm_close (instance->handle);
      instance->handle = 0;
    }
  if (instance->thread)
    {
      sem_post (&instance->semaphore);
      sem_destroy (&instance->semaphore);
      pthread_join (instance->thread, &retVal);
    }
  if (instance->buffer)
    {
      if (FPI_Mem_Free)
	FPI_Mem_Free (instance->buffer);
    }
  if (FPI_Mem_Free)
    FPI_Mem_Free (instance);
  return 0;
}

static int
FPX_SoundOutput_Latency (void *ptr)
	// ptr = instance pointer
	// return = 0 on success, < 0 on error
{
  struct SoundOutput_Instance *instance = (struct SoundOutput_Instance *) ptr;
  if (instance->handle)
    {
      snd_pcm_sframes_t delay = 0;
      snd_pcm_delay (instance->handle, &delay);
      if (snd_pcm_state (instance->handle) == SND_PCM_STATE_RUNNING
	  && delay > 0)
	{
	  return delay;
	}
      else
	{
	  return 0;
	}
    }
  return -1;
}
#elif defined(OSS)
struct SoundOutput_Instance
{
  int oss_fd;
  int signal;
  int blen;
  pthread_t thread;
  char *buffer;
};

static void *
oss_thread (void *ptr)
{
  struct SoundOutput_Instance *instance = (struct SoundOutput_Instance *) ptr;
  int len = 0;
  int written = 0;
  for (;;)
    {
      FPI_SoundOutput_FillBuffer (ptr, instance->buffer, instance->blen);
      len = instance->blen;
      while (len)
	{
	  written = write (instance->oss_fd, instance->buffer + instance->blen - len, len);
	  if (written >= 0)
	    {
	      len -= written;
	    }
	  if (instance->signal)
	    {
	      pthread_exit (0);
	    }
	  if (written < 0)
	    {
	      usleep (100);
	    }
	}
    }
}

static int
open_dsp (struct SoundOutput_Instance * instance)
{
  size_t len;
  char s[1024] = "", *devdsp = "/dev/dsp", *home;
  FILE *fp = NULL;

  home = getenv ("HOME");
  if (home == NULL) goto dexit;
  len = snprintf (s, sizeof (s), "%s/.flashsupport", home);
  if ((len < 0) || (len >= sizeof (s))) goto dexit;
  fp = fopen (s, "r");
  if (fp == NULL) goto dexit;
  do {
    if (fgets (s, sizeof(s), fp) == NULL) goto dexit;
  } while (s[0] == '#');
  len = strlen (s);
  if (len <= 7) goto dexit;
  if (s[len-1] == '\n') s[len-1] = '\0';
  if (strncmp ("device=", s, 7) || (s[7] == '\0')) goto dexit;
  devdsp = (char *)s + 7;

dexit:
  if (fp != NULL) fclose (fp);
  if ((instance->oss_fd = open (devdsp, O_WRONLY)) < 0) return -1;
  return 0;
}

static void *
FPX_SoundOutput_Open ()
	// return = instance pointer
{
  struct SoundOutput_Instance *instance = 0;
  int format = AFMT_S16_LE;
  int channels = 2;
  int speed = 44100;
  int frags = 0x0004000c;	/* 4 fragments of 4k -> 16k total */

  if (!FPI_SoundOutput_FillBuffer)
    goto fail;
  if (!FPI_Mem_Alloc)
    goto fail;

  instance =
    (struct SoundOutput_Instance *)
    FPI_Mem_Alloc (sizeof (struct SoundOutput_Instance));
  memset (instance, 0, sizeof (struct SoundOutput_Instance));

  if (open_dsp (instance))
    goto fail;

  if (ioctl (instance->oss_fd, SNDCTL_DSP_SETFRAGMENT, &frags) < 0)
    goto fail;

  if ((ioctl (instance->oss_fd, SNDCTL_DSP_SETFMT, &format) < 0) ||
      (format != AFMT_S16_LE))
    goto fail;

  if ((ioctl (instance->oss_fd, SNDCTL_DSP_CHANNELS, &channels) < 0) ||
      (channels != 2))
    goto fail;

  if (ioctl (instance->oss_fd, SNDCTL_DSP_SPEED, &speed) < 0)
    goto fail;

  instance->blen = 4096;
  instance->buffer = (char *)FPI_Mem_Alloc (instance->blen);
  memset (instance->buffer, 0, instance->blen);
  if (!instance->buffer) goto fail;

  if (pthread_create (&instance->thread, 0, oss_thread, instance) < 0)
    goto fail;

  return instance;
fail:
  if (instance)
    {
      if (FPI_Mem_Free)
	{
	  if (instance->buffer) FPI_Mem_Free (instance->buffer);
	  FPI_Mem_Free (instance);
	}
    }
  return 0;
}

static int
FPX_SoundOutput_Close (void *ptr)
	// ptr = instance pointer
	// return = 0 on success, < 0 on error
{
  struct SoundOutput_Instance *instance = (struct SoundOutput_Instance *) ptr;
  void *retVal = 0;

  instance->signal = 1;

#if 0
  if (instance->oss_fd)
    {
      ioctl (instance->oss_fd, SNDCTL_DSP_RESET, 0);
    }
#endif

  if (instance->thread)
    {
      pthread_join (instance->thread, &retVal);
    }

  if (instance->oss_fd)
    {
      close (instance->oss_fd);
    }

  if (FPI_Mem_Free)
    FPI_Mem_Free (instance);

  return 0;
}

static int
FPX_SoundOutput_Latency (void *ptr)
	// ptr = instance pointer
	// return = 0 on success, < 0 on error
{
  struct SoundOutput_Instance *instance = (struct SoundOutput_Instance *) ptr;
  if (instance->oss_fd)
    {
      int value = 0;
      if (ioctl (instance->oss_fd, SNDCTL_DSP_GETODELAY, &value) == -1)
	{
	  return 0;
	}
      return value / 4;
    }
  return -1;
}
#endif // defined(OSS)

#ifdef V4L1
struct VideoOutput_Instance
{
  int v4l_fd;
  pthread_t thread;
  int signal;
  char *buffer[2];
  int buffercurrent;
  int buffersize;
  struct video_window window;
  struct video_picture picture;
};

static void *
v4l_thread (void *ptr)
{
  struct VideoOutput_Instance *instance = (struct VideoOutput_Instance *) ptr;
  int result;
  int status;

  for (;;)
    {

      result =
	read (instance->v4l_fd, instance->buffer[instance->buffercurrent],
	      instance->buffersize);
      if (result > 0)
	{
	}

      if (result < 0)
	{
	  usleep (10000);
	}

      if (instance->signal)
	{
	  status = 0;
	  ioctl (instance->v4l_fd, VIDIOCCAPTURE, &status);
	  pthread_exit (0);
	}
    }
}

static void *
FPX_VideoInput_Open ()
{
  struct VideoOutput_Instance *instance = 0;

  if (!FPI_Mem_Alloc)
    goto fail;

  instance =
    (struct VideoOutput_Instance *)
    FPI_Mem_Alloc (sizeof (struct VideoOutput_Instance));
  memset (instance, 0, sizeof (struct VideoOutput_Instance));

  if ((instance->v4l_fd = open ("/dev/video", O_RDONLY)) < 0)
    goto fail;

  if (ioctl (instance->v4l_fd, VIDIOCGPICT, &instance->picture) < 0)
    goto fail;

  switch (instance->picture.palette)
    {
    case VIDEO_PALETTE_YUV420P:
      break;
    case VIDEO_PALETTE_RGB24:
    case VIDEO_PALETTE_YUV422P:
    case VIDEO_PALETTE_YUV411P:
    case VIDEO_PALETTE_YUV410P:
    case VIDEO_PALETTE_GREY:
    case VIDEO_PALETTE_HI240:
    case VIDEO_PALETTE_RGB565:
    case VIDEO_PALETTE_RGB32:
    case VIDEO_PALETTE_RGB555:
    case VIDEO_PALETTE_YUV422:
    case VIDEO_PALETTE_YUYV:
    case VIDEO_PALETTE_UYVY:
    case VIDEO_PALETTE_YUV420:
    case VIDEO_PALETTE_YUV411:
    case VIDEO_PALETTE_RAW:
    default:
      goto fail;
    }

  if (ioctl (instance->v4l_fd, VIDIOCGWIN, &instance->window) < 0)
    goto fail;

  instance->buffer[0] =
    FPI_Mem_Alloc (instance->window.width * instance->window.height * 2);
  instance->buffer[1] =
    FPI_Mem_Alloc (instance->window.width * instance->window.height * 2);

  if (pthread_create (&instance->thread, 0, v4l_thread, instance) < 0)
    goto fail;

  return instance;

fail:
  if (FPI_Mem_Free)
    {
      if (instance->buffer[0])
	{
	  FPI_Mem_Free (instance->buffer[0]);
	}
      if (instance->buffer[1])
	{
	  FPI_Mem_Free (instance->buffer[1]);
	}
      FPI_Mem_Free (instance);
    }
  return 0;
}

static int
FPX_VideoInput_Close (void *ptr)
{
  struct VideoOutput_Instance *instance = (struct VideoOutput_Instance *) ptr;
  void *retVal = 0;

  instance->signal = 1;

  if (instance->thread)
    {
      pthread_join (instance->thread, &retVal);
    }

  if (instance->v4l_fd)
    {
      close (instance->v4l_fd);
    }

  if (FPI_Mem_Free)
    {
      if (instance->buffer[0])
	{
	  FPI_Mem_Free (instance->buffer[0]);
	}
      if (instance->buffer[1])
	{
	  FPI_Mem_Free (instance->buffer[1]);
	}
      FPI_Mem_Free (instance);
    }

  return 0;
}

static int
FPX_VideoInput_GetFrame (void *ptr, char *data, int width, int height,
			 int pitch_n_bytes)
{
  struct VideoOutput_Instance *instance = (struct VideoOutput_Instance *) ptr;
  int ix, iy, ox, oy, ow, oh, dx, dy, Y, U, V, R, G, B;
  unsigned char *y, *u, *v;

  switch (instance->picture.palette)
    {
    case VIDEO_PALETTE_YUV420P:
      {
	ow = instance->window.width;
	oh = instance->window.height;

	dx = (ow << 16) / width;
	dy = (oh << 16) / height;

	y = (unsigned char *) instance->buffer[instance->buffercurrent ^ 1];
	u = y + ow * oh;
	v = u + ow * oh / 4;

	oy = 0;

	for (iy = 0; iy < height; iy++)
	  {

	    ox = 0;

	    for (ix = 0; ix < width; ix++)
	      {

		Y =
		  (149 * ((int) (y[(oy >> 16) * (ow) + (ox >> 16)]) - 16)) /
		  2;
		U = (int) (u[(oy >> 17) * (ow / 2) + (ox >> 17)]) - 128;
		V = (int) (v[(oy >> 17) * (ow / 2) + (ox >> 17)]) - 128;

		R = (Y + V * 102) / 64;
		G = (Y - V * 52 - U * 25) / 64;
		B = (Y + U * 129) / 64;

		R = R < 0 ? 0 : (R > 255 ? 255 : R);
		G = G < 0 ? 0 : (G > 255 ? 255 : G);
		B = B < 0 ? 0 : (B > 255 ? 255 : B);

		data[ix * 3 + 0] = R;
		data[ix * 3 + 1] = G;
		data[ix * 3 + 2] = B;

		ox += dx;
	      }

	    oy += dy;

	    data += pitch_n_bytes;
	  }
      } break;
    default:
      goto fail;
    }

  instance->buffercurrent ^= 1;

  return 0;

fail:
  return -1;
}
#endif // V4L1
