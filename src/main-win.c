/***************************************************************************
 *   Copyright (C) 2007, 2008 by PCMan (Hong Jen Yee)   *
 *   pcman.tw@gmail.com   *
 *                                                                         *
 *   mw program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   mw program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with mw program; if not, write to the                         *
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

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>

#include "pref.h"

#include "image-view.h"
#include "image-list.h"
#include "working-area.h"
#include "ptk-menu.h"
#include "jpeg-tran.h"

// For drag & drop
static GtkTargetEntry drop_targets[] =
{
    {"text/uri-list", 0, 0},
    {"text/plain", 0, 1}
};

static void main_win_init( MainWin*mw );
static void main_win_finalize( GObject* obj );

static void create_nav_bar( MainWin* mw, GtkWidget* box);
GtkWidget* add_nav_btn( MainWin* mw, const char* icon, const char* tip, GCallback cb, gboolean toggle);
// GtkWidget* add_menu_item(  GtkMenuShell* menu, const char* label, const char* icon, GCallback cb, gboolean toggle=FALSE );
static void rotate_image( MainWin* mw, int angle );
static void show_popup_menu( MainWin* mw, GdkEventButton* evt );

/* signal handlers */
static gboolean on_delete_event( GtkWidget* widget, GdkEventAny* evt );
static void on_size_allocate( GtkWidget* widget, GtkAllocation    *allocation );
static gboolean on_win_state_event( GtkWidget* widget, GdkEventWindowState* state );
static void on_zoom_fit( GtkToggleButton* btn, MainWin* mw );
static void on_zoom_fit_menu( GtkMenuItem* item, MainWin* mw );
static void on_full_screen( GtkWidget* btn, MainWin* mw );
static void on_next( GtkWidget* btn, MainWin* mw );
static void on_orig_size( GtkToggleButton* btn, MainWin* mw );
static void on_orig_size_menu( GtkToggleButton* btn, MainWin* mw );
static void on_prev( GtkWidget* btn, MainWin* mw );
static void on_rotate_clockwise( GtkWidget* btn, MainWin* mw );
static void on_rotate_counterclockwise( GtkWidget* btn, MainWin* mw );
static void on_save_as( GtkWidget* btn, MainWin* mw );
static void on_save( GtkWidget* btn, MainWin* mw );
static void on_open( GtkWidget* btn, MainWin* mw );
static void on_zoom_in( GtkWidget* btn, MainWin* mw );
static void on_zoom_out( GtkWidget* btn, MainWin* mw );
static void on_preference( GtkWidget* btn, MainWin* mw );
static void on_quit( GtkWidget* btn, MainWin* mw );
static gboolean on_button_press( GtkWidget* widget, GdkEventButton* evt, MainWin* mw );
static gboolean on_button_release( GtkWidget* widget, GdkEventButton* evt, MainWin* mw );
static gboolean on_mouse_move( GtkWidget* widget, GdkEventMotion* evt, MainWin* mw );
static gboolean on_scroll_event( GtkWidget* widget, GdkEventScroll* evt, MainWin* mw );
static gboolean on_key_press_event(GtkWidget* widget, GdkEventKey * key);
static void on_drag_data_received( GtkWidget* widget, GdkDragContext *drag_context,
                                                                                       int x, int y, GtkSelectionData* data, guint info,
                                                                                       guint time, MainWin* mw );
static void on_delete( GtkWidget* btn, MainWin* mw );
static void on_about( GtkWidget* menu, MainWin* mw );

static GdkPixbuf* RotateByEXIF(const char* FileName, GdkPixbuf* pix);
static void updateTitle(const char *filename, MainWin *mw );

void on_flip_vertical( GtkWidget* btn, MainWin* mw );
void on_flip_horizontal( GtkWidget* btn, MainWin* mw );
static int trans_angle_to_id(int i);
static int get_new_angle( int orig_angle, int rotate_angle );

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
    gdk_cursor_unref( mw->hand_cursor );

    g_object_unref( mw->tooltips );

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
    gtk_window_set_icon_from_file( (GtkWindow*)mw, PACKAGE_DATA_DIR"/pixmaps/gpicview.png", NULL );
    gtk_window_set_default_size( (GtkWindow*)mw, 640, 480 );

    GtkWidget* box = gtk_vbox_new( FALSE, 0 );
    gtk_container_add( (GtkContainer*)mw, box);

    // image area
    mw->evt_box = gtk_event_box_new();
    GTK_WIDGET_SET_FLAGS( mw->evt_box, GTK_CAN_FOCUS );
    gtk_widget_add_events( mw->evt_box,
                           GDK_POINTER_MOTION_MASK|GDK_BUTTON_PRESS_MASK|
                           GDK_BUTTON_RELEASE_MASK|GDK_SCROLL_MASK );
    g_signal_connect( mw->evt_box, "button-press-event", G_CALLBACK(on_button_press), mw );
    g_signal_connect( mw->evt_box, "button-release-event", G_CALLBACK(on_button_release), mw );
    g_signal_connect( mw->evt_box, "motion-notify-event", G_CALLBACK(on_mouse_move), mw );
    g_signal_connect( mw->evt_box, "scroll-event", G_CALLBACK(on_scroll_event), mw );
    // Set bg color to white
    GdkColor white = {0, 65535, 65535, 65535};
    gtk_widget_modify_bg( mw->evt_box, GTK_STATE_NORMAL, &white );

    mw->img_view = image_view_new();
    gtk_container_add( (GtkContainer*)mw->evt_box, (GtkWidget*)mw->img_view);

    const char scroll_style[]=
            "style \"gpicview-scroll\" {"
            "GtkScrolledWindow::scrollbar-spacing=0"
            "}"
            "class \"GtkScrolledWindow\" style \"gpicview-scroll\"";
    gtk_rc_parse_string( scroll_style );
    mw->scroll = gtk_scrolled_window_new( NULL, NULL );
    gtk_scrolled_window_set_shadow_type( (GtkScrolledWindow*)mw->scroll, GTK_SHADOW_NONE );
    gtk_scrolled_window_set_policy((GtkScrolledWindow*)mw->scroll,
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    GtkAdjustment *hadj, *vadj;
    hadj = gtk_scrolled_window_get_hadjustment((GtkScrolledWindow*)mw->scroll);
    hadj->page_increment = 10;
    gtk_adjustment_changed(hadj);
    vadj = gtk_scrolled_window_get_vadjustment((GtkScrolledWindow*)mw->scroll);
    vadj->page_increment = 10;
    gtk_adjustment_changed(vadj);

    image_view_set_adjustments( IMAGE_VIEW(mw->img_view), hadj, vadj );    // dirty hack :-(
    gtk_scrolled_window_add_with_viewport( (GtkScrolledWindow*)mw->scroll, mw->evt_box );
    GtkWidget* viewport = gtk_bin_get_child( (GtkBin*)mw->scroll );
    gtk_viewport_set_shadow_type( (GtkViewport*)viewport, GTK_SHADOW_NONE );
    gtk_container_set_border_width( (GtkContainer*)viewport, 0 );

    gtk_box_pack_start( (GtkBox*)box, mw->scroll, TRUE, TRUE, 0 );

    // build toolbar
    mw->tooltips = gtk_tooltips_new();
#if GTK_CHECK_VERSION(2, 8, 0)
    g_object_ref_sink(mw->tooltips);
#else
    gtk_object_sink((GtkObject*)mw->tooltips);
#endif

    create_nav_bar( mw, box );
    gtk_widget_show_all( box );

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
    add_nav_btn( mw, GTK_STOCK_FULLSCREEN, _(" Full Screen"), G_CALLBACK(on_full_screen), FALSE );   // gtk+ 2.8+

    gtk_box_pack_start( (GtkBox*)mw->nav_bar, gtk_vseparator_new(), FALSE, FALSE, 0 );

    add_nav_btn( mw, "gtk-counterclockwise", _("Rotate Counterclockwise"),
                 G_CALLBACK(on_rotate_counterclockwise), FALSE );
    add_nav_btn( mw, "gtk-clockwise", _("Rotate Clockwise"), G_CALLBACK(on_rotate_clockwise), FALSE );

    gtk_box_pack_start( (GtkBox*)mw->nav_bar, gtk_vseparator_new(), FALSE, FALSE, 0 );

    add_nav_btn( mw, GTK_STOCK_OPEN, _("Open File"), G_CALLBACK(on_open), FALSE );
    add_nav_btn( mw, GTK_STOCK_SAVE, _("Save File"), G_CALLBACK(on_save), FALSE );
    add_nav_btn( mw, GTK_STOCK_SAVE_AS, _("Save File As"), G_CALLBACK(on_save_as), FALSE );
    add_nav_btn( mw, GTK_STOCK_DELETE, _("Delete File"), G_CALLBACK(on_delete), FALSE );

    gtk_box_pack_start( (GtkBox*)mw->nav_bar, gtk_vseparator_new(), FALSE, FALSE, 0 );
    add_nav_btn( mw, GTK_STOCK_PREFERENCES, _("Preferences"), G_CALLBACK(on_preference), FALSE );

    GtkWidget* align = gtk_alignment_new( 0.5, 0, 0, 0 );
    gtk_container_add( (GtkContainer*)align, mw->nav_bar );
    gtk_box_pack_start( (GtkBox*)box, align, FALSE, TRUE, 2 );
}

gboolean on_delete_event( GtkWidget* widget, GdkEventAny* evt )
{
    gtk_widget_destroy( widget );
    return TRUE;
}

static GdkPixbuf* RotateByEXIF(const char* FileName, GdkPixbuf* pix)
{
    GdkPixbuf* tmppixbuf = pix;
#if GTK_CHECK_VERSION( 2, 12, 0 )
    // apply orientation provided by EXIF (Use gtk+ 2.12 specific API)
    tmppixbuf = gdk_pixbuf_apply_embedded_orientation(pix);
    g_object_unref( pix );
#else
    // use jhead functions
    ResetJpgfile();

    // Start with an empty image information structure.
    memset(&ImageInfo, 0, sizeof(ImageInfo));

    if (!ReadJpegFile( FileName, READ_METADATA)) return;

    // Do Rotate
    switch(ImageInfo.Orientation)
    {
        case 0:	// Undefined
        case 1:	// Normal
          break;
        case 2:	// flip horizontal: left right reversed mirror
          tmppixbuf = gdk_pixbuf_flip(pix, TRUE);
          g_object_unref( pix );
          break;
        case 3:	// rotate 180
          tmppixbuf = gdk_pixbuf_rotate_simple(pix, 180);
          g_object_unref( pix );
          break;
        case 4:	// flip vertical: upside down mirror
          tmppixbuf = gdk_pixbuf_flip(pix, FALSE);
          g_object_unref( pix );
          break;
        case 5:	// transpose: Flipped about top-left <--> bottom-right axis.
          tmppixbuf = gtk_pixbuf_flip(pix, FALSE);
          g_object_unref( pix );
          pix = tmppixbuf;
          tmppixbuf = gtk_pixbuf_rotate_simple(pix, 270);
          g_object_unref( pix );
          break;
        case 6:	// rotate 90: rotate 90 cw to right it.
          tmppixbuf = gdk_pixbuf_rotate_simple(pix, 270);
          g_object_unref( pix );
          break;
        case 7:	// transverse: flipped about top-right <--> bottom-left axis
          tmppixbuf = gtk_pixbuf_flip(pix, FALSE);
          g_object_unref( pix );
          pix = tmppixbuf;
          tmppixbuf = gtk_pixbuf_rotate_simple(pix, 90);
          g_object_unref( pix );
          break;
        case 8:	// rotate 270: rotate 270 to right it.
          tmppixbuf = gdk_pixbuf_rotate_simple(pix, 90);
          g_object_unref( pix );
          break;
        default:
          break;
    }

    DiscardData();
#endif

    return tmppixbuf;
}

static void updateTitle(const char *filename, MainWin *mw )
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

gboolean main_win_open( MainWin* mw, const char* file_path, ZoomMode zoom )
{
    GError* err = NULL;
    GdkPixbufFormat* info;
    info = gdk_pixbuf_get_file_info( file_path, NULL, NULL );
    char* type = gdk_pixbuf_format_get_name( info );

    main_win_close( mw );
    mw->pix = gdk_pixbuf_new_from_file( file_path, &err );

    if( ! mw->pix )
    {
        main_win_show_error( mw, err->message );
        return FALSE;
    }
    else if(!strcmp(type,"jpeg"))
    {
        // Only jpeg should rotate by EXIF
        mw->pix = RotateByEXIF( file_path, mw->pix);
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
        gtk_toggle_button_set_active( (GtkToggleButton*)mw->btn_fit, TRUE );
        main_win_fit_window_size( mw, FALSE, GDK_INTERP_BILINEAR );
    }
    else  if( mw->zoom_mode == ZOOM_SCALE )  // scale
    {
        gtk_toggle_button_set_active( (GtkToggleButton*)mw->btn_orig, FALSE );
        gtk_toggle_button_set_active( (GtkToggleButton*)mw->btn_fit, FALSE );
        main_win_scale_image( mw, mw->scale, GDK_INTERP_BILINEAR );
    }
    else  if( mw->zoom_mode == ZOOM_ORIG )  // original size
    {
        gtk_toggle_button_set_active( (GtkToggleButton*)mw->btn_orig, TRUE );
        image_view_set_scale( (ImageView*)mw->img_view, mw->scale, GDK_INTERP_BILINEAR );
        main_win_center_image( mw );
    }

    image_view_set_pixbuf( (ImageView*)mw->img_view, mw->pix );

//    while (gtk_events_pending ())
//        gtk_main_iteration ();

    // build file list
    char* dir_path = g_path_get_dirname( file_path );
    image_list_open_dir( mw->img_list, dir_path, NULL );
    //image_list_sort_by_name( mw->img_list, GTK_SORT_ASCENDING );
    image_list_sort_by_name( mw->img_list, GTK_SORT_DESCENDING );
    g_free( dir_path );

    char* base_name = g_path_get_basename( file_path );
    image_list_set_current( mw->img_list, base_name );

    char* disp_name = g_filename_display_name( base_name );
    g_free( base_name );

    updateTitle( disp_name, mw );
    g_free( disp_name );

    return TRUE;
}

void main_win_close( MainWin* mw )
{
    if( mw->pix )
    {
        g_object_unref( mw->pix );
        mw->pix = NULL;
    }
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
    if( GTK_WIDGET_REALIZED (widget) )
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
    if( state->new_window_state == GDK_WINDOW_STATE_FULLSCREEN )
    {
        static GdkColor black = {0};
        gtk_widget_modify_bg( mw->evt_box, GTK_STATE_NORMAL, &black );
        gtk_widget_hide( gtk_widget_get_parent(mw->nav_bar) );
        mw->full_screen = TRUE;
    }
    else
    {
//        gtk_widget_reset_rc_styles( mw->evt_box );
        static GdkColor white = {0, 65535, 65535, 65535};
        gtk_widget_modify_bg( mw->evt_box, GTK_STATE_NORMAL, &white );
        gtk_widget_show( gtk_widget_get_parent(mw->nav_bar) );
        mw->full_screen = FALSE;
    }
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
        
        updateTitle(NULL, mw);
    }
}

void main_win_fit_window_size(  MainWin* mw, gboolean can_strech, GdkInterpType type )
{
    mw->zoom_mode = ZOOM_FIT;

    if( mw->pix == NULL )
        return;

    main_win_fit_size( mw, mw->scroll->allocation.width, mw->scroll->allocation.height, can_strech, type );
}

GtkWidget* add_nav_btn( MainWin* mw, const char* icon, const char* tip, GCallback cb, gboolean toggle )
{
    GtkWidget* img = gtk_image_new_from_stock(icon, GTK_ICON_SIZE_SMALL_TOOLBAR);
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
    gtk_tooltips_set_tip( mw->tooltips, btn, tip, NULL );
    gtk_box_pack_start( (GtkBox*)mw->nav_bar, btn, FALSE, FALSE, 0 );
    return btn;
}

void on_zoom_fit_menu( GtkMenuItem* item, MainWin* mw )
{
    gtk_button_clicked( (GtkButton*)mw->btn_fit );
}

void on_zoom_fit( GtkToggleButton* btn, MainWin* mw )
{
    if( ! btn->active )
    {
        if( mw->zoom_mode == ZOOM_FIT )
            gtk_toggle_button_set_active( btn, TRUE );
        return;
    }
    gtk_toggle_button_set_active( (GtkToggleButton*)mw->btn_orig, FALSE );

    main_win_fit_window_size( mw, FALSE, GDK_INTERP_BILINEAR );
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

    if( ! btn->active )
    {
        if( mw->zoom_mode == ZOOM_ORIG )
            gtk_toggle_button_set_active( btn, TRUE );
        return;
    }
    mw->zoom_mode = ZOOM_ORIG;
    mw->scale = 1.0;
//    gtk_scrolled_window_set_policy( (GtkScrolledWindow*)mw->scroll,
//                                     GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );

    // update scale
    updateTitle(NULL, mw);
    
    gtk_toggle_button_set_active( (GtkToggleButton*)mw->btn_fit, FALSE );

    if( ! mw->pix )
        return;

    image_view_set_scale( (ImageView*)mw->img_view, 1.0, GDK_INTERP_BILINEAR );

    while (gtk_events_pending ())
        gtk_main_iteration ();

    main_win_center_image( mw ); // FIXME:  mw doesn't work well. Why?
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
        main_win_open( mw, file_path, ZOOM_FIT );
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
        main_win_open( mw, file_path, ZOOM_FIT );
        g_free( file_path );
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
    else if(i == -180)	return 4;
}

static int get_new_angle( int orig_angle, int rotate_angle )
{
    // defined in exif.c
    extern int ExifRotateFlipMapping[9][9];
    static int angle_trans_back[] = {0, 0, -90, 180, -180, -135, 90, -45, 270};
    
    orig_angle = trans_angle_to_id(orig_angle);
    rotate_angle = trans_angle_to_id(rotate_angle);
  
    return angle_trans_back[ ExifRotateFlipMapping[orig_angle][rotate_angle] ];
}

void on_rotate_clockwise( GtkWidget* btn, MainWin* mw )
{
    rotate_image( mw, GDK_PIXBUF_ROTATE_CLOCKWISE );
    mw->rotation_angle = get_new_angle(mw->rotation_angle, 90);
    if(pref.auto_save_rotated){
        pref.ask_before_save = FALSE;
        on_save(btn,mw);
        pref.ask_before_save = TRUE;
    }
}

void on_rotate_counterclockwise( GtkWidget* btn, MainWin* mw )
{
    rotate_image( mw, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE );
    mw->rotation_angle = get_new_angle(mw->rotation_angle, 270);
    if(pref.auto_save_rotated){
        pref.ask_before_save = FALSE;
        on_save(btn,mw);
        pref.ask_before_save = TRUE;
    }
}

void on_flip_vertical( GtkWidget* btn, MainWin* mw )
{
    rotate_image( mw, -180 );
    mw->rotation_angle = get_new_angle(mw->rotation_angle, -180);
    if(pref.auto_save_rotated){
        pref.ask_before_save = FALSE;
        on_save(btn,mw);
        pref.ask_before_save = TRUE;
    }
}

void on_flip_horizontal( GtkWidget* btn, MainWin* mw )
{
    rotate_image( mw, -90 );
    mw->rotation_angle = get_new_angle(mw->rotation_angle, -90);
    if(pref.auto_save_rotated){
        pref.ask_before_save = FALSE;
        on_save(btn,mw);
        pref.ask_before_save = TRUE;
    }
}

///////////////////// end of rotate & flip

static void on_update_preview( GtkFileChooser *chooser, GtkImage* img )
{
    char* file = gtk_file_chooser_get_preview_filename( chooser );
    GdkPixbuf* pix = NULL;
    if( file )
    {
        pix = gdk_pixbuf_new_from_file_at_scale( file, 128, 128, TRUE, NULL );
        g_free( file );
    }
    if( pix )
    {
        gtk_image_set_from_pixbuf( img, pix );
        g_object_unref( pix );
    }
}

void on_save_as( GtkWidget* btn, MainWin* mw )
{
    if( ! mw->pix )
        return;

    GtkFileChooser* dlg = (GtkFileChooser*)gtk_file_chooser_dialog_new( NULL, (GtkWindow*)mw,
            GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL,
            GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL );

    gtk_file_chooser_set_current_folder( dlg, image_list_get_dir( mw->img_list ) );

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
        main_win_save( mw, file, type, TRUE );
        g_free( file );
        g_free( type );
    }
    gtk_widget_destroy( (GtkWidget*)dlg );
}

#ifdef HAVE_LIBJPEG
int rotate_and_save_jpeg_lossless(char *  filename,int angle){

    char tmpfilename[] = {"/tmp/rot.jpg.XXXXXX"};
    int tmpfilefd;
    FILE *cp_in;
    FILE *cp_out;
    char cp_buf[512];

    if(angle < 0)
        return -1;
    
    JXFORM_CODE code = JXFORM_NONE;

    angle = angle % 360;

    if(angle == 90)
        code = JXFORM_ROT_90;
    else if(angle == 180)
        code = JXFORM_ROT_180;
    else if(angle == 270)
        code = JXFORM_ROT_270;

    /* create tmp file */
    tmpfilefd = mkstemp(tmpfilename);
    if (tmpfilefd == -1) {
      return -1;
    }
    close(tmpfilefd);

    //rotate the image and save it to /tmp/rot.jpg
    int error = jpegtran (filename, tmpfilename , code);
    if(error)
        return error;

    //now rename /tmp/rot.jpg back to the original file
    if (g_rename(tmpfilename, filename) == -1) {
      unlink(tmpfilename);
      g_error("Unable to rename %s to %s", tmpfilename, filename);
      /* remove tmp file */
    }
    return 0;
}
#endif

void on_save( GtkWidget* btn, MainWin* mw )
{
    if( ! mw->pix )
        return;

    char* file_name = g_build_filename( image_list_get_dir( mw->img_list ),
                                        image_list_get_current( mw->img_list ), NULL );
    GdkPixbufFormat* info;
    info = gdk_pixbuf_get_file_info( file_name, NULL, NULL );
    char* type = gdk_pixbuf_format_get_name( info );

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
            if(rotate_and_save_jpeg_lossless(file_name,mw->rotation_angle)!=0)
            {
                main_win_show_error(mw, "Save failed! Check permissions.");
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
    GtkFileChooser* dlg = (GtkFileChooser*)gtk_file_chooser_dialog_new( NULL, (GtkWindow*)mw,
            GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL,
            GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL );

    if( image_list_get_dir( mw->img_list ) )
        gtk_file_chooser_set_current_folder( dlg, image_list_get_dir( mw->img_list ) );

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
        main_win_open( mw, file, ZOOM_NONE );
        g_free( file );
    }
}

void on_zoom_in( GtkWidget* btn, MainWin* mw )
{
    mw->zoom_mode = ZOOM_SCALE;
    gtk_toggle_button_set_active( (GtkToggleButton*)mw->btn_fit, FALSE );
    gtk_toggle_button_set_active( (GtkToggleButton*)mw->btn_orig, FALSE );
//    gtk_scrolled_window_set_policy( (GtkScrolledWindow*)mw->scroll,
//                                     GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );

    double scale = mw->scale;
    if( mw->pix && scale < 20.0 )
    {
//        busy(TRUE);
            scale *= 1.05;
            if( scale > 20.0 )
                scale = 20.0;
            if( mw->scale != scale )
                main_win_scale_image( mw, scale, GDK_INTERP_BILINEAR );
//        adjust_adjustment_on_zoom(oldscale);
//        busy(FALSE);
    }
}

void on_zoom_out( GtkWidget* btn, MainWin* mw )
{
    mw->zoom_mode = ZOOM_SCALE;
    gtk_toggle_button_set_active( (GtkToggleButton*)mw->btn_fit, FALSE );
    gtk_toggle_button_set_active( (GtkToggleButton*)mw->btn_orig, FALSE );
//    gtk_scrolled_window_set_policy( (GtkScrolledWindow*)mw->scroll,
//                                     GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );

    double scale = mw->scale;
    if( mw->pix && scale > 0.02 )
    {
//        busy(TRUE);

        scale /= 1.05;
        if( scale < 0.02 )
            scale = 0.02;
        if( mw->scale != scale )
            main_win_scale_image( mw, scale, GDK_INTERP_BILINEAR );
//        adjust_adjustment_on_zoom(oldscale);
//        busy(FALSE);
    }
}

void on_preference( GtkWidget* btn, MainWin* mw )
{
    edit_preferences( (GtkWindow*)mw );
}

void on_quit( GtkWidget* btn, MainWin* mw )
{
    gtk_widget_destroy( (GtkWidget*)mw );
}

gboolean on_button_press( GtkWidget* widget, GdkEventButton* evt, MainWin* mw )
{
    if( ! GTK_WIDGET_HAS_FOCUS( widget ) )
        gtk_widget_grab_focus( widget );

    if( evt->type == GDK_BUTTON_PRESS)
    {
        if( evt->button == 1 )    // left button
        {
            if( ! mw->pix )
                return FALSE;
            mw->dragging = TRUE;
            gtk_widget_get_pointer( (GtkWidget*)mw, &mw->drag_old_x ,&mw->drag_old_y );
            gdk_window_set_cursor( widget->window, mw->hand_cursor );
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

    if( ABS(dx) > 4 )
    {
        mw->drag_old_x = cur_x;
        if( req.width > hadj->page_size )
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
        if( req.height > vadj->page_size )
        {
            mw->drag_old_y = cur_y;
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

gboolean on_button_release( GtkWidget* widget, GdkEventButton* evt, MainWin* mw )
{
    mw->dragging = FALSE;
    gdk_window_set_cursor( widget->window, NULL );
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
//        case GDK_S:
            on_save( NULL, mw );
            break;
        case GDK_l:
//        case GDK_L:
            on_rotate_counterclockwise( NULL, mw );
            break;
        case GDK_r:
//        case GDK_R:
            on_rotate_clockwise( NULL, mw );
            break;
        case GDK_f:
//        case GDK_F:
            if( mw->zoom_mode != ZOOM_FIT )
                gtk_button_clicked((GtkButton*)mw->btn_fit );
            break;
        case GDK_g:
//        case GDK_G:
            if( mw->zoom_mode != ZOOM_ORIG )
                gtk_button_clicked((GtkButton*)mw->btn_orig );
            break;
        case GDK_o:
//        case GDK_O:
            on_open( NULL, mw );
            break;
        case GDK_Delete:
        case GDK_d:
//        case GDK_D:
            on_delete( NULL, mw );
            break;
        case GDK_Escape:
            if( mw->full_screen )
                on_full_screen( NULL, mw );
            else
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

    if( req.width > hadj->page_size )
        gtk_adjustment_set_value(hadj, ( hadj->upper - hadj->page_size ) / 2 );

    if( req.height > vadj->page_size )
        gtk_adjustment_set_value(vadj, ( vadj->upper - vadj->page_size ) / 2 );
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
    
    updateTitle( NULL, mw );
    
    return TRUE;
}

gboolean main_win_save( MainWin* mw, const char* file_path, const char* type, gboolean confirm )
{
    gboolean result1,gdk_save_supported;
    GSList *gdk_formats;
    GSList *gdk_formats_i;
    if( ! mw->pix )
        return FALSE;

    if( confirm )   // check existing file
    {
        if( g_file_test( file_path, G_FILE_TEST_EXISTS ) )
        {
            GtkWidget* dlg = gtk_message_dialog_new( (GtkWindow*)mw,
                    GTK_DIALOG_MODAL,
                    GTK_MESSAGE_QUESTION,
                    GTK_BUTTONS_YES_NO,
                    _("The file name you selected already exist.\nDo you want to overwrite existing file?\n(Warning: The quality of original image might be lost)") );
            if( gtk_dialog_run( (GtkDialog*)dlg ) != GTK_RESPONSE_YES )
            {
                gtk_widget_destroy( dlg );
                return FALSE;
            }
            gtk_widget_destroy( dlg );
        }
    }

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
        /* FIXME: we should show some error messages here when the type
           is not supported to save */
        return FALSE;
    }
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
    char* file_path = image_list_get_current_file_path( mw->img_list );
    if( file_path )
    {
        GtkWidget* dlg = gtk_message_dialog_new( (GtkWindow*)mw,
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_QUESTION,
                GTK_BUTTONS_YES_NO,
                _("Are you sure you want to delete current file?\n\nWarning: Once deleted, the file cannot be recovered.") );
        int resp = gtk_dialog_run( (GtkDialog*)dlg );
        gtk_widget_destroy( dlg );

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
		    gtk_main_quit();
	    }
        }
	g_free( file_path );
    }
}

void show_popup_menu( MainWin* mw, GdkEventButton* evt )
{
    static PtkMenuItemEntry menu_def[] =
    {
        PTK_IMG_MENU_ITEM( N_( "Previous" ), GTK_STOCK_GO_BACK, on_prev, GDK_leftarrow, 0 ),
        PTK_IMG_MENU_ITEM( N_( "Next" ), GTK_STOCK_GO_FORWARD, on_next, GDK_rightarrow, 0 ),
        PTK_SEPARATOR_MENU_ITEM,
        PTK_IMG_MENU_ITEM( N_( "Zoom Out" ), GTK_STOCK_ZOOM_OUT, on_zoom_out, GDK_minus, 0 ),
        PTK_IMG_MENU_ITEM( N_( "Zoom In" ), GTK_STOCK_ZOOM_IN, on_zoom_in, GDK_plus, 0 ),
        PTK_IMG_MENU_ITEM( N_( "Fit Image To Window Size" ), GTK_STOCK_ZOOM_FIT, on_zoom_fit_menu, GDK_F, 0 ),
        PTK_IMG_MENU_ITEM( N_( "Original Size" ), GTK_STOCK_ZOOM_100, on_orig_size_menu, GDK_G, 0 ),
        PTK_SEPARATOR_MENU_ITEM,
        PTK_IMG_MENU_ITEM( N_( "Full Screen" ), GTK_STOCK_FULLSCREEN, on_full_screen, GDK_F11, 0 ),
        PTK_SEPARATOR_MENU_ITEM,
        PTK_IMG_MENU_ITEM( N_( "Rotate Counterclockwise" ), "gtk-counterclockwise", on_rotate_counterclockwise, GDK_L, 0 ),
        PTK_IMG_MENU_ITEM( N_( "Rotate Clockwise" ), "gtk-clockwise", on_rotate_clockwise, GDK_R, 0 ),
//        PTK_IMG_MENU_ITEM( N_( "Flip Vertical" ), "gtk-clockwise", on_flip_vertical, GDK_V, 0 ),
//        PTK_IMG_MENU_ITEM( N_( "Flip Horizontal" ), "gtk-clockwise", on_flip_horizontal, GDK_H, 0 ),
        PTK_SEPARATOR_MENU_ITEM,
        PTK_IMG_MENU_ITEM( N_("Open File"), GTK_STOCK_OPEN, G_CALLBACK(on_open), GDK_O, 0 ),
        PTK_IMG_MENU_ITEM( N_("Save File"), GTK_STOCK_SAVE, G_CALLBACK(on_save), GDK_S, 0 ),
        PTK_IMG_MENU_ITEM( N_("Save As"), GTK_STOCK_SAVE_AS, G_CALLBACK(on_save_as), GDK_A, 0 ),
//        PTK_IMG_MENU_ITEM( N_("Save As Other Size"), GTK_STOCK_SAVE_AS, G_CALLBACK(on_save_as), GDK_A, 0 ),
        PTK_IMG_MENU_ITEM( N_("Delete File"), GTK_STOCK_DELETE, G_CALLBACK(on_delete), GDK_Delete, 0 ),
        PTK_SEPARATOR_MENU_ITEM,
        PTK_STOCK_MENU_ITEM( GTK_STOCK_ABOUT, on_about ),
        PTK_MENU_END
    };

    // mw accel group is useless. It's only used to display accels in popup menu
    GtkAccelGroup* accel_group = gtk_accel_group_new();
    GtkMenuShell* popup = (GtkMenuShell*)ptk_menu_new_from_data( menu_def, mw, accel_group );

    gtk_widget_show_all( (GtkWidget*)popup );
    g_signal_connect( popup, "selection-done", G_CALLBACK(gtk_widget_destroy), NULL );
    gtk_menu_popup( (GtkMenu*)popup, NULL, NULL, NULL, NULL, evt->button, evt->time );
}

/* callback used to open default browser when URLs got clicked */
static void open_url( GtkAboutDialog *dlg, const gchar *url, gpointer data)
{
    /* FIXME: is there any better way to do this? */
    char* programs[] = { "xdg-open", "gnome-open" /* Sorry, KDE users. :-P */, "exo-open" };
    int i;
    for(  i = 0; i < G_N_ELEMENTS(programs); ++i)
    {
        char* open_cmd = NULL;
        if( (open_cmd = g_find_program_in_path( programs[i] )) )
        {
             gchar* argv [3];
             argv [0] = programs[i];
             argv [1] = url;
             argv [2] = NULL;
             g_spawn_async (NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL); 
             g_free( open_cmd );
             break;
        }
    }
}

void on_about( GtkWidget* menu, MainWin* mw )
{
    GtkWidget * about_dlg;
    const gchar *authors[] =
    {
        "洪任諭 Hong Jen Yee <pcman.tw@gmail.com>",
        "Martin Siggel <martinsiggel@googlemail.com>",
        "Hialan Liu <hialan.liu@gmail.com>",
        _(" * Refer to source code of EOG image viewer and GThumb"),
        NULL
    };
    /* TRANSLATORS: Replace mw string with your names, one name per line. */
    gchar *translators = _( "translator-credits" );

    gtk_about_dialog_set_url_hook( open_url, mw, NULL);

    about_dlg = gtk_about_dialog_new ();

    gtk_container_set_border_width ( ( GtkContainer*)about_dlg , 2 );
    gtk_about_dialog_set_version ( (GtkAboutDialog*)about_dlg, VERSION );
    gtk_about_dialog_set_name ( (GtkAboutDialog*)about_dlg, _( "GPicView" ) );
    gtk_about_dialog_set_logo( (GtkAboutDialog*)about_dlg, gdk_pixbuf_new_from_file(  PACKAGE_DATA_DIR"/pixmaps/gpicview.png", NULL ) );
    gtk_about_dialog_set_copyright ( (GtkAboutDialog*)about_dlg, _( "Copyright (C) 2007" ) );
    gtk_about_dialog_set_comments ( (GtkAboutDialog*)about_dlg, _( "Lightweight image viewer from LXDE project" ) );
    gtk_about_dialog_set_license ( (GtkAboutDialog*)about_dlg, "GPicView\n\nCopyright (C) 2007 Hong Jen Yee (PCMan)\n\nmw program is free software; you can redistribute it and/or\nmodify it under the terms of the GNU General Public License\nas published by the Free Software Foundation; either version 2\nof the License, or (at your option) any later version.\n\nmw program is distributed in the hope that it will be useful,\nbut WITHOUT ANY WARRANTY; without even the implied warranty of\nMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\nGNU General Public License for more details.\n\nYou should have received a copy of the GNU General Public License\nalong with mw program; if not, write to the Free Software\nFoundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA." );
    gtk_about_dialog_set_website ( (GtkAboutDialog*)about_dlg, "http://lxde.org/gpicview/" );
    gtk_about_dialog_set_authors ( (GtkAboutDialog*)about_dlg, authors );
    gtk_about_dialog_set_translator_credits ( (GtkAboutDialog*)about_dlg, translators );
    gtk_window_set_transient_for( (GtkWindow*) about_dlg, GTK_WINDOW( mw ) );

    gtk_dialog_run( ( GtkDialog*)about_dlg );
    gtk_widget_destroy( about_dlg );
}

void on_drag_data_received( GtkWidget* widget, GdkDragContext *drag_context,
                int x, int y, GtkSelectionData* data, guint info, guint time, MainWin* mw )
{
    if( ! data || data->length <= 0)
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
