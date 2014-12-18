/***************************************************************************
 *   Copyright (C) 2007, 2008 by PCMan (Hong Jen Yee)                      *
 *   pcman.tw@gmail.com                                                    *
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

#include "main-win.h"

#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkkeysyms-compat.h>

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include "pref.h"

#include "image-view.h"
#include "image-list.h"
#include "working-area.h"
#include "ptk-menu.h"
#include "file-dlgs.h"
#include "jpeg-tran.h"

/* For drag & drop */
static GtkTargetEntry drop_targets[] =
{
    {"text/uri-list", 0, 0},
    {"text/plain", 0, 1}
};

extern int ExifRotate(const char * fname, int new_angle);
// defined in exif.c
extern int ExifRotateFlipMapping[9][9];

static void main_win_init( MainWin*mw );
static void main_win_finalize( GObject* obj );

static void create_nav_bar( MainWin* mw, GtkWidget* box);
GtkWidget* add_nav_btn( MainWin* mw, const char* icon, const char* tip, GCallback cb, gboolean toggle);
GtkWidget* add_nav_btn_img( MainWin* mw, const char* icon, const char* tip, GCallback cb, gboolean toggle, GtkWidget** ret_img);
// GtkWidget* add_menu_item(  GtkMenuShell* menu, const char* label, const char* icon, GCallback cb, gboolean toggle=FALSE );
static void rotate_image( MainWin* mw, int angle );
static void show_popup_menu( MainWin* mw, GdkEventButton* evt );

/* signal handlers */
static gboolean on_delete_event( GtkWidget* widget, GdkEventAny* evt );
static void on_size_allocate( GtkWidget* widget, GtkAllocation    *allocation );
static gboolean on_win_state_event( GtkWidget* widget, GdkEventWindowState* state );
static void on_scroll_size_allocate(GtkWidget* widget, GtkAllocation* allocation, MainWin* mv);
static void on_zoom_fit( GtkToggleButton* btn, MainWin* mw );
static void on_zoom_fit_menu( GtkMenuItem* item, MainWin* mw );
static void on_full_screen( GtkWidget* btn, MainWin* mw );
static void on_next( GtkWidget* btn, MainWin* mw );
static void on_orig_size( GtkToggleButton* btn, MainWin* mw );
static void on_orig_size_menu( GtkToggleButton* btn, MainWin* mw );
static void on_prev( GtkWidget* btn, MainWin* mw );
static void on_rotate_auto_save( GtkWidget* btn, MainWin* mw );
static void on_rotate_clockwise( GtkWidget* btn, MainWin* mw );
static void on_rotate_counterclockwise( GtkWidget* btn, MainWin* mw );
static void on_save_as( GtkWidget* btn, MainWin* mw );
static void on_save( GtkWidget* btn, MainWin* mw );
static void cancel_slideshow(MainWin* mw);
static gboolean next_slide(MainWin* mw);
static void on_slideshow_menu( GtkMenuItem* item, MainWin* mw );
static void on_slideshow( GtkToggleButton* btn, MainWin* mw );
static void on_open( GtkWidget* btn, MainWin* mw );
static void on_zoom_in( GtkWidget* btn, MainWin* mw );
static void on_zoom_out( GtkWidget* btn, MainWin* mw );
static void on_preference( GtkWidget* btn, MainWin* mw );
static void on_toggle_toolbar( GtkMenuItem* item, MainWin* mw );
static void on_quit( GtkWidget* btn, MainWin* mw );
static gboolean on_button_press( GtkWidget* widget, GdkEventButton* evt, MainWin* mw );
static gboolean on_button_release( GtkWidget* widget, GdkEventButton* evt, MainWin* mw );
static gboolean on_mouse_move( GtkWidget* widget, GdkEventMotion* evt, MainWin* mw );
static gboolean on_scroll_event( GtkWidget* widget, GdkEventScroll* evt, MainWin* mw );
static gboolean on_key_press_event(GtkWidget* widget, GdkEventKey * key);
static gboolean save_confirm( MainWin* mw, const char* file_path );
static void on_drag_data_received( GtkWidget* widget, GdkDragContext *drag_context,
                int x, int y, GtkSelectionData* data, guint info, guint time, MainWin* mw );
static void on_delete( GtkWidget* btn, MainWin* mw );
static void on_about( GtkWidget* menu, MainWin* mw );
static gboolean on_animation_timeout( MainWin* mw );

static void update_title(const char *filename, MainWin *mw );

void on_flip_vertical( GtkWidget* btn, MainWin* mw );
void on_flip_horizontal( GtkWidget* btn, MainWin* mw );
static int trans_angle_to_id(int i);
static int get_new_angle( int orig_angle, int rotate_angle );

static void main_win_set_zoom_scale(MainWin* mw, double scale);
static void main_win_set_zoom_mode(MainWin* mw, ZoomMode mode);
static void main_win_update_zoom_buttons_state(MainWin* mw);

// Begin of GObject-related stuff

G_DEFINE_TYPE( MainWin, main_win, GTK_TYPE_WINDOW )

void main_win_class_init( MainWinClass* klass )
{
    GObjectClass * obj_class;
    GtkWidgetClass *widget_class;

    obj_class = ( GObjectClass * ) klass;
//    obj_class->set_property = _set_property;
//   obj_class->get_property = _get_property;
    obj_class->finalize = main_win_finalize;

    widget_class = GTK_WIDGET_CLASS ( klass );
    widget_class->delete_event = on_delete_event;
    widget_class->size_allocate = on_size_allocate;
    widget_class->key_press_event = on_key_press_event;
    widget_class->window_state_event = on_win_state_event;
}

void main_win_finalize( GObject* obj )
{
    MainWin *mw = (MainWin*)obj;

    main_win_close(mw);

    if( G_LIKELY(mw->img_list) )
        image_list_free( mw->img_list );
#if GTK_CHECK_VERSION(3, 0, 0)
    g_object_unref( mw->hand_cursor );
#else
    gdk_cursor_unref( mw->hand_cursor );
#endif
    // FIXME: Put this here is weird
    gtk_main_quit();
}

GtkWidget* main_win_new()
{
    return (GtkWidget*)g_object_new ( MAIN_WIN_TYPE, NULL );
}

// End of GObject-related stuff

void main_win_init( MainWin*mw )
{
    gtk_window_set_title( (GtkWindow*)mw, _("Image Viewer"));
    if (gtk_icon_theme_has_icon(gtk_icon_theme_get_default(), "gpicview"))
    {
        gtk_window_set_icon_name((GtkWindow*)mw, "gpicview");
    }
    else
    {
        gtk_window_set_icon_from_file((GtkWindow*)mw, PACKAGE_DATA_DIR "/icons/hicolor/48x48/apps/gpicview.png", NULL);
    }
    gtk_window_set_default_size( (GtkWindow*)mw, 640, 480 );

#if GTK_CHECK_VERSION(3, 0, 0)
    GtkWidget* box = gtk_box_new( GTK_ORIENTATION_VERTICAL, 0 );
#else
    GtkWidget* box = gtk_vbox_new( FALSE, 0 );
#endif
    gtk_container_add( (GtkContainer*)mw, box);

    // image area
    mw->evt_box = gtk_event_box_new();
#if GTK_CHECK_VERSION(2, 18, 0)
    gtk_widget_set_can_focus(mw->evt_box,TRUE);
#else
    GTK_WIDGET_SET_FLAGS( mw->evt_box, GTK_CAN_FOCUS );
#endif
    gtk_widget_add_events( mw->evt_box,
                           GDK_POINTER_MOTION_MASK|GDK_BUTTON_PRESS_MASK|
                           GDK_BUTTON_RELEASE_MASK|GDK_SCROLL_MASK );
    g_signal_connect( mw->evt_box, "button-press-event", G_CALLBACK(on_button_press), mw );
    g_signal_connect( mw->evt_box, "button-release-event", G_CALLBACK(on_button_release), mw );
    g_signal_connect( mw->evt_box, "motion-notify-event", G_CALLBACK(on_mouse_move), mw );
    g_signal_connect( mw->evt_box, "scroll-event", G_CALLBACK(on_scroll_event), mw );
    // Set bg color to white

    gtk_widget_modify_bg( mw->evt_box, GTK_STATE_NORMAL, &pref.bg );

    mw->img_view = image_view_new();
    gtk_container_add( (GtkContainer*)mw->evt_box, (GtkWidget*)mw->img_view);

#if GTK_CHECK_VERSION(3, 0, 0)
#else
    const char scroll_style[]=
            "style \"gpicview-scroll\" {"
            "GtkScrolledWindow::scrollbar-spacing=0"
            "}"
            "class \"GtkScrolledWindow\" style \"gpicview-scroll\"";
    gtk_rc_parse_string( scroll_style );
#endif
    mw->scroll = gtk_scrolled_window_new( NULL, NULL );
    g_signal_connect(G_OBJECT(mw->scroll), "size-allocate", G_CALLBACK(on_scroll_size_allocate), (gpointer) mw);
    gtk_scrolled_window_set_shadow_type( (GtkScrolledWindow*)mw->scroll, GTK_SHADOW_NONE );
    gtk_scrolled_window_set_policy((GtkScrolledWindow*)mw->scroll,
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    GtkAdjustment *hadj, *vadj;
    hadj = gtk_scrolled_window_get_hadjustment((GtkScrolledWindow*)mw->scroll);
#if GTK_CHECK_VERSION(2, 14, 0)
    gtk_adjustment_set_page_increment(hadj, 10);
#else
    hadj->page_increment = 10;
#endif
    gtk_adjustment_changed(hadj);
    vadj = gtk_scrolled_window_get_vadjustment((GtkScrolledWindow*)mw->scroll);
#if GTK_CHECK_VERSION(2, 14, 0)
    gtk_adjustment_set_page_increment(vadj, 10);
#else
    vadj->page_increment = 10;
#endif
    gtk_adjustment_changed(vadj);

    image_view_set_adjustments( IMAGE_VIEW(mw->img_view), hadj, vadj );    // dirty hack :-(
    gtk_scrolled_window_add_with_viewport( (GtkScrolledWindow*)mw->scroll, mw->evt_box );
    GtkWidget* viewport = gtk_bin_get_child( (GtkBin*)mw->scroll );
    gtk_viewport_set_shadow_type( (GtkViewport*)viewport, GTK_SHADOW_NONE );
    gtk_container_set_border_width( (GtkContainer*)viewport, 0 );

    gtk_box_pack_start( (GtkBox*)box, mw->scroll, TRUE, TRUE, 0 );

    // build toolbar
    create_nav_bar( mw, box );
    gtk_widget_show_all( box );

    if (pref.show_toolbar)
        gtk_widget_show(gtk_widget_get_parent(mw->nav_bar));
    else
        gtk_widget_hide(gtk_widget_get_parent(mw->nav_bar));


    mw->hand_cursor = gdk_cursor_new_for_display( gtk_widget_get_display((GtkWidget*)mw), GDK_FLEUR );

//    zoom_mode = ZOOM_NONE;
    mw->zoom_mode = ZOOM_FIT;

    // Set up drag & drop
    gtk_drag_dest_set( (GtkWidget*)mw, GTK_DEST_DEFAULT_ALL,
                                                    drop_targets,
                                                    G_N_ELEMENTS(drop_targets),
                                                    GDK_ACTION_COPY | GDK_ACTION_ASK );
    g_signal_connect( mw, "drag-data-received", G_CALLBACK(on_drag_data_received), mw );

    mw->img_list = image_list_new();

    // rotation angle is zero on startup
    mw->rotation_angle = 0;
}

void create_nav_bar( MainWin* mw, GtkWidget* box )
{
    mw->nav_bar = gtk_hbox_new( FALSE, 0 );

    add_nav_btn( mw, GTK_STOCK_GO_BACK, _("Previous"), G_CALLBACK(on_prev), FALSE );
    add_nav_btn( mw, GTK_STOCK_GO_FORWARD, _("Next"), G_CALLBACK(on_next), FALSE );
    mw->btn_play_stop = add_nav_btn_img( mw, GTK_STOCK_MEDIA_PLAY, _("Start Slideshow"), G_CALLBACK(on_slideshow), TRUE, &mw->img_play_stop );

    gtk_box_pack_start( (GtkBox*)mw->nav_bar, gtk_vseparator_new(), FALSE, FALSE, 0 );

    add_nav_btn( mw, GTK_STOCK_ZOOM_OUT, _("Zoom Out"), G_CALLBACK(on_zoom_out), FALSE );
    add_nav_btn( mw, GTK_STOCK_ZOOM_IN, _("Zoom In"), G_CALLBACK(on_zoom_in), FALSE );

//    percent = gtk_entry_new();    // show scale (in percentage)
//    g_signal_connect( percent, "activate", G_CALLBACK(on_percentage), mw );
//    gtk_widget_set_size_request( percent, 45, -1 );
//    gtk_box_pack_start( (GtkBox*)nav_bar, percent, FALSE, FALSE, 2 );

    mw->btn_fit = add_nav_btn( mw, GTK_STOCK_ZOOM_FIT, _("Fit Image To Window Size"),
                           G_CALLBACK(on_zoom_fit), TRUE );
    mw->btn_orig = add_nav_btn( mw, GTK_STOCK_ZOOM_100, _("Original Size"),
                           G_CALLBACK(on_orig_size), TRUE );
    gtk_toggle_button_set_active( (GtkToggleButton*)mw->btn_fit, TRUE );

#ifndef GTK_STOCK_FULLSCREEN
#define GTK_STOCK_FULLSCREEN    "gtk-fullscreen"
#endif
    add_nav_btn( mw, GTK_STOCK_FULLSCREEN, _("Full Screen"), G_CALLBACK(on_full_screen), FALSE );   // gtk+ 2.8+

    gtk_box_pack_start( (GtkBox*)mw->nav_bar, gtk_vseparator_new(), FALSE, FALSE, 0 );

    mw->btn_rotate_ccw = add_nav_btn( mw, "object-rotate-left", _("Rotate Counterclockwise"), G_CALLBACK(on_rotate_counterclockwise), FALSE );
    mw->btn_rotate_cw = add_nav_btn( mw, "object-rotate-right", _("Rotate Clockwise"), G_CALLBACK(on_rotate_clockwise), FALSE );

    mw->btn_flip_h = add_nav_btn( mw, "object-flip-horizontal", _("Flip Horizontal"), G_CALLBACK(on_flip_horizontal), FALSE );
    mw->btn_flip_v = add_nav_btn( mw, "object-flip-vertical", _("Flip Vertical"), G_CALLBACK(on_flip_vertical), FALSE );

    gtk_box_pack_start( (GtkBox*)mw->nav_bar, gtk_vseparator_new(), FALSE, FALSE, 0 );

    add_nav_btn( mw, GTK_STOCK_OPEN, _("Open File"), G_CALLBACK(on_open), FALSE );
    add_nav_btn( mw, GTK_STOCK_SAVE, _("Save File"), G_CALLBACK(on_save), FALSE );
    add_nav_btn( mw, GTK_STOCK_SAVE_AS, _("Save File As"), G_CALLBACK(on_save_as), FALSE );
    add_nav_btn( mw, GTK_STOCK_DELETE, _("Delete File"), G_CALLBACK(on_delete), FALSE );

    gtk_box_pack_start( (GtkBox*)mw->nav_bar, gtk_vseparator_new(), FALSE, FALSE, 0 );
    add_nav_btn( mw, GTK_STOCK_PREFERENCES, _("Preferences"), G_CALLBACK(on_preference), FALSE );
    add_nav_btn( mw, GTK_STOCK_QUIT, _("Quit"), G_CALLBACK(on_quit), FALSE );

    GtkWidget* align = gtk_alignment_new( 0.5, 0, 0, 0 );
    gtk_container_add( (GtkContainer*)align, mw->nav_bar );
    gtk_box_pack_start( (GtkBox*)box, align, FALSE, TRUE, 2 );
}

gboolean on_delete_event( GtkWidget* widget, GdkEventAny* evt )
{
    gtk_widget_destroy( widget );
    return TRUE;
}

static void update_title(const char *filename, MainWin *mw )
{
    static char fname[50];
    static int wid, hei;

    char buf[100];

    if(filename != NULL)
    {
      strncpy(fname, filename, 49);
      fname[49] = '\0';

      wid = gdk_pixbuf_get_width( mw->pix );
      hei = gdk_pixbuf_get_height( mw->pix );
    }

    snprintf(buf, 100, "%s (%dx%d) %d%%", fname, wid, hei, (int)(mw->scale * 100));
    gtk_window_set_title( (GtkWindow*)mw, buf );

    return;
}

gboolean on_animation_timeout( MainWin* mw )
{
    int delay;
    if ( gdk_pixbuf_animation_iter_advance( mw->animation_iter, NULL ) )
    {
        mw->pix = gdk_pixbuf_animation_iter_get_pixbuf( mw->animation_iter );
        image_view_set_pixbuf( (ImageView*)mw->img_view, mw->pix );
    }
    delay = gdk_pixbuf_animation_iter_get_delay_time( mw->animation_iter );
    mw->animation_timeout = g_timeout_add(delay, (GSourceFunc) on_animation_timeout, mw );
    return FALSE;
}

static void update_btns(MainWin* mw)
{
    gboolean enable = (mw->animation == NULL);
    gtk_widget_set_sensitive(mw->btn_rotate_cw, enable);
    gtk_widget_set_sensitive(mw->btn_rotate_ccw, enable);
    gtk_widget_set_sensitive(mw->btn_flip_v, enable);
    gtk_widget_set_sensitive(mw->btn_flip_h, enable);
}

gboolean main_win_open( MainWin* mw, const char* file_path, ZoomMode zoom )
{
    if (g_file_test(file_path, G_FILE_TEST_IS_DIR))
    {
        image_list_open_dir( mw->img_list, file_path, NULL );
        image_list_sort_by_name( mw->img_list, GTK_SORT_DESCENDING );
        if (image_list_get_first(mw->img_list))
            main_win_open(mw, image_list_get_current_file_path(mw->img_list), zoom);
        return;
    }


    GError* err = NULL;
    GdkPixbufFormat* info;
    info = gdk_pixbuf_get_file_info( file_path, NULL, NULL );
    char* type = ((info != NULL) ? gdk_pixbuf_format_get_name(info) : "");

    main_win_close( mw );

    /* grabs a file as if it were an animation */
    mw->animation = gdk_pixbuf_animation_new_from_file( file_path, &err );
    if( ! mw->animation )
    {
        main_win_show_error( mw, err->message );
        g_error_free(err);
        update_btns( mw );
        return FALSE;
    }

    /* tests if the file is actually just a normal picture */
    if ( gdk_pixbuf_animation_is_static_image( mw->animation ) )
    {
       mw->pix = gdk_pixbuf_animation_get_static_image( mw->animation );
       g_object_ref(mw->pix);
       g_object_unref(mw->animation);
       mw->animation = NULL;
    }
    else
    {
        int delay;
        /* we found an animation */
        mw->animation_iter = gdk_pixbuf_animation_get_iter( mw->animation, NULL );
        mw->pix = gdk_pixbuf_animation_iter_get_pixbuf( mw->animation_iter );
        delay = gdk_pixbuf_animation_iter_get_delay_time( mw->animation_iter );
        mw->animation_timeout = g_timeout_add( delay, (GSourceFunc) on_animation_timeout, mw );
    }
    update_btns( mw );

    if(!strcmp(type,"jpeg"))
    {
        GdkPixbuf* tmp;
        // Only jpeg should rotate by EXIF
        tmp = gdk_pixbuf_apply_embedded_orientation(mw->pix);
        g_object_unref(mw->pix);
        mw->pix = tmp;
    }

    mw->zoom_mode = zoom;

    // select most suitable viewing mode
    if( zoom == ZOOM_NONE )
    {
        int w = gdk_pixbuf_get_width( mw->pix );
        int h = gdk_pixbuf_get_height( mw->pix );

        GdkRectangle area;
        get_working_area( gtk_widget_get_screen((GtkWidget*)mw), &area );
        // g_debug("determine best zoom mode: orig size:  w=%d, h=%d", w, h);
        // FIXME: actually this is a little buggy :-(
        if( w < area.width && h < area.height && (w >= 640 || h >= 480) )
        {
            gtk_scrolled_window_set_policy( (GtkScrolledWindow*)mw->scroll, GTK_POLICY_NEVER, GTK_POLICY_NEVER );
            gtk_widget_set_size_request( (GtkWidget*)mw->img_view, w, h );
            GtkRequisition req;
            gtk_widget_size_request ( (GtkWidget*)mw, &req );
            if( req.width < 640 )   req.width = 640;
            if( req.height < 480 )   req.height = 480;
            gtk_window_resize( (GtkWindow*)mw, req.width, req.height );
            gtk_widget_set_size_request( (GtkWidget*)mw->img_view, -1, -1 );
            gtk_scrolled_window_set_policy( (GtkScrolledWindow*)mw->scroll, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );
            mw->zoom_mode = ZOOM_ORIG;
            mw->scale = 1.0;
        }
        else
            mw->zoom_mode = ZOOM_FIT;
    }

    if( mw->zoom_mode == ZOOM_FIT )
    {
        main_win_fit_window_size( mw, FALSE, GDK_INTERP_BILINEAR );
    }
    else  if( mw->zoom_mode == ZOOM_SCALE )  // scale
    {
        main_win_scale_image( mw, mw->scale, GDK_INTERP_BILINEAR );
    }
    else  if( mw->zoom_mode == ZOOM_ORIG )  // original size
    {
        image_view_set_scale( (ImageView*)mw->img_view, mw->scale, GDK_INTERP_BILINEAR );
        main_win_center_image( mw );
    }

    image_view_set_pixbuf( (ImageView*)mw->img_view, mw->pix );

//    while (gtk_events_pending ())
//        gtk_main_iteration ();

    // build file list
    char* dir_path = g_path_get_dirname( file_path );
    image_list_open_dir( mw->img_list, dir_path, NULL );
    image_list_sort_by_name( mw->img_list, GTK_SORT_DESCENDING );
    g_free( dir_path );

    char* base_name = g_path_get_basename( file_path );
    image_list_set_current( mw->img_list, base_name );

    char* disp_name = g_filename_display_name( base_name );
    g_free( base_name );

    update_title( disp_name, mw );
    g_free( disp_name );

    main_win_update_zoom_buttons_state(mw);

    return TRUE;
}

void main_win_start_slideshow( MainWin* mw )
{
    on_slideshow_menu(NULL, mw);
}

void main_win_close( MainWin* mw )
{
    if( mw->animation )
    {
        g_object_unref( mw->animation );
        mw->animation = NULL;
        if( mw->animation_timeout );
        {
            g_source_remove( mw->animation_timeout );
            mw->animation_timeout = 0;
        }
    }
    else if( mw->pix )
    {
        g_object_unref( mw->pix );
    }
    mw->pix = NULL;
}

void main_win_show_error( MainWin* mw, const char* message )
{
    GtkWidget* dlg = gtk_message_dialog_new( (GtkWindow*)mw,
                                              GTK_DIALOG_MODAL,
                                              GTK_MESSAGE_ERROR,
                                              GTK_BUTTONS_OK,
                                              "%s", message );
    gtk_dialog_run( (GtkDialog*)dlg );
    gtk_widget_destroy( dlg );
}

void on_size_allocate( GtkWidget* widget, GtkAllocation    *allocation )
{
    GTK_WIDGET_CLASS(main_win_parent_class)->size_allocate( widget, allocation );
#if GTK_CHECK_VERSION(2, 20, 0)
    if(gtk_widget_get_realized (widget) )
#else
    if( GTK_WIDGET_REALIZED (widget) )
#endif
    {
        MainWin* mw = (MainWin*)widget;

        if( mw->zoom_mode == ZOOM_FIT )
        {
            while(gtk_events_pending ())
                gtk_main_iteration(); // makes it more fluid

            main_win_fit_window_size( mw, FALSE, GDK_INTERP_BILINEAR );
        }
    }
}

gboolean on_win_state_event( GtkWidget* widget, GdkEventWindowState* state )
{
    MainWin* mw = (MainWin*)widget;
    if( (state->new_window_state & GDK_WINDOW_STATE_FULLSCREEN) != 0 )
    {
        gtk_widget_modify_bg( mw->evt_box, GTK_STATE_NORMAL, &pref.bg_full );
        gtk_widget_hide( gtk_widget_get_parent(mw->nav_bar) );
        mw->full_screen = TRUE;
    }
    else
    {
        gtk_widget_modify_bg( mw->evt_box, GTK_STATE_NORMAL, &pref.bg );
        if (pref.show_toolbar)
            gtk_widget_show( gtk_widget_get_parent(mw->nav_bar) );
        mw->full_screen = FALSE;
    }

    int previous = pref.open_maximized;
    pref.open_maximized = (state->new_window_state == GDK_WINDOW_STATE_MAXIMIZED);
    if (previous != pref.open_maximized)
        save_preferences();
    return TRUE;
}

void main_win_fit_size( MainWin* mw, int width, int height, gboolean can_strech, GdkInterpType type )
{
    if( ! mw->pix )
        return;

    int orig_w = gdk_pixbuf_get_width( mw->pix );
    int orig_h = gdk_pixbuf_get_height( mw->pix );

    if( can_strech || (orig_w > width || orig_h > height) )
    {
        gdouble xscale = ((gdouble)width) / orig_w;
        gdouble yscale = ((gdouble)height)/ orig_h;
        gdouble final_scale = xscale < yscale ? xscale : yscale;

        main_win_scale_image( mw, final_scale, type );
    }
    else    // use original size if the image is smaller than the window
    {
        mw->scale = 1.0;
        image_view_set_scale( (ImageView*)mw->img_view, 1.0, type );

        update_title(NULL, mw);
    }
}

void main_win_fit_window_size(  MainWin* mw, gboolean can_strech, GdkInterpType type )
{
    mw->zoom_mode = ZOOM_FIT;

    if( mw->pix == NULL )
        return;
    main_win_fit_size( mw, mw->scroll_allocation.width, mw->scroll_allocation.height, can_strech, type );
}

GtkWidget* add_nav_btn( MainWin* mw, const char* icon, const char* tip, GCallback cb, gboolean toggle)
{
    GtkWidget* unused;
    return add_nav_btn_img(mw, icon, tip, cb, toggle, &unused);
}

GtkWidget* add_nav_btn_img( MainWin* mw, const char* icon, const char* tip, GCallback cb, gboolean toggle, GtkWidget** ret_img )
{
    GtkWidget* img;
    if( g_str_has_prefix(icon, "gtk-") )
        img = gtk_image_new_from_stock(icon, GTK_ICON_SIZE_SMALL_TOOLBAR);
    else
        img = gtk_image_new_from_icon_name(icon, GTK_ICON_SIZE_SMALL_TOOLBAR);
    GtkWidget* btn;
    if( G_UNLIKELY(toggle) )
    {
        btn = gtk_toggle_button_new();
        g_signal_connect( btn, "toggled", cb, mw );
    }
    else
    {
        btn = gtk_button_new();
        g_signal_connect( btn, "clicked", cb, mw );
    }
    gtk_button_set_relief( (GtkButton*)btn, GTK_RELIEF_NONE );
    gtk_button_set_focus_on_click( (GtkButton*)btn, FALSE );
    gtk_container_add( (GtkContainer*)btn, img );
    gtk_widget_set_tooltip_text( btn, tip );
    gtk_box_pack_start( (GtkBox*)mw->nav_bar, btn, FALSE, FALSE, 0 );
    *ret_img = img;
    return btn;
}

void on_scroll_size_allocate(GtkWidget* widget, GtkAllocation* allocation, MainWin* mv)
{
    mv->scroll_allocation = *allocation;
}

void on_zoom_fit_menu( GtkMenuItem* item, MainWin* mw )
{
    gtk_button_clicked( (GtkButton*)mw->btn_fit );
}

void on_zoom_fit( GtkToggleButton* btn, MainWin* mw )
{
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(btn)))
        main_win_set_zoom_mode(mw, ZOOM_FIT);
}

void on_full_screen( GtkWidget* btn, MainWin* mw )
{
    if( ! mw->full_screen )
        gtk_window_fullscreen( (GtkWindow*)mw );
    else
        gtk_window_unfullscreen( (GtkWindow*)mw );
}

void on_orig_size_menu( GtkToggleButton* btn, MainWin* mw )
{
    gtk_button_clicked( (GtkButton*)mw->btn_orig );
}

void on_orig_size( GtkToggleButton* btn, MainWin* mw )
{
    // this callback could be called from activate signal of menu item.
    if( GTK_IS_MENU_ITEM(btn) )
    {
        gtk_button_clicked( (GtkButton*)mw->btn_orig );
        return;
    }

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(btn)))
        main_win_set_zoom_mode(mw, ZOOM_ORIG);
}

void on_prev( GtkWidget* btn, MainWin* mw )
{
    const char* name;
    if( image_list_is_empty( mw->img_list ) )
        return;

    name = image_list_get_prev( mw->img_list );

    if( ! name && image_list_has_multiple_files( mw->img_list ) )
    {
        // FIXME: need to ask user first?
        name = image_list_get_last( mw->img_list );
    }

    if( name )
    {
        char* file_path = image_list_get_current_file_path( mw->img_list );
        main_win_open( mw, file_path, mw->zoom_mode );
        g_free( file_path );
    }
}

void on_next( GtkWidget* btn, MainWin* mw )
{
    if( image_list_is_empty( mw->img_list ) )
        return;

    const char* name = image_list_get_next( mw->img_list );

    if( ! name && image_list_has_multiple_files( mw->img_list ) )
    {
        // FIXME: need to ask user first?
        name = image_list_get_first( mw->img_list );
    }

    if( name )
    {
        char* file_path = image_list_get_current_file_path( mw->img_list );
        main_win_open( mw, file_path, mw->zoom_mode );
        g_free( file_path );
    }
}

void cancel_slideshow(MainWin* mw)
{
    mw->slideshow_cancelled = TRUE;
    mw->slideshow_running = FALSE;
    if (mw->slide_timeout != 0)
        g_source_remove(mw->slide_timeout);
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(mw->btn_play_stop), FALSE );
}

gboolean next_slide(MainWin* mw)
{
    /* Timeout causes an implicit "Next". */
    if (mw->slideshow_running)
        on_next( NULL, mw );

    return mw->slideshow_running;
}

void on_slideshow_menu( GtkMenuItem* item, MainWin* mw )
{
    gtk_button_clicked( (GtkButton*)mw->btn_play_stop );
}

void on_slideshow( GtkToggleButton* btn, MainWin* mw )
{
    if ((mw->slideshow_running) || (mw->slideshow_cancelled))
    {
        mw->slideshow_running = FALSE;
        mw->slideshow_cancelled = FALSE;
        gtk_image_set_from_stock( GTK_IMAGE(mw->img_play_stop), GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_SMALL_TOOLBAR );
        gtk_widget_set_tooltip_text( GTK_WIDGET(btn), _("Start Slideshow") );
        gtk_toggle_button_set_active( btn, FALSE );
    }
    else
    {
        gtk_toggle_button_set_active( btn, TRUE );
        mw->slideshow_running = TRUE;
        gtk_image_set_from_stock( GTK_IMAGE(mw->img_play_stop), GTK_STOCK_MEDIA_STOP, GTK_ICON_SIZE_SMALL_TOOLBAR );
        gtk_widget_set_tooltip_text( GTK_WIDGET(btn), _("Stop Slideshow") );
        mw->slide_timeout = g_timeout_add(1000 * pref.slide_delay, (GSourceFunc) next_slide, mw);
    }
}

//////////////////// rotate & flip

static int trans_angle_to_id(int i)
{
    if(i == 0) 		return 1;
    else if(i == 90)	return 6;
    else if(i == 180)	return 3;
    else if(i == 270)	return 8;
    else if(i == -45)	return 7;
    else if(i == -90)	return 2;
    else if(i == -135)	return 5;
    else     /* -180 */ return 4;
}

static int get_new_angle( int orig_angle, int rotate_angle )
{
    // defined in exif.c
    static int angle_trans_back[] = {0, 0, -90, 180, -180, -135, 90, -45, 270};

    orig_angle = trans_angle_to_id(orig_angle);
    rotate_angle = trans_angle_to_id(rotate_angle);

    return angle_trans_back[ ExifRotateFlipMapping[orig_angle][rotate_angle] ];
}

void on_rotate_auto_save( GtkWidget* btn, MainWin* mw )
{
    if(pref.auto_save_rotated){
//      gboolean ask_before_save = pref.ask_before_save;
//      pref.ask_before_save = FALSE;
        on_save(btn,mw);
//      pref.ask_before_save = ask_before_save;
    }
}

void on_rotate_clockwise( GtkWidget* btn, MainWin* mw )
{
    cancel_slideshow(mw);
    rotate_image( mw, GDK_PIXBUF_ROTATE_CLOCKWISE );
    mw->rotation_angle = get_new_angle(mw->rotation_angle, 90);
    on_rotate_auto_save(btn, mw);
}

void on_rotate_counterclockwise( GtkWidget* btn, MainWin* mw )
{
    cancel_slideshow(mw);
    rotate_image( mw, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE );
    mw->rotation_angle = get_new_angle(mw->rotation_angle, 270);
    on_rotate_auto_save(btn, mw);
}

void on_flip_vertical( GtkWidget* btn, MainWin* mw )
{
    cancel_slideshow(mw);
    rotate_image( mw, -180 );
    mw->rotation_angle = get_new_angle(mw->rotation_angle, -180);
    on_rotate_auto_save(btn, mw);
}

void on_flip_horizontal( GtkWidget* btn, MainWin* mw )
{
    cancel_slideshow(mw);
    rotate_image( mw, -90 );
    mw->rotation_angle = get_new_angle(mw->rotation_angle, -90);
    on_rotate_auto_save(btn, mw);
}

/* end of rotate & flip */

void on_save_as( GtkWidget* btn, MainWin* mw )
{
    char *file, *type;

    cancel_slideshow(mw);
    if( ! mw->pix )
        return;

    file = get_save_filename( GTK_WINDOW(mw), image_list_get_dir( mw->img_list ), &type );
    if( file )
    {
        char* dir;
        main_win_save( mw, file, type, TRUE );
        dir = g_path_get_dirname(file);
        const char* name = file + strlen(dir) + 1;

        if( strcmp( image_list_get_dir(mw->img_list), dir ) == 0 )
        {
            /* if the saved file is located in the same dir */
            /* simply add it to image list */
            image_list_add_sorted( mw->img_list, name, TRUE );
        }
        else /* otherwise reload the whole image list. */
        {
            /* switch to the dir containing the saved file. */
            image_list_open_dir( mw->img_list, dir, NULL );
        }
        update_title( name, mw );
        g_free( dir );
        g_free( file );
        g_free( type );
    }
}

void on_save( GtkWidget* btn, MainWin* mw )
{
    cancel_slideshow(mw);
    if( ! mw->pix )
        return;

    char* file_name = g_build_filename( image_list_get_dir( mw->img_list ),
                                        image_list_get_current( mw->img_list ), NULL );
    GdkPixbufFormat* info;
    info = gdk_pixbuf_get_file_info( file_name, NULL, NULL );
    char* type = gdk_pixbuf_format_get_name( info );

    /* Confirm save if requested. */
    if ((pref.ask_before_save) && ( ! save_confirm(mw, file_name)))
        return;

    if(strcmp(type,"jpeg")==0)
    {
        if(!pref.rotate_exif_only || ExifRotate(file_name, mw->rotation_angle) == FALSE)
        {
            // hialan notes:
            // ExifRotate retrun FALSE when
            //   1. Can not read file
            //   2. Exif do not have TAG_ORIENTATION tag
            //   3. Format unknown
            // And then we apply rotate_and_save_jpeg_lossless() ,
            // the result would not effected by EXIF Orientation...
#ifdef HAVE_LIBJPEG
            int status = rotate_and_save_jpeg_lossless(file_name,mw->rotation_angle);
	    if(status != 0)
            {
                main_win_show_error( mw, g_strerror(status) );
            }
#else
            main_win_save( mw, file_name, type, pref.ask_before_save );
#endif
        }
    } else
        main_win_save( mw, file_name, type, pref.ask_before_save );
    mw->rotation_angle = 0;
    g_free( file_name );
    g_free( type );
}

void on_open( GtkWidget* btn, MainWin* mw )
{
    cancel_slideshow(mw);
    char* file = get_open_filename( (GtkWindow*)mw, image_list_get_dir( mw->img_list ) );
    if( file )
    {
        main_win_open( mw, file, ZOOM_NONE );
        g_free( file );
    }
}

void on_zoom_in( GtkWidget* btn, MainWin* mw )
{
    double scale = mw->scale;
    scale *= 1.05;
    main_win_set_zoom_scale(mw, scale);
}

void on_zoom_out( GtkWidget* btn, MainWin* mw )
{
    double scale = mw->scale;
    scale /= 1.05;
    main_win_set_zoom_scale(mw, scale);
}

void on_preference( GtkWidget* btn, MainWin* mw )
{
    edit_preferences( (GtkWindow*)mw );
}

void on_quit( GtkWidget* btn, MainWin* mw )
{
    cancel_slideshow(mw);
    gtk_widget_destroy( (GtkWidget*)mw );
}

gboolean on_button_press( GtkWidget* widget, GdkEventButton* evt, MainWin* mw )
{
#if GTK_CHECK_VERSION(2, 14, 0)
    if( ! gtk_widget_has_focus( widget ) )
#else
    if( ! GTK_WIDGET_HAS_FOCUS( widget ) )
#endif
        gtk_widget_grab_focus( widget );

    if( evt->type == GDK_BUTTON_PRESS)
    {
        if( evt->button == 1 )    // left button
        {
            if( ! mw->pix )
                return FALSE;
            mw->dragging = TRUE;
            gtk_widget_get_pointer( (GtkWidget*)mw, &mw->drag_old_x ,&mw->drag_old_y );
            gdk_window_set_cursor( gtk_widget_get_window(widget), mw->hand_cursor );
        }
        else if( evt->button == 3 )   // right button
        {
            show_popup_menu( mw, evt );
        }
    }
    else if( evt->type == GDK_2BUTTON_PRESS && evt->button == 1 )    // double clicked
    {
         on_full_screen( NULL, mw );
    }
    return FALSE;
}

gboolean on_mouse_move( GtkWidget* widget, GdkEventMotion* evt, MainWin* mw )
{
    if( ! mw->dragging )
        return FALSE;

    int cur_x, cur_y;
    gtk_widget_get_pointer( (GtkWidget*)mw, &cur_x ,&cur_y );

    int dx = (mw->drag_old_x - cur_x);
    int dy = (mw->drag_old_y - cur_y);

    GtkAdjustment *hadj, *vadj;
    hadj = gtk_scrolled_window_get_hadjustment((GtkScrolledWindow*)mw->scroll);
    vadj = gtk_scrolled_window_get_vadjustment((GtkScrolledWindow*)mw->scroll);

    GtkRequisition req;
    gtk_widget_size_request( (GtkWidget*)mw->img_view, &req );

#if GTK_CHECK_VERSION(2, 14, 0)
    gdouble hadj_page_size = gtk_adjustment_get_page_size(hadj);
    gdouble hadj_lower = gtk_adjustment_get_lower(hadj);
    gdouble hadj_upper = gtk_adjustment_get_upper(hadj);
#else
    gdouble hadj_page_size = hadj->page_size;
    gdouble hadj_lower = hadj->lower;
    gdouble hadj_upper = hadj->upper;
#endif

    if( ABS(dx) > 4 )
    {
        mw->drag_old_x = cur_x;
        if( req.width > hadj_page_size )
        {
            gdouble value = gtk_adjustment_get_value (hadj);
            gdouble x = value + dx;
            if( x < hadj_lower )
                x = hadj_lower;
            else if( (x + hadj_page_size) > hadj_upper )
                x = hadj_upper - hadj_page_size;

            if( x != value )
                gtk_adjustment_set_value (hadj, x );
        }
    }

#if GTK_CHECK_VERSION(2, 14, 0)
    gdouble vadj_page_size = gtk_adjustment_get_page_size(vadj);
    gdouble vadj_lower = gtk_adjustment_get_lower(vadj);
    gdouble vadj_upper = gtk_adjustment_get_upper(vadj);
#else
    gdouble vadj_page_size = vadj->page_size;
    gdouble vadj_lower = vadj->lower;
    gdouble vadj_upper = vadj->upper;
#endif

    if( ABS(dy) > 4 )
    {
        if( req.height > vadj_page_size )
        {
            mw->drag_old_y = cur_y;
            gdouble value = gtk_adjustment_get_value (vadj);
            gdouble y = value + dy;
            if( y < vadj_lower )
                y = vadj_lower;
            else if( (y + vadj_page_size) > vadj_upper )
                y = vadj_upper - vadj_page_size;

            if( y != value )
                gtk_adjustment_set_value (vadj, y );
        }
    }
    return FALSE;
}

gboolean on_button_release( GtkWidget* widget, GdkEventButton* evt, MainWin* mw )
{
    mw->dragging = FALSE;
    gdk_window_set_cursor( gtk_widget_get_window(widget), NULL );
    return FALSE;
}

gboolean on_scroll_event( GtkWidget* widget, GdkEventScroll* evt, MainWin* mw )
{
    guint modifiers = gtk_accelerator_get_default_mod_mask();
    switch( evt->direction )
    {
    case GDK_SCROLL_UP:
        if ((evt->state & modifiers) == GDK_CONTROL_MASK)
            on_zoom_in( NULL, mw );
        else
            on_prev( NULL, mw );
        break;
    case GDK_SCROLL_DOWN:
        if ((evt->state & modifiers) == GDK_CONTROL_MASK)
            on_zoom_out( NULL, mw );
        else
            on_next( NULL, mw );
        break;
    case GDK_SCROLL_LEFT:
        if( gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL )
            on_next( NULL, mw );
        else
            on_prev( NULL, mw );
        break;
    case GDK_SCROLL_RIGHT:
        if( gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL )
            on_prev( NULL, mw );
        else
            on_next( NULL, mw );
        break;
    }
    return TRUE;
}

gboolean on_key_press_event(GtkWidget* widget, GdkEventKey * key)
{
    MainWin* mw = (MainWin*)widget;
    switch( key->keyval )
    {
        case GDK_Right:
        case GDK_KP_Right:
        case GDK_rightarrow:
            if( gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL )
                on_prev( NULL, mw );
            else
                on_next( NULL, mw );
            break;
        case GDK_Return:
        case GDK_space:
        case GDK_Next:
        case GDK_KP_Down:
        case GDK_Down:
        case GDK_downarrow:
            on_next( NULL, mw );
            break;
        case GDK_Left:
        case GDK_KP_Left:
        case GDK_leftarrow:
            if( gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL )
                on_next( NULL, mw );
            else
                on_prev( NULL, mw );
            break;
        case GDK_Prior:
        case GDK_BackSpace:
        case GDK_KP_Up:
        case GDK_Up:
        case GDK_uparrow:
            on_prev( NULL, mw );
            break;
        case GDK_w:
        case GDK_W:
            on_slideshow_menu( NULL, mw );
            break;
        case GDK_KP_Add:
        case GDK_plus:
        case GDK_equal:
            on_zoom_in( NULL, mw );
            break;
        case GDK_KP_Subtract:
        case GDK_minus:
            on_zoom_out( NULL, mw );
            break;
        case GDK_s:
        case GDK_S:
            on_save( NULL, mw );
            break;
        case GDK_a:
        case GDK_A:
            on_save_as( NULL, mw );
            break;
        case GDK_l:
        case GDK_L:
            on_rotate_counterclockwise( NULL, mw );
            break;
        case GDK_r:
        case GDK_R:
            on_rotate_clockwise( NULL, mw );
            break;
        case GDK_f:
        case GDK_F:
            if( mw->zoom_mode != ZOOM_FIT )
                gtk_button_clicked((GtkButton*)mw->btn_fit );
            break;
        case GDK_g:
        case GDK_G:
            if( mw->zoom_mode != ZOOM_ORIG )
                gtk_button_clicked((GtkButton*)mw->btn_orig );
            break;
        case GDK_h:
        case GDK_H:
            on_flip_horizontal( NULL, mw );
            break;
        case GDK_v:
        case GDK_V:
            on_flip_vertical( NULL, mw );
            break;
        case GDK_o:
        case GDK_O:
            on_open( NULL, mw );
            break;
        case GDK_Delete:
        case GDK_d:
        case GDK_D:
            on_delete( NULL, mw );
            break;
        case GDK_p:
        case GDK_P:
            on_preference( NULL, mw );
	    break;
        case GDK_t:
        case GDK_T:
            on_toggle_toolbar( NULL, mw );
	    break;
        case GDK_Escape:
            if( mw->full_screen )
                on_full_screen( NULL, mw );
            else
                on_quit( NULL, mw );
            break;
        case GDK_q:
	case GDK_Q:
            on_quit( NULL, mw );
            break;
        case GDK_F11:
            on_full_screen( NULL, mw );
            break;

        default:
            GTK_WIDGET_CLASS(main_win_parent_class)->key_press_event( widget, key );
    }
    return FALSE;
}

void main_win_center_image( MainWin* mw )
{
    GtkAdjustment *hadj, *vadj;
    hadj = gtk_scrolled_window_get_hadjustment((GtkScrolledWindow*)mw->scroll);
    vadj = gtk_scrolled_window_get_vadjustment((GtkScrolledWindow*)mw->scroll);

    GtkRequisition req;
    gtk_widget_size_request( (GtkWidget*)mw->img_view, &req );

#if GTK_CHECK_VERSION(2, 14, 0)
    gdouble hadj_page_size = gtk_adjustment_get_page_size(hadj);
    gdouble hadj_upper = gtk_adjustment_get_upper(hadj);
#else
    gdouble hadj_page_size = hadj->page_size;
    gdouble hadj_upper = hadj->upper;
#endif

    if( req.width > hadj_page_size )
        gtk_adjustment_set_value(hadj, ( hadj_upper - hadj_page_size ) / 2 );

#if GTK_CHECK_VERSION(2, 14, 0)
    gdouble vadj_page_size = gtk_adjustment_get_page_size(vadj);
    gdouble vadj_upper = gtk_adjustment_get_upper(vadj);
#else
    gdouble vadj_page_size = vadj->page_size;
    gdouble vadj_upper = vadj->upper;
#endif

    if( req.height > vadj_page_size )
        gtk_adjustment_set_value(vadj, ( vadj_upper - vadj_page_size ) / 2 );
}

void rotate_image( MainWin* mw, int angle )
{
    GdkPixbuf* rpix = NULL;

    if( ! mw->pix )
        return;

    if(angle > 0)
    {
        rpix = gdk_pixbuf_rotate_simple( mw->pix, angle );
    }
    else
    {
        if(angle == -90)
            rpix = gdk_pixbuf_flip( mw->pix, TRUE );
        else if(angle == -180)
            rpix = gdk_pixbuf_flip( mw->pix, FALSE );
    }

    if (!rpix) {
        return;
    }

    g_object_unref( mw->pix );

    mw->pix = rpix;
    image_view_set_pixbuf( (ImageView*)mw->img_view, mw->pix );

    if( mw->zoom_mode == ZOOM_FIT )
        main_win_fit_window_size( mw, FALSE, GDK_INTERP_BILINEAR );
}

gboolean main_win_scale_image( MainWin* mw, double new_scale, GdkInterpType type )
{
    if( G_UNLIKELY( new_scale == 1.0 ) )
    {
        gtk_toggle_button_set_active( (GtkToggleButton*)mw->btn_orig, TRUE );
        mw->scale = 1.0;
        return TRUE;
    }
    mw->scale = new_scale;
    image_view_set_scale( (ImageView*)mw->img_view, new_scale, type );

    update_title( NULL, mw );

    return TRUE;
}

gboolean save_confirm( MainWin* mw, const char* file_path )
{
    if( g_file_test( file_path, G_FILE_TEST_EXISTS ) )
    {
        GtkWidget* dlg = gtk_message_dialog_new( (GtkWindow*)mw,
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_QUESTION,
                GTK_BUTTONS_YES_NO,
                _("The file name you selected already exists.\nDo you want to overwrite existing file?\n(Warning: The quality of original image might be lost)") );
        if( gtk_dialog_run( (GtkDialog*)dlg ) != GTK_RESPONSE_YES )
        {
            gtk_widget_destroy( dlg );
            return FALSE;
        }
        gtk_widget_destroy( dlg );
    }
    return TRUE;
}

gboolean main_win_save( MainWin* mw, const char* file_path, const char* type, gboolean confirm )
{
    gboolean result1,gdk_save_supported;
    GSList *gdk_formats;
    GSList *gdk_formats_i;
    if( ! mw->pix )
        return FALSE;

    /* detect if the current type can be save by gdk_pixbuf_save() */
    gdk_save_supported = FALSE;
    gdk_formats = gdk_pixbuf_get_formats();
    for (gdk_formats_i = gdk_formats; gdk_formats_i;
         gdk_formats_i = g_slist_next(gdk_formats_i))
    {
        GdkPixbufFormat *data;
        data = gdk_formats_i->data;
        if (gdk_pixbuf_format_is_writable(data))
        {
            if ( strcmp(type, gdk_pixbuf_format_get_name(data))==0)
            {
                gdk_save_supported = TRUE;
                break;
            }
        }
    }
    g_slist_free (gdk_formats);

    GError* err = NULL;
    if (!gdk_save_supported)
    {
        main_win_show_error( mw, _("Writing this image format is not supported.") );
        return FALSE;
    }
    if( strcmp( type, "jpeg" ) == 0 )
    {
        char tmp[32];
        g_sprintf(tmp, "%d", pref.jpg_quality);
        result1 = gdk_pixbuf_save( mw->pix, file_path, type, &err, "quality", tmp, NULL );
    }
    else if( strcmp( type, "png" ) == 0 )
    {
        char tmp[32];
        g_sprintf(tmp, "%d", pref.png_compression);
        result1 = gdk_pixbuf_save( mw->pix, file_path, type, &err, "compression", tmp, NULL );
    }
    else
        result1 = gdk_pixbuf_save( mw->pix, file_path, type, &err, NULL );
    if( ! result1 )
    {
        main_win_show_error( mw, err->message );
        return FALSE;
    }
    return TRUE;
}

void on_delete( GtkWidget* btn, MainWin* mw )
{
    cancel_slideshow(mw);
    char* file_path = image_list_get_current_file_path( mw->img_list );
    if( file_path )
    {
        int resp = GTK_RESPONSE_YES;
	if ( pref.ask_before_delete )
	{
            GtkWidget* dlg = gtk_message_dialog_new( (GtkWindow*)mw,
                    GTK_DIALOG_MODAL,
                    GTK_MESSAGE_QUESTION,
                    GTK_BUTTONS_YES_NO,
                    _("Are you sure you want to delete current file?\n\nWarning: Once deleted, the file cannot be recovered.") );
            resp = gtk_dialog_run( (GtkDialog*)dlg );
            gtk_widget_destroy( dlg );
        }

	if( resp == GTK_RESPONSE_YES )
        {
            const char* name = image_list_get_current( mw->img_list );

	    if( g_unlink( file_path ) != 0 )
		main_win_show_error( mw, g_strerror(errno) );
	    else
	    {
		const char* next_name = image_list_get_next( mw->img_list );
		if( ! next_name )
		    next_name = image_list_get_prev( mw->img_list );

		if( next_name )
		{
		    char* next_file_path = image_list_get_current_file_path( mw->img_list );
		    main_win_open( mw, next_file_path, ZOOM_FIT );
		    g_free( next_file_path );
		}

		image_list_remove ( mw->img_list, name );

		if ( ! next_name )
		{
		    main_win_close( mw );
		    image_list_close( mw->img_list );
		    image_view_set_pixbuf( (ImageView*)mw->img_view, NULL );
		    gtk_window_set_title( (GtkWindow*) mw, _("Image Viewer"));
		}
	    }
        }
	g_free( file_path );
    }
}

void on_toggle_toolbar( GtkMenuItem* item, MainWin* mw )
{
    pref.show_toolbar = !pref.show_toolbar;

    if (pref.show_toolbar)
        gtk_widget_show(gtk_widget_get_parent(mw->nav_bar));
    else
        gtk_widget_hide(gtk_widget_get_parent(mw->nav_bar));

    save_preferences();
}


void show_popup_menu( MainWin* mw, GdkEventButton* evt )
{
    static PtkMenuItemEntry menu_def[] =
    {
        PTK_IMG_MENU_ITEM( N_( "Previous" ), GTK_STOCK_GO_BACK, on_prev, GDK_leftarrow, 0 ),
        PTK_IMG_MENU_ITEM( N_( "Next" ), GTK_STOCK_GO_FORWARD, on_next, GDK_rightarrow, 0 ),
        PTK_IMG_MENU_ITEM( N_( "Start/Stop Slideshow" ), GTK_STOCK_MEDIA_PLAY, on_slideshow_menu, GDK_W, 0 ),
        PTK_SEPARATOR_MENU_ITEM,
        PTK_IMG_MENU_ITEM( N_( "Zoom Out" ), GTK_STOCK_ZOOM_OUT, on_zoom_out, GDK_minus, 0 ),
        PTK_IMG_MENU_ITEM( N_( "Zoom In" ), GTK_STOCK_ZOOM_IN, on_zoom_in, GDK_plus, 0 ),
        PTK_IMG_MENU_ITEM( N_( "Fit Image To Window Size" ), GTK_STOCK_ZOOM_FIT, on_zoom_fit_menu, GDK_F, 0 ),
        PTK_IMG_MENU_ITEM( N_( "Original Size" ), GTK_STOCK_ZOOM_100, on_orig_size_menu, GDK_G, 0 ),
        PTK_SEPARATOR_MENU_ITEM,
        PTK_IMG_MENU_ITEM( N_( "Full Screen" ), GTK_STOCK_FULLSCREEN, on_full_screen, GDK_F11, 0 ),
        PTK_SEPARATOR_MENU_ITEM,
        PTK_IMG_MENU_ITEM( N_( "Rotate Counterclockwise" ), "object-rotate-left", on_rotate_counterclockwise, GDK_L, 0 ),
        PTK_IMG_MENU_ITEM( N_( "Rotate Clockwise" ), "object-rotate-right", on_rotate_clockwise, GDK_R, 0 ),
        PTK_IMG_MENU_ITEM( N_( "Flip Horizontal" ), "object-flip-horizontal", on_flip_horizontal, GDK_H, 0 ),
        PTK_IMG_MENU_ITEM( N_( "Flip Vertical" ), "object-flip-vertical", on_flip_vertical, GDK_V, 0 ),
        PTK_SEPARATOR_MENU_ITEM,
        PTK_IMG_MENU_ITEM( N_("Open File"), GTK_STOCK_OPEN, G_CALLBACK(on_open), GDK_O, 0 ),
        PTK_IMG_MENU_ITEM( N_("Save File"), GTK_STOCK_SAVE, G_CALLBACK(on_save), GDK_S, 0 ),
        PTK_IMG_MENU_ITEM( N_("Save As"), GTK_STOCK_SAVE_AS, G_CALLBACK(on_save_as), GDK_A, 0 ),
//        PTK_IMG_MENU_ITEM( N_("Save As Other Size"), GTK_STOCK_SAVE_AS, G_CALLBACK(on_save_as), GDK_A, 0 ),
        PTK_IMG_MENU_ITEM( N_("Delete File"), GTK_STOCK_DELETE, G_CALLBACK(on_delete), GDK_Delete, 0 ),
        PTK_SEPARATOR_MENU_ITEM,
        PTK_IMG_MENU_ITEM( N_("Preferences"), GTK_STOCK_PREFERENCES, G_CALLBACK(on_preference), GDK_P, 0 ),
        PTK_IMG_MENU_ITEM( N_("Show/Hide toolbar"), NULL, G_CALLBACK(on_toggle_toolbar), GDK_T, 0 ),
        PTK_STOCK_MENU_ITEM( GTK_STOCK_ABOUT, on_about ),
        PTK_SEPARATOR_MENU_ITEM,
        PTK_IMG_MENU_ITEM( N_("Quit"), GTK_STOCK_QUIT, G_CALLBACK(on_quit), GDK_Q, 0 ),
        PTK_MENU_END
    };
    GtkWidget* rotate_cw;
    GtkWidget* rotate_ccw;
    GtkWidget* flip_v;
    GtkWidget* flip_h;

    menu_def[10].ret = &rotate_ccw;
    menu_def[11].ret = &rotate_cw;
    menu_def[12].ret = &flip_h;
    menu_def[13].ret = &flip_v;

    // mw accel group is useless. It's only used to display accels in popup menu
    GtkAccelGroup* accel_group = gtk_accel_group_new();
    GtkMenuShell* popup = (GtkMenuShell*)ptk_menu_new_from_data( menu_def, mw, accel_group );

    if( mw->animation )
    {
        gtk_widget_set_sensitive( rotate_ccw, FALSE );
        gtk_widget_set_sensitive( rotate_cw, FALSE );
        gtk_widget_set_sensitive( flip_h, FALSE );
        gtk_widget_set_sensitive( flip_v, FALSE );
    }

    gtk_widget_show_all( (GtkWidget*)popup );
    g_signal_connect( popup, "selection-done", G_CALLBACK(gtk_widget_destroy), NULL );
    gtk_menu_popup( (GtkMenu*)popup, NULL, NULL, NULL, NULL, evt->button, evt->time );
}

void on_about( GtkWidget* menu, MainWin* mw )
{
    GtkWidget * about_dlg;
    const gchar *authors[] =
    {
        "洪任諭 Hong Jen Yee <pcman.tw@gmail.com>",
        "Martin Siggel <martinsiggel@googlemail.com>",
        "Hialan Liu <hialan.liu@gmail.com>",
        "Marty Jack <martyj19@comcast.net>",
        "Louis Casillas <oxaric@gmail.com>",
        "Will Davies",
        _(" * Refer to source code of EOG image viewer and GThumb"),
        NULL
    };
    /* TRANSLATORS: Replace this string with your names, one name per line. */
    gchar *translators = _( "translator-credits" );

    about_dlg = gtk_about_dialog_new ();

    gtk_container_set_border_width ( ( GtkContainer*)about_dlg , 2 );
    gtk_about_dialog_set_version ( (GtkAboutDialog*)about_dlg, VERSION );
    gtk_about_dialog_set_program_name ( (GtkAboutDialog*)about_dlg, _( "GPicView" ) );
    gtk_about_dialog_set_logo_icon_name ( (GtkAboutDialog*)about_dlg, "gpicview" );
    gtk_about_dialog_set_copyright ( (GtkAboutDialog*)about_dlg, _( "Copyright (C) 2007 - 2011" ) );
    gtk_about_dialog_set_comments ( (GtkAboutDialog*)about_dlg, _( "Lightweight image viewer from LXDE project" ) );
    gtk_about_dialog_set_license ( (GtkAboutDialog*)about_dlg, "GPicView\n\nCopyright (C) 2007 Hong Jen Yee (PCMan)\n\nThis program is free software; you can redistribute it and/or\nmodify it under the terms of the GNU General Public License\nas published by the Free Software Foundation; either version 2\nof the License, or (at your option) any later version.\n\nThis program is distributed in the hope that it will be useful,\nbut WITHOUT ANY WARRANTY; without even the implied warranty of\nMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\nGNU General Public License for more details.\n\nYou should have received a copy of the GNU General Public License\nalong with this program; if not, write to the Free Software\nFoundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA." );
    gtk_about_dialog_set_website ( (GtkAboutDialog*)about_dlg, "http://wiki.lxde.org/en/GPicView" );
    gtk_about_dialog_set_authors ( (GtkAboutDialog*)about_dlg, authors );
    gtk_about_dialog_set_translator_credits ( (GtkAboutDialog*)about_dlg, translators );
    gtk_window_set_transient_for( (GtkWindow*) about_dlg, GTK_WINDOW( mw ) );

    gtk_dialog_run( ( GtkDialog*)about_dlg );
    gtk_widget_destroy( about_dlg );
}

void on_drag_data_received( GtkWidget* widget, GdkDragContext *drag_context,
                int x, int y, GtkSelectionData* data, guint info, guint time, MainWin* mw )
{
    if( !data)
        return;

#if GTK_CHECK_VERSION(2, 14, 0)
    int data_length = gtk_selection_data_get_length(data);
#else
    int data_length = data->length;
#endif

    if( data_length <= 0)
        return;

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
        main_win_open( mw, file, ZOOM_FIT );
        g_free( file );
    }
}

static void main_win_set_zoom_scale(MainWin* mw, double scale)
{
    main_win_set_zoom_mode(mw, ZOOM_SCALE);

    if (scale > 20.0)
        scale = 20.0;
    if (scale < 0.02)
        scale = 0.02;

    if (mw->scale != scale)
        main_win_scale_image(mw, scale, GDK_INTERP_BILINEAR);
}

static void main_win_set_zoom_mode(MainWin* mw, ZoomMode mode)
{
    if (mw->zoom_mode == mode)
       return;

    mw->zoom_mode = mode;

    main_win_update_zoom_buttons_state(mw);

    if (mode == ZOOM_ORIG)
    {
        mw->scale = 1.0;
        if (!mw->pix)
           return;

        update_title(NULL, mw);

        image_view_set_scale( (ImageView*)mw->img_view, 1.0, GDK_INTERP_BILINEAR );

        while (gtk_events_pending ())
            gtk_main_iteration ();

        main_win_center_image( mw ); // FIXME:  mw doesn't work well. Why?
    }
    else if (mode == ZOOM_FIT)
    {
        main_win_fit_window_size( mw, FALSE, GDK_INTERP_BILINEAR );
    }
}

static void main_win_update_zoom_buttons_state(MainWin* mw)
{
    gboolean button_fit_active = mw->zoom_mode == ZOOM_FIT;
    gboolean button_orig_active = mw->zoom_mode == ZOOM_ORIG;

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mw->btn_fit)) != button_fit_active)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mw->btn_fit), button_fit_active);

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mw->btn_orig)) != button_orig_active)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mw->btn_orig), button_orig_active);
}


