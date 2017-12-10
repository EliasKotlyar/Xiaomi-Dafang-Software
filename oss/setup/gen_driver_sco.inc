static void
generate_driver (char *name, conf_t * conf, char *cfg_name, char *cfg_header,
		 char *dirname, char *topdir)
{

  /* SCO DDI8 version */


  FILE *f, *src;
  char tmp[256], line[256], *p, *s;
  int i, n = 0;
  int is_pseudo = 0;

  if (strcmp (conf->bus, "VIRTUAL") == 0)
    is_pseudo = 1;

  sprintf (tmp, "%s/%s", dirname, cfg_name);

  if ((src = fopen (tmp, "w")) == NULL)
    {
      perror (tmp);
      exit (-1);
    }

  fprintf (src, "/*\n");
  fprintf (src, " * Automatically generated file - do not edit.\n");
  fprintf (src, " */\n");
  fprintf (src, "#include \"%s\"\n", cfg_header);
  fprintf (src, "\n");

  fprintf (src, "#include \"%s/%s/module.inc\"\n", topdir, this_os);

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
  fprintf (src, "\n");

  fprintf (src, "extern int DRIVER_ATTACH(oss_device_t *ossdev);\n");
  fprintf (src, "extern int DRIVER_DETACH(oss_device_t *ossdev);\n");

  fclose (src);
/*
 * Generate SCO DDI8 specific config files
 */
  sprintf (tmp, "target/build/%s", name);
  if (mkdir (tmp, 0755) == -1)
    {
      perror (tmp);
      fprintf (stderr, "Cannot make module target directory\n");
      exit (-1);
    }

/*
 * Master file
 */
  sprintf (tmp, "target/build/%s/Master", name);
  if ((src = fopen (tmp, "w")) == NULL)
    {
      perror (tmp);
      exit (-1);
    }
  fprintf (src, "$version 2\n");
  fprintf (src, "$contact 4Front Technologies (http://www.opensound.com)\n");
  fprintf (src, "$interface ddi 8mp\n");
  fprintf (src, "$depend osscore\n");

  if (is_pseudo)
    fprintf (src, "%s	-  R\n", name);
  else
    fprintf (src, "%s	-  h\n", name);
  fclose (src);

/*
 * System file
 */
  sprintf (tmp, "target/build/%s/System", name);
  if ((src = fopen (tmp, "w")) == NULL)
    {
      perror (tmp);
      exit (-1);
    }
  fprintf (src,
	   "* Don't edit this file manually! This information will be ignored.\n");
  fprintf (src, "*\n");
  fprintf (src, "$version 2\n");
  if (is_pseudo)
    fprintf (src,
	     "%s	Y	0	5	0	0	0	0	0	0	-1\n",
	     name);
  else
    fprintf (src,
	     "%s	Y	0	5	4	0	0	0	0	0	-1\n",
	     name);
  fclose (src);

/*
 * Node file
 */
  sprintf (tmp, "target/build/%s/Node", name);
  if ((src = fopen (tmp, "w")) == NULL)
    {
      perror (tmp);
      exit (-1);
    }
  fprintf (src, "$maxchan 255\n");
  fprintf (src, "%s	%s%%i	c	0	0	0	0600\n", name,
	   name);
  fclose (src);

/*
 * Drvmap file
 */
  sprintf (tmp, "target/build/%s/Drvmap", name);
  if ((src = fopen (tmp, "w")) == NULL)
    {
      perror (tmp);
      exit (-1);
    }
  fprintf (src, "%s|Y|N|Sound Boards|OSS %s module\n", name, name);

  if (strcmp (conf->bus, "VIRTUAL") == 0)
    {
      fprintf (src, "|||OSS %s pseudo device\n", name);
    }

  if (strcmp (conf->bus, "PCI") == 0)
    {

      if ((f = fopen ("devices.list", "r")) == NULL)
	{
	  perror ("devices.list");
	  exit (-1);
	}

      while (fgets (line, sizeof (line) - 1, f) != NULL)
	{
	  int vendor, product;
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

	  if (strncmp (s, "pci", 3) == 0)
	    {
	      if (sscanf (s + 3, "%x,%x", &vendor, &product) != 2)
		{
		  fprintf (stderr, "Bad PCI id %s\n", s);
		}

	      fprintf (src, "|PCI|0x%04X%04X|%s\n", vendor, product, p);
	    }

	}

      fclose (f);
    }
  fclose (src);

#if 0
/*
 * config.h file
 */
  sprintf (tmp, "target/build/%s/config.h", name);
  if ((src = fopen (tmp, "w")) == NULL)
    {
      perror (tmp);
      exit (-1);
    }
  fclose (src);
#endif

/*
 * Space.c file
 */
  sprintf (tmp, "target/build/%s/Space.c", name);
  if ((src = fopen (tmp, "w")) == NULL)
    {
      perror (tmp);
      exit (-1);
    }

  fprintf (src, "/*\n");
  fprintf (src, " * NOTICE!\n");
  fprintf (src, " *\n");

  fprintf (src,
	   " * You need to re-install OSS modules after modifying this file.\n");
  fprintf (src, " * You need to do the following:\n");
  fprintf (src, " *\n");
  fprintf (src, " * \tcd /usr/lib/oss/build\n");
  fprintf (src, " * \tsh install.sh\n");
  fprintf (src, " *\n");
  fprintf (src, " * Each option is documented in the comments below them.\n");
  fprintf (src, " */\n\n");

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
	    }
	}

      fclose (f);
    }

  fprintf (src, "\n");
  fclose (src);
}
