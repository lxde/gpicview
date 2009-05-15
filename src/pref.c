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
    GError *error;
    char* path;

    pref.auto_save_rotated = FALSE;
    pref.ask_before_save = TRUE;
    pref.ask_before_delete = TRUE;
    pref.open_maximized = FALSE;
    pref.background_color.pixel = 0;
    pref.background_color.red = 65535;
    pref.background_color.green = 65535;
    pref.background_color.blue = 65535;

    kf = g_key_file_new();
    path = g_build_filename( g_get_user_config_dir(),  CFG_FILE, NULL );
    if( g_key_file_load_from_file( kf, path, 0, NULL ) )
    {
        error = NULL;
        pref.auto_save_rotated = g_key_file_get_boolean( kf, "General", "auto_save_rotated", &error );
	if ( error != NULL )  // checking to see if the auto_save_rotated preference is in the configuration file
	{
            pref.auto_save_rotated = FALSE;
        }

        error = NULL;
        pref.ask_before_save = g_key_file_get_boolean( kf, "General", "ask_before_save", &error );
	if ( error != NULL )  // checking to see if the ask_before_save preference is in the configuration file
        {
            pref.ask_before_save = TRUE;
        }

        error = NULL;
        pref.ask_before_delete = g_key_file_get_boolean( kf, "General", "ask_before_delete", &error );	
	if ( error != NULL )  // checking to see if the ask_before_delete preference is in the configuration file
        {
            pref.ask_before_delete = TRUE;
        }

        error = NULL;
        pref.open_maximized = g_key_file_get_boolean( kf, "General", "open_maximized", &error );
	if ( error != NULL )  // checking to see if the should_maximize preference is in the configuration file
        {
            pref.open_maximized = FALSE;
        }

        error = NULL;
        gint *colors = g_key_file_get_integer_list( kf, "General", "background_color", NULL, &error );
        if ( error != NULL ) // checking to see if the background_color preference is in the configuration file
        {
            pref.background_color.pixel = 0;
            pref.background_color.red = 65535;
            pref.background_color.green = 65535;
            pref.background_color.blue = 65535;
        }
        else
        {
            pref.background_color.pixel = colors[0];
            pref.background_color.red = colors[1];
            pref.background_color.green = colors[2];
            pref.background_color.blue = colors[3];
        }
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
        fprintf( f, "ask_before_delete=%d\n", pref.ask_before_delete );
        fprintf( f, "open_maximized=%d\n", pref.open_maximized );
        fprintf( f, "background_color=%d;%d;%d;%d\n", pref.background_color.pixel, pref.background_color.red, 
                                                    pref.background_color.green, pref.background_color.blue );
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

void edit_preferences( MainWin* parent )
{
    GtkWidget *auto_save_btn, *ask_before_save_btn, *ask_before_delete_btn, *set_default_btn;
    GtkWidget *background_color_non_fullscreen, *background_color_fullscreen;
    GtkWidget *background_color_non_fullscreen_label, *background_color_fullscreen_label;
    GtkWidget *background_color_non_fullscreen_btn, *background_color_fullscreen_btn;

    GtkDialog* dlg = (GtkDialog*)gtk_dialog_new_with_buttons( _("Preferences"), GTK_WINDOW(parent), GTK_DIALOG_MODAL,
                                                               GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, NULL );
    GtkWidget* content_area = gtk_dialog_get_content_area( (GtkDialog*)dlg );
    gtk_box_set_spacing( GTK_BOX(content_area), 10 );

    ask_before_save_btn = gtk_check_button_new_with_label( _("Ask before saving images") );
    gtk_toggle_button_set_active( (GtkToggleButton*)ask_before_save_btn, pref.ask_before_save );
    gtk_container_add( GTK_CONTAINER(content_area), ask_before_save_btn );

    ask_before_delete_btn = gtk_check_button_new_with_label( _("Ask before deleting images") );
    gtk_toggle_button_set_active( (GtkToggleButton*)ask_before_delete_btn, pref.ask_before_delete );
    gtk_container_add( GTK_CONTAINER(content_area), ask_before_delete_btn );

    auto_save_btn = gtk_check_button_new_with_label( _("Automatically save rotated images") );
    gtk_toggle_button_set_active( (GtkToggleButton*)auto_save_btn, pref.auto_save_rotated );
    gtk_container_add( GTK_CONTAINER(content_area), auto_save_btn );

    background_color_non_fullscreen = gtk_hbox_new( FALSE, 5 );
    background_color_non_fullscreen_label = gtk_label_new( "Select Background Color:" );
    background_color_non_fullscreen_btn = gtk_color_button_new_with_color( &pref.background_color );
    gtk_widget_set_size_request( background_color_non_fullscreen_btn, 15, 50 );

    //TODO: currently the function redraw_background will not actually redraw the background
    //TODO: fix this
    //g_signal_connect( background_color_non_fullscreen_btn, "color-set", G_CALLBACK(redraw_background), parent );

    gtk_container_add( GTK_CONTAINER(background_color_non_fullscreen), background_color_non_fullscreen_label );
    gtk_container_add( GTK_CONTAINER(background_color_non_fullscreen), background_color_non_fullscreen_btn );

    gtk_container_add( GTK_CONTAINER(content_area), background_color_non_fullscreen );
    
    set_default_btn = gtk_button_new_with_label( _("Make GPicView the default viewer for images") );
    g_signal_connect( set_default_btn, "clicked", G_CALLBACK(on_set_default), parent );
    gtk_container_add( GTK_CONTAINER(content_area), set_default_btn );

    gtk_widget_show_all( GTK_WIDGET(dlg) );
    gtk_dialog_run( dlg );

    pref.ask_before_save = gtk_toggle_button_get_active( (GtkToggleButton*)ask_before_save_btn );
    pref.ask_before_delete = gtk_toggle_button_get_active( (GtkToggleButton*)ask_before_delete_btn );
    pref.auto_save_rotated = gtk_toggle_button_get_active( (GtkToggleButton*)auto_save_btn );
    gtk_color_button_get_color( GTK_COLOR_BUTTON(background_color_non_fullscreen_btn), &pref.background_color );

    gtk_widget_destroy( (GtkWidget*)dlg );

    gtk_widget_modify_bg( parent->evt_box, GTK_STATE_NORMAL, &pref.background_color );
    gtk_widget_draw( parent->evt_box, NULL );
}

    //TODO: currently the function redraw_background will not actually redraw the background
    //TODO: fix this
void redraw_background( GtkColorButton *widget, gpointer user_data )
{printf("\nhere\n");
    MainWin* parent = (MainWin*)user_data;
    gtk_widget_modify_bg( (GtkWidget*)parent->evt_box, GTK_STATE_NORMAL, &pref.background_color );

    gtk_widget_draw( (GtkWidget*)parent->evt_box, NULL );
}

