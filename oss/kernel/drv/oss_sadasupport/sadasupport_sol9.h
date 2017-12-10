/*
 * Purpose: Solaris 9 compatible version of sadasupport_open/close
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

static int
sadasupport_open (queue_t * q, dev_t * devp, int flag, int sflag,
		  cred_t * credp)
{
  int retval, i;
  int tmpdev;
  int dev = *devp;
  char *cmd;
  unsigned int bl_flags = 0;
  int oss_mode = 0;
#ifndef SOL9
  extern void oss_forceload_drivers (int dev, cred_t * cred_p);
#endif

  DDB (cmn_err
       (CE_CONT, "sadasupport_open(q=%x, dev=%x, flag=%x, sflag=%x)\n", q,
	dev, flag, sflag));

  cmd = oss_get_procname ();
  DDB (cmn_err (CE_CONT, "Command %s\n", cmd));

  if (devc == NULL)
    return EIO;

  oss_mode = OPEN_READWRITE;

  mutex_enter (&devc->inst_lock);

  if (devc->audio_busy++ == 0)
    {
      mutex_exit (&devc->inst_lock);

      tmp_file.acc_flags = 0;
      tmp_file.mode = oss_mode;

      tmpdev = 0;
      devc->audio_mode = tmp_file.mode;
#if 0
      DDB (cmn_err (CE_CONT, "Opening OSS audio device file %d (mode=%x)\n",
		    devc->masterdev, tmp_file.mode));

      if (devc->masterdev >= 0 && devc->masterdev < num_audio_devfiles)
#endif
	{
	  /* Open /dev/dsp# */
	  tmp_file.mode = oss_mode = OPEN_READ | OPEN_WRITE;
	  if ((retval =
	       oss_audio_open_devfile (0, OSS_DEV_DSP,
				       &tmp_file, 1, OF_DEVAUDIO,
				       &tmpdev)) < 0)
	    {
	      mutex_enter (&devc->inst_lock);
	      devc->audio_busy = 0;
	      mutex_exit (&devc->inst_lock);
	      return -retval;
	    }
	}
#if 0
      else

	{
	  /* Open /dev/dsp */

	  if ((retval =
	       oss_open_vdsp (0, OSS_DEV_VDSP, &tmp_file, 1, OF_DEVAUDIO,
			      &tmpdev)) < 0)
	    {
	      mutex_enter (&devc->inst_lock);
	      devc->audio_busy = 0;
	      mutex_exit (&devc->inst_lock);
	      return -retval;
	    }
	}
#endif
      DDB (cmn_err
	   (CE_CONT, "Opened OSS audio engine %d, minor=%d\n", retval,
	    tmpdev));

      devc->oss_audiodev = retval;

      if (tmp_file.mode & OPEN_READ)
	audio_engines[retval]->dmap_in->audio_callback = input_callback;
      if (tmp_file.mode & OPEN_WRITE)
	audio_engines[retval]->dmap_out->audio_callback = output_callback;
      strcpy (audio_engines[retval]->cmd, "SADA");
      strcpy (audio_engines[retval]->label, "SADA");
      audio_engines[retval]->pid = 0;

      if ((retval = setup_device (devc)) != AUDIO_SUCCESS)
	{
	  cmn_err (CE_NOTE, "setup_device failed\n");
	  mutex_enter (&devc->inst_lock);
	  if (--devc->audio_busy == 0)
	    {
	      mutex_exit (&devc->inst_lock);
	      oss_audio_release (devc->oss_audiodev, &tmp_file);
	    }
	  else
	    mutex_exit (&devc->inst_lock);

	  return EIO;
	}
    }
  else
    {
      if (oss_mode & ~devc->audio_mode)
	{
	  cmn_err (CE_NOTE, "Conflicting access modes\n");

	  if (--devc->audio_busy == 0)
	    {
	      mutex_exit (&devc->inst_lock);
	      oss_audio_release (devc->oss_audiodev, &tmp_file);
	      return EBUSY;
	    }

	  mutex_exit (&devc->inst_lock);
	  return EBUSY;
	}

      mutex_exit (&devc->inst_lock);

    }

  DDB (cmn_err (CE_CONT, "Open count %d\n", devc->audio_busy));

  DDB (cmn_err
       (CE_CONT, "audio_sup_open(q=%x, dev=%x, flag=%x, sflag=%x)\n", q, dev,
	flag, sflag));
  if ((retval = audio_sup_open (q, devp, flag, sflag, credp)) != 0)
    {
      cmn_err (CE_NOTE, "audio_sup_open() returned %d\n", retval);
      mutex_enter (&devc->inst_lock);
      if (--devc->audio_busy == 0)
	{
	  mutex_exit (&devc->inst_lock);
	  oss_audio_release (devc->oss_audiodev, &tmp_file);
	}
      else
	mutex_exit (&devc->inst_lock);
    }

  return retval;
}

static int
sadasupport_close (queue_t * q, int flag, cred_t * credp)
{
  int retval, count;

  DDB (cmn_err (CE_CONT, "sadasupport_close(q=%x, flag=%x)\n", q, flag));

  if (!devc->audio_busy)
    cmn_err (CE_WARN, "Close while not busy.\n");

  retval = audio_sup_close (q, flag, credp);

  mutex_enter (&devc->inst_lock);
  count = --devc->audio_busy;
  mutex_exit (&devc->inst_lock);

  DDB (cmn_err (CE_CONT, "Open count (close) %d\n", count));
  if (count == 0)
    {
      DDB (cmn_err
	   (CE_CONT, "Closing OSS audiodev %d\n", devc->oss_audiodev));
      oss_audio_release (devc->oss_audiodev, &tmp_file);
      devc->audio_mode = 0;
    }

  return retval;
}
