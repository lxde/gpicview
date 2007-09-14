//
// C++ Implementation: image-view
//
// Description: 
//
//
// Author: PCMan (Hong Jen Yee) <pcman.tw@gmail.com>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "image-view.h"

#include <new>
#include <math.h>

gpointer ImageView::_parent_class = NULL;

void ImageView::_init( GTypeInstance *instance, gpointer g_class )
{
    ::new( instance ) ImageView();    // placement new
}

GType ImageView::_get_type()
{
    static GType g_define_type_id = 0;
    if (G_UNLIKELY (g_define_type_id == 0))
    {
        static const GTypeInfo g_define_type_info = {
            sizeof (ImageView::Class),
            (GBaseInitFunc) NULL,
            (GBaseFinalizeFunc) NULL,
            (GClassInitFunc) ImageView::_class_init,
            (GClassFinalizeFunc) NULL,
            NULL,   /* class_data */
            sizeof (ImageView),
            0,      /* n_preallocs */
            (GInstanceInitFunc) ImageView::_init,
        };
        g_define_type_id = g_type_register_static (GTK_TYPE_MISC, "ImageView", &g_define_type_info, (GTypeFlags)0);
    }
    return g_define_type_id;
}

/*
static void _marshal_VOID__OBJECT_OBJECT (GClosure     *closure,
                                          GValue       *return_value,
                                          guint         n_param_values,
                                          const GValue *param_values,
                                          gpointer      invocation_hint,
                                          gpointer      marshal_data)
{
    typedef void (*GMarshalFunc_VOID__OBJECT_OBJECT) (gpointer     data1,
                   gpointer     arg_1,
                   gpointer     arg_2,
                   gpointer     data2);
    register GMarshalFunc_VOID__OBJECT_OBJECT callback;
    register GCClosure *cc = (GCClosure*) closure;
    register gpointer data1, data2;

    g_return_if_fail (n_param_values == 3);

    if (G_CCLOSURE_SWAP_DATA (closure))
    {
        data1 = closure->data;
        data2 = g_value_peek_pointer (param_values + 0);
    }
    else
    {
        data1 = g_value_peek_pointer (param_values + 0);
        data2 = closure->data;
    }
    callback = (GMarshalFunc_VOID__OBJECT_OBJECT) (marshal_data ? marshal_data : cc->callback);

    callback (data1,
                        g_value_get_object (param_values + 1),
                        g_value_get_object (param_values + 2),
                        data2);
}
*/

void ImageView::_class_init( ImageView::Class* klass )
{
    _parent_class = g_type_class_peek_parent (klass);

    GObjectClass * obj_class;
    GtkWidgetClass *widget_class;

    obj_class = ( GObjectClass * ) klass;
//    obj_class->set_property = _set_property;
//   obj_class->get_property = _get_property;
    obj_class->finalize = _finalize;

    widget_class = GTK_WIDGET_CLASS ( klass );
    widget_class->size_request = on_size_request;
    widget_class->size_allocate = on_size_allocate;
    widget_class->expose_event = on_expose_event;

/*
    // set up scrolling support
    klass->set_scroll_adjustments = set_adjustments;
    widget_class->set_scroll_adjustments_signal =
                g_signal_new ( "set_scroll_adjustments", 
                        G_TYPE_FROM_CLASS (obj_class),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (ImageView::Class, set_scroll_adjustments),
                        NULL, NULL,
                        _marshal_VOID__OBJECT_OBJECT,
                        G_TYPE_NONE, 2,
                        GTK_TYPE_ADJUSTMENT, GTK_TYPE_ADJUSTMENT);
*/
}

void ImageView::_finalize(GObject *self)
{
    ((ImageView*)self)->~ImageView();
}

// End of GObject-related stuff

ImageView::ImageView()
    : pix(NULL), scale( 1.0 ), interp_type(GDK_INTERP_BILINEAR), idle_handler(0)
{
    GTK_WIDGET_SET_FLAGS ((GtkWidget*)this, GTK_CAN_FOCUS);
}

ImageView::~ImageView()
{
    clear();
/*
    if( vadj )
        g_object_unref( vadj );
    if( hadj )
        g_object_unref( hadj );
*/
}

void ImageView::set_adjustments( GtkAdjustment* h, GtkAdjustment* v )
{
    if( hadj != h )
    {
        if( hadj )
            g_object_unref( hadj );
        if( G_LIKELY(h) )
        {
    #if GTK_CHECK_VERSION( 2, 10, 0 )
            hadj = (GtkAdjustment*)g_object_ref_sink( h );
    #else
            hadj = (GtkAdjustment*)h;
            gtk_object_sink( (GtkObject*)h );
    #endif
        }
        else
            hadj = NULL;
    }
    if( vadj != v )
    {
        if( vadj )
            g_object_unref( hadj );
        if( G_LIKELY(v) )
        {
#if GTK_CHECK_VERSION( 2, 10, 0 )
            vadj = (GtkAdjustment*)g_object_ref_sink( v );
#else
            vadj = (GtkAdjustment*)v;
            gtk_object_sink( (GtkObject*)v );
#endif
        }
        else
            vadj = NULL;
    }
}

void ImageView::on_size_request( GtkWidget* w, GtkRequisition* req )
{
    ImageView* self = (ImageView*)w;

    w->requisition.width = self->img_area.width;
    w->requisition.height = self->img_area.height;

    GTK_WIDGET_CLASS(_parent_class)->size_request (w, req);
}

void ImageView::on_size_allocate( GtkWidget* widget, GtkAllocation   *allocation )
{
    GTK_WIDGET_CLASS(_parent_class)->size_allocate( widget, allocation );
    ImageView* self = (ImageView*)widget;
/*
    if( !self->buffer || allocation->width != widget->allocation.width ||
        allocation->height != widget->allocation.height )
    {
        if( self->buffer )
            g_object_unref( self->buffer );
        self->buffer = gdk_pixmap_new( (GdkDrawable*)widget->window,
                                        allocation->width, allocation->height, -1 );
        g_debug( "off screen buffer created: %d x %d", allocation->width, allocation->height );
    }
*/
    self->calc_image_area();
}

gboolean ImageView::on_expose_event( GtkWidget* widget, GdkEventExpose* evt )
{
    ImageView* self = (ImageView*)widget;
    if( GTK_WIDGET_MAPPED (widget) )
        self->paint( *evt );
    return FALSE;
}

void ImageView::paint( GdkEventExpose& evt )
{
    GtkWidget* widget = (GtkWidget*)this;
/*
    if( cached )
    {
//        gdk_draw_drawable( drawable, widget->style->fg_gc[GTK_STATE_NORMAL], buffer,
//                         0, 0,  );
        return;
    }
*/

    if( pix )
    {
        GdkRectangle* rects = NULL;
        int n_rects = 0;
        gdk_region_get_rectangles( evt.region, &rects, &n_rects );

        for( int i = 0; i < n_rects; ++i )
        {
            // GdkRectangle& rect = rects[i];
            paint( rects + i, GDK_INTERP_NEAREST );
#if 0
            g_debug("dirty dest: x=%d, y=%d, w=%d, h=%d\nx_off=%d, y_off=%d",
                    rect.x, rect.y, rect.width, rect.height, img_area.x, img_area.y );

            if( ! gdk_rectangle_intersect( &rect, &img_area, &rect ) )
                continue;

            int dest_x = rect.x;
            int dest_y = rect.y;

            rect.x -= img_area.x;
            rect.y -= img_area.y;

            GdkPixbuf* src_pix = NULL;
            int src_x, src_y;
            if( scale == 1.0 )  // original size
            {
                src_pix = (GdkPixbuf*)g_object_ref( pix );
                src_x = rect.x;
                src_y = rect.y;
            }
            else    // scaling is needed
            {
                GdkPixbuf* scaled_pix = NULL;
                int src_w, src_h;
                src_x = (int)floor( gdouble(rect.x) / scale + 0.5 );
                src_y = (int)floor( gdouble(rect.y) / scale + 0.5 );
                src_w = (int)floor( gdouble(rect.width) / scale + 0.5 );
                src_h = (int)floor( gdouble(rect.height) / scale + 0.5 );
                if( src_y > gdk_pixbuf_get_height(pix) )
                    src_y = gdk_pixbuf_get_height(pix);
                if( src_x + src_w > gdk_pixbuf_get_width(pix) )
                    src_w = gdk_pixbuf_get_width(pix) - src_x;
                if( src_y + src_h > gdk_pixbuf_get_height(pix) )
                    src_h = gdk_pixbuf_get_height(pix) - src_y;
                g_debug("orig src: x=%d, y=%d, w=%d, h=%d",
                        src_x, src_y, src_w, src_h );

                src_pix = gdk_pixbuf_new_subpixbuf( pix, src_x, src_y,  src_w, src_h );
                scaled_pix = gdk_pixbuf_scale_simple( src_pix, rect.width, rect.height, interp_type );
                g_object_unref( src_pix );
                src_pix = scaled_pix;

                src_x = 0;
                src_y = 0;
            }

            if( G_LIKELY(src_pix) )
            {
                gdk_draw_pixbuf( widget->window,
                                widget->style->fg_gc[GTK_STATE_NORMAL],
                                src_pix,
                                src_x, src_y,
                                dest_x, dest_y,
                                rect.width, rect.height,
                                GDK_RGB_DITHER_NORMAL, 0, 0 );
                g_object_unref( src_pix );
            }
#endif
        }
        g_free( rects );

        if( 0 == idle_handler )
            idle_handler = g_idle_add( (GSourceFunc)on_idle, this );

    }
}

void ImageView::clear()
{
    if( idle_handler )
    {
        g_source_remove( idle_handler );
        idle_handler = 0;
    }

    if( pix )
    {
        g_object_unref( pix );
        pix = NULL;
        calc_image_area();
    }
/*
    if( buffer )
    {
        g_object_unref( buffer );
        buffer = NULL;
    }
*/
}

void ImageView::set_pixbuf( GdkPixbuf* pixbuf )
{
    if( pixbuf != pix )
    {
        clear();
        if( G_LIKELY(pixbuf) )
            pix = (GdkPixbuf*)g_object_ref( pixbuf );
        calc_image_area();
        gtk_widget_queue_resize( (GtkWidget*)this );
//        gtk_widget_queue_draw( (GtkWidget*)this );
    }
}

void ImageView::set_scale( gdouble new_scale, GdkInterpType type )
{
    if( new_scale == scale )
        return;
    gdouble factor = new_scale / scale;
    scale = new_scale;
    if( pix )
    {
        calc_image_area();
        gtk_widget_queue_resize( (GtkWidget*)this );
//        gtk_widget_queue_draw( (GtkWidget*)this );

/*
    // adjust scroll bars
        int vis_w = ((GtkWidget*)this)->allocation.width;
        if( hadj && vis_w < img_area.width )
        {
            hadj->upper = img_area.width;
            hadj->page_size = vis_w;
            gdouble new_w = (hadj->value + vis_w / 2) * factor - vis_w/2;
            hadj->value = CLAMP(  new_w, 0, hadj->upper - hadj->page_size );
            gtk_adjustment_value_changed( hadj );
        }
        int vis_h = ((GtkWidget*)this)->allocation.height;
        if( vadj && vis_h < img_area.height )
        {
            vadj->upper = img_area.height;
            vadj->page_size = vis_h;
            gdouble new_h = (vadj->value + vis_h / 2) * factor - vis_h/2;
            vadj->value = CLAMP(  new_h, 0, vadj->upper - vadj->page_size );
            gtk_adjustment_value_changed( vadj );
        }
*/
    }
}

gboolean ImageView::on_idle( ImageView* self )
{
    GDK_THREADS_ENTER();

    // FIXME: redraw the whole window everytime is very inefficient
    // There must be some way to optimize this. :-(
    GdkRectangle rect;

    if( G_LIKELY(self->hadj) )
    {
        rect.x = (int)gtk_adjustment_get_value(self->hadj);
        rect.width = (int)self->hadj->page_size;
    }
    else
    {
        rect.x = ((GtkWidget*)self)->allocation.x;
        rect.width = ((GtkWidget*)self)->allocation.width;
    }

    if( G_LIKELY(self->vadj) )
    {
        rect.y = (int)gtk_adjustment_get_value(self->vadj);
        rect.height = (int)self->vadj->page_size;
    }
    else
    {
        rect.y = ((GtkWidget*)self)->allocation.y;
        rect.height = ((GtkWidget*)self)->allocation.height;
    }

    self->paint( &rect, self->interp_type );

    GDK_THREADS_LEAVE();

    self->idle_handler = 0;
    return FALSE;
}

void ImageView::calc_image_area()
{
    GtkAllocation& allocation = ((GtkWidget*)this)->allocation;

    if( G_LIKELY(pix) )
    {
        img_area.width = (int)floor( gdouble(gdk_pixbuf_get_width( pix )) * scale + 0.5);
        img_area.height = (int)floor( gdouble(gdk_pixbuf_get_height( pix )) * scale + 0.5);
        // g_debug( "width=%d, height=%d", width, height );

        int x_offset = 0, y_offset = 0;
        if( img_area.width < allocation.width )
            x_offset = (int)floor(gdouble(allocation.width - img_area.width) / 2 + 0.5);
        if( img_area.height < allocation.height )
            y_offset = (int)floor(gdouble(allocation.height - img_area.height) / 2 + 0.5);

        img_area.x = x_offset;
        img_area.y = y_offset;
    }
    else
    {
        img_area.x = img_area.y = img_area.width = img_area.height = 0;
    }
}

void ImageView::paint( GdkRectangle* invalid_rect, GdkInterpType type )
{
    GdkRectangle rect;
    if( ! gdk_rectangle_intersect( invalid_rect, &img_area, &rect ) )
        return;

    int dest_x = rect.x;
    int dest_y = rect.y;

    rect.x -= img_area.x;
    rect.y -= img_area.y;

    GdkPixbuf* src_pix = NULL;
    int src_x, src_y;
    if( scale == 1.0 )  // original size
    {
        src_pix = (GdkPixbuf*)g_object_ref( pix );
        src_x = rect.x;
        src_y = rect.y;
    }
    else    // scaling is needed
    {
        GdkPixbuf* scaled_pix = NULL;
        int src_w, src_h;
        src_x = (int)floor( gdouble(rect.x) / scale + 0.5 );
        src_y = (int)floor( gdouble(rect.y) / scale + 0.5 );
        src_w = (int)floor( gdouble(rect.width) / scale + 0.5 );
        src_h = (int)floor( gdouble(rect.height) / scale + 0.5 );
        if( src_y > gdk_pixbuf_get_height(pix) )
            src_y = gdk_pixbuf_get_height(pix);
        if( src_x + src_w > gdk_pixbuf_get_width(pix) )
            src_w = gdk_pixbuf_get_width(pix) - src_x;
        if( src_y + src_h > gdk_pixbuf_get_height(pix) )
            src_h = gdk_pixbuf_get_height(pix) - src_y;
        //g_debug("orig src: x=%d, y=%d, w=%d, h=%d",
        //        src_x, src_y, src_w, src_h );

        src_pix = gdk_pixbuf_new_subpixbuf( pix, src_x, src_y,  src_w, src_h );
        scaled_pix = gdk_pixbuf_scale_simple( src_pix, rect.width, rect.height, type );
        g_object_unref( src_pix );
        src_pix = scaled_pix;

        src_x = 0;
        src_y = 0;
    }

    if( G_LIKELY(src_pix) )
    {
        GtkWidget* widget = (GtkWidget*)this;
        gdk_draw_pixbuf( widget->window,
                         widget->style->fg_gc[GTK_STATE_NORMAL],
                         src_pix,
                         src_x, src_y,
                         dest_x, dest_y,
                         rect.width, rect.height,
                         GDK_RGB_DITHER_NORMAL, 0, 0 );
        g_object_unref( src_pix );
    }
}

void ImageView::get_size( int* w, int* h )
{
    if( G_LIKELY(w) )
        *w = img_area.width;
    if( G_LIKELY(h) )
        *h = img_area.height;
}
