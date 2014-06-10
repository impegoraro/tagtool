#ifndef VORBIS_FILE_H
#define VORBIS_FILE_H

#include <vorbis/vorbisfile.h>
#include "audio_file.h"


typedef struct { 
	/* common part -- must match struct audio_file defined in audio_file.h */
	guint8 type;
	FILE *file;
	char *name;
	gboolean editable;
	gboolean changed;

	af_delete_func		delete;
	af_get_desc_func	get_desc;
	af_get_info_func	get_info;
	af_has_tag_func		has_tag;
	af_create_tag_func	create_tag;
	af_remove_tag_func	remove_tag;
	af_write_changes_func	write_changes;
	af_set_field_func	set_field;
	af_get_field_func	get_field;
	af_dump_func		dump;
	af_edit_load_func	edit_load;
	af_edit_unload_func	edit_unload;

	/* ogg vorbis specific part */
	OggVorbis_File ov;
	GHashTable *comments;

} vorbis_file;


/* 
 * Vorbis implementation of the standard audio_file functions
 */

int vorbis_file_new(vorbis_file **f, const char *filename, gboolean editable);
void vorbis_file_delete(vorbis_file *f);
const gchar *vorbis_file_get_desc(vorbis_file *f);
const gchar *vorbis_file_get_info(vorbis_file *f);
gboolean vorbis_file_has_tag(vorbis_file *f);
void vorbis_file_create_tag(vorbis_file *f);
void vorbis_file_remove_tag(vorbis_file *f);
int vorbis_file_write_changes(vorbis_file *f);
int vorbis_file_set_field(vorbis_file *f, int field, const char *value);
int vorbis_file_get_field(vorbis_file *f, int field, const char **value);
void vorbis_file_dump(vorbis_file *f);
void vorbis_file_edit_load(vorbis_file *f);
void vorbis_file_edit_unload(vorbis_file *f);


/*
 * Vorbis specific functions
 */

int vorbis_file_get_field_by_name(vorbis_file *f, const char *name, const char **value);
int vorbis_file_set_field_by_name(vorbis_file *f, const char *name, const char *value);
int vorbis_file_append_field_by_name(vorbis_file *f, const char *name, const char *value);

int vorbis_file_get_std_fields(const char ***names);


#endif

