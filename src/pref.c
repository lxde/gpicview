/*
 *      pref.h
 *
 *      Copyright (C) 2007 PCMan <pcman.tw@gmail.com>
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

#define CFG_DIR    "gpicview"
#define CFG_FILE    CFG_DIR"/gpicview.conf"

 Pref pref = {0};

void load_preferences()
{
    /* FIXME: GKeyFile is not fast enough.
     *  Need to replace it with our own config loader in the future. */

    GKeyFile* kf;
    char* path;

    pref.auto_save_rotated = TRUE;
    pref.ask_before_save = TRUE;

    kf = g_key_file_new();
    path = g_build_filename( g_get_user_config_dir(),  CFG_FILE, NULL );
    if( g_key_file_load_from_file( kf, path, 0, NULL ) )
    {
        pref.auto_save_rotated = g_key_file_get_boolean( kf, "General", "auto_save_rotated", NULL );
        pref.ask_before_save = g_key_file_get_boolean( kf, "General", "ask_before_save", NULL );
        pref.rotate_exif_only = g_key_file_get_boolean( kf, "General", "rotate_exif_only", NULL );
    }
    g_free( path );
    g_key_file_free( kf );
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
        fprintf( f, "auto_save_rotated=%d\n", pref.auto_save_rotated );
        fprintf( f, "ask_before_save=%d\n", pref.ask_before_save );
        fprintf( f, "rotate_exif_only=%d\n", pref.rotate_exif_only );
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
        const char cmd[]="xdg-mime default gpicview.desktop image/bmp image/gif image/jpeg image/jpg image/png image/tiff image/x-bmp image/x-pcx image/x-tga image/x-portable-pixmap image/x-portable-bitmap image/x-targa image/x-portable-greymap application/pcx image/svg+xml image/svg-xml";
        g_spawn_command_line_sync( cmd, NULL, NULL, NULL, NULL );
    }
    gtk_widget_destroy( dlg );
}

void edit_preferences( GtkWindow* parent )
{
    GtkWidget *auto_save_btn, *ask_before_save_btn, *set_default_btn,
              *rotate_exif_only_btn;
    GtkDialog* dlg = (GtkDialog*)gtk_dialog_new_with_buttons( _("Preferences"), parent, GTK_DIALOG_MODAL,
                                                               GTK_STOCK_CLOSE ,GTK_RESPONSE_CLOSE, NULL );

    ask_before_save_btn = gtk_check_button_new_with_label( _("Ask before saving  images") );
    gtk_toggle_button_set_active( (GtkToggleButton*)ask_before_save_btn, pref.ask_before_save );
    gtk_box_pack_start( (GtkBox*)dlg->vbox, ask_before_save_btn, FALSE, FALSE, 2 );

    auto_save_btn = gtk_check_button_new_with_label( _("Automatically save rotated images ( Currently only JPEG is supported )") );
    gtk_toggle_button_set_active( (GtkToggleButton*)auto_save_btn, pref.auto_save_rotated );
    gtk_box_pack_start( (GtkBox*)dlg->vbox, auto_save_btn, FALSE, FALSE, 2 );
    
    rotate_exif_only_btn = gtk_check_button_new_with_label( _("Change EXIF Oritation value only while rotating and fliping if EXIF Oritation tag exist ( JPEG only )") );
    gtk_toggle_button_set_active( (GtkToggleButton*)rotate_exif_only_btn, pref.rotate_exif_only );
    gtk_box_pack_start( (GtkBox*)dlg->vbox, rotate_exif_only_btn, FALSE, FALSE, 2 );

    set_default_btn = gtk_button_new_with_label( _("Make GPicview the default viewer for images") );
    g_signal_connect( set_default_btn, "clicked", G_CALLBACK(on_set_default), parent );
    gtk_box_pack_start( (GtkBox*)dlg->vbox, set_default_btn, FALSE, FALSE, 2 );

    gtk_widget_show_all( (GtkWidget*)dlg->vbox );
    gtk_dialog_run( dlg );

    pref.ask_before_save = gtk_toggle_button_get_active( (GtkToggleButton*)ask_before_save_btn );
    pref.auto_save_rotated = gtk_toggle_button_get_active( (GtkToggleButton*)auto_save_btn );
    pref.rotate_exif_only = gtk_toggle_button_get_active( (GtkToggleButton*)rotate_exif_only_btn );

    gtk_widget_destroy( (GtkWidget*)dlg );
}
