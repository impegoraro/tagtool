#ifndef FILE_UTIL_H
#define FILE_UTIL_H

/****************************************************************************
 *	Utility functions dealing with files and directories
 ****************************************************************************/


#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "elist.h"


/*
 * Callback function type for fu_get_file_list()
 *
 * <dirs>	Number of directories visited so far.
 * <files>	Number of matching files found so far.
 *
 * return	TRUE to stop the scan.
 */
typedef gboolean fu_progress_callback(int dirs, int files);


/* 
 * Scans a directory tree for files matching a pattern and returns them in an 
 * GEList. The current directory is changed to 'dir_path' as side-effect.
 *
 * <dir_path>	Directory to scan
 * <patterns>	NULL terminated array of glob pattern strings
 * <callback>	If not NULL, this callback will be called after visiting each 
 *		directory.  If the callback returns TRUE the scan will be
 *		aborted and the (incomplete) result returned.
 * <recurse>	If TRUE subdirectories will be included in the scan
 * <sort>	If TRUE the result will be sorted
 *
 * return	GEList with matching files. File names will include the path
 *		relative to <dir_path>.
 *		This list should be freed with g_elist_free_data().
 */
GEList *fu_get_file_list( const gchar *dir_path, 
			  const gchar **patterns, 
			  fu_progress_callback callback, 
			  gboolean recurse, 
			  gboolean sort );

/*
 * Checks if the current process has the requested permissions on a filesystem 
 * object (represented by it's 'stat' data). The real (as opposed to effective) 
 * uid and gid of the process are used.
 *
 * <stat_data>	Structure obtained from stat(2)
 * <perm>	String with one or more desired permissions (r, w, x)
 *
 * return	TRUE if the process has all the requested permissions
 */
gboolean fu_check_permission( struct stat* stat_data,
			      const gchar *perm );

/*
 * Compares two file names and returns TRUE if they have the same path.
 */
gboolean fu_compare_file_paths( const gchar *f1, 
				const gchar *f2 );

/*
 * Returns the number of path components
 * Examples:
 *	"foo"		-> 1
 *	"/foo"		-> 1
 *	"/foo/bar"	-> 2
 *	"/foo/bar/"	-> 2
 *	"foo/bar/baz"	-> 3
 *	"/"		-> 1
 *	""		-> 0
 */
gint fu_count_path_components( const gchar *path );

/*
 * Returns the last <n> components of <path>. If the path has less components 
 * all of it is returned.
 * The return value is a pointer into <path>.
 * Examples:
 *	"foo", 1		-> "foo"
 *	"foo/", 1		-> "foo/"
 *	"foo/", 2		-> "foo/"
 *	"/foo/", 1		-> "foo/"
 *	"/foo/", 2		-> "/foo/"
 *	"/foo/bar", 2		-> "foo/bar"
 *	"/foo/bar/baz", 2	-> "bar/baz/"
 *	"/", 1			-> "/"
 *	"", 1			-> ""
 */
gchar *fu_last_n_path_components( const gchar *path, 
				  gint n );

/*
 * Returns the last component of the given path, without leading or trailing 
 * path separator characters.
 * The returned string must be freed by the caller.
 * Examples:
 *	"foo"		-> "foo"
 *	"/foo"		-> "foo"
 *	"/foo/bar"	-> "bar"
 *	"/foo/bar/"	-> "bar"
 *	"/"		-> ""
 *	""		-> ""
 */
gchar *fu_last_path_component( const gchar *path );

/*
 * Joins two paths. <path2> should be a relative path.
 * The returned string must be freed by the caller.
 */
gchar *fu_join_path( const gchar *path1,
		     const gchar *path2 );

/*
 * Returns TRUE if the given path exists.
 */
gboolean fu_exists( const gchar *path );

/*
 * Returns TRUE if the given path exists and is a regular file.
 */
gboolean fu_file_exists( const gchar *path );

/*
 * Returns TRUE if the given path exists and is a directory.
 */
gboolean fu_dir_exists( const gchar *path );

/*
 * Create the given directory tree if it does not already exist.
 * In case of failure errno will have the value set by mkdir(2).
 *
 * <path>	Directory tree
 * <new>	If not NULL, the number of directories actually created will 
 *		be stored here
 *
 * return	TRUE if the operation was successfull (i.e., <path> is now 
 *		guarenteed to exist)
 */
gboolean fu_make_dir_tree( const gchar *path, 
			   gint *new );


#endif
