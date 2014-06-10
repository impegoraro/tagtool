#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#include "vcedit.h"
#include "str_util.h"
#include "vorbis_file.h"


/*** private functions ******************************************************/

/* insert a hash table entry into a vorbis_comment struct */
static void insert_comment(gpointer key, gpointer val, gpointer data)
{
	vorbis_comment_add_tag((vorbis_comment *)data, key, val);
}

/* print a hash table entry */
static void print_comment(gpointer key, gpointer val, gpointer data)
{
	printf("%-12s \"%s\"\n", (char *)key, (char *)val);
}

/* free a hash table entry */
static gboolean remove_comment(gpointer key, gpointer val, gpointer data)
{
	free(key);
	free(val);
	return TRUE;
}


/* inserts a new comment into the hash table, or replaces an existing comment */
static void table_insert_or_replace(GHashTable *table, const char *key, const char *val)
{
	void *old_key, *old_val;
	gboolean found;

	found = g_hash_table_lookup_extended(table, key, &old_key, &old_val);

	if (strcmp(val, "") != 0) {
		g_hash_table_insert(table, found ? old_key : strdup(key), strdup(val));

		/* Word of warning: contrary to what the glib documentation says,
		   only the old value must be freed, *not* the key. */
		if (found) free(old_val);

	} else if (found) {
		g_hash_table_remove(table, old_key);
		free(old_key);
		free(old_val);
	}
}

/* inserts a new comment into the hash table, or appends to an existing comment */
static void table_insert_or_append(GHashTable *table, const char *key, const char *val)
{
	void *old_key, *old_val;
	char *new_val;
	gboolean found;

	found = g_hash_table_lookup_extended(table, key, &old_key, &old_val);

	if (found) {
		new_val = g_strjoin(", ", old_val, val, NULL);
		g_hash_table_insert(table, old_key, new_val);
		free(old_val);
	} else {
		g_hash_table_insert(table, strdup(key), strdup(val));
	}
}


static void format_time(double rsec, int *min, int *sec, int *msec)
{
	double isec;

	*msec = modf(rsec, &isec) * 1000;
	*sec = (int)isec % 60;
	*min = (int)isec / 60;
}


static void vorbis_comment_to_table(vorbis_comment *vc, GHashTable *table)
{
	char *key, *val, *p;
	size_t count;
	int i;

	for (i = 0; i < vc->comments; i++) {
		p = strchr(vc->user_comments[i], '=');
		if (!p) {
			g_warning("invalid vorbis comment: %s\n", vc->user_comments[i]);
			continue;
		}

		count = p - vc->user_comments[i] + 1;
		key = malloc(count);
		str_safe_strncpy(key, vc->user_comments[i], count);
		str_ascii_tolower(key);

		val = strdup(p+1);

		table_insert_or_append(table, key, val);
		free(key);
		free(val);
	}
}


static char *get_field_name(int field_id)
{
	switch (field_id) {
		case AF_TITLE:
			return "title";
		case AF_ARTIST:
			return "artist";
		case AF_ALBUM:
			return "album";
		case AF_YEAR:
			return "date";
		case AF_GENRE:
			return "genre";
		case AF_COMMENT:
			/* XXX
			   "comment" is not part of the standard field names, but it's 
			   what Winamp uses.  XMMS is horribly broken uses "" (empty 
			   name).  Using "description" might be better */
			return "comment";
		case AF_TRACK:
			return "tracknumber";
		default:
			return NULL;
	}
}


/*** public functions *******************************************************/

/* 
 * Vorbis implementation of the standard audio_file functions
 */

int vorbis_file_new(vorbis_file **f, const char *filename, gboolean editable)
{
	vorbis_file newfile;
	int res;

	*f = NULL;

	newfile.file = fopen(filename, "r");
	if (!newfile.file)
		return AF_ERR_FILE;

	res = ov_open(newfile.file, &newfile.ov, NULL, 0);
	if (res < 0) {
		fclose(newfile.file);
		return AF_ERR_FORMAT;
	}

	newfile.comments = g_hash_table_new(g_str_hash, g_str_equal);
	vorbis_comment_to_table(ov_comment(&newfile.ov, -1), newfile.comments);


	newfile.type = AF_VORBIS;
	newfile.name = strdup(filename);
	/* XXX - editable flag is meaningless, write will fail if dir is read-only */
	newfile.editable = editable;
	newfile.changed = FALSE;

	newfile.delete = (af_delete_func) vorbis_file_delete;
	newfile.get_desc = (af_get_desc_func) vorbis_file_get_desc;
	newfile.get_info = (af_get_info_func) vorbis_file_get_info;
	newfile.has_tag = (af_has_tag_func) vorbis_file_has_tag;
	newfile.create_tag = (af_create_tag_func) vorbis_file_create_tag;
	newfile.remove_tag = (af_remove_tag_func) vorbis_file_remove_tag;
	newfile.write_changes = (af_write_changes_func) vorbis_file_write_changes;
	newfile.set_field = (af_set_field_func) vorbis_file_set_field;
	newfile.get_field = (af_get_field_func) vorbis_file_get_field;
	newfile.dump = (af_dump_func) vorbis_file_dump;
	newfile.edit_load = (af_edit_load_func) vorbis_file_edit_load;
	newfile.edit_unload = (af_edit_unload_func) vorbis_file_edit_unload;

	*f = malloc(sizeof(vorbis_file));
	memcpy(*f, &newfile, sizeof(vorbis_file));

	return AF_OK;
}


void vorbis_file_delete(vorbis_file *f)
{
	if (f) {
		ov_clear(&f->ov);	/* closes the file */

		g_hash_table_foreach_remove(f->comments, remove_comment, NULL);
		g_hash_table_destroy(f->comments);

		free(f->name);
		
		free(f);
	}
}


const gchar *vorbis_file_get_desc(vorbis_file *f)
{
	return "Ogg Vorbis";
}

const gchar *vorbis_file_get_info(vorbis_file *f)
{
	static char buf[256];
	int min, sec, msec;
	OggVorbis_File *ov = &f->ov;
	vorbis_info *vi = ov_info(ov, -1);

	format_time(ov_time_total(ov, -1), &min, &sec, &msec);

	snprintf(buf, sizeof(buf), 
		 _("Average Bit Rate\n%ld kbps\n"
		   "Nominal Bit Rate\n%ld kbps\n"
		   "Sample Rate\n%ld Hz\n"
		   "Channels\n%d\n"
		   "Playing Time\n%dm %ds %dms\n"),
		 ov_bitrate(ov, -1)/1000, vi->bitrate_nominal/1000, 
		 vi->rate, vi->channels, min, sec, msec);

	return buf;
}


gboolean vorbis_file_has_tag(vorbis_file *f)
{
	/* vorbis files always have a comment header, even if it's empty */
	return TRUE;
}


void vorbis_file_create_tag(vorbis_file *f)
{
	/* nothing to do (see above) */
}


void vorbis_file_remove_tag(vorbis_file *f)
{
	/* erase all comments */
	g_hash_table_foreach_remove(f->comments, remove_comment, NULL);
	f->changed = TRUE;
}


int vorbis_file_write_changes(vorbis_file *f)
{
	/* 
	 * Currently using vcedit.c from the vorbistools distribution, because
	 * libvorbisfile does not provide writing functionality.
	 */

	char *temp_name;
	FILE *new_file;
	int save_errno;
	vcedit_state *state;
	vorbis_comment *vc;
	int res;

	/* move the original file out of the way */
	temp_name = malloc(6 + strlen(f->name));
	strcpy(temp_name, f->name);
	strcat(temp_name, ".orig");
	res = rename(f->name, temp_name);
	if (res != 0)
		return AF_ERR_FILE;

	/* create the new file */
	new_file = fopen(f->name, "w+");
	if (!new_file) {
		save_errno = errno;
		rename(temp_name, f->name);
		errno = save_errno;
		return AF_ERR_FILE;
	}

	/* init vcedit */
	fseek(f->file, 0, SEEK_SET);

	state = vcedit_new_state();
	res = vcedit_open(state, f->file);
	if (res < 0) {
		/* this should never happen, file was validated when first opened */
		save_errno = errno;
		g_warning("vcedit_open failed: %s\n", vcedit_error(state));
		vcedit_clear(state);

		rename(temp_name, f->name);
		errno = save_errno;
		return AF_ERR_FILE;
	}

	/* fill in the vorbis_comments structure */
	vc = vcedit_comments(state);
	vorbis_comment_clear(vc);
	vorbis_comment_init(vc);
	g_hash_table_foreach(f->comments, insert_comment, vc);

	/* write out the new file */
	res = vcedit_write(state, new_file);
	if (res < 0) {
		save_errno = errno;
		g_warning("vcedit_write failed: %s\n", vcedit_error(state));
		vcedit_clear(state);

		rename(temp_name, f->name);
		errno = save_errno;
		return AF_ERR_FILE;
	}

	vcedit_clear(state);
	fclose(new_file);

	/* remove the original file. */
	remove(temp_name);

	/* 
	 * Note that we're still holding the original file open. There is no 
	 * point in switching to the new one because only the comments header 
	 * changed, and we won't be needing it again (the hash table has the
	 * up-to-date values)
	 */

	f->changed = FALSE;
	return AF_OK;
}


int vorbis_file_set_field(vorbis_file *f, int field, const char *value)
{
	char *name;

	name = get_field_name(field);
	if (!name)
		return AF_ERR_NO_FIELD;
	else
		return vorbis_file_set_field_by_name(f, name, value);
}


int vorbis_file_get_field(vorbis_file *f, int field, const char **value)
{
	char *name;

	name = get_field_name(field);
	if (!name)
		return AF_ERR_NO_FIELD;
	else
		return vorbis_file_get_field_by_name(f, name, value);
}


void vorbis_file_dump(vorbis_file *f)
{
	OggVorbis_File *ov = &f->ov;
	vorbis_info *vi;
	vorbis_comment *vc;
	int nstreams, i, j;

	printf("\nFile type: Ogg Vorbis\n\n");

	nstreams = ov_streams(ov);
	printf("Physical stream contains %d logical stream(s)\n", nstreams);

	for (i = 0; i < nstreams; i++) { 
		vi = ov_info(ov, i);
		vc = ov_comment(ov, i);

		printf("\n---- Logical Stream #%d ----\n\n"
		       "Serial number   : 0x%08lX\n"
		       "Encoder version : %d\n"
		       "Encoder vendor  : %s\n",
		       i+1, ov_serialnumber(ov,i), vi->version, vc->vendor);
		
		printf("Channels        : %d\n"
		       "Sample rate     : %ld Hz\n",
		       vi->channels, vi->rate);
		
		if (vi->bitrate_nominal > 0)
			printf("Nominal bit rate: %ld bps\n", vi->bitrate_nominal);
		if (vi->bitrate_upper > 0)
			printf("Upper bit rate  : %ld bps\n", vi->bitrate_upper);
		if (vi->bitrate_lower > 0)
			printf("Lower bit rate  : %ld bps\n", vi->bitrate_lower);

		printf("Average bit rate: %ld bps\n"
		       "Playing time    : %g s\n",
		       ov_bitrate(ov, i), ov_time_total(ov, i));

		if (vc->comments == 0)
			printf("\nNo user comments\n");
		else {
			printf("\nUser comments (raw):\n");
			for (j = 0; j < vc->comments; j++)
				printf("%s\n", vc->user_comments[j]);
		}
	}

	printf("\n---- Processed Comments ----\n\n");

	if (g_hash_table_size(f->comments) == 0)
		printf("No user comments\n");
	else
		g_hash_table_foreach(f->comments, print_comment, NULL);

	printf("\n");
}


extern void vorbis_edit_load(vorbis_file *f);

void vorbis_file_edit_load(vorbis_file *f)
{
	vorbis_edit_load(f);
}


extern void vorbis_edit_unload();

void vorbis_file_edit_unload(vorbis_file *f)
{
	vorbis_edit_unload();
}


/*
 * Vorbis specific functions
 */

int vorbis_file_get_field_by_name(vorbis_file *f, const char *name, const char **value)
{
	char *table_value;

	table_value = g_hash_table_lookup(f->comments, name);
	if (table_value)
		*value = table_value;
	else
		*value = "";

	return AF_OK;
}


int vorbis_file_set_field_by_name(vorbis_file *f, const char *name, const char *value)
{
	table_insert_or_replace(f->comments, name, value);
	f->changed = TRUE;
	return AF_OK;
}


int vorbis_file_append_field_by_name(vorbis_file *f, const char *name, const char *value)
{
	table_insert_or_append(f->comments, name, value);
	f->changed = TRUE;
	return AF_OK;
}


int vorbis_file_get_std_fields(const char ***names)
{
	static const char *std_comments[] =
	{
		"album",
		"artist",
		"contact",
		"copyright",
		"date",
		"description",
		"genre",
		"isrc",
		"license",
		"location",
		"organization",
		"performer",
		"title",
		"tracknumber",
		"version"
	};

	*names = std_comments;
	return sizeof(std_comments) / sizeof(char*);
}
