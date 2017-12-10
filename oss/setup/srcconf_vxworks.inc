static void
generate_vxworks_driver (char *name, conf_t * conf, char *cfg_name, char *cfg_header,
                 char *dirname, char *topdir);

static void
vxworks_setup(conf_t *conf, char *arch)
{
#ifndef linux
	char tmp[256];
#endif

	printf ("\n*** Setting up cross compiling for VxWorks %s target ***\n\n", arch);

	hostcc="cc";

#ifdef linux
	targetcc="ccpentium";
#else
	sprintf(tmp, "cc%s", arch);
	targetcc=strdup(tmp);
#endif
	strcpy(conf->OSflags, "");
	strcpy(conf->platform, "vxworks");
	strcpy(conf->ccomp, targetcc);
	strcpy(this_os, "kernel/OS/VxWorks");
	strcpy(conf->system, "VxWorks");
	strcpy(conf->arch, arch);
	sprintf(conf->cplusplus, "c++%s", arch);

	shlib_cflags = "-shared -fPIC";
	shlib_ldflags = "-shared -fPIC";

	strcpy (conf->endianess, "LITTLE"); // TODO

	driver_gen = generate_vxworks_driver;
}

static void
vxworks_genheader(FILE *f, char *path)
{
#ifdef linux
	fprintf (f, "\n");
	fprintf (f, "WIND_BASE=/WindRiver/vxworks-6.6\n");
	fprintf (f, "LD=ldpentium\n");
	fprintf (f, "AR=arpentium\n");
	fprintf (f, "CFLAGS+=-mpentium -nostdinc -O2 -fvolatile -nostdlib -fno-builtin -fno-defer-pop -Wall -I$(WIND_BASE)/target/config/all -I-I/WindRiver/vxworks-6.6/target/h/wrn/coreip -I/WindRiver/vxworks-6.6/target/h/wrn/coreip\n");
	//fprintf (f, "CFLAGS+=-nostdinc -O2 -nostdlib -fno-builtin -fno-defer-pop -Wall -I$(WIND_BASE)/target/config/all\n");
	fprintf (f, "CFLAGS+=-I$(WIND_BASE)/target/h -I$(WIND_BASE)/target/src/config -I$(WIND_BASE)/target/src/drv -I/WindRiver/diab/5.6.0.0/include -DCPU=PENTIUM\n");
	fprintf (f, "PATH+=:/WindRiver/gnu/4.1.2-vxworks-6.6/x86-linux2/bin\n");
	fprintf (f, "\n");
#else
	fprintf (f, "\n");
	fprintf (f, "WIND_BASE=/usr/tornado-2.0\n");
	fprintf (f, "CPU=PENTIUM\n");
	fprintf (f, "TOOL=gnu\n");
	fprintf (f, "WIND_HOST_TYPE=sun4-solaris2\n");
	fprintf (f, "\n");
	fprintf (f, "include $(WIND_BASE)/target/h/make/defs.bsp\n");
	fprintf (f, "include $(WIND_BASE)/target/h/make/make.$(CPU)$(TOOL)\n");
	fprintf (f, "include $(WIND_BASE)/target/h/make/defs.$(WIND_HOST_TYPE)\n");
	fprintf (f, "\n");
	fprintf (f, "exe: all\n");
	fprintf (f, "\n");
#endif
}

static void
generate_vxworks_driver (char *name, conf_t * conf, char *cfg_name, char *cfg_header,
                 char *dirname, char *topdir)
{
  /* VxWorks version */

  FILE *f, *src;
  char tmp[256], line[256], *p, *s;
  int n = 0;
  char *options[MAXOPTS];
  char *option_desc[MAXOPTS];
  char desc[65536];
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

  fprintf (src, "#include \"%s\"\n\n", cfg_header);

/*
 * Handle driver specific configuration options
 */
  *desc = 0;
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

	      if (nopts != 0 && *desc != 0)
		option_desc[nopts - 1] = strdup (desc);
	      option_desc[nopts] = 0;
	      options[nopts++] = strdup (s);
	      *desc = 0;
	    }
	  else
	    {
	      char *s = line, *p;
	      char tmp[4096];
	      while (*s == ' ' || *s == '/' || *s == '*')
		s++;

	      p = tmp;

	      while (*s)
		{
		  if (*s == '"')
		    *p++ = '\\';

		  *p++ = *s++;;
		}
	      *p = 0;

	      p = desc + strlen (desc);
	      sprintf (p, "\n\"%s\\n\"", tmp);
	    }
	}

      fclose (f);
    }

  if (nopts > 0 && *desc != 0)
    option_desc[nopts - 1] = strdup (desc);

  if ((f = fopen ("devices.list", "r")) == NULL)
    {
      perror ("devices.list");
      exit (-1);
    }

  if (strcmp (conf->bus, "PCI") == 0)
    {
      fprintf (src, "static unsigned int id_table[] = {\n");

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

	      fprintf (src,
		       "\t0x%04x%04x,\n",
		       vendor, product);
	    }

	}

      fclose (f);

      fprintf (src, "\t0\n");
      fprintf (src, "};\n");
      fprintf (src, "\n");
    }

  if (strcmp (conf->bus, "USB") == 0)
    {
	fprintf (stderr, "Error: No  support available for USB yet\n");
	exit (EXIT_FAILURE);
    }
  fprintf (src, "#include \"module.inc\"\n");
  fclose(src);

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
  fprintf (src, "#define DRIVER_ATTACH\t%s_attach\n", name);
  fprintf (src, "#define DRIVER_DETACH\t%s_detach\n", name);
  fprintf (src, "#define DRIVER_TYPE\tDRV_%s\n", conf->bus);
  fprintf (src, "\n");

  fprintf (src, "extern int DRIVER_ATTACH(oss_device_t *ossdev);\n");
  fprintf (src, "extern int DRIVER_DETACH(oss_device_t *ossdev);\n");

  fclose (src);
}
