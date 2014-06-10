#ifndef FILE_LIST_H
#define FILE_LIST_H

#include "elist.h"


void fl_init(GladeXML *xml);

/*
 * Initializes the working dir. Must be called once at the start 
 * of the program. <dir> is in the filesystem encoding.
 * If <dir> is NULL, loads the most recently used dir.
 */
void fl_set_initial_dir(const gchar *dir);

/*
 * Sets the working dir. <dir> is in the filesystem encoding.
 */
void fl_set_working_dir(const gchar *dir);

/*
 * Sets the working dir. <dir> is in UTF-8 encoding.
 */
void fl_set_working_dir_utf8(const gchar *dir);

/*
 * Gets the working dir. Result is in the filesystem encoding.
 */
const gchar *fl_get_working_dir();

/*
 * Gets the working dir. Result is in UTF-8 encoding.
 */
const gchar *fl_get_working_dir_utf8();

/*
 * Refreshes the file list.
 * If <keep_pos> is TRUE, attempts to keep the scroll position.
 */
void fl_refresh(gboolean keep_pos);

/*
 * Advances the cursor in the file list treeview.
 */
void fl_select_next_file();

/*
 * Returns the number of selected files in the file list treeview.
 */
int fl_count_selected();

/*
 * Returns the the first selected file, or NULL if none is selected.
 * File name is in the filesystem encoding.
 */
const gchar *fl_get_selected_file();

/*
 * Returns the list of selected files.
 * File names are in the filesystem encoding.
 * The caller must free the result with g_elist_free()
 */
GEList *fl_get_selected_files();

/*
 * Returns the list of all files.
 * File names are in the filesystem encoding.
 * The caller must free the result with g_elist_free()
 */
GEList *fl_get_all_files();



#endif
