//
// C++ Interface: image-view
//
// Description: 
//
//
// Author: PCMan (Hong Jen Yee) <pcman.tw@gmail.com>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef IMAGE_VIEW_H
#define IMAGE_VIEW_H

#include <gtk/gtk.h>
#include <gdk/gdk.h>

/**
	@author PCMan (Hong Jen Yee) <pcman.tw@gmail.com>
*/

#define IMAGE_VIEW_TYPE        (ImageView::_get_type ())
#define IMAGE_VIEW(obj)        (G_TYPE_CHECK_INSTANCE_CAST ((obj), IMAGE_VIEW_TYPE, ImageView))
#define IMAGE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), IMAGE_VIEW_TYPE, ImageView::Class))
#define IS_IMAGE_VIEW(obj)     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IMAGE_VIEW_TYPE))
#define IS_IMAGE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), IMAGE_VIEW_TYPE))
#define IMAGE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), IMAGE_VIEW_TYPE, ImageView::Class))

class ImageView : public GtkMisc
{
public:
    struct Class : public GtkMiscClass
    {
        void (*set_scroll_adjustments)( GtkWidget* w, GtkAdjustment* h, GtkAdjustment* v );
    };

    static ImageView* create(){ return (ImageView*)g_object_new ( IMAGE_VIEW_TYPE, NULL ); }

    static void _init( GTypeInstance *instance, gpointer g_class );
    static void _class_init( ImageView::Class* klass );
    static void _finalize(GObject *self);
    static GType _get_type();

    void set_pixbuf( GdkPixbuf* pixbuf );

    gdouble get_scale() const { return scale;  }
    void set_scale( gdouble new_scale, GdkInterpType type = GDK_INTERP_BILINEAR );
    void set_adjustments( GtkAdjustment* h, GtkAdjustment* v );

protected:
    static gpointer _parent_class;

    GtkAdjustment *hadj, *vadj;
    GdkPixbuf* pix;
    gdouble scale;
    GdkInterpType interp_type;
    guint idle_handler;
    GdkRectangle img_area;

    ImageView();
    ~ImageView();

    static void on_size_request( GtkWidget* w, GtkRequisition* req );
    static gboolean on_expose_event( GtkWidget* widget, GdkEventExpose* evt );
    static void on_size_allocate( GtkWidget* widget, GtkAllocation    *allocation );

    void paint( GdkEventExpose& evt );
    void clear();
    static gboolean on_idle( ImageView* self );
    void calc_image_area();
    void paint( GdkRectangle* invalid_rect, GdkInterpType type = GDK_INTERP_BILINEAR );
};

#endif
