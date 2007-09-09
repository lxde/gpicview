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
#ifndef MAINWIN_H
#define MAINWIN_H

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "imagelist.h"

/**
	@author PCMan (Hong Jen Yee) <pcman.tw@gmail.com>
*/

#define MAIN_WIN_TYPE        (MainWin::_get_type ())
#define MAIN_WIN(obj)        (G_TYPE_CHECK_INSTANCE_CAST ((obj), MAIN_WIN_TYPE, MainWin))
#define MAIN_WIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MAIN_WIN_TYPE, MainWin::Class))
#define IS_MAIN_WIN(obj)     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MAIN_WIN_TYPE))
#define IS_MAIN_WIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MAIN_WIN_TYPE))
#define MAIN_WIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MAIN_WIN_TYPE, MainWin::Class))

class MainWin : public GtkWindow
{
public:
    struct Class : public GtkWindowClass
    {
    };
    enum ZoomMode{ ZOOM_FIT = 0, ZOOM_ORIG, ZOOM_OTHER };

    static MainWin* create(){ return (MainWin*)g_object_new ( MAIN_WIN_TYPE, NULL ); }

    static void _init( GTypeInstance *instance, gpointer g_class );
    static void _class_init( MainWin::Class* klass );
    static void _finalize(GObject *self);
    static GType _get_type();
    bool open( const char* file_path );
    void close();
    bool save( const char* file_path, const char* type, bool confirm=true );
    void show_error( const char* message );
    void fit_size( int width, int height, GdkInterpType type = GDK_INTERP_BILINEAR );
    void fit_window_size( GdkInterpType type = GDK_INTERP_BILINEAR );
    void center_image();
    bool scale_image( double new_scale, GdkInterpType type = GDK_INTERP_BILINEAR );

protected:
    MainWin();
    ~MainWin();

    static gpointer _parent_class;
    GtkWidget* zoom_btn;
    GtkWidget* img_view;
    GtkWidget* scroll;
    GdkPixbuf* pic_orig;
    GdkPixbuf* pic;
    GtkWidget* evt_box;
    GtkWidget* nav_bar;
//    GtkWidget* btn_zoom_in;
//    GtkWidget* btn_zoom_out;
    GtkWidget* btn_orig;
    GtkWidget* btn_fit;
    GtkTooltips* tooltips;
    GdkCursor* hand_cursor;
    bool full_screen;
    ZoomMode zoom_mode;

    bool dragging;
    double scale;
    int drag_old_x;
    int drag_old_y;
    ImageList img_list;

    void create_nav_bar( GtkWidget* box);
    static gboolean on_delete_event( GtkWidget* widget, GdkEventAny* evt );
    static void on_size_allocate( GtkWidget* widget, GtkAllocation    *allocation );
    GtkWidget* add_nav_btn( const char* icon, const char* tip, GCallback cb, bool toggle = false );
    GtkWidget* add_menu_item(  GtkMenuShell* menu, const char* label, const char* icon, GCallback cb, bool toggle=false );
    static void on_zoom_fit( GtkToggleButton* btn, MainWin* self );
    static void on_zoom_fit_menu( GtkMenuItem* item, MainWin* self );
    static void on_full_screen( GtkWidget* btn, MainWin* self );
    static void on_next( GtkWidget* btn, MainWin* self );
    static void on_orig_size( GtkToggleButton* btn, MainWin* self );
    static void on_orig_size_menu( GtkToggleButton* btn, MainWin* self );
    static void on_prev( GtkWidget* btn, MainWin* self );
    static void on_rotate_clockwise( GtkWidget* btn, MainWin* self );
    static void on_rotate_counterclockwise( GtkWidget* btn, MainWin* self );
    static void on_save_as( GtkWidget* btn, MainWin* self );
    static void on_save( GtkWidget* btn, MainWin* self );
    static void on_open( GtkWidget* btn, MainWin* self );
    static void on_zoom_in( GtkWidget* btn, MainWin* self );
    static void on_zoom_out( GtkWidget* btn, MainWin* self );
    static void on_preference( GtkWidget* btn, MainWin* self );
    static void on_quit( GtkWidget* btn, MainWin* self );
    static gboolean on_button_press( GtkWidget* widget, GdkEventButton* evt, MainWin* self );
    static gboolean on_button_release( GtkWidget* widget, GdkEventButton* evt, MainWin* self );
    static gboolean on_mouse_move( GtkWidget* widget, GdkEventMotion* evt, MainWin* self );
    static gboolean on_key_press_event(GtkWidget* widget, GdkEventKey * key);
    static void on_drag_data_received( GtkWidget* widget, GdkDragContext *drag_context, 
                                                                                           int x, int y, GtkSelectionData* data, guint info, 
                                                                                           guint time, MainWin* self );
    void rotate_image( GdkPixbufRotation angle );

protected:
    static void on_delete( GtkWidget* btn, MainWin* self );
    void show_popup_menu( GdkEventButton* evt );
    static void on_about( GtkWidget* menu, MainWin* self );
};

#endif
