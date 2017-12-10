static void
generate_driver (char *name, conf_t * conf, char *cfg_name, char *cfg_header,
		 char *dirname, char *topdir)
{

  /* Solaris version */


  FILE *f, *src;
  char tmp[256], line[256], *p, *s;
  int i, n = 0;
  char *options[MAXOPTS];
  int nopts = 0;

  sprintf (tmp, "%s/%s", dirname, cfg_name);

  if ((src = fopen (tmp, "w")) == NULL)
    {
      perror (tmp);
      exit (-1);
    }

  fprintf (src, "/*\n");
  fprintf (src, " * Automatically generated file - do not edit.\n");
  fprintf (src, " */\n");

/*
 * Handle driver specific configuration options
 */
  sprintf (tmp, "%s/.params", dirname);
  if ((f = fopen (tmp, "r")) != NULL)
    {
      while (fgets (line, sizeof (line) - 1, f) != NULL)
	{
	  p = line + strlen (line) - 1;
	  if (*p == '\n')
	    *p = 0;

	  fprintf (src, "%s\n", line);
	  if (strncmp (line, "int ", 4) == 0)
	    {
	      char *s = line + 4, *p = s;

	      while (*p && *p != '=' && *p != ';')
		p++;
	      *p = 0;
	      if (nopts >= MAXOPTS)
		{
		  fprintf (stderr, "Too many options for driver '%s' (%d)\n",
			   name, nopts);
		  exit (-1);
		}

	      options[nopts++] = strdup (s);
	    }
	}

      fclose (f);
    }

  fprintf (src, "\n");
  fprintf (src, "#include \"%s\"\n", cfg_header);
  fprintf (src, "\n");

#if 0
  /* Not needed for Solaris */
  fprintf (src, "oss_device_table_t %s_devid_list[]=\n", name);
  fprintf (src, "{\n");

  if ((f = fopen ("devices.list", "r")) == NULL)
    {
      perror ("devices.list");
      exit (-1);
    }

  while (fgets (line, sizeof (line) - 1, f) != NULL)
    {
      p = line + strlen (line) - 1;
      if (*p == '\n')
	*p = 0;

      p = line;
      while (*p && *p != '\t')
	p++;
      if (*p == '\t')
	*p++ = 0;

      if (strcmp (line, name) != 0)
	continue;

      n++;

      s = p;
      while (*p && *p != '\t')
	p++;
      if (*p == '\t')
	*p++ = 0;

      fprintf (src, "\t{\"%s\", \"%s\"},\n", s, p);
    }

  fclose (f);

  fprintf (src, "\t{NULL}\n");
  fprintf (src, "};\n");
#endif

/*
 * Create the driver specific option list
 */
  fprintf (src, "oss_option_map_t local_driver_options[] =\n");
  fprintf (src, "{\n");
  for (i = 0; i < nopts; i++)
    fprintf (src, "\t{\"%s\", &%s},\n", options[i], options[i]);
  fprintf (src, "\t{NULL, NULL}\n");
  fprintf (src, "};\n");
  fprintf (src, "\n");

  fprintf (src, "#include \"%s/%s/module.inc\"\n", topdir, this_os);

#if 0
  if (n == 0)
    {
      fprintf (stderr,
	       "Warning: No device entries (devices.list) for driver %s\n",
	       name);
    }
#endif

  fclose (src);

  sprintf (tmp, "%s/%s", dirname, cfg_header);
  if ((src = fopen (tmp, "w")) == NULL)
    {
      perror (tmp);
      exit (-1);
    }

  fprintf (src, "/*\n");
  fprintf (src, " * Automatically generated file - do not edit.\n");
  fprintf (src, " */\n");

  fprintf (src, "#include <oss_config.h>\n");
  fprintf (src, "\n");

  fprintf (src, "#define DRIVER_NAME\t%s\n", name);
  fprintf (src, "#define DRIVER_NICK\t\"%s\"\n", name);
  fprintf (src, "#define DRIVER_PURPOSE\t\"%s\"\n", conf->purpose);
  fprintf (src, "#define DRIVER_STR_INFO\t%s_str_info\n", name);
  fprintf (src, "#define DRIVER_ATTACH\t%s_attach\n", name);
  fprintf (src, "#define DRIVER_DETACH\t%s_detach\n", name);
  fprintf (src, "#define DRIVER_TYPE\tDRV_%s\n", conf->bus);

  if (conf->power_manage)
     {
  	fprintf (src, "#define DRIVER_POWER\t%s_power\n", name);
	fprintf (src, "#define OSS_POWER_MANAGE\n");
     }

  if (conf->suspend_resume)
     {
  	fprintf (src, "#define DRIVER_SUSPEND\t%s_suspend\n", name);
  	fprintf (src, "#define DRIVER_RESUME\t%s_resume\n", name);
	fprintf (src, "#define OSS_SUSPEND_RESUME\n");
     }

  fprintf (src, "\n");

  fprintf (src, "extern int DRIVER_ATTACH(oss_device_t *ossdev);\n");
  fprintf (src, "extern int DRIVER_DETACH(oss_device_t *ossdev);\n");
  fprintf (src, "extern int DRIVER_POWER(oss_device_t *ossdev, int component, int level);\n");

  fclose (src);
}
