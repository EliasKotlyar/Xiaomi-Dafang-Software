static void
generate_driver (char *name, conf_t * conf, char *cfg_name, char *cfg_header,
                 char *dirname, char *topdir)
{
  /* Linux version */

  FILE *f, *src;
  char tmp[256], line[256], *p, *s;
  int i, n = 0;
  char *options[MAXOPTS];
  char *option_desc[MAXOPTS];
  char desc[65536];
  int nopts = 0;

  sprintf (tmp, "target/build/%s.c", name);
  if ((src = fopen (tmp, "w")) == NULL)
    {
      perror (tmp);
      exit (-1);
    }

  fprintf (src, "/*\n");
  fprintf (src, " * Automatically generated file - do not edit.\n");
  fprintf (src, " */\n");

  fprintf (src, "#define DRIVER_NAME\t%s\n", name);
  fprintf (src, "#define DRIVER_NICK\t\"%s\"\n", name);
  fprintf (src, "#define DRIVER_PURPOSE\t\"%s\"\n", conf->purpose);
  fprintf (src, "#define DRIVER_STR_INFO\t%s_str_info\n", name);
  fprintf (src, "#define DRIVER_ATTACH\t%s_attach\n", name);
  fprintf (src, "#define DRIVER_DETACH\t%s_detach\n", name);
  fprintf (src, "#define DRIVER_TYPE\tDRV_%s\n", conf->bus);
  fprintf (src, "\n");

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
      fprintf (src, "#include <linux/mod_devicetable.h>\n\n");
      fprintf (src, "#include <linux/pci_ids.h>\n\n");
      fprintf (src, "static struct pci_device_id id_table[] = {\n");

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
		       "\t{.vendor=0x%x,\t.device=0x%x,\t.subvendor=PCI_ANY_ID,\t.subdevice=PCI_ANY_ID,\t.class=PCI_CLASS_MULTIMEDIA_AUDIO},\n",
		       vendor, product);
	    }

	}

      fclose (f);

      fprintf (src, "\t{0}\n");
      fprintf (src, "};\n");
      fprintf (src, "\n");
    }

  if (strcmp (conf->bus, "USB") == 0)
    {
      fprintf (src, "#include <linux/mod_devicetable.h>\n\n");
      fprintf (src, "#undef strcpy\n");
      fprintf (src, "#define strcpy dummy_strcpy\n");
      fprintf (src, "#include <linux/usb.h>\n");
      fprintf (src, "#undef strcpy\n");
      fprintf (src, "static struct usb_device_id udi_usb_table[] = {\n");

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

	  if (strcmp (s, "usbif,class1") == 0)
	    continue;

	  if (strncmp (s, "usbi", 4) == 0)
	    {
	      if (sscanf (s + 4, "%x,%x", &vendor, &product) != 2)
		{
		  fprintf (stderr, "Bad USB id %s\n", s);
		}

	      fprintf (src,
		       "\t{.match_flags=USB_DEVICE_ID_MATCH_DEVICE,\t.idVendor=0x%x,\t.idProduct=0x%x},\n",
		       vendor, product);
	      continue;
	    }

	  if (strncmp (s, "usb", 3) == 0)
	    {
	      if (sscanf (s + 3, "%x,%x", &vendor, &product) != 2)
		{
		  fprintf (stderr, "Bad USB id %s\n", s);
		}

	      fprintf (src,
		       "\t{.match_flags=USB_DEVICE_ID_MATCH_DEVICE,\t.idVendor=0x%x,\t.idProduct=0x%x},\n",
		       vendor, product);
	      continue;
	    }


	}

      fclose (f);

      fprintf (src, "\t{match_flags:USB_DEVICE_ID_MATCH_INT_CLASS,\n");
      fprintf (src, "\tbInterfaceClass: USB_CLASS_AUDIO},\n");
      fprintf (src, "\t{0}\n");
      fprintf (src, "};\n");
      fprintf (src, "\n");
    }

  fprintf (src, "#include \"module.inc\"\n");
  fprintf (src, "\n");

  for (i = 0; i < nopts; i++)
    {
      fprintf (src, "module_param(%s, int, S_IRUGO);\n", options[i]);
      if (option_desc[i] != NULL)
	fprintf (src, "MODULE_PARM_DESC(%s, %s);\n", options[i],
		 option_desc[i]);
    }
  fprintf (src, "\n");
  fprintf (src, "\n");

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
  fprintf (src, "#define DRIVER_STR_INFO\t%s_str_info\n", name);
  fprintf (src, "#define DRIVER_ATTACH\t%s_attach\n", name);
  fprintf (src, "#define DRIVER_DETACH\t%s_detach\n", name);
  fprintf (src, "#define DRIVER_TYPE\tDRV_%s\n", conf->bus);
  fprintf (src, "\n");

  fprintf (src, "extern int DRIVER_ATTACH(oss_device_t *ossdev);\n");
  fprintf (src, "extern int DRIVER_DETACH(oss_device_t *ossdev);\n");

  fclose (src);
}
