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
#ifndef IMAGELIST_H
#define IMAGELIST_H

#include <glib.h>
#include <gtk/gtk.h>

/**
	@author PCMan (Hong Jen Yee) <pcman.tw@gmail.com>
*/
class ImageList{
public:
    ImageList();
    ~ImageList();

    const char* get_dir() const { return dir_path; }
    bool open_dir( const char* path, GError** error = NULL  );
    bool set_current( const char* name );
    const char* get_current() const { return current ? (char*)current->data : NULL; }
    const char* get_first();
    const char* get_next();
    const char* get_prev();
    const char* get_last();
    void close();
//    static bool is_file_supported( char* file_path );
    static bool is_file_supported( const char* name );
    bool is_empty() const { return (list == NULL); }
    bool has_multiple_files() const { return (list && list->next); }
    char* get_current_file_path()  const;
    void sort_by_name( GtkSortType type=GTK_SORT_ASCENDING );

protected:
    gchar* dir_path;
    GList* list;
    GList* current;
    static GSList* supported_formats;
};

#endif
