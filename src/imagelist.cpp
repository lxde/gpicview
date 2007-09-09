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

#include "imagelist.h"

#include <string.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

GSList* ImageList::supported_formats = NULL;

ImageList::ImageList()
{
    if( ! supported_formats )
    {
        GSList* formats = gdk_pixbuf_get_formats();
        GSList* format;
        for( format = formats; format; format = format->next )
        {
            char** exts = gdk_pixbuf_format_get_extensions( (GdkPixbufFormat*)format->data );
            char** ext;
            for( ext  = exts; *ext ; ++ext )
                supported_formats = g_slist_prepend( supported_formats, *ext );
            g_free( exts ); // g_strfreev is not needed since we stole its stirngs
        }
        // supported_formats = g_slist_reverse( supported_formats, *ext );
    }
}

ImageList::~ImageList()
{
}

bool ImageList::open_dir( const char* path, GError** error )
{
    if( dir_path && 0 == strcmp( path, dir_path ) )
        return true;

    close();

    GDir* dir = g_dir_open( path, 0, error );
    if( ! dir )
        return false;

    dir_path = g_strdup( path );

    const char* name = NULL;
    while( name = g_dir_read_name ( dir ) )
    {
//        char* file_path = g_build_filename( dir_path, name, NULL );
        if( is_file_supported( name ) )
            list = g_list_prepend( list, g_strdup(name) );
//        g_free( file_path );
    }
    g_dir_close( dir );
    list = g_list_reverse(list);
    current = list;
    return true;
}

bool ImageList::set_current( const char* name )
{
    if( ! list || !name )
        return false;

    GList* cur = g_list_find_custom( list, name, (GCompareFunc)strcmp );
    if( ! cur )
        return false;
    current = cur;
    return true;
}

const char* ImageList::get_first()
{
    current = list;
    return get_current();
}

const char* ImageList::get_next()
{
    if( current && current->next )
    {
        current = current->next;
        return get_current();
    }
    return NULL;
}

const char* ImageList::get_prev()
{
    if( current && current->prev )
    {
        current = current->prev;
        return get_current();
    }
    return NULL;
}

const char* ImageList::get_last()
{
    current = g_list_last( list );
    return get_current();
}

void ImageList::close()
{
    g_list_foreach( list, (GFunc)g_free, NULL );
    g_list_free( list );
    list = NULL;

    g_free( dir_path );
    dir_path = NULL;
}

bool ImageList::is_file_supported( const char* name )
{
    const char* ext = strrchr( name, '.' );
    if( ! ext )
        return false;
    ++ext;

    return !!g_slist_find_custom ( supported_formats, ext,  (GCompareFunc)g_ascii_strcasecmp);
}

char* ImageList::get_current_file_path()  const
{
    const char* name;
    if( dir_path && (name = get_current()) )
        return g_build_filename( dir_path, name, NULL );
    return NULL;
}
