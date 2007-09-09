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
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "mainwin.h"

using namespace std;

/*
static GOptionEntry entries[] = 
{
    { NULL }
};
*/

#define PIXMAP_DIR		PACKAGE_DATA_DIR "/gpicview/pixmaps/"

void register_icons()
{
    GtkIconFactory* factory = gtk_icon_factory_new();
    GdkPixbuf* pix = NULL;
    GtkIconSet* set = NULL;

#ifndef GTK_STOCK_FULLSCREEN    // before gtk+ 2.8
#define GTK_STOCK_FULLSCREEN    "gtk-fullscreen"
    pix = gdk_pixbuf_new_from_file( PIXMAP_DIR "fullscreen.png", NULL);
    set = gtk_icon_set_new_from_pixbuf(pix);
    gdk_pixbuf_unref( pix );
    gtk_icon_factory_add( factory, GTK_STOCK_FULLSCREEN, set );
#endif
    pix = gdk_pixbuf_new_from_file( PIXMAP_DIR"clockwise.png", NULL);
    set = gtk_icon_set_new_from_pixbuf(pix);
    gdk_pixbuf_unref( pix );
    gtk_icon_factory_add( factory, "gtk-clockwise", set );

    pix = gdk_pixbuf_new_from_file( PIXMAP_DIR"counterclockwise.png", NULL);
    set = gtk_icon_set_new_from_pixbuf(pix);
    gdk_pixbuf_unref( pix );
    gtk_icon_factory_add( factory, "gtk-counterclockwise", set );

    gtk_icon_factory_add_default( factory );
}

int main(int argc, char *argv[])
{
/*
    GError *error = NULL;
    GOptionContext *context;

    context = g_option_context_new ("- simple image viewer");
    g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
    g_option_context_add_group (context, gtk_get_option_group (TRUE));
    g_option_context_parse (context, &argc, &argv, &error);
*/

    gtk_init( &argc, &argv );

#ifdef ENABLE_NLS
    bindtextdomain ( GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR );
    bind_textdomain_codeset ( GETTEXT_PACKAGE, "UTF-8" );
    textdomain ( GETTEXT_PACKAGE );
#endif

    register_icons();

    MainWin* win = MainWin::create();
    gtk_widget_show( GTK_WIDGET(win) );

    if( argc > 1 ) {
        g_debug(argv[1]);
        win->open( argv[1] );
    }

    gtk_main();
    return 0;
}
