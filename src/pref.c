/*
 *      pref.h
 *
 *      Copyright (C) 2007 PCMan <pcman.tw@gmail.com>
 *      Copyright (C) 2023 Ingo Brückl
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include <stdio.h>
#include "pref.h"
#include "main-win.h"

#define CFG_DIR    "gpicview"
#define CFG_FILE    CFG_DIR"/gpicview.conf"

Pref pref = {0};

static gboolean kf_get_bool(GKeyFile* kf, const char* grp, const char* name, gboolean* ret )
{
    GError* err = NULL;
    gboolean val = g_key_file_get_boolean(kf, grp, name, &err);
    if( G_UNLIKELY(err) )
    {
        g_error_free(err);
        return FALSE;
    }
    if(G_LIKELY(ret))
        *ret = val;
    return TRUE;
}

static int kf_get_int(GKeyFile* kf, const char* grp, const char* name, int* ret )
{
    GError* err = NULL;
    int val = g_key_file_get_integer(kf, grp, name, &err);
    if( G_UNLIKELY(err) )
    {
        g_error_free(err);
        return FALSE;
    }
    if(G_LIKELY(ret))
        *ret = val;
    return TRUE;
}

static gboolean kf_get_int_list (GKeyFile *kf, const gchar *grp, const gchar *name, gint *ret, gsize len )
{
    gint *list;
    gsize i, length;
    GError *error = NULL;

    list = g_key_file_get_integer_list(kf, grp, name, &length, &error);

    if (G_UNLIKELY(error))
    {
        g_error_free(error);
        return FALSE;
    }

    if (G_LIKELY(list))
    {
        for (i = 0; i < len && i < length; i++)
          ret[i] = list[i];

        g_free(list);
        return TRUE;
    }

    return FALSE;
}

void load_preferences()
{
    /* FIXME: GKeyFile is not fast enough.
     *  Need to replace it with our own config loader in the future. */

    GKeyFile* kf;
    char* path;
    char* color;

    /* pref.open_maximized = FALSE; */
    pref.save_window = TRUE;
    pref.show_toolbar = TRUE;
    pref.ask_before_save = TRUE;
    pref.ask_before_delete = TRUE;
    /* pref.auto_save_rotated = FALSE; */
    pref.rotate_exif_only = TRUE;
    pref.bg.red = pref.bg.green = pref.bg.blue = 65535;
    pref.bg_full.red = pref.bg_full.green = pref.bg_full.blue = 0;

    pref.jpg_quality = 90;
    pref.png_compression = 9;

    kf = g_key_file_new();
    path = g_build_filename( g_get_user_config_dir(),  CFG_FILE, NULL );
    if( g_key_file_load_from_file( kf, path, 0, NULL ) )
    {
        kf_get_bool( kf, "General", "open_maximized", &pref.open_maximized );
        kf_get_bool( kf, "General", "save_window", &pref.save_window );
        kf_get_bool( kf, "General", "show_toolbar", &pref.show_toolbar );
        kf_get_bool( kf, "General", "ask_before_save", &pref.ask_before_save );
        kf_get_bool( kf, "General", "ask_before_delete", &pref.ask_before_delete );
        kf_get_bool( kf, "General", "auto_save_rotated", &pref.auto_save_rotated );
        kf_get_bool( kf, "General", "rotate_exif_only", &pref.rotate_exif_only );
        kf_get_int( kf, "General", "slide_delay", &pref.slide_delay );
        kf_get_int( kf, "General", "zoom_factor", &pref.zoom_factor );

        kf_get_int_list( kf, "General", "window", &pref.win_x, 4 );

        kf_get_int( kf, "General", "jpg_quality", &pref.jpg_quality);
        kf_get_int( kf, "General", "png_compression", &pref.png_compression );


        color = g_key_file_get_string(kf, "General", "bg", NULL);
        if( color )
        {
            gdk_color_parse(color, &pref.bg);
            g_free(color);
        }

        color = g_key_file_get_string(kf, "General", "bg_full", NULL);
        if( color )
        {
            gdk_color_parse(color, &pref.bg_full);
            g_free(color);
        }
    }
    g_free( path );
    g_key_file_free( kf );

    if (pref.slide_delay == 0)
        pref.slide_delay = 5;

    if (pref.zoom_factor == 0)
        pref.zoom_factor = 5;

    if (pref.win_w == 0)
        pref.win_w = 640;

    if (pref.win_h == 0)
        pref.win_h = 480;
}

void save_preferences()
{
    FILE* f;
    char* dir = g_build_filename( g_get_user_config_dir(), CFG_DIR, NULL );
    char* path = g_build_filename( g_get_user_config_dir(),  CFG_FILE, NULL );
    if( ! g_file_test( dir, G_FILE_TEST_IS_DIR ) )
    {
        g_mkdir( g_get_user_config_dir(), 0766 );
        g_mkdir( dir, 0766 );
    }
    g_free( dir );

    if(  (f = fopen( path, "w" )) )
    {
        fputs( "[General]\n", f );
        fprintf( f, "open_maximized=%d\n", pref.open_maximized );
        fprintf( f, "save_window=%d\n", pref.save_window );
        fprintf( f, "show_toolbar=%d\n", pref.show_toolbar );
        fprintf( f, "ask_before_save=%d\n", pref.ask_before_save );
        fprintf( f, "ask_before_delete=%d\n", pref.ask_before_delete );
        fprintf( f, "auto_save_rotated=%d\n", pref.auto_save_rotated );
        fprintf( f, "rotate_exif_only=%d\n", pref.rotate_exif_only );
        fprintf( f, "slide_delay=%d\n", pref.slide_delay );
        fprintf( f, "zoom_factor=%d\n", pref.zoom_factor );
        fprintf( f, "bg=#%02x%02x%02x\n", pref.bg.red/256, pref.bg.green/256, pref.bg.blue/256 );
        fprintf( f, "bg_full=#%02x%02x%02x\n", pref.bg_full.red/256, pref.bg_full.green/256, pref.bg_full.blue/256 );

        fprintf( f, "window=%d;%d;%d;%d;\n", pref.win_x, pref.win_y, pref.win_w, pref.win_h);

        fprintf( f, "jpg_quality=%d\n", pref.jpg_quality );
        fprintf( f, "png_compression=%d\n", pref.png_compression );

        fclose( f );
    }
    g_free( path );
}

static void on_set_default( GtkButton* btn, gpointer user_data )
{
    GtkWindow* parent=(GtkWindow*)user_data;
    GtkWidget* dlg=gtk_message_dialog_new_with_markup( parent, 0,
            GTK_MESSAGE_WARNING, GTK_BUTTONS_OK_CANCEL,
            _("GPicView will become the default viewer for all supported image files on your system.\n"
                "(This will be done through \'xdg-mime\' program)\n\n"
                "<b>Are you sure you really want to do this?</b>") );
    if( gtk_dialog_run( (GtkDialog*)dlg ) == GTK_RESPONSE_OK )
    {
        const char cmd[]="xdg-mime default gpicview.desktop application/x-navi-animation image/bmp image/gif image/x-icns image/jpeg image/png image/x-portable-bitmap image/x-portable-graymap image/x-portable-pixmap image/x-quicktime image/svg+xml image/x-tga image/tiff image/vnd.microsoft.icon image/vnd.zbrush.pcx image/x-pcx image/x-win-bitmap image/wmf image/x-wmf image/x-xbitmap image/x-xpixmap";
        g_spawn_command_line_sync( cmd, NULL, NULL, NULL, NULL );
    }
    gtk_widget_destroy( dlg );
}

static void on_set_bg( GtkColorButton* btn, gpointer user_data )
{
    MainWin* parent=(MainWin*)user_data;
    gtk_color_button_get_color(GTK_COLOR_BUTTON(btn), &pref.bg);
    if( !parent->full_screen )
    {
        gtk_widget_modify_bg( parent->evt_box, GTK_STATE_NORMAL, &pref.bg );
        gtk_widget_queue_draw(parent->evt_box );
    }
}

static void on_set_bg_full( GtkColorButton* btn, gpointer user_data )
{
    MainWin* parent=(MainWin*)user_data;
    gtk_color_button_get_color(GTK_COLOR_BUTTON(btn), &pref.bg_full);
    if( parent->full_screen )
    {
        gtk_widget_modify_bg( parent->evt_box, GTK_STATE_NORMAL, &pref.bg_full );
        gtk_widget_queue_draw(parent->evt_box );
    }
}

void edit_preferences( GtkWindow* parent )
{
    GtkWidget *open_maximized_btn, *save_window_btn, *show_toolbar_btn, *auto_save_btn, *ask_before_save_btn, *set_default_btn,
              *rotate_exif_only_btn, *slide_delay_spinner, *zoom_factor_spinner, *ask_before_del_btn, *bg_btn, *bg_full_btn;
    GtkBuilder* builder = gtk_builder_new();
    GtkDialog* dlg;
    gtk_builder_add_from_file(builder, PACKAGE_DATA_DIR "/gpicview/ui/pref-dlg.ui", NULL);
    gint response;

    dlg = (GtkDialog*)gtk_builder_get_object(builder, "dlg");
    gtk_window_set_transient_for((GtkWindow*)dlg, parent);

    open_maximized_btn = (GtkWidget*)gtk_builder_get_object(builder, "open_maximized");
    gtk_toggle_button_set_active( (GtkToggleButton*)open_maximized_btn, pref.open_maximized );

    save_window_btn = (GtkWidget*)gtk_builder_get_object(builder, "save_window");
    gtk_toggle_button_set_active( (GtkToggleButton*)save_window_btn, pref.save_window );

    show_toolbar_btn = (GtkWidget*)gtk_builder_get_object(builder, "show_toolbar");
    gtk_toggle_button_set_active( (GtkToggleButton*)show_toolbar_btn, pref.show_toolbar );

    ask_before_save_btn = (GtkWidget*)gtk_builder_get_object(builder, "ask_before_save");
    gtk_toggle_button_set_active( (GtkToggleButton*)ask_before_save_btn, pref.ask_before_save );

    ask_before_del_btn = (GtkWidget*)gtk_builder_get_object(builder, "ask_before_delete");
    gtk_toggle_button_set_active( (GtkToggleButton*)ask_before_del_btn, pref.ask_before_delete );

    auto_save_btn = (GtkWidget*)gtk_builder_get_object(builder, "auto_save_rotated");
    gtk_toggle_button_set_active( (GtkToggleButton*)auto_save_btn, pref.auto_save_rotated );

    rotate_exif_only_btn = (GtkWidget*)gtk_builder_get_object(builder, "rotate_exif_only");
    gtk_toggle_button_set_active( (GtkToggleButton*)rotate_exif_only_btn, pref.rotate_exif_only );

    slide_delay_spinner = (GtkWidget*)gtk_builder_get_object(builder, "slide_delay");
    gtk_spin_button_set_value( (GtkSpinButton*)slide_delay_spinner, pref.slide_delay) ;

    zoom_factor_spinner = (GtkWidget*)gtk_builder_get_object(builder, "zoom_factor");
    gtk_spin_button_set_value( (GtkSpinButton*)zoom_factor_spinner, pref.zoom_factor) ;

    set_default_btn = (GtkWidget*)gtk_builder_get_object(builder, "make_default");
    g_signal_connect( set_default_btn, "clicked", G_CALLBACK(on_set_default), parent );

    bg_btn = (GtkWidget*)gtk_builder_get_object(builder, "bg");
    gtk_color_button_set_color(GTK_COLOR_BUTTON(bg_btn), &pref.bg);
    g_signal_connect( bg_btn, "color-set", G_CALLBACK(on_set_bg), parent );

    bg_full_btn = (GtkWidget*)gtk_builder_get_object(builder, "bg_full");
    gtk_color_button_set_color(GTK_COLOR_BUTTON(bg_full_btn), &pref.bg_full);
    g_signal_connect( bg_full_btn, "color-set", G_CALLBACK(on_set_bg_full), parent );

    g_object_unref( builder );

    response = gtk_dialog_run( dlg );

    if (response == GTK_RESPONSE_OK)
    {
        pref.open_maximized = gtk_toggle_button_get_active( (GtkToggleButton*)open_maximized_btn );
        pref.save_window = gtk_toggle_button_get_active( (GtkToggleButton*)save_window_btn );
        pref.show_toolbar = gtk_toggle_button_get_active( (GtkToggleButton*)show_toolbar_btn );
        pref.ask_before_save = gtk_toggle_button_get_active( (GtkToggleButton*)ask_before_save_btn );
        pref.ask_before_delete = gtk_toggle_button_get_active( (GtkToggleButton*)ask_before_del_btn );
        pref.auto_save_rotated = gtk_toggle_button_get_active( (GtkToggleButton*)auto_save_btn );
        pref.rotate_exif_only = gtk_toggle_button_get_active( (GtkToggleButton*)rotate_exif_only_btn );
        pref.slide_delay = gtk_spin_button_get_value_as_int( (GtkSpinButton*)slide_delay_spinner );
        pref.zoom_factor = gtk_spin_button_get_value_as_int( (GtkSpinButton*)zoom_factor_spinner );
    }

    gtk_widget_destroy( (GtkWidget*)dlg );
}
