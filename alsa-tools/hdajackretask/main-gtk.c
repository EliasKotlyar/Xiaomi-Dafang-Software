/* Copyright 2011 David Henningsson, Canonical Ltd.
   License: GPLv2+ 
*/

#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <stdbool.h>

#include "sysfs-pin-configs.h"
#include "apply-changes.h"

typedef struct ui_data_t ui_data_t;

typedef struct pin_ui_data_t {
    pin_configs_t* pin_config;
    typical_pins_t pins_info[32];
    GtkWidget *frame, *override, *jacktype;
    GtkWidget* free_override_cb[FREE_OVERRIDES_COUNT];
    ui_data_t* owner;
} pin_ui_data_t;

typedef struct hints_ui_data_t {
    gboolean visible;
    GtkWidget *frame;
    GtkListStore *store;
    gchar *values;
} hints_ui_data_t;

struct ui_data_t {
    GList* pin_ui_data;
    GtkWidget *main_window;
    GtkWidget *content_scroll_widget;
    GtkWidget *content_inner_box;
    GtkWidget *codec_selection_combo;

    codec_name_t* current_codec;
    int sysfs_pincount;
    codec_name_t sysfs_codec_names[128];
    pin_configs_t sysfs_pins[32];
    gboolean free_overrides;
    gboolean trust_codec;
    gboolean trust_defcfg;
    gboolean model_auto;

    hints_ui_data_t hints;
};

static void update_user_pin_config(ui_data_t* ui, pin_configs_t* cfg);

static void update_override_sensitive(GtkWidget* sender, pin_ui_data_t* data)
{
    int i;
    gboolean checked = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sender));
    gtk_widget_set_sensitive(data->jacktype, checked); 
    for (i = 0; i < FREE_OVERRIDES_COUNT; i++)
        gtk_widget_set_sensitive(data->free_override_cb[i], checked);
}

static void override_toggled(GtkWidget* sender, pin_ui_data_t* data)
{
    update_override_sensitive(sender, data);
    update_user_pin_config(data->owner, data->pin_config);
}

static void jacktype_changed(GtkWidget* sender, pin_ui_data_t* data)
{
    update_user_pin_config(data->owner, data->pin_config);
}

static GtkWidget* create_pin_ui(ui_data_t* ui, pin_configs_t* pin_cfg)
{
    GtkWidget* result;
    GtkContainer* box;
    pin_ui_data_t* data;

    int port_conn = get_port_conn(pin_cfg->init_pin_config);
    /* Do not show unconnected pins */
    if (ui->trust_defcfg && port_conn == 1) 
        return NULL;

    data = calloc(1, sizeof(pin_ui_data_t));
    data->pin_config = pin_cfg;
    data->owner = ui;

    { /* Frame */
        gchar* d = get_config_description(pin_cfg->init_pin_config);
        gchar* c = g_strdup_printf("Pin ID: 0x%02x", pin_cfg->nid);
        GtkWidget* label = gtk_label_new(c);
        result = gtk_frame_new(d);
        data->frame = result;
        box = GTK_CONTAINER(gtk_box_new(GTK_ORIENTATION_VERTICAL, 2));
        gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
        gtk_container_add(box, label);
        g_free(d);
        g_free(c);
    }

    { /* Capabilities 
        gchar* s = get_caps_description(pin_cfg->pin_caps);
        gchar* s2 = g_strdup_printf("Capabilities: %s", strlen(s) > 2 ? s+2 : ""); // Hack for initial comma
        GtkWidget* label = gtk_label_new(s2);
        gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
        gtk_container_add(box, label);
        g_free(s);
        g_free(s2); */
    }

    { /* Override */
        GtkWidget* override = data->override = gtk_check_button_new_with_label("Override");
        GtkWidget* jacktype = data->jacktype = gtk_combo_box_text_new();
        GtkWidget* jacktype_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
        int index = get_typical_pins(data->pins_info, 32, pin_cfg, ui->trust_codec);
        typical_pins_t* current = data->pins_info;
        while (current->name) {
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(jacktype), current->name);
            current++;
        }
        gtk_combo_box_set_active(GTK_COMBO_BOX(jacktype), index);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(override), pin_cfg->user_override);
        g_signal_connect(override, "toggled", G_CALLBACK(override_toggled), data);
        g_signal_connect(jacktype, "changed", G_CALLBACK(jacktype_changed), data);

        gtk_container_add(box, override);
        gtk_container_add(GTK_CONTAINER(jacktype_box), jacktype);
        if (!ui->free_overrides)
            gtk_container_add(box, jacktype_box);
    }

    /* Advanced override */
    {
        int i;
        GtkGrid* grid = GTK_GRID(gtk_grid_new());
        gtk_grid_set_row_spacing(grid, 2);
        gtk_grid_set_column_spacing(grid, 4);

        for (i = 0; i < FREE_OVERRIDES_COUNT; i++) {
            int index = -1;
            int j = 0;
            unsigned long act_pincfg = actual_pin_config(pin_cfg);
            unsigned long mask = get_free_override_mask(i);
            free_override_t* values = get_free_override_list(i);
            data->free_override_cb[i] = gtk_combo_box_text_new();
            if (!values)
                continue;
            while (values->name) {
                gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(data->free_override_cb[i]), values->name);
                if ((act_pincfg & mask) == values->value)
                    index = j;
                values++;
                j++;
            }
            if (index >= 0)
                gtk_combo_box_set_active(GTK_COMBO_BOX(data->free_override_cb[i]), index);
            g_signal_connect(data->free_override_cb[i], "changed", G_CALLBACK(jacktype_changed), data);
        }

        gtk_grid_attach(grid, gtk_label_new("Connectivity"), 0, 0, 1, 1);
        gtk_grid_attach(grid, data->free_override_cb[0], 0, 1, 1, 1);
        gtk_grid_attach(grid, gtk_label_new("Location"), 1, 0, 1, 1);
        gtk_grid_attach(grid, data->free_override_cb[1], 1, 1, 1, 1);
        gtk_grid_attach(grid, gtk_label_new("Device"), 2, 0, 1, 1);
        gtk_grid_attach(grid, data->free_override_cb[2], 2, 1, 1, 1);
        gtk_grid_attach(grid, gtk_label_new("Jack"), 3, 0, 1, 1);
        gtk_grid_attach(grid, data->free_override_cb[3], 3, 1, 1, 1);

        gtk_grid_attach(grid, gtk_label_new("Color"), 0, 2, 1, 1);
        gtk_grid_attach(grid, data->free_override_cb[4], 0, 3, 1, 1);
        gtk_grid_attach(grid, gtk_label_new("Jack detection"), 1, 2, 1, 1);
        gtk_grid_attach(grid, data->free_override_cb[5], 1, 3, 1, 1);
        gtk_grid_attach(grid, gtk_label_new("Channel group"), 2, 2, 1, 1);
        gtk_grid_attach(grid, data->free_override_cb[6], 2, 3, 1, 1);
        gtk_grid_attach(grid, gtk_label_new("Channel (in group)"), 3, 2, 1, 1);
        gtk_grid_attach(grid, data->free_override_cb[7], 3, 3, 1, 1);

        if (ui->free_overrides)
            gtk_container_add(box, GTK_WIDGET(grid));
    }
    update_override_sensitive(data->override, data);

    gtk_container_add(GTK_CONTAINER(result), GTK_WIDGET(box));
    ui->pin_ui_data = g_list_prepend(ui->pin_ui_data, data);
    return result;
}

static void free_pin_ui_data(pin_ui_data_t* data)
{
    if (!data)
        return;
    if (data->frame)
        gtk_widget_destroy(data->frame);
    free(data);
}

static gint pin_config_find(pin_ui_data_t* pin_ui, pin_configs_t* cfg)
{
    return pin_ui->pin_config == cfg ? 0 : 1;
}

static void update_user_pin_config(ui_data_t* ui, pin_configs_t* cfg)
{
    pin_ui_data_t* pin_ui;
    GList *pos = g_list_find_custom(ui->pin_ui_data, cfg, (GCompareFunc) pin_config_find);
    cfg->user_override = FALSE;
    if (!pos)
        return;
    pin_ui = pos->data;
    if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pin_ui->override)))
        return;

    if (ui->free_overrides) {
        int j;
        int index;
        unsigned long val = 0;
        for (j = 0; j < FREE_OVERRIDES_COUNT; j++) {
            index = gtk_combo_box_get_active(GTK_COMBO_BOX(pin_ui->free_override_cb[j]));
            if (index < 0)
                break;
            val += get_free_override_list(j)[index].value;
        }
        if (index < 0)
            return;
        cfg->user_pin_config = val;
    } else {
        int index;
        index = gtk_combo_box_get_active(GTK_COMBO_BOX(pin_ui->jacktype));
        if (index < 0)
            return;
        cfg->user_pin_config = pin_ui->pins_info[index].pin_set;
    }
    cfg->user_override = TRUE;
}

static void update_all_user_pin_config(ui_data_t* ui)
{
    int i;
    for (i = 0; i < ui->sysfs_pincount; i++)
        update_user_pin_config(ui, &ui->sysfs_pins[i]);
}

static gboolean update_one_hint(GtkTreeModel *model, GtkTreePath *path,
                            GtkTreeIter *iter, gpointer userdata)
{
    gchar *name, *value;
    ui_data_t *ui = userdata;
    gtk_tree_model_get(GTK_TREE_MODEL(ui->hints.store), iter, 0, &name, 1, &value, -1);
    if (g_strcmp0(value, "default")) {
        gchar *s = g_strconcat(name, "=", value, "\n", ui->hints.values, NULL);
        g_free(ui->hints.values);
        ui->hints.values = s;
    }
    g_free(name);
    g_free(value);
    return FALSE;
}

static void update_hints(ui_data_t* ui)
{
    g_free(ui->hints.values);
    ui->hints.values = NULL;
    if (ui->hints.visible)
        gtk_tree_model_foreach(GTK_TREE_MODEL(ui->hints.store), update_one_hint, ui);
}

static GQuark quark()
{
    return g_quark_from_static_string("hda-jack-retask-error");
}

static gboolean validate_user_pin_config(ui_data_t* ui, GError** err)
{
    int i;

    if (!ui->current_codec) {
        g_set_error(err, quark(), 0, "You must first select a codec!");
        return FALSE;
    }
    update_hints(ui);
    update_all_user_pin_config(ui);
    if (ui->free_overrides)
        return TRUE;

    /* Check surround configs */
    for (i = 0; i < ui->sysfs_pincount; i++) {
        unsigned long v = ui->sysfs_pins[i].user_pin_config;
        if (!ui->sysfs_pins[i].user_override)
            continue;
        if ((v & 0xf0) != 0x10)
            continue;
        if (((v & 0xf) != 0) && !find_pin_channel_match(ui->sysfs_pins, ui->sysfs_pincount, v & 0xf0)) {
            g_set_error(err, quark(), 0, "This surround setup also requires a \"front\" channel override.");
            return FALSE;
        }
        if (((v & 0xf) >= 3) && !find_pin_channel_match(ui->sysfs_pins, ui->sysfs_pincount, 2 + (v & 0xf0))) {
            g_set_error(err, quark(), 0, "This surround setup also requires a \"back\" channel override.");
            return FALSE;
        }
        if ((v & 0xf) >= 3 && !find_pin_channel_match(ui->sysfs_pins, ui->sysfs_pincount, 1 + (v & 0xf0))) {
            g_set_error(err, quark(), 0, "This surround setup also requires a \"Center/LFE\" channel override.");
            return FALSE;
        }
    }
    return TRUE;
}

static gboolean update_tree_one_hint(GtkTreeModel *model, GtkTreePath *path,
                            GtkTreeIter *iter, gpointer userdata)
{
    gchar *name;
    ui_data_t *ui = userdata;
    gtk_tree_model_get(GTK_TREE_MODEL(ui->hints.store), iter, 0, &name, -1);
    gchar *s = strstr(ui->hints.values, name);
    if (!s) {
        g_free(name);
        gtk_list_store_set(ui->hints.store, iter, 1, "default", -1);
        return FALSE;
    }
    s += strlen(name);
    while (*s == ' ' || *s == '=') s++;
    gchar *s2 = s;
    while (*s != '\n' && *s != '\0') s++;
    s2 = g_strndup(s2, s - s2);
    gtk_list_store_set(ui->hints.store, iter, 1, s2, -1);
    g_free(s2);
    g_free(name);
    return FALSE;
}

static void show_action_result(ui_data_t* ui, GError* err, const gchar* ok_msg)
{
    GtkWidget* dialog;
    const gchar* msg = err ? err->message : ok_msg;
    dialog = gtk_message_dialog_new (GTK_WINDOW(ui->main_window),
        GTK_DIALOG_DESTROY_WITH_PARENT, err ? GTK_MESSAGE_ERROR : GTK_MESSAGE_INFO, 
        GTK_BUTTONS_CLOSE, "%s", msg);
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    if (err)
        g_error_free(err);
}

static void apply_now_clicked(GtkButton* button, gpointer user_data)
{
    GError* err = NULL;
    ui_data_t* ui = user_data;
    gboolean ok = validate_user_pin_config(ui, &err);
    if (ok)
        apply_changes_reconfig(ui->sysfs_pins, ui->sysfs_pincount, 
            ui->current_codec->card, ui->current_codec->device, 
            ui->model_auto ? "auto" : NULL, ui->hints.values, &err);
    show_action_result(ui, err, 
        "Ok, now go ahead and test to see if it actually worked!\n"
        "(Remember, this stuff is still experimental.)");
}

static void apply_boot_clicked(GtkButton* button, gpointer user_data)
{
    GError* err = NULL;
    ui_data_t* ui = user_data;
    gboolean ok = validate_user_pin_config(ui, &err);
    if (ok) 
        apply_changes_boot(ui->sysfs_pins, ui->sysfs_pincount, 
            ui->current_codec->card, ui->current_codec->device, 
            ui->model_auto ? "auto" : NULL, ui->hints.values, &err);
    show_action_result(ui, err,
        "Ok, now reboot to test to see if it actually worked!\n"
        "(Remember, this stuff is still experimental.)");
}


static void reset_boot_clicked(GtkButton* button, gpointer user_data)
{
    GError* err = NULL;
    ui_data_t* ui = user_data;
    reset_changes_boot(&err);
    show_action_result(ui, err, 
        "The previous installed files (if any) of this program have been removed.\n"
        "Reboot to finish the uninstallation.");
}

static void resize_main_window(ui_data_t* ui)
{
    GtkAllocation a;
    GtkRequisition r;
    gint oldw, oldh, neww, newh, maxw, maxh;
    GdkScreen* screen = gtk_window_get_screen(GTK_WINDOW(ui->main_window));
    gtk_widget_size_request(GTK_WIDGET(ui->content_inner_box), &r);
    gtk_widget_get_allocation(ui->content_scroll_widget, &a);
//    fprintf(stderr, "W: %d, H: %d, W: %d, H: %d\n", a.width, a.height, r.width, r.height);
    gtk_window_get_size(GTK_WINDOW(ui->main_window), &oldw, &oldh);
    maxw = screen ? (gdk_screen_get_width(screen)*3)/4 : INT_MAX / 4;
    maxh = screen ? (gdk_screen_get_height(screen)*3)/4 : INT_MAX / 4;
//    fprintf(stderr, "Before: W: %d, H: %d\n", oldw, oldh);
    neww = oldw;
    newh = oldh;
    if (a.width < r.width) {
        neww += 8 + r.width - a.width;
        if (neww > maxw)
            neww = maxw;
    }
    if (a.height < r.height) {
        newh += 8 + r.height - a.height;
        if (newh > maxh)
            newh = maxh;
    }
    if (neww != oldw || newh != oldh) {
        gtk_window_resize(GTK_WINDOW(ui->main_window), neww, newh);
//        fprintf(stderr, "After: W: %d, H: %d\n", neww, newh);   
    }
}

static void update_codec_ui(ui_data_t* ui, bool codec_change)
{
    int codec_index = gtk_combo_box_get_active(GTK_COMBO_BOX(ui->codec_selection_combo));
    int i;

    g_list_free_full(ui->pin_ui_data, (GDestroyNotify) free_pin_ui_data);
    ui->pin_ui_data = NULL;
    ui->current_codec = NULL;

    if (codec_index < 0)
        return;
    ui->current_codec = &ui->sysfs_codec_names[codec_index];
    if (codec_change) {
        ui->sysfs_pincount = get_pin_configs_list(ui->sysfs_pins, 32, ui->current_codec->card, ui->current_codec->device);
        ui->hints.values = get_hint_overrides(ui->current_codec->card, ui->current_codec->device);
        gtk_tree_model_foreach(GTK_TREE_MODEL(ui->hints.store), update_tree_one_hint, ui);
    }
    for (i = 0; i < ui->sysfs_pincount; i++) {
        GtkWidget *w = create_pin_ui(ui, &ui->sysfs_pins[i]);
        if (w)
            gtk_container_add(GTK_CONTAINER(ui->content_inner_box), w);
    }

    gtk_widget_show_all(GTK_WIDGET(ui->content_inner_box));

    if (ui->hints.visible)
        gtk_widget_show_all(ui->hints.frame);
    else
        gtk_widget_hide(ui->hints.frame);

    resize_main_window(ui);
}

static void codec_selected(GtkComboBox* combo, gpointer user_data) 
{
    update_codec_ui(user_data, true);
}

static void showallpins_toggled(GtkWidget* sender, ui_data_t* ui_data)
{
    gboolean checked = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sender));
    ui_data->trust_defcfg = !checked;
    update_codec_ui(ui_data, false);
}

static void automodel_toggled(GtkWidget* sender, ui_data_t* ui_data)
{
    ui_data->model_auto = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sender));
}

static void free_override_toggled(GtkWidget* sender, ui_data_t* ui_data)
{
    ui_data->free_overrides = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sender));
    update_codec_ui(ui_data, false);
}

static void hints_toggled(GtkWidget* sender, ui_data_t* ui_data)
{
    ui_data->hints.visible = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sender));
    update_codec_ui(ui_data, false);
}

static void hints_row_activated(GtkTreeView *tree_view, GtkTreePath *path,
                                GtkTreeViewColumn *column, ui_data_t* ui_data)
{
    GtkTreeIter iter;
    gchar *value;
    const gchar *newvalue = "default";

    gtk_tree_model_get_iter(GTK_TREE_MODEL(ui_data->hints.store), &iter, path);
    gtk_tree_model_get(GTK_TREE_MODEL(ui_data->hints.store), &iter, 1, &value, -1);

    if (!g_strcmp0(value, "default"))
        newvalue = "yes";
    else if (!g_strcmp0(value, "yes"))
        newvalue = "no";
    gtk_list_store_set(ui_data->hints.store, &iter, 1, newvalue, -1);

    g_free(value);
}


static const char* readme_text = 
#include "README.generated.h"
;

static void documentation_clicked(GtkWidget* sender, ui_data_t* ui)
{
    GtkDialog* dlg = GTK_DIALOG(gtk_dialog_new_with_buttons("Jack retasking documentation", 
        GTK_WINDOW(ui->main_window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_STOCK_CLOSE, GTK_RESPONSE_NONE, NULL));
    GtkTextView* textview = GTK_TEXT_VIEW(gtk_text_view_new());
    GtkContainer* content_area = GTK_CONTAINER(gtk_dialog_get_content_area(dlg));
    GtkScrolledWindow* content_scroll = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new(NULL, NULL));

    gtk_text_buffer_set_text(gtk_text_view_get_buffer(textview), readme_text, -1);
    gtk_text_view_set_editable(textview, FALSE);
    gtk_text_view_set_wrap_mode(textview, GTK_WRAP_WORD);
    gtk_text_view_set_cursor_visible(textview, FALSE);
    gtk_scrolled_window_add_with_viewport(content_scroll, GTK_WIDGET(textview));
    gtk_container_add(content_area, GTK_WIDGET(content_scroll));
    gtk_box_set_child_packing(GTK_BOX(content_area), GTK_WIDGET(content_scroll), TRUE, TRUE, 2, GTK_PACK_START);

    gtk_widget_show_all(GTK_WIDGET(content_area));

    { /* Resize to fit screen */
        GdkScreen* screen = gtk_window_get_screen(GTK_WINDOW(ui->main_window));
        int neww = screen ? (gdk_screen_get_width(screen)*3)/4 : 800;
        int newh = screen ? (gdk_screen_get_height(screen)*3)/4 : 600;
        
        gtk_window_set_default_size(GTK_WINDOW(dlg), neww, newh);
    }

    gtk_dialog_run(dlg);
    gtk_widget_destroy(GTK_WIDGET(dlg));
}

static ui_data_t* create_ui()
{
    ui_data_t* ui = calloc(sizeof(ui_data_t), 1);
    GtkContainer* toplevel_box = GTK_CONTAINER(gtk_box_new(GTK_ORIENTATION_VERTICAL, 2));
    GtkContainer* toplevel_2ndbox = GTK_CONTAINER(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2));
    ui->content_inner_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkContainer* rightside_box = GTK_CONTAINER(gtk_box_new(GTK_ORIENTATION_VERTICAL, 2));
    ui->main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(ui->main_window), "Jack retasking for HDA Intel sound cards");
    g_signal_connect (ui->main_window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

    ui->trust_codec = TRUE;
    ui->trust_defcfg = TRUE;

    /* Select codec to work with */
    {
        GtkWidget* combo = ui->codec_selection_combo = gtk_combo_box_text_new();
        GtkWidget* box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
        codec_name_t *n = ui->sysfs_codec_names;
        get_codec_name_list(ui->sysfs_codec_names, 128);
        while (n->card != -1) {
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), n->name);
            n++;
        }
        /* Select the first codec */
        if (ui->sysfs_codec_names->card != -1) {
            gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
            g_signal_connect(combo, "changed", G_CALLBACK(codec_selected), ui);
            gtk_container_add(GTK_CONTAINER(box), gtk_label_new("Select a codec:"));
            gtk_container_add(GTK_CONTAINER(box), combo);
        }
        else {
            gtk_container_add(GTK_CONTAINER(box), gtk_label_new("No codecs found. Sorry."));
            gtk_widget_destroy(combo);
        }

        gtk_container_add(toplevel_box, box);
    }

    /* Add pin content area */
    {
        GtkWidget* frame = gtk_frame_new("Pin configuration");
        GtkScrolledWindow* content_scroll = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new(NULL, NULL));
        ui->content_scroll_widget = GTK_WIDGET(content_scroll);
        gtk_frame_set_label_align(GTK_FRAME(frame), 0.5, 0.5);
        gtk_scrolled_window_add_with_viewport(content_scroll, ui->content_inner_box);
        gtk_container_add(GTK_CONTAINER(frame), GTK_WIDGET(content_scroll));
        gtk_container_add(toplevel_2ndbox, frame);
        gtk_box_set_child_packing(GTK_BOX(toplevel_2ndbox), frame, TRUE, TRUE, 2, GTK_PACK_START);
    }

    /* Create hints */
    {
        GtkWidget* frame = gtk_frame_new("Hints");
        ui->hints.frame = frame;

        GtkListStore *store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
        ui->hints.store = store;
        const gchar** names = get_standard_hint_names();
        for (; *names; names++) {
            GtkTreeIter iter;
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, 0, *names, 1, "default", -1);
        }

        GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree), gtk_tree_view_column_new_with_attributes
            ("Name", gtk_cell_renderer_text_new(), "text", 0, NULL));
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree), gtk_tree_view_column_new_with_attributes
            ("Value", gtk_cell_renderer_text_new(), "text", 1, NULL));
        g_signal_connect(tree, "row-activated", G_CALLBACK(hints_row_activated), ui);

        gtk_container_add(GTK_CONTAINER(frame), tree);
        gtk_container_add(toplevel_2ndbox, frame);
    }

    /* Create settings */
    {
        GtkWidget* frame = gtk_frame_new("Options");
        GtkContainer* box = GTK_CONTAINER(gtk_button_box_new(GTK_ORIENTATION_VERTICAL));
        GtkWidget* check;

        check = gtk_check_button_new_with_label("Show unconnected pins");
        g_signal_connect(check, "toggled", G_CALLBACK(showallpins_toggled), ui);
        gtk_container_add(box, check);

        check = gtk_check_button_new_with_label("Set model=auto");
        g_signal_connect(check, "toggled", G_CALLBACK(automodel_toggled), ui);
        gtk_container_add(box, check);

        check = gtk_check_button_new_with_label("Advanced override");
        g_signal_connect(check, "toggled", G_CALLBACK(free_override_toggled), ui);
        gtk_container_add(box, check);

        check = gtk_check_button_new_with_label("Parser hints");
        g_signal_connect(check, "toggled", G_CALLBACK(hints_toggled), ui);
        gtk_container_add(box, check);

        gtk_container_add(GTK_CONTAINER(frame), GTK_WIDGET(box));
        gtk_container_add(rightside_box, frame);
    }

    /* Create bottom right buttons */
    {
        GtkWidget* button;
        GtkWidget* box = gtk_button_box_new(GTK_ORIENTATION_VERTICAL);
        button = gtk_button_new_with_label("Read documentation");
        g_signal_connect(button, "clicked", G_CALLBACK(documentation_clicked), ui);
        gtk_container_add(GTK_CONTAINER(box), button);
        button = gtk_button_new_with_label("Apply now");
        g_signal_connect(button, "clicked", G_CALLBACK(apply_now_clicked), ui);
        gtk_container_add(GTK_CONTAINER(box), button);
        button = gtk_button_new_with_label("Install boot override");
        g_signal_connect(button, "clicked", G_CALLBACK(apply_boot_clicked), ui);
        gtk_container_add(GTK_CONTAINER(box), button);
        button = gtk_button_new_with_label("Remove boot override");
        g_signal_connect(button, "clicked", G_CALLBACK(reset_boot_clicked), ui);
        gtk_container_add(GTK_CONTAINER(box), button);

        gtk_container_add(rightside_box, box);
        gtk_box_set_child_packing(GTK_BOX(rightside_box), box, FALSE, FALSE, 2, GTK_PACK_END);
    }
    
    gtk_container_add(toplevel_2ndbox, GTK_WIDGET(rightside_box));

    gtk_container_add(GTK_CONTAINER(toplevel_box), GTK_WIDGET(toplevel_2ndbox));
    gtk_box_set_child_packing(GTK_BOX(toplevel_box), GTK_WIDGET(toplevel_2ndbox), TRUE, TRUE, 2, GTK_PACK_END);
    gtk_container_add(GTK_CONTAINER(ui->main_window), GTK_WIDGET(toplevel_box));

    return ui;
}

int main(int argc, char *argv[])
{
    ui_data_t* ui;
    gtk_init(&argc, &argv);
    ui = create_ui();
    gtk_widget_show_all(ui->main_window);
    if (ui->codec_selection_combo)
        update_codec_ui(ui, true);
    gtk_main();
    return 0;
}
