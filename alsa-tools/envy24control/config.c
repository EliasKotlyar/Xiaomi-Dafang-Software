#include <gtk/gtk.h>
#include "envy24control.h"

#if GLIB_CHECK_VERSION(2,2,0)

#if GLIB_CHECK_VERSION(2,8,0)
#define MYMKDIR g_mkdir_with_parents
#else
#include <sys/stat.h>
#include <sys/types.h>
#define MYMKDIR mkdir
#endif

GKeyFile *config_file;
gboolean config_stereo[20];
gchar *config_filename;

void config_open()
{
  config_filename=g_strdup_printf("%s/%s", g_get_user_config_dir(), "envy24control");
  config_file=g_key_file_new();
  g_key_file_load_from_file(config_file, config_filename, G_KEY_FILE_KEEP_COMMENTS, NULL);
}

void config_close()
{
  gsize len=0;
  gchar *s;
  g_key_file_set_boolean_list(config_file, "mixer", "stereo",
			      config_stereo, sizeof(config_stereo)/sizeof(config_stereo[0]));
  s=g_key_file_to_data(config_file, &len, NULL);
  if(s && len)
    {
      MYMKDIR(g_get_user_config_dir(), 0700);
      FILE *f=fopen(config_filename, "wb");
      if(f)
	{
	  fwrite(s, len, 1, f);
	  fclose(f);
	}
    }

  g_free(config_filename); config_filename=0;
  g_key_file_free(config_file); config_file=0;
}

void config_set_stereo(GtkWidget *but, gpointer data)
{
  gint i=(gint)data;
  config_stereo[i]=GTK_TOGGLE_BUTTON(but)->active;
}

void config_restore_stereo()
{
  gint i;
  gsize len=0;
  gboolean *s=g_key_file_get_boolean_list(config_file, "mixer", "stereo", &len, NULL);
  if(s)
      for(i=0; i!=len; ++i)
	{
	  config_stereo[i]=s[i];
	  if(mixer_stereo_toggle[i])
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mixer_stereo_toggle[i]), s[i]);
	}
}

#else

/* to be done */
void config_open() { }
void config_close() { }
void config_set_stereo(GtkWidget *but, gpointer data) { }
void config_restore_stereo() { }

#endif
