#ifndef MPEG_FILE_H
#define MPEG_FILE_H

#include <id3.h>

#include "audio_file.h"


typedef struct {
	size_t offset;		// file offset of this frame
	guint8 raw[4];		// raw header (4 bytes)
	guint8 major_version;
	guint8 minor_version;
	guint8 layer;
	gint bit_rate;
	gint sample_rate;
	gchar *channel_mode;
	gboolean copyright;
	gboolean original;
} frame_header;


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

	/* mpeg specific part */
	frame_header header;	/* data taken from the first frame's header */
	ID3Tag* v1_tag;		/* ID3 tag, v1 */
	ID3Tag* v2_tag;		/* ID3 tag, v2 */
	gboolean has_v1_tag;	/* TRUE if file currently has a v1 tag */
	gboolean has_v2_tag;	/* TRUE if file currently has a v2 tag */
	gboolean orig_v1_tag;	/* TRUE if file originally had a v1 tag */
	gboolean orig_v2_tag;	/* TRUE if file originally had a v2 tag */

} mpeg_file;


/* 
 * MPEG implementation of the standard audio_file functions
 */

int mpeg_file_new(mpeg_file **f, const char *filename, gboolean editable);
void mpeg_file_delete(mpeg_file *f);
const gchar *mpeg_file_get_desc(mpeg_file *f);
const gchar *mpeg_file_get_info(mpeg_file *f);
gboolean mpeg_file_has_tag(mpeg_file *f);
void mpeg_file_create_tag(mpeg_file *f);
void mpeg_file_remove_tag(mpeg_file *f);
int mpeg_file_write_changes(mpeg_file *f);
int mpeg_file_set_field(mpeg_file *f, int field, const char *value);
int mpeg_file_get_field(mpeg_file *f, int field, const char **value);
void mpeg_file_dump(mpeg_file *f);
void mpeg_file_edit_load(mpeg_file *f);
void mpeg_file_edit_unload(mpeg_file *f);


/*
 * MPEG specific functions
 */

/* These are the same as the standard functions, but allow specifying which 
 * tag version to work on.  Version is either ID3TT_ID3V1 or ID3TT_ID3V2. */
gboolean mpeg_file_has_tag_v(mpeg_file *f, int version);
void mpeg_file_create_tag_v(mpeg_file *f, int version);
void mpeg_file_remove_tag_v(mpeg_file *f, int version);
int mpeg_file_set_field_v(mpeg_file *f, int version, int field, const char *value);
int mpeg_file_get_field_v(mpeg_file *f, int version, int field, const char **value);

/* Get/set a frame's text.  For non-text frames, set has no effect and 
 * get returns a NULL value */
int mpeg_file_set_frame_text(mpeg_file *f, int version, ID3Frame *frame, const char *value);
int mpeg_file_get_frame_text(mpeg_file *f, int version, ID3Frame *frame, const char **value);

/* Remove a frame from a tag */
int mpeg_file_remove_frame(mpeg_file *f, int version, ID3Frame *frame);

/* Gets an array with the frames present in the tag. The return value is 
 * the number of frames returned.
 * Caller is responsible for freeing the array */
int mpeg_file_get_frames(mpeg_file *f, int version, ID3Frame ***frames);

/* Gets information about a frame type.  Unwanted output parameters can be NULL. */
void mpeg_file_get_frame_info(int frame_id, const char **name, const char** desc, gboolean *editable, gboolean *simple);

/* Gets the list of frame IDs that are editable.  The array must not be altered 
 * or freed.  The return value is the number of elements in the array. */
int mpeg_file_get_editable_frame_ids(int **ids);


#endif
