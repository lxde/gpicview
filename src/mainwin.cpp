/***************************************************************************
 *   Copyright (C) 2007 by PCMan (Hong Jen Yee)   *
 *   pcman.tw@gmail.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "mainwin.h"
#include <new>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gdk/gdkkeysyms.h>

#include <string.h>
#include <errno.h>

#include "working-area.h"

gpointer MainWin::_parent_class = NULL;

// For drag & drop
GtkTargetEntry drop_targets[] = 
{
    {"text/uri-list", 0, 0},
    {"text/plain", 0, 1}  
};

// Begin of GObject-related stuff

void MainWin::_init( GTypeInstance *instance, gpointer g_class )
{
    ::new( instance ) MainWin();    // placement new
}

GType MainWin::_get_type()
{
    static GType g_define_type_id = 0;
    if (G_UNLIKELY (g_define_type_id == 0))
    {
        static const GTypeInfo g_define_type_info = {
            sizeof (MainWin::Class),
            (GBaseInitFunc) NULL,
            (GBaseFinalizeFunc) NULL,
            (GClassInitFunc) MainWin::_class_init,
            (GClassFinalizeFunc) NULL,
            NULL,   /* class_data */
            sizeof (MainWin),
            0,      /* n_preallocs */
            (GInstanceInitFunc) MainWin::_init,
        };
        g_define_type_id = g_type_register_static (GTK_TYPE_WINDOW, "MainWin", &g_define_type_info, (GTypeFlags)0);
    }
    return g_define_type_id;
}

void MainWin::_class_init( MainWin::Class* klass )
{
    _parent_class = g_type_class_peek_parent (klass);

    GObjectClass * obj_class;
    GtkWidgetClass *widget_class;

    obj_class = ( GObjectClass * ) klass;
//    obj_class->set_property = _set_property;
//   obj_class->get_property = _get_property;
    obj_class->finalize = _finalize;

    widget_class = GTK_WIDGET_CLASS ( klass );
    widget_class->delete_event = on_delete_event;
    widget_class->size_allocate = on_size_allocate;
    widget_class->key_press_event = on_key_press_event;
}

void MainWin::_finalize(GObject *self)
{
    ((MainWin*)self)->~MainWin();
}

// End of GObject-related stuff

MainWin::MainWin()
{
    gtk_window_set_title( (GtkWindow*)this, _("Image Viewer"));
    gtk_window_set_icon_from_file( (GtkWindow*)this, PACKAGE_DATA_DIR"/pixmaps/gpicview.png", NULL );
    gtk_window_set_default_size( (GtkWindow*)this, 640, 480 );

    GtkWidget* box = gtk_vbox_new( FALSE, 0 );
    gtk_container_add( (GtkContainer*)this, box);

    // image area
    evt_box = gtk_event_box_new();
    gtk_widget_add_events( evt_box,
                           GDK_POINTER_MOTION_MASK|GDK_BUTTON_PRESS_MASK|
                           GDK_BUTTON_RELEASE_MASK|GDK_SCROLL_MASK );
    g_signal_connect( evt_box, "button-press-event", G_CALLBACK(on_button_press), this );
    g_signal_connect( evt_box, "button-release-event", G_CALLBACK(on_button_release), this );
    g_signal_connect( evt_box, "motion-notify-event", G_CALLBACK(on_mouse_move), this );

    img_view = gtk_image_new();
    gtk_container_add( (GtkContainer*)evt_box, img_view);

    scroll = gtk_scrolled_window_new( NULL, NULL );
    gtk_scrolled_window_set_shadow_type( (GtkScrolledWindow*)scroll, GTK_SHADOW_NONE );
    gtk_scrolled_window_set_policy((GtkScrolledWindow*)scroll,
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    GtkAdjustment *hadj, *vadj;
    hadj = gtk_scrolled_window_get_hadjustment((GtkScrolledWindow*)scroll);
    hadj->page_increment = 10;
    gtk_adjustment_changed(hadj);
//    g_object_set( hadj, "page-increment", 10, NULL );
    vadj = gtk_scrolled_window_get_vadjustment((GtkScrolledWindow*)scroll);
    vadj->page_increment = 10;
    gtk_adjustment_changed(vadj);
//    g_object_set( vadj, "page-increment", 10, NULL );
    gtk_scrolled_window_add_with_viewport( (GtkScrolledWindow*)scroll, evt_box );

    gtk_box_pack_start( (GtkBox*)box, scroll, TRUE, TRUE, 0 );

    // build toolbar
    tooltips = gtk_tooltips_new();
#if GTK_CHECK_VERSION( 2, 10, 0 )
    g_object_ref_sink( tooltips );
#else
    gtk_object_sink( tooltips );
#endif
    create_nav_bar( box );

    gtk_widget_show_all( box );

    hand_cursor = gdk_cursor_new_for_display( gtk_widget_get_display((GtkWidget*)this), GDK_FLEUR );

    zoom_mode = ZOOM_FIT;

    // Set up drag & drop
    gtk_drag_dest_set( (GtkWidget*)this, GTK_DEST_DEFAULT_ALL, 
                                                    drop_targets, G_N_ELEMENTS(drop_targets), GdkDragAction(GDK_ACTION_COPY | GDK_ACTION_ASK) );
    g_signal_connect( this, "drag-data-received", G_CALLBACK(on_drag_data_received), this );
}

MainWin::~MainWin()
{
    close();
    gdk_cursor_unref( hand_cursor );
    g_object_unref( tooltips );

    // FIXME: Put this here is weird
    gtk_main_quit();
}

void MainWin::create_nav_bar( GtkWidget* box )
{
    nav_bar = gtk_hbox_new( FALSE, 0 );

    add_nav_btn( GTK_STOCK_GO_BACK, _("Previous"), G_CALLBACK(on_prev) );
    add_nav_btn( GTK_STOCK_GO_FORWARD, _("Next"), G_CALLBACK(on_next) );

    gtk_box_pack_start( (GtkBox*)nav_bar, gtk_vseparator_new(), FALSE, FALSE, 0 );

    add_nav_btn( GTK_STOCK_ZOOM_OUT, _("Zoom Out"), G_CALLBACK(on_zoom_out) );
    add_nav_btn( GTK_STOCK_ZOOM_IN, _("Zoom In"), G_CALLBACK(on_zoom_in) );
    btn_fit = add_nav_btn( GTK_STOCK_ZOOM_FIT, _("Fit Image To Window Size"),
                           G_CALLBACK(on_zoom_fit), true );
    btn_orig = add_nav_btn( GTK_STOCK_ZOOM_100, _("Original Size"),
                           G_CALLBACK(on_orig_size), true );

    gtk_toggle_button_set_active( (GtkToggleButton*)btn_fit, TRUE );
#ifndef GTK_STOCK_FULLSCREEN
#define GTK_STOCK_FULLSCREEN    "gtk-fullscreen"
#endif
    add_nav_btn( GTK_STOCK_FULLSCREEN, _(" Full Screen"), G_CALLBACK(on_full_screen) );   // gtk+ 2.8+

    gtk_box_pack_start( (GtkBox*)nav_bar, gtk_vseparator_new(), FALSE, FALSE, 0 );

    add_nav_btn( "gtk-counterclockwise", _("Rotate Counterclockwise"),
                 G_CALLBACK(on_rotate_counterclockwise) );
    add_nav_btn( "gtk-clockwise", _("Rotate Clockwise"), G_CALLBACK(on_rotate_clockwise) );

    gtk_box_pack_start( (GtkBox*)nav_bar, gtk_vseparator_new(), FALSE, FALSE, 0 );

    add_nav_btn( GTK_STOCK_OPEN, _("Open File"), G_CALLBACK(on_open) );
    add_nav_btn( GTK_STOCK_SAVE, _("Save File"), G_CALLBACK(on_save) );
    add_nav_btn( GTK_STOCK_SAVE_AS, _("Save File As"), G_CALLBACK(on_save_as) );
    add_nav_btn( GTK_STOCK_DELETE, _("Delete File"), G_CALLBACK(on_delete) );

/*
    gtk_box_pack_start( (GtkBox*)nav_bar, gtk_vseparator_new(), FALSE, FALSE, 0 );

    add_nav_btn( GTK_STOCK_PREFERENCES, _("Preference"), G_CALLBACK(on_preference) );
*/

    GtkWidget* align = gtk_alignment_new( 0.5, 0, 0, 0 );
    gtk_container_add( (GtkContainer*)align, nav_bar );
    gtk_box_pack_start( (GtkBox*)box, align, FALSE, TRUE, 2 );
}

gboolean MainWin::on_delete_event( GtkWidget* widget, GdkEventAny* evt )
{
    gtk_widget_destroy( widget );
    return TRUE;
}

bool MainWin::open( const char* file_path )
{
    close();
    GError* err = NULL;
    pic_orig = gdk_pixbuf_new_from_file( file_path, &err );
    if( ! pic_orig )
    {
        show_error( err->message );
        return false;
    }

    if( zoom_mode == ZOOM_FIT )
        fit_window_size();
    else  if( zoom_mode == ZOOM_OTHER )  // scale
        scale_image( scale );
    else  if( zoom_mode == ZOOM_ORIG )  // original size
    {
        pic = gdk_pixbuf_ref( pic_orig );
        gtk_image_set_from_pixbuf( (GtkImage*)img_view, pic );
        int w = gdk_pixbuf_get_width( pic );
        int h = gdk_pixbuf_get_height( pic );

/*
        GdkRectangle area;
        get_working_area( gtk_widget_get_screen((GtkWidget*)this), &area );
        if( w < area.width && h < area.height )
        {
            gtk_scrolled_window_set_policy( (GtkScrolledWindow*)scroll,
                                             GTK_POLICY_NEVER, GTK_POLICY_NEVER );
            int space = 0;
            gtk_widget_style_get( scroll, "scrollbar-spacing", &space, NULL );
            space *= 2;

            gtk_window_resize( (GtkWindow*)this, w, h );
            gtk_scrolled_window_set_policy( (GtkScrolledWindow*)scroll,
                                             GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );
        }
        else
        {
            gtk_scrolled_window_set_policy( (GtkScrolledWindow*)scroll,
                                             GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );
            center_image();
        }
*/
        center_image();
    }

//    while (gtk_events_pending ())
//        gtk_main_iteration ();

    // build file list
    char* dir_path = g_path_get_dirname( file_path );
    img_list.open_dir( dir_path );
    g_free( dir_path );
    char* base_name = g_path_get_basename( file_path );
    img_list.set_current( base_name );
    g_free( base_name );

    char* disp_path = g_filename_display_name( file_path );
    gtk_window_set_title( (GtkWindow*)this, disp_path );
    g_free( disp_path );

    return true;
}

void MainWin::close()
{
    if( pic ) {
        gdk_pixbuf_unref( pic );
        pic = NULL;
    }
    if( pic_orig ) {
        gdk_pixbuf_unref( pic_orig );
        pic_orig = NULL;
    }
}

void MainWin::show_error( const char* message )
{
    GtkWidget* dlg = gtk_message_dialog_new( (GtkWindow*)this,
                                              GTK_DIALOG_MODAL,
                                              GTK_MESSAGE_ERROR,
                                              GTK_BUTTONS_OK,
                                              message );
    gtk_dialog_run( (GtkDialog*)dlg );
    gtk_widget_destroy( dlg );
}

void MainWin::on_size_allocate( GtkWidget* widget, GtkAllocation    *allocation )
{
    GTK_WIDGET_CLASS(_parent_class)->size_allocate( widget, allocation );
    MainWin* self = (MainWin*)widget;
    if( self->zoom_mode == ZOOM_FIT )
    {
        while(gtk_events_pending ())
            gtk_main_iteration(); // makes it more fluid

        allocation = &self->scroll->allocation;
        self->fit_window_size();
    }
}

void MainWin::fit_size( int width, int height, GdkInterpType type )
{
    if( ! pic_orig )
        return;

    if( pic ){
        gdk_pixbuf_unref( pic );
        pic = NULL;
    }

    int orig_w = gdk_pixbuf_get_width(pic_orig);
    int orig_h = gdk_pixbuf_get_height(pic_orig);

    double xscale = double(width) / orig_w;
    double yscale = double(height) / orig_h;
    double final_scale = xscale < yscale ? xscale : yscale;

    scale_image( final_scale, type );
}


void MainWin::fit_window_size(  GdkInterpType type )
{
    int space = 0;
    gtk_widget_style_get( scroll, "scrollbar-spacing", &space, NULL );
    space *= 2;
    fit_size( scroll->allocation.width - space, scroll->allocation.height - space, type );
}

GtkWidget* MainWin::add_nav_btn( const char* icon, const char* tip, GCallback cb, bool toggle )
{
    GtkWidget* img = gtk_image_new_from_stock(icon, GTK_ICON_SIZE_SMALL_TOOLBAR);
    GtkWidget* btn;
    if( G_UNLIKELY(toggle) )
    {
        btn = gtk_toggle_button_new();
        g_signal_connect( btn, "toggled", cb, this );
    }
    else
    {
        btn = gtk_button_new();
        g_signal_connect( btn, "clicked", cb, this );
    }
    gtk_button_set_relief( (GtkButton*)btn, GTK_RELIEF_NONE );
    gtk_button_set_focus_on_click( (GtkButton*)btn, FALSE );
    gtk_container_add( (GtkContainer*)btn, img );
    gtk_tooltips_set_tip( tooltips, btn, tip, NULL );
    gtk_box_pack_start( (GtkBox*)nav_bar, btn, FALSE, FALSE, 0 );
    return btn;
}

void MainWin::on_zoom_fit_menu( GtkMenuItem* item, MainWin* self )
{
    gtk_button_clicked( (GtkButton*)self->btn_fit );
}

void MainWin::on_zoom_fit( GtkToggleButton* btn, MainWin* self )
{
    if( ! btn->active )
    {
        if( self->zoom_mode == ZOOM_FIT )
            gtk_toggle_button_set_active( btn, TRUE );
        return;
    }
    self->zoom_mode = ZOOM_FIT;

    gtk_toggle_button_set_active( (GtkToggleButton*)self->btn_orig, FALSE );

    self->fit_window_size();
}

void MainWin::on_full_screen( GtkWidget* btn, MainWin* self )
{
    if( ! self->full_screen )
    {
        gtk_widget_hide( gtk_widget_get_parent(self->nav_bar) );
        gtk_window_fullscreen( (GtkWindow*)self );
    }
    else
    {
        gtk_widget_show( gtk_widget_get_parent(self->nav_bar) );
        gtk_window_unfullscreen( (GtkWindow*)self );
    }
    self->full_screen = ! self->full_screen;
}

void MainWin::on_orig_size_menu( GtkToggleButton* btn, MainWin* self )
{
    gtk_button_clicked( (GtkButton*)self->btn_orig );
}

void MainWin::on_orig_size( GtkToggleButton* btn, MainWin* self )
{
    // This callback could be called from activate signal of menu item.
    if( GTK_IS_MENU_ITEM(btn) )
    {
        gtk_button_clicked( (GtkButton*)self->btn_orig );
        return;
    }

    if( ! btn->active )
    {
        if( self->zoom_mode == ZOOM_ORIG )
            gtk_toggle_button_set_active( btn, TRUE );
        return;
    }
    self->zoom_mode = ZOOM_ORIG;
    self->scale = 1.0;

    gtk_toggle_button_set_active( (GtkToggleButton*)self->btn_fit, FALSE );

    if( ! self->pic_orig )
        return;

    if( self->pic )
        gdk_pixbuf_unref( self->pic );
    self->pic = gdk_pixbuf_ref( self->pic_orig );
    gtk_image_set_from_pixbuf( (GtkImage*)self->img_view, self->pic );

//    self->center_image(); // FIXME:  This doesn't work well. Why?
}

void MainWin::on_prev( GtkWidget* btn, MainWin* self )
{
    if( self->img_list.is_empty() )
        return;

    const char* name = self->img_list.get_prev();

    if( ! name && self->img_list.has_multiple_files() )
    {
        // FIXME: need to ask user first?
        name = self->img_list.get_last();
    }

    if( name )
    {
        char* file_path = self->img_list.get_current_file_path();
        self->open( file_path );
        g_free( file_path );
    }
}

void MainWin::on_next( GtkWidget* btn, MainWin* self )
{
    if( self->img_list.is_empty() )
        return;

    const char* name = self->img_list.get_next();

    if( ! name && self->img_list.has_multiple_files() )
    {
        // FIXME: need to ask user first?
        name = self->img_list.get_first();
    }

    if( name )
    {
        char* file_path = self->img_list.get_current_file_path();
        self->open( file_path );
        g_free( file_path );
    }
}

void MainWin::on_rotate_clockwise( GtkWidget* btn, MainWin* self )
{
    self->rotate_image( GDK_PIXBUF_ROTATE_CLOCKWISE );
}

void MainWin::on_rotate_counterclockwise( GtkWidget* btn, MainWin* self )
{
    self->rotate_image( GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE );
}

static void on_update_preview( GtkFileChooser *chooser, GtkImage* img )
{
    char* file = gtk_file_chooser_get_preview_filename( chooser );
    GdkPixbuf* pix = NULL;
    if( file )
    {
        pix = gdk_pixbuf_new_from_file_at_scale( file, 128, 128, TRUE, NULL );
        g_free( file );
    }
    gtk_image_set_from_pixbuf( img, pix );
    if( pix )
        gdk_pixbuf_unref( pix );
}

void MainWin::on_save_as( GtkWidget* btn, MainWin* self )
{
    if( ! self->pic_orig )
        return;

    GtkFileChooser* dlg = (GtkFileChooser*)gtk_file_chooser_dialog_new( NULL, (GtkWindow*)self,
            GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL,
            GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL );

    gtk_file_chooser_set_current_folder( dlg, self->img_list.get_dir() );

    GtkWidget* img = gtk_image_new();
    gtk_widget_set_size_request( img, 128, 128 );
    gtk_file_chooser_set_preview_widget( dlg, img );
    g_signal_connect( dlg, "update-preview", G_CALLBACK(on_update_preview), img );

    GtkFileFilter *filter;

    /*
    /// TODO: determine file type from file name
    filter = gtk_file_filter_new();
    gtk_file_filter_set_name( filter, _("Determined by File Name") );
    gtk_file_filter_add_pixbuf_formats( filter );
    gtk_file_chooser_add_filter( dlg, filter );
    */

    GSList* modules = gdk_pixbuf_get_formats();
    GSList* module;
    for( module = modules; module; module = module->next )
    {
        GdkPixbufFormat* format = (GdkPixbufFormat*)module->data;
        if( ! gdk_pixbuf_format_is_writable( format ) )
            continue;

        filter = gtk_file_filter_new();

        char* desc = gdk_pixbuf_format_get_description( format );
        char* name = gdk_pixbuf_format_get_name( format );
        char* tmp = g_strjoin( ":  ", name, desc, NULL );
        g_free( desc );
        g_free( name );
        gtk_file_filter_set_name( filter, tmp );
        g_free( tmp );

        char** mimes = gdk_pixbuf_format_get_mime_types( format ), **mime;
        for( mime  = mimes; *mime ; ++mime )
            gtk_file_filter_add_mime_type( filter, *mime );
        g_strfreev( mimes );
        gtk_file_chooser_add_filter( dlg, filter );
    }

    if( gtk_dialog_run( (GtkDialog*)dlg ) == GTK_RESPONSE_OK )
    {
        filter = gtk_file_chooser_get_filter( dlg );
        const char* filter_name = gtk_file_filter_get_name( filter );
        char* p = strstr( filter_name, ": " );
        char* type = NULL;
        if( ! p )   // auto detection
        {
            /// TODO: auto file type
        }
        else
        {
            type = g_strndup( filter_name, (p - filter_name)  );
        }
        char* file = gtk_file_chooser_get_filename( dlg );
        // g_debug("type = %s", type);
        self->save( file, type );
        g_free( file );
        g_free( type );
    }
    gtk_widget_destroy( (GtkWidget*)dlg );
}

void MainWin::on_save( GtkWidget* btn, MainWin* self )
{
    if( ! self->pic_orig )
        return;

    char* file_name = g_build_filename( self->img_list.get_dir(),
                                        self->img_list.get_current(), NULL );
    GdkPixbufFormat* info;
    info = gdk_pixbuf_get_file_info( file_name, NULL, NULL );
    char* type = gdk_pixbuf_format_get_name( info );
    self->save( file_name, type, true );
    g_free( file_name );
    g_free( type );
}

void MainWin::on_open( GtkWidget* btn, MainWin* self )
{
    GtkFileChooser* dlg = (GtkFileChooser*)gtk_file_chooser_dialog_new( NULL, (GtkWindow*)self,
            GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL,
            GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL );

    if( self->img_list.get_dir() )
        gtk_file_chooser_set_current_folder( dlg, self->img_list.get_dir() );

    GtkWidget* img = gtk_image_new();
    gtk_widget_set_size_request( img, 128, 128 );
    gtk_file_chooser_set_preview_widget( dlg, img );
    g_signal_connect( dlg, "update-preview", G_CALLBACK(on_update_preview), img );

    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name( filter, _("All Supported Images") );
    gtk_file_filter_add_pixbuf_formats( filter );
    gtk_file_chooser_add_filter( dlg, filter );

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name( filter, _("All Files") );
    gtk_file_filter_add_pattern( filter, "*" );
    gtk_file_chooser_add_filter( dlg, filter );

    char* file = NULL;
    if( gtk_dialog_run( (GtkDialog*)dlg ) == GTK_RESPONSE_OK )
        file = gtk_file_chooser_get_filename( dlg );
    gtk_widget_destroy( (GtkWidget*)dlg );

    if( file )
    {
        self->open( file );
        g_free( file );
    }
}

void MainWin::on_zoom_in( GtkWidget* btn, MainWin* self )
{
    self->zoom_mode = ZOOM_OTHER;
    gtk_toggle_button_set_active( (GtkToggleButton*)self->btn_fit, FALSE );
    gtk_toggle_button_set_active( (GtkToggleButton*)self->btn_orig, FALSE );

    double scale = self->scale;
    if( self->pic_orig && scale < 3.0 )
    {
//        busy(true);
        (scale > 0.2) ?   scale += scale*0.2 : scale *= 1.5;
        self->scale_image( scale );
//        adjust_adjustment_on_zoom(oldscale);
//        busy(false);
    }
}

void MainWin::on_zoom_out( GtkWidget* btn, MainWin* self )
{
    self->zoom_mode = ZOOM_OTHER;
    gtk_toggle_button_set_active( (GtkToggleButton*)self->btn_fit, FALSE );
    gtk_toggle_button_set_active( (GtkToggleButton*)self->btn_orig, FALSE );

    double scale = self->scale;
    if( self->pic_orig && scale > 0.05 )
    {
//        busy(true);
        (scale > 0.2) ?   scale -= scale*0.2 : scale *= 0.667;
        self->scale_image( scale );
//        adjust_adjustment_on_zoom(oldscale);
//        busy(false);
    }
}

void MainWin::on_preference( GtkWidget* btn, MainWin* self )
{
    self->show_error( "Not implemented yet!" );
}

void MainWin::on_quit( GtkWidget* btn, MainWin* self )
{
    gtk_widget_destroy( (GtkWidget*)self );
}

gboolean MainWin::on_button_press( GtkWidget* widget, GdkEventButton* evt, MainWin* self )
{
    if( evt->type == GDK_BUTTON_PRESS)
    {
        if( evt->button == 1 )    // left button
        {
            if( ! self->pic_orig )
                return FALSE;
            self->dragging = true;
            gtk_widget_get_pointer( (GtkWidget*)self, &self->drag_old_x ,&self->drag_old_y );
            gdk_window_set_cursor( widget->window, self->hand_cursor );
        }
        else if( evt->button == 3 )   // right button
        {
            self->show_popup_menu( evt );
        }
    }
    else if( evt->type == GDK_2BUTTON_PRESS && evt->button == 1 )    // double clicked
    {
         on_full_screen( NULL, self );
    }
    
    return FALSE;
}

gboolean MainWin::on_mouse_move( GtkWidget* widget, GdkEventMotion* evt, MainWin* self )
{
    if( ! self->dragging )
    {
/*
        if( self->full_screen )
        {
            int barh;
            gtk_widget_get_size_request( self->nav_bar, NULL, &barh );
            if( evt->y > ( ((GtkWidget*)self)->allocation.height - barh) )
                gtk_widget_show( self->nav_bar );
            else
                gtk_widget_hide( self->nav_bar );
        }
*/
        return FALSE;
    }
    int cur_x, cur_y;
    gtk_widget_get_pointer( (GtkWidget*)self, &cur_x ,&cur_y );

    int dx = (self->drag_old_x - cur_x);
    int dy = (self->drag_old_y - cur_y);

    GtkAdjustment *hadj, *vadj;
    hadj = gtk_scrolled_window_get_hadjustment((GtkScrolledWindow*)self->scroll);
    vadj = gtk_scrolled_window_get_vadjustment((GtkScrolledWindow*)self->scroll);

    int imgw = gdk_pixbuf_get_width( self->pic );
    int imgh = gdk_pixbuf_get_height( self->pic );

    if( ABS(dx) > 4 )
    {
        self->drag_old_x = cur_x;
        if( imgw > hadj->page_size )
        {
            gdouble x = gtk_adjustment_get_value (hadj) + dx;
            if( x < hadj->lower )
                x = hadj->lower;
            else if( (x + hadj->page_size) > hadj->upper )
                x = hadj->upper - hadj->page_size;

            if( x != hadj->value )
                gtk_adjustment_set_value (hadj, x );
        }
    }

    if( ABS(dy) > 4 )
    {
        if( imgh > vadj->page_size )
        {
            self->drag_old_y = cur_y;
            gdouble y = gtk_adjustment_get_value (vadj) + dy;
            if( y < vadj->lower )
                y = vadj->lower;
            else if( (y + vadj->page_size) > vadj->upper )
                y = vadj->upper - vadj->page_size;

            if( y != vadj->value )
                gtk_adjustment_set_value (vadj, y  );
        }
    }
    return FALSE;
}

gboolean MainWin::on_button_release( GtkWidget* widget, GdkEventButton* evt, MainWin* self )
{
    self->dragging = false;
    gdk_window_set_cursor( widget->window, NULL );
    return FALSE;
}

gboolean MainWin::on_key_press_event(GtkWidget* widget, GdkEventKey * key)
{
    MainWin* self = (MainWin*)widget;
    switch( key->keyval )
    {
        case GDK_Return:
        case GDK_space:
        case GDK_Next:
            on_next( NULL, self );
            break;
        case GDK_Prior:
        case GDK_BackSpace:
            on_prev( NULL, self );
            break;
        case GDK_KP_Add:
        case GDK_plus:
            on_zoom_in( NULL, self );
            break;
        case GDK_KP_Subtract:
        case GDK_minus:
            on_zoom_out( NULL, self );
            break;
        case GDK_Left:
        case GDK_KP_Left:
        case GDK_leftarrow:
            break;
        case GDK_Right:
        case GDK_KP_Right:
        case GDK_rightarrow:
            break;
        case GDK_KP_Down:
        case GDK_Down:
        case GDK_downarrow:
            break;
        case GDK_KP_Up:
        case GDK_Up:
        case GDK_uparrow:
            break;
        case GDK_s:
        case GDK_S:
            on_save( NULL, self );
            break;
        case GDK_l:
        case GDK_L:
            on_rotate_counterclockwise( NULL, self );
            break;
        case GDK_r:
        case GDK_R:
            on_rotate_clockwise( NULL, self );
            break;
        case GDK_f:
        case GDK_F:
            if( self->zoom_mode != ZOOM_FIT )
                gtk_button_clicked((GtkButton*)self->btn_fit );
            break;
        case GDK_g:
        case GDK_G:
            if( self->zoom_mode != ZOOM_ORIG )
                gtk_button_clicked((GtkButton*)self->btn_orig );
            break;
        case GDK_o:
        case GDK_O:
//            on_open_file( self->btn_open, self );
            break;
        case GDK_Escape:
            if( self->full_screen )
                on_full_screen( NULL, self );
            else
                on_quit( NULL, self );
            break;
        case GDK_F11:
            on_full_screen( NULL, self );
            break;

        default:
            GTK_WIDGET_CLASS(_parent_class)->key_press_event( widget, key );
    }
    return FALSE;
}

void MainWin::center_image()
{
    GtkAdjustment *hadj, *vadj;
    hadj = gtk_scrolled_window_get_hadjustment((GtkScrolledWindow*)scroll);
    vadj = gtk_scrolled_window_get_vadjustment((GtkScrolledWindow*)scroll);

    int imgw = gdk_pixbuf_get_width( pic );
    int imgh = gdk_pixbuf_get_height( pic );

    if( imgw > hadj->page_size )
        gtk_adjustment_set_value(hadj, (imgw - hadj->page_size ) / 2 );

    if( imgh > vadj->page_size )
        gtk_adjustment_set_value(vadj, (imgh - vadj->page_size ) / 2 );
}

void MainWin::rotate_image( GdkPixbufRotation angle )
{
    if( ! pic_orig )
        return;

    GdkPixbuf* orig;
    orig = gdk_pixbuf_rotate_simple( pic_orig, angle );
    gdk_pixbuf_unref( pic_orig );
    pic_orig = orig;

    gdk_pixbuf_unref( pic );
    pic = NULL;
    if( zoom_mode == ZOOM_FIT )
        fit_window_size();
    else    // original size
    {
        pic = gdk_pixbuf_ref( pic_orig );
        gtk_image_set_from_pixbuf( (GtkImage*)img_view, pic );
        center_image();
    }
}

bool MainWin::scale_image( double new_scale, GdkInterpType type )
{
    if( G_UNLIKELY( new_scale == 1.0 ) )
    {
        gtk_toggle_button_set_active( (GtkToggleButton*)btn_orig, TRUE );
        return true;
    }

    int orig_w = gdk_pixbuf_get_width(pic_orig);
    int orig_h = gdk_pixbuf_get_height(pic_orig);

    int width = int(orig_w * new_scale);
    int height = int(orig_h * new_scale);

    GdkPixbuf* new_pic = gdk_pixbuf_scale_simple( pic_orig, width, height, type );
    if( G_LIKELY(new_pic) )
    {
        if( pic )
            gdk_pixbuf_unref( pic );
        pic = new_pic;
        gtk_image_set_from_pixbuf( (GtkImage*)img_view, pic );
        scale = new_scale;
        return true;
    }
    return false;
}

bool MainWin::save( const char* file_path, const char* type, bool confirm )
{
    if( ! pic_orig )
        return false;

    if( confirm )   // check existing file
    {
        if( g_file_test( file_path, G_FILE_TEST_EXISTS ) )
        {
            GtkWidget* dlg = gtk_message_dialog_new( (GtkWindow*)this,
                    GTK_DIALOG_MODAL,
                    GTK_MESSAGE_QUESTION,
                    GTK_BUTTONS_YES_NO,
                    _("The file name you selected already exist.\nDo you want to overwrite existing file?\n(Warning: The quality of original image might be lost)") );
            if( gtk_dialog_run( (GtkDialog*)dlg ) != GTK_RESPONSE_YES )
            {
                gtk_widget_destroy( dlg );
                return false;
            }
            gtk_widget_destroy( dlg );
        }
    }

    GError* err = NULL;
    if( ! gdk_pixbuf_save( pic_orig, file_path, type, &err, NULL ) )
    {
        show_error( err->message );
        return false;
    }

    return true;
}

void MainWin::on_delete( GtkWidget* btn, MainWin* self )
{
    char* file_path = self->img_list.get_current_file_path();
    if( file_path )
    {
        GtkWidget* dlg = gtk_message_dialog_new( (GtkWindow*)self,
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_QUESTION,
                GTK_BUTTONS_YES_NO,
                _("Are you sure you want to delete current file?\n\nWarning: Once deleted, the file cannot be recovered.") );
        int resp = gtk_dialog_run( (GtkDialog*)dlg );
        gtk_widget_destroy( dlg );

        if( resp == GTK_RESPONSE_YES )
        {
            g_unlink( file_path );
            if( errno )
                self->show_error( g_strerror(errno) );
            g_free( file_path );
        }
    }
}

void MainWin::show_popup_menu( GdkEventButton* evt )
{
    GtkMenuShell* popup = (GtkMenuShell*)gtk_menu_new();

    GtkWidget *item;
    add_menu_item( popup, _("Previous"), GTK_STOCK_GO_BACK, G_CALLBACK(on_prev) );
    add_menu_item( popup, _("Next"), GTK_STOCK_GO_FORWARD, G_CALLBACK(on_next) );

    gtk_menu_shell_append( popup, gtk_separator_menu_item_new() );

    add_menu_item( popup, _("Zoom Out"), GTK_STOCK_ZOOM_OUT, G_CALLBACK(on_zoom_out) );
    add_menu_item( popup, _("Zoom In"), GTK_STOCK_ZOOM_IN, G_CALLBACK(on_zoom_in) );
    add_menu_item( popup, _("Fit Image To Window Size"), GTK_STOCK_ZOOM_OUT,
                   G_CALLBACK(on_zoom_fit_menu) );
    add_menu_item( popup, _("Original Size"), GTK_STOCK_ZOOM_100,
                   G_CALLBACK(on_orig_size_menu) );

#ifndef GTK_STOCK_FULLSCREEN
// This stock item is only available after gtk+ 2.8
#define GTK_STOCK_FULLSCREEN    "gtk-fullscreen"
#endif
    add_menu_item( popup, _("Full Screen"), GTK_STOCK_FULLSCREEN, G_CALLBACK(on_full_screen) );

    gtk_menu_shell_append( popup, gtk_separator_menu_item_new() );

    add_menu_item( popup, _("Rotate Counterclockwise"), "gtk-counterclockwise",
                   G_CALLBACK(on_rotate_counterclockwise) );
    add_menu_item( popup, _("Rotate Clockwise"), "gtk-clockwise",
                   G_CALLBACK(on_rotate_clockwise) );

    gtk_menu_shell_append( popup, gtk_separator_menu_item_new() );

    add_menu_item( popup, _("Open File"), GTK_STOCK_OPEN, G_CALLBACK(on_open) );
    add_menu_item( popup, _("Save File"), GTK_STOCK_SAVE, G_CALLBACK(on_save) );
    add_menu_item( popup, _("Save As"), GTK_STOCK_SAVE_AS, G_CALLBACK(on_save_as) );
    add_menu_item( popup, _("Delete File"), GTK_STOCK_DELETE, G_CALLBACK(on_delete) );

    gtk_menu_shell_append( popup, gtk_separator_menu_item_new() );

    item = gtk_image_menu_item_new_from_stock( GTK_STOCK_ABOUT, NULL );
    g_signal_connect(item, "activate", G_CALLBACK(on_about), this);
    gtk_menu_shell_append( popup, item );

//    gtk_toggle_button_set_active( (GtkToggleButton*)btn_fit, TRUE );

    gtk_widget_show_all( (GtkWidget*)popup );
    g_signal_connect( popup, "selection-done", G_CALLBACK(gtk_widget_destroy), NULL );
    gtk_menu_popup( (GtkMenu*)popup, NULL, NULL, NULL, NULL, evt->button, evt->time );
}

GtkWidget* MainWin::add_menu_item( GtkMenuShell* menu, const char* label,
                                   const char* icon, GCallback cb, bool toggle )
{
    GtkWidget* item;
    if( G_UNLIKELY(toggle) )
    {
        item = gtk_check_menu_item_new_with_mnemonic( label );
        g_signal_connect( item, "toggled", cb, this );
    }
    else
    {
        if( icon )
        {
            item = gtk_image_menu_item_new_with_mnemonic( label);
            GtkWidget* img = gtk_image_new_from_stock( icon, GTK_ICON_SIZE_MENU );
            gtk_image_menu_item_set_image( (GtkImageMenuItem*)item, img );
        }
        else {
            item = gtk_menu_item_new_with_mnemonic( label );
        }
        g_signal_connect( item, "activate", cb, this );
    }
    gtk_menu_shell_append( (GtkMenuShell*)menu, item );
}

void MainWin::on_about( GtkWidget* menu, MainWin* self )
{
    GtkWidget * about_dlg;
    const gchar *authors[] =
    {
        "洪任諭 Hong Jen Yee <pcman.tw@gmail.com>",
        _(" * Some icons are taken from gimmage"),
        NULL
    };
    /* TRANSLATORS: Replace this string with your names, one name per line. */
    gchar *translators = _( "translator-credits" );

    about_dlg = gtk_about_dialog_new ();
    gtk_container_set_border_width ( GTK_CONTAINER ( about_dlg ), 2 );
    gtk_about_dialog_set_version ( GTK_ABOUT_DIALOG ( about_dlg ), VERSION );
    gtk_about_dialog_set_name ( GTK_ABOUT_DIALOG ( about_dlg ), _( "GPicView" ) );
    gtk_about_dialog_set_copyright ( GTK_ABOUT_DIALOG ( about_dlg ), _( "Copyright (C) 2007" ) );
    gtk_about_dialog_set_comments ( GTK_ABOUT_DIALOG ( about_dlg ), _( "Lightweight image viewer\n\nDeveloped by Hon Jen Yee (PCMan)" ) );
    gtk_about_dialog_set_license ( GTK_ABOUT_DIALOG ( about_dlg ), "GPicView\n\nCopyright (C) 2007 Hong Jen Yee (PCMan)\n\nThis program is free software; you can redistribute it and/or\nmodify it under the terms of the GNU General Public License\nas published by the Free Software Foundation; either version 2\nof the License, or (at your option) any later version.\n\nThis program is distributed in the hope that it will be useful,\nbut WITHOUT ANY WARRANTY; without even the implied warranty of\nMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\nGNU General Public License for more details.\n\nYou should have received a copy of the GNU General Public License\nalong with this program; if not, write to the Free Software\nFoundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA." );
    gtk_about_dialog_set_website ( GTK_ABOUT_DIALOG ( about_dlg ), "http://lxde.sourceforge.net/gpicview/" );
    gtk_about_dialog_set_authors ( GTK_ABOUT_DIALOG ( about_dlg ), authors );
    gtk_about_dialog_set_translator_credits ( GTK_ABOUT_DIALOG ( about_dlg ), translators );
    gtk_window_set_transient_for( GTK_WINDOW( about_dlg ), GTK_WINDOW( self ) );

    gtk_dialog_run( GTK_DIALOG( about_dlg ) );
    gtk_widget_destroy( about_dlg );
}

void MainWin::on_drag_data_received( GtkWidget* widget, GdkDragContext *drag_context, 
                int x, int y, GtkSelectionData* data, guint info, guint time, MainWin* self )
{
    if( ! data || data->length <= 0)
        return;

    // g_debug("drag data receved, info = %d", info);
    char* file = NULL;
    if( info == 0 )    // text/uri-list
    {
        char** uris = gtk_selection_data_get_uris( data );
        if( uris )
        {
            file = g_filename_from_uri(*uris, NULL, NULL);
            g_strfreev( uris );
        }
    }
    else if( info == 1 )    // text/plain
    {
        file = (char*)gtk_selection_data_get_text( data );
    }
    if( file )
    {
        self->open( file );
        g_free( file );
    }
}
