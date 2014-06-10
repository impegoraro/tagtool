#ifndef AUDIO_FILE_H
#define AUDIO_FILE_H

#include <stdio.h>


/*** private stuff ******************************************************/

/* file types */
#define AF_MPEG   0
#define AF_VORBIS 1

typedef void		(*af_delete_func)	 (void*);
typedef const gchar *	(*af_get_desc_func)	 (void*);
typedef const gchar *	(*af_get_info_func)	 (void*);
typedef gboolean	(*af_has_tag_func)	 (void*);
typedef void		(*af_create_tag_func)	 (void*);
typedef void		(*af_remove_tag_func)	 (void*);
typedef int		(*af_write_changes_func) (void*);
typedef int		(*af_set_field_func)	 (void*, int, const char*);
typedef int		(*af_get_field_func)	 (void*, int, const char**);
typedef void		(*af_dump_func)		 (void*);
typedef void		(*af_edit_load_func)	 (void*);
typedef void		(*af_edit_unload_func)	 ();

typedef struct { 
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

} audio_file;


/*** public stuff ******************************************************/

/* tag fields */
#define AF_TITLE	0
#define AF_ARTIST	1
#define AF_ALBUM	2
#define AF_YEAR		3
#define AF_GENRE	4
#define AF_COMMENT	5
#define AF_TRACK	6

/* return codes */
#define AF_OK		0
#define AF_ERR_FILE	1
#define AF_ERR_FORMAT	2
#define AF_ERR_READONLY	3
#define AF_ERR_NO_TAG	4
#define AF_ERR_NO_FIELD	5


/*
 * Creates an audio_file.
 *
 * <f>		Adress of the pointer that will hold the new audio_file
 * <filename>	Name of the file to open
 * <writable>	TRUE to open the file for writing, FALSE to open read-only.
 *
 * return	AF_OK on success
 *		AF_ERR_FORMAT if file is not a recognized audio format
 *		AF_ERR_FILE if there was a filesystem error (errno will be set)
 */
int audio_file_new(audio_file **f, const char *filename, gboolean editable);

/*
 * Frees an audio_file and associated data.
 */
void audio_file_delete(audio_file *f);

/*
 * Returns the file name. 
 * Return value points to an internal string and must not be modified.
 */
const gchar *audio_file_get_name(audio_file *f);

/*
 * Returns the file name extension of this file (incl. dot), e.g. ".mp3" 
 * Return value points to an internal string and must not be modified.
 */
const gchar *audio_file_get_extension(audio_file *f);

/*
 * Returns TRUE if the file was opened for editing, FALSE if it is read-only.
 */
gboolean audio_file_is_editable(audio_file *f);

/*
 * Gets a string description of the file type, e.g. "MPEG Version 1, Layer 3" 
 * 
 * return	String description. Points to an internal buffer valid
 * 		until the next call.
 */
const gchar *audio_file_get_desc(audio_file *f);

/* 
 * Gets a strings with a list of properties. The property names and values
 * alternate, each on its own line, e.g. "NAME1\nVALUE1\nNAME2\nNAME2"
 *
 * return	String. Points to an internal buffer valid until the 
 *		next call.
 */
const gchar *audio_file_get_info(audio_file *f);

/*
 * Returns TRUE if file has a tag.
 */
gboolean audio_file_has_tag(audio_file *f);

/*
 * Returns TRUE if tag has changes that need to be saved.
 */
gboolean audio_file_has_changes(audio_file *f);

/*
 * Creates the tag if it did not already exist. Nothing is written to file
 * until audio_file_write_changes() is called.
 */
void audio_file_create_tag(audio_file *f);

/*
 * Removes the tag. Nothing is written to file until audio_file_write_changes() 
 * is called.
 */
void audio_file_remove_tag(audio_file *f);

/*
 * Writes tag changes to file.
 *
 * return	AF_OK on success
 *		AF_ERR_READONLY if file was opened for reading
 *		AF_ERR_FILE if there was a filesystem error (errno will be set)
 */
int audio_file_write_changes(audio_file *f);

/* 
 * Sets the value of a tag field. Nothing is written to file until
 * audio_file_write_changes() is called.
 *
 * <field>	Tag field to set
 * <value>	New value (will be copied)
 *
 * return	AF_OK on success
 *		AF_ERR_NO_TAG if file has no tag
 *		AF_ERR_NO_FIELD if <field> is invalid
 */
int audio_file_set_field(audio_file *f, int field, const char *value);

/* 
 * Gets the value of a tag field. The pointer stored in <value> will point
 * to internal data and must not be modified.
 *
 * <field>	Tag field to get
 * <value>	Adress of a pointer that will hold the adress of the data.
 *		Valid only until the field is modified or audio_file_get_field()
 *		is called again.
 *
 * return	AF_OK on success
 *		AF_ERR_NO_TAG if file has no tag
 *		AF_ERR_NO_FIELD if <field> is invalid
 */
int audio_file_get_field(audio_file *f, int field, const char **value);

/* 
 * Dumps all available information on an audio_file to stdout
 */
void audio_file_dump(audio_file *f);

/*
 * Loads a file to be edited in the "Edit Tag" tab.
 */
void audio_file_edit_load(audio_file *f);

/*
 * Unloads the file being edited in the "Edit Tag" tab.
 */
void audio_file_edit_unload(audio_file *f);


#endif
