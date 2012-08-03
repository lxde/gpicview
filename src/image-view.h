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

#ifndef IMAGE_VIEW_H
#define IMAGE_VIEW_H

#include <gtk/gtk.h>
#include <gdk/gdk.h>

/**
    @author PCMan (Hong Jen Yee) <pcman.tw@gmail.com>
*/

#define IMAGE_VIEW_TYPE        ( image_view_get_type ())
#define IMAGE_VIEW(obj)        (G_TYPE_CHECK_INSTANCE_CAST ((obj), IMAGE_VIEW_TYPE, ImageView))
#define IMAGE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), IMAGE_VIEW_TYPE, ImageViewClass))
#define IS_IMAGE_VIEW(obj)     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IMAGE_VIEW_TYPE))
#define IS_IMAGE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), IMAGE_VIEW_TYPE))
#define IMAGE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), IMAGE_VIEW_TYPE, ImageViewClass))

typedef struct _ImageViewClass
{
    GtkMiscClass parent_class;
    void (*set_scroll_adjustments)( GtkWidget* w, GtkAdjustment* h, GtkAdjustment* v );
}ImageViewClass;

typedef struct _ImageView
{
    GtkMisc parent;

    GtkAdjustment *hadj, *vadj;
    GdkPixbuf* pix;
//    GdkPixmap* buffer;
//    bool cached;
    gdouble scale;
    GdkInterpType interp_type;
    guint idle_handler;
    GdkRectangle img_area;
    GtkAllocation allocation;
}ImageView;

GtkWidget* image_view_new();

void image_view_set_pixbuf( ImageView* iv, GdkPixbuf* pixbuf );

gdouble image_view_get_scale( ImageView* iv );
void image_view_set_scale( ImageView* iv, gdouble new_scale, GdkInterpType type );
void image_view_get_size( ImageView* iv, int* w, int* h );

void image_view_set_adjustments( ImageView* iv,
		GtkAdjustment* h, GtkAdjustment* v );

GType image_view_get_type();

#endif
