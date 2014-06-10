#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <fnmatch.h>
#include <string.h>
#include <glib.h>

#include "str_util.h"
#include "glib_util.h"
#include "elist.h"

#include "file_util.h"


/*** private functions ******************************************************/

/*
 * Unique file identifier (device # + inode #)
 */
typedef struct file_uid_tag {
	dev_t device;
	ino_t inode;
} file_uid;

static gint file_uid_equal(gconstpointer v1, gconstpointer v2)
{
	return ((file_uid*)v1)->device == ((file_uid*)v2)->device &&
		((file_uid*)v1)->inode == ((file_uid*)v2)->inode;
}

static guint file_uid_hash(gconstpointer v)
{
	return (guint)(((file_uid*)v)->inode);
}


/* 
 * Reverse string compare
 */
gint rev_strcoll(gconstpointer s1, gconstpointer s2)
{
	return -strcoll(s1, s2);
}


/*
 * Remembers a directory by storing its inode number. Once a directory is 
 * "remembered" subsequent calls to remember_dir with that directory will 
 * return TRUE.
 *
 * <dir_table>	Table to store visited directories
 * <dir_stat>	Structure obtained from stat(2)
 *
 * return	FALSE if the directory was not encountered before, TRUE
 *		otherwise.
 */
static gboolean remember_dir( GHashTable *dir_table,
			      struct stat *dir_stat )
{
	file_uid *f = malloc(sizeof(file_uid));

	f->device = dir_stat->st_dev;
	f->inode = dir_stat->st_ino;

	if (g_hash_table_lookup(dir_table, f) != NULL) {
		free(f);
		return TRUE;
	} else {
		/* Q: Why not just store NULL as the value?
		 * A: Because g_hash_table_lookup() returns NULL to indicate
		 *    the key was not found. Stupid, eh?
		 */
		g_hash_table_insert(dir_table, f, (gpointer)1);
		return FALSE;
	}
}


/* 
 * Scans a directory for files matching a pattern and returns them in an GEList
 *
 * <dir_stack>	If not NULL, will be used to store the names of subdirectories 
 *		found (not subject to matching)
 * <dir_memory>	If not NULL, will be used to remember visited dirs. This 
 *		guarantees that each dir is only visited once. 
 *		ALWAYS USE THIS PARAMETER WHEN DOING RECURSIVE SCANS!
 * <base_dir>	Directory to scan
 * <patterns>	NULL terminated array of glob pattern strings
 * <sort>	If TRUE the result will be sorted
 *
 * return	GEList with matching files. File names will be prefixed with
 *		the <base_dir>, unless it is "."
 *		This list should be freed with g_elist_free_data().
 */
static GEList *aux_file_list( GEList *dir_stack,
			      GHashTable *dir_memory,
			      const gchar *base_dir,
			      const gchar **patterns, 
			      gboolean sort )
{
	GEList *result;
	GEList *aux_stack = NULL;
	DIR* dir;
	int dir_fd;
	struct stat dir_stat;
	struct dirent *dir_entry;
	struct stat stat_data;
	gint res;
	gchar *buffer;
	gchar *buffer_fname;
	gint buffer_size;
	gboolean matched;
	gint i;

	result = g_elist_new();
	if (dir_stack != NULL)
		aux_stack = g_elist_new();

	dir = opendir(base_dir);
	if (dir == NULL) {
		g_warning("couldn't open dir: %s", base_dir);
		return result;
	}

	/* guard against loops in the directory structure */
	if (dir_memory != NULL) {
		dir_fd = dirfd(dir);
		if (dir_fd >= 0)
			fstat(dir_fd, &dir_stat);
		else 
			stat(base_dir, &dir_stat);
		if (remember_dir(dir_memory, &dir_stat)) {
			//printf("skipping directory, already been here: %s", base_dir);
			closedir(dir);
			return result;
		}
	}

	// XXX - fixed size buffer
	buffer = g_malloc(buffer_size = 1024);
	if (strcmp(base_dir, ".") == 0) {
		buffer_fname = buffer;
	} else {
		buffer_fname = memccpy(buffer, base_dir, 0, buffer_size);
		if (buffer_fname == NULL) {
			g_warning("FIXME - buffer too small for file name!!!");
			closedir(dir);
			g_free(buffer);
			return result;
		} else {
			*(buffer_fname-1) = '/';
			buffer_size -= (gint)(buffer_fname - buffer);
		}
	}

	while ( (dir_entry = readdir(dir)) ) {
		if (memccpy(buffer_fname, dir_entry->d_name, 0, buffer_size) == NULL) {
			g_warning("FIXME - buffer too small for file name!!!");
			continue;
		}

		res = stat(buffer, &stat_data);
		if (res < 0) {
			//printf("couldn't stat file: %s", buffer);
			continue;
		}
		if (S_ISDIR(stat_data.st_mode)) {
			if ((aux_stack != NULL) &&
			    (strcmp(dir_entry->d_name, ".") != 0) &&
			    (strcmp(dir_entry->d_name, "..") != 0)) {
				g_elist_push(aux_stack, g_strdup(buffer));
			}
			continue;
		}
		if (patterns == NULL) {
			matched = TRUE;
		} else {
			matched = FALSE;
			for (i = 0; patterns[i]; i++) {
				if (fnmatch(patterns[i], dir_entry->d_name, FNM_NOESCAPE) == 0) {
					matched = TRUE;
					break;
				}
			}
		}	
		if (matched)
			g_elist_append(result, g_strdup(buffer));
	}

	if (sort) {
		g_elist_sort(result, (GCompareFunc)strcoll);
		if (aux_stack != NULL)
			g_elist_sort(aux_stack, rev_strcoll);
	}
	
	if (aux_stack != NULL)
		g_elist_concat(dir_stack, aux_stack);

	closedir(dir);
	g_free(buffer);
	return result;
}


/*** public functions *******************************************************/

GEList *fu_get_file_list( const gchar *dir_path, 
			  const gchar **patterns, 
			  fu_progress_callback callback, 
			  gboolean recurse,
			  gboolean sort )
{
	GEList *result;

	chdir(dir_path);

	if (!recurse) {
		result = aux_file_list(NULL, NULL, ".", patterns, sort);

		if (callback != NULL)
			callback(1, (int)result->length);
	}
	else {
		GEList *dir_stack;
		GHashTable *dir_memory;
		gchar *cur_dir;
		int dircount = 0;

		result = g_elist_new();
		dir_stack = g_elist_new();
		dir_memory = g_hash_table_new(file_uid_hash, file_uid_equal);

		g_elist_push(dir_stack, g_strdup("."));
		while (g_elist_length(dir_stack) > 0) {
			cur_dir = g_elist_pop(dir_stack);
			g_elist_concat(result, 
				       aux_file_list(dir_stack, dir_memory, 
						     cur_dir, patterns, sort));
			g_free(cur_dir);
			dircount++;

			if (callback != NULL)
				if (callback(dircount, (int)result->length))
					break;
		}

		g_elist_free_data(dir_stack);
		g_hash_table_free(dir_memory, TRUE, FALSE);
	}

	return result;
}


gboolean fu_check_permission( struct stat* stat_data,
			      const gchar *perm )
{
	mode_t rmask, wmask, xmask;

	// XXX - should use getgroups instead of getgid

	if (stat_data->st_uid == getuid()) {
		rmask = S_IRUSR;
		wmask = S_IWUSR;
		xmask = S_IXUSR;
	}
	else if (stat_data->st_gid == getgid()) {
		rmask = S_IRGRP;
		wmask = S_IWGRP;
		xmask = S_IXGRP;
	}
	else {
		rmask = S_IROTH;
		wmask = S_IWOTH;
		xmask = S_IXOTH;
	}
	
	if (strchr(perm, 'r') && !(stat_data->st_mode & rmask)) 
		return FALSE;
	if (strchr(perm, 'w') && !(stat_data->st_mode & wmask)) 
		return FALSE;
	if (strchr(perm, 'x') && !(stat_data->st_mode & xmask)) 
		return FALSE;

	return TRUE;
}


gboolean fu_compare_file_paths( const gchar *f1, 
				const gchar *f2 )
{
	char *aux;
	gint i1, i2;

	aux = strrchr(f1, '/');
	i1 = (aux == NULL ? -1 : (gint)(aux - f1));
	aux = strrchr(f2, '/');
	i2 = (aux == NULL ? -1 : (gint)(aux - f2));

	if (i1 != i2)
		return FALSE;
	else if (i1 == -1)
		return TRUE;

	if (strncmp(f1, f2, i1) != 0)
		return FALSE;
	else
		return TRUE;
}


gint fu_count_path_components( const gchar *path )
{
	/*
	 * count = (number of path separators) + 1, with two exceptions:
	 * - path separators at the beggining and end don't count
	 * - if path is empty count is 0
	 */
	
	gint count = 0;
	gint len = strlen(path);
	gint pos;

	for (pos = 0; pos < len; pos++) {
		if (path[pos] == '/' && pos != 0 && pos != len-1)
			count++;
	}

	if (len > 0)
		count++;

	return count;
}


gchar *fu_last_n_path_components( const gchar *path, 
				  gint n )
{
	gint count;
	gint len;
	gchar *start, *end, *p;

	len = strlen(path);
	if (len == 0)
		return (gchar*)path;

	start = (gchar*)path;
	end = (gchar*)path + len - 1;
	count = 0;
	for (p = end; p >= start && count < n; p--) {
		if (*p == '/' && p != end)
			count++;
	}

	p++;
	if (*p == '/' && count == n)
		return p+1;
	else
		return p;
}


gchar *fu_last_path_component( const gchar *path )
{
	gchar *result;
	gchar *start, *end;

	end = (gchar*)path + strlen(path);

	if (*(end-1) == '/') {
		end--;
		start = str_strnrchr(path, '/', 2);
		start = (start == NULL ? (gchar*)path : start+1);
	} else {
		start = str_strnrchr(path, '/', 1);
		start = (start == NULL ? (gchar*)path : start+1);
	}

	result = calloc(end-start+1, 1);
	memcpy(result, start, end-start);
	return result;
}


gchar *fu_join_path( const gchar *path1,
		     const gchar *path2 )
{
	gchar *ret;
	int len1, len2;

	len1 = strlen(path1);
	len2 = strlen(path2);
	ret = malloc(len1 + len2 + 2);

	if (path1[len1-1] == '/')
		sprintf(ret, "%s%s", path1, path2);
	else
		sprintf(ret, "%s/%s", path1, path2);

	return ret;
}


gboolean fu_exists( const gchar *path )
{
	struct stat stat_data;
	int res;

	res = stat(path, &stat_data);
	if (res == 0)
		return TRUE;
	else
		return FALSE;
}


gboolean fu_file_exists( const gchar *path )
{
	struct stat stat_data;
	int res;

	res = stat(path, &stat_data);
	if (res == 0)
		return S_ISREG(stat_data.st_mode);
	else
		return FALSE;
}


gboolean fu_dir_exists( const gchar *path )
{
	struct stat stat_data;
	int res;

	res = stat(path, &stat_data);
	if (res == 0)
		return S_ISDIR(stat_data.st_mode);
	else
		return FALSE;
}


gboolean fu_make_dir_tree( const gchar *path, 
			   gint *new )
{
	gchar *partial_path, *p;
	gint i, res;
	gboolean ret = TRUE;

	if (new) *new = 0;

	if (fu_dir_exists(path))
		return ret;

	partial_path = calloc(strlen(path)+1, 1);
	i = 1;
	do {
		p = str_strnchr(path, '/', i++);
		if (p == path)
			continue;

		strncpy(partial_path, path, (p ? p-path : strlen(path)));

		if (!fu_dir_exists(partial_path)) {
			res = mkdir(partial_path, 0777);
			if (res < 0) {
				ret = FALSE;
				break;
			}
			if (new) (*new)++;
		}
	} while (p);

	free(partial_path);
	return ret;
}

