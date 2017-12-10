/*
 * Purpose: MIDI message queue management
 */
/*
 *
 * This file is part of Open Sound System.
 *
 * Copyright (C) 4Front Technologies 1996-2008.
 *
 * This this source file is released under GPL v2 license (no other versions).
 * See the COPYING file included in the main directory of this source
 * distribution for the license terms and conditions.
 *
 */

#include "oss_config.h"
#include "midi_core.h"
#define MDB(x)

typedef struct
{
  int len;
  midi_packet_header_t hdr;
  int next;
  int size;

  unsigned char *data;
  unsigned char buf[1];
  /* Do _NOT_ add any fields after data[] */
  /* Do _NOT_ add any fields after data[] */
  /* Do _NOT_ add any fields after data[] */
  /* Do _NOT_ add any fields after data[] */
  /* Do _NOT_ add any fields after data[] */
} midi_buf_t;

#define BUF_PREFIX_SIZE (sizeof(midi_buf_t)-1)
#define MAX_QUEUE_DEPTH		31
#define QUEUE_BYTES		((MAX_QUEUE_DEPTH+1)*(BUF_PREFIX_SIZE+MIDI_PAYLOAD_SIZE))

struct midi_queue_t
{
  oss_mutex_t mutex;
  char name[16];		/* For debugging purposes */

/*
 * Memeory management variables.
 */
  unsigned char buffer[QUEUE_BYTES];
  int buf_head, buf_tail;
  int debugging;
  int q_head, q_tail;
  int avail;
  int writecount, readcount;

/*
 * Message queue
 */

  midi_buf_t *buffers[MAX_QUEUE_DEPTH];
  int dummy;
};

midi_queue_t *
midi_queue_alloc (oss_device_t * osdev, const char *name)
{
  midi_queue_t *q;

  if ((q = KERNEL_MALLOC (sizeof (*q))) == NULL)
    return NULL;

  memset (q, 0, sizeof (*q));

  MUTEX_INIT (osdev, q->mutex, MH_FRAMEW + 2);
  if (name != NULL)
    strcpy (q->name, name);

  q->avail = QUEUE_BYTES;
  return q;
}

void
midi_queue_free (midi_queue_t * queue)
{
  if (queue != NULL)
    {
      MUTEX_CLEANUP (queue->mutex);
      KERNEL_FREE (queue);
    }
}

static int
queue_concat (midi_queue_t * queue, unsigned char *data, int len,
	      midi_packet_header_t * hdr)
{
/*
 * Check if the MIDI event can be appended to the previous event (to improve
 * buffering performance).
 *
 * This is possible if:
 *
 * 1) The queue is not empty.
 * 2) The previous event has the same time stamp.
 * 3) The previous event is not too close to the end of the allocated buffer
 * area.
 */

  midi_buf_t *buf;
  int p, i;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (queue->mutex, flags);
  if (queue->buf_tail == queue->buf_head)	/* Nothing in the queue */
    {
      MUTEX_EXIT_IRQRESTORE (queue->mutex, flags);
      return 0;
    }

  if (queue->avail < len)
    {
      MUTEX_EXIT_IRQRESTORE (queue->mutex, flags);
      return 0;
    }

  if (queue->q_tail == queue->q_head)	/* No events in the queue */
    {
      MUTEX_EXIT_IRQRESTORE (queue->mutex, flags);
      return 0;
    }

  p = queue->q_head - 1;
  if (p < 0)
    p = MAX_QUEUE_DEPTH - 1;

  buf = queue->buffers[p];

  if (hdr == NULL || buf->hdr.time != hdr->time)
    {
      MUTEX_EXIT_IRQRESTORE (queue->mutex, flags);
      return 0;
    }

  if (buf->len + len > MIDI_PAYLOAD_SIZE)	/* Event would become too long */
    {
      MUTEX_EXIT_IRQRESTORE (queue->mutex, flags);
      return 0;
    }

  if (buf->next + len >= QUEUE_BYTES - 1)	/* No space to grow */
    {
      MUTEX_EXIT_IRQRESTORE (queue->mutex, flags);
      return 0;
    }

  /* Check that we are not too close to the buffer tail */
  if (buf->next < queue->buf_tail && buf->next + len >= queue->buf_tail)
    {
      MUTEX_EXIT_IRQRESTORE (queue->mutex, flags);
      return 0;
    }

/*
 * Ok. All checks passed. Now append the data.
 */

  queue->avail -= len;
  queue->writecount += len;
  buf->next += len;
  buf->size += len;
  for (i = 0; i < len; i++)
    buf->data[buf->len + i] = data[i];
  buf->len += len;
  queue->buf_head = buf->next;

  MUTEX_EXIT_IRQRESTORE (queue->mutex, flags);
  return 1;
}

int
midi_queue_alloc_record (midi_queue_t * queue, unsigned char **data, int len,
			 midi_packet_header_t * hdr)
{
  int p;
  int avail = 0, n, next;
  void *ptr;
  midi_buf_t *buf;
  oss_native_word flags;

  if (queue == NULL)
    {
      cmn_err (CE_WARN, "midi_queue_alloc_record: Queue==NULL\n");
      return OSS_EIO;
    }

  if (len < 1 && hdr == NULL)	/* Nothing was given */
    {
      cmn_err (CE_WARN, "midi_queue_alloc_record: No data\n");
      return OSS_EIO;
    }

  if (len > MIDI_PAYLOAD_SIZE)
    {
      cmn_err (CE_WARN, "Too long MIDI block\n");
      len = MIDI_PAYLOAD_SIZE;
    }

  MUTEX_ENTER_IRQDISABLE (queue->mutex, flags);

  next = (queue->q_head + 1) % MAX_QUEUE_DEPTH;

  if (next == queue->q_tail)
    {
      MUTEX_EXIT_IRQRESTORE (queue->mutex, flags);
      return OSS_ENOSPC;
    }

  if (queue->avail < BUF_PREFIX_SIZE + len)
    {
      MUTEX_EXIT_IRQRESTORE (queue->mutex, flags);
      return OSS_ENOSPC;
    }

  n = queue->q_head - queue->q_tail;
  if (n <= 0)
    n += MAX_QUEUE_DEPTH;

  if (n < 1)
    {
      MUTEX_EXIT_IRQRESTORE (queue->mutex, flags);
      return OSS_ENOSPC;
    }

  if (queue->buf_tail > queue->buf_head)
    {
      avail = queue->buf_tail - queue->buf_head;

      if (avail < BUF_PREFIX_SIZE + len)
	{
	  queue->buf_head = 0;
	}
    }

  if (queue->buf_tail == queue->buf_head)
    {
      avail = QUEUE_BYTES;
      queue->buf_tail = queue->buf_head = 0;
      queue->avail = QUEUE_BYTES;
    }
  else if (queue->buf_tail < queue->buf_head)
    avail = QUEUE_BYTES - queue->buf_head;

  if (avail < BUF_PREFIX_SIZE + len)
    {
#if 1
      len = avail - BUF_PREFIX_SIZE;

      if (len <= 0)
#endif
	{
	  MUTEX_EXIT_IRQRESTORE (queue->mutex, flags);
	  return OSS_ENOSPC;
	}
    }

  if (queue->buf_head >= QUEUE_BYTES)
    {
      cmn_err (CE_CONT, "MIDI queue damaged (%d/%d/alloc)\n", queue->buf_head,
	       QUEUE_BYTES);
      MUTEX_EXIT_IRQRESTORE (queue->mutex, flags);
      return OSS_EIO;
    }

  if (queue->buf_head + BUF_PREFIX_SIZE + len > QUEUE_BYTES)
    {
      cmn_err (CE_CONT, "Potential MIDI queue overflow detected\n");
      cmn_err (CE_CONT, "Head=%d (%d)\n", queue->buf_head, QUEUE_BYTES);
      cmn_err (CE_CONT, "Tail=%d (%d)\n", queue->buf_tail, QUEUE_BYTES);
      cmn_err (CE_CONT, "Avail=%d\n", avail);
      cmn_err (CE_CONT, "Required=%d\n", len + BUF_PREFIX_SIZE);
      MUTEX_EXIT_IRQRESTORE (queue->mutex, flags);
      return OSS_EIO;
    }

  ptr = queue->buffer + queue->buf_head;

  p = queue->buf_head;
  queue->buf_head = (queue->buf_head + BUF_PREFIX_SIZE + len) % QUEUE_BYTES;

  buf = ptr;
  buf->size = BUF_PREFIX_SIZE + len;
  queue->avail -= buf->size;
  buf->data = buf->buf;
  buf->next = queue->buf_head;
  if (hdr == NULL)
    memset (&buf->hdr, 0, sizeof (midi_packet_header_t));
  else
    memcpy (&buf->hdr, hdr, sizeof (midi_packet_header_t));
  buf->len = len;
  *data = buf->data;
  queue->writecount += len;
  MDB (cmn_err (CE_CONT, "%s: alloc %d bytes\n", queue->name, len));

  queue->buffers[queue->q_head] = buf;

  queue->q_head = (queue->q_head + 1) % MAX_QUEUE_DEPTH;

  MUTEX_EXIT_IRQRESTORE (queue->mutex, flags);
  return len;
}

int
midi_queue_put (midi_queue_t * queue, unsigned char *data, int len,
		midi_packet_header_t * hdr)
{
  int ret;
  unsigned char *targetbuf;

  if (queue == NULL)
    {
      return OSS_EIO;
    }
#if 0
  {
    char tmp[1024];
    memcpy (tmp, data, len);
    tmp[len] = 0;
    cmn_err (CE_CONT, "Q Put='%s', %d\n", tmp, len);
  }
#endif

  if (len < 1)
    return 0;

  if (len > MIDI_PAYLOAD_SIZE)
    return OSS_E2BIG;

#if 1
  if (len == 1)
    if (queue_concat (queue, data, len, hdr))
      return len;
#endif

  queue->debugging = 1;
  if ((ret = midi_queue_alloc_record (queue, &targetbuf, len, hdr)) <= 0)
    return ret;
  queue->debugging = 0;

  if (ret < len)		/* All data didn't fit */
    len = ret;

  memcpy (targetbuf, data, len);

  if (queue->buf_head >= QUEUE_BYTES || queue->buf_tail >= QUEUE_BYTES)
    {
      cmn_err (CE_CONT, "MIDI queue damaged (%d/%d/%d/put)\n",
	       queue->buf_head, queue->buf_tail, QUEUE_BYTES);
      return OSS_EIO;
    }
  return len;
}

int
midi_queue_get (midi_queue_t * queue, unsigned char **data, int max_len,
		midi_packet_header_t ** hdr)
{
  midi_buf_t *buf;
  oss_native_word flags;

  *hdr = NULL;
  *data = NULL;

  if (queue == NULL)
    return OSS_EIO;

  if (queue->buf_head >= QUEUE_BYTES || queue->buf_tail >= QUEUE_BYTES)
    {
      cmn_err (CE_CONT, "MIDI queue damaged (%d/%d/%d/get)\n",
	       queue->buf_head, queue->buf_tail, QUEUE_BYTES);
      return OSS_EIO;
    }

  MUTEX_ENTER_IRQDISABLE (queue->mutex, flags);

  if (queue->q_head == queue->q_tail)
    {
      MUTEX_EXIT_IRQRESTORE (queue->mutex, flags);
      return 0;
    }

  buf = queue->buffers[queue->q_tail];
  queue->q_tail = (queue->q_tail + 1) % MAX_QUEUE_DEPTH;

  queue->buf_tail = buf->next;
  queue->avail += buf->size;
  queue->readcount += buf->size;

  *data = buf->data;
  *hdr = &buf->hdr;

#if 0
  if (queue->buf_tail == queue->buf_head)	/* Buffer empty */
    queue->buf_tail = queue->buf_head = 0;	/* Rewind */

  if (queue->q_tail == queue->q_head)
    queue->q_tail = queue->q_head = 0;
#endif

  MUTEX_EXIT_IRQRESTORE (queue->mutex, flags);
  return buf->len;
}

int
midi_queue_find_buffer (midi_queue_t * queue, unsigned char **data,
			midi_packet_header_t ** hdr)
{
  midi_buf_t *buf;
  oss_native_word flags;

  *hdr = NULL;
  *data = NULL;

  if (queue == NULL)
    return OSS_EIO;

  MUTEX_ENTER_IRQDISABLE (queue->mutex, flags);
  if (queue->q_head == queue->q_tail)
    {
      MUTEX_EXIT_IRQRESTORE (queue->mutex, flags);
      return 0;
    }

  buf = queue->buffers[queue->q_tail];

  *data = buf->data;
  *hdr = &buf->hdr;

  MUTEX_EXIT_IRQRESTORE (queue->mutex, flags);
  return buf->len;
}

void
midi_queue_remove_chars (midi_queue_t * queue, int len)
{
  midi_buf_t *buf;
  oss_native_word flags;

  if (queue == NULL)
    return;

  MUTEX_ENTER_IRQDISABLE (queue->mutex, flags);

  if (queue->q_head == queue->q_tail)
    {
      MUTEX_EXIT_IRQRESTORE (queue->mutex, flags);
      MDB (cmn_err (CE_CONT, "%s: Q Nothing to remove\n", queue->name));
      return;
    }

  buf = queue->buffers[queue->q_tail];

  if (len < buf->len)
    {
      /* Buffer not completely used. Just remove len characters from the beginning */
      /* unsigned char *data = buf->data; */

      MDB (cmn_err
	   (CE_CONT, "%s: Q Remove chars %d (%02x)\n", queue->name, len,
	    buf->data[len]));
      /* memcpy(data, data+len, buf->len-len); */
      buf->data += len;
      buf->len -= len;
      queue->readcount += len;
      MDB (cmn_err (CE_CONT, "%s: Q left %d\n", queue->name, buf->len));
      MUTEX_EXIT_IRQRESTORE (queue->mutex, flags);
      return;
    }

/*
 * Remove the whole buffer
 */
  MDB (cmn_err (CE_CONT, "%s: Q Remove all\n", queue->name));
  queue->q_tail = (queue->q_tail + 1) % MAX_QUEUE_DEPTH;
  queue->buf_tail = buf->next;
  queue->avail += buf->size;
  queue->readcount += len;

  if (queue->buf_tail == queue->buf_head)	/* Buffer empty */
    queue->buf_tail = queue->buf_head = 0;	/* Rewind */

  if (queue->q_tail == queue->q_head)
    queue->q_tail = queue->q_head = 0;
  MUTEX_EXIT_IRQRESTORE (queue->mutex, flags);
}

void
midi_queue_removeall (midi_queue_t * queue)
{
/*
 * Make the queue completely empty
 */
  oss_native_word flags;

  if (queue == NULL)
    return;

  MUTEX_ENTER_IRQDISABLE (queue->mutex, flags);

  queue->buf_tail = queue->buf_head = 0;	/* Rewind */
  queue->q_tail = queue->q_head = 0;
  queue->avail = QUEUE_BYTES;
  queue->readcount = 0;
  queue->writecount = 0;

  MUTEX_EXIT_IRQRESTORE (queue->mutex, flags);
}

int
midi_queue_isempty (midi_queue_t * queue)
{
  int is_empty;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (queue->mutex, flags);
  is_empty = (queue->q_tail == queue->q_head);
  MUTEX_EXIT_IRQRESTORE (queue->mutex, flags);

  return is_empty;
}

int
midi_queue_spaceleft (midi_queue_t * queue)
{
  oss_native_word flags;
  int space = 1;
  int next;

  MUTEX_ENTER_IRQDISABLE (queue->mutex, flags);
  next = (queue->q_head + 1) % MAX_QUEUE_DEPTH;
  if (next == queue->q_tail)
    space = 0;
  else
    {
      if (queue->avail < BUF_PREFIX_SIZE + 1)
	space = 0;
      else
	{
	  space = queue->avail;
	  if (space > MIDI_PAYLOAD_SIZE)
	    space = MIDI_PAYLOAD_SIZE;
	}
    }
  MUTEX_EXIT_IRQRESTORE (queue->mutex, flags);

  return space;
}

void
midi_queue_debugging (midi_queue_t * queue)
{
  queue->debugging = 1;
}

void
midi_queue_trace (midi_queue_t * queue)
{
  MDB (cmn_err
       (CE_CONT, "Write %d, read %d\n", queue->writecount, queue->readcount));
}
