#include <string.h>
#include <fnmatch.h>
#include <glib.h>

#include "audio_file.h"


#ifdef ENABLE_MP3
extern int mpeg_file_new(audio_file **, const char *, gboolean);
#endif

#ifdef ENABLE_VORBIS
extern int vorbis_file_new(audio_file **, const char *, gboolean);
#endif


int audio_file_new(audio_file **f, const char *filename, gboolean editable)
{
	
#ifdef ENABLE_MP3
	if (fnmatch("*.[mM][pP][aA23]", filename, FNM_NOESCAPE) == 0)
		return mpeg_file_new(f, filename, editable);
#endif

#ifdef ENABLE_VORBIS
	if (fnmatch("*.[oO][gG][gG]", filename, FNM_NOESCAPE) == 0)
		return vorbis_file_new(f, filename, editable);
#endif

	return AF_ERR_FORMAT;
}


const gchar *audio_file_get_name(audio_file *f)
{
	return f->name;
}

const gchar *audio_file_get_extension(audio_file *f)
{
	char *p;

	p = strrchr(f->name, '.');
	if (p == NULL || strlen(p) > 5)
		return "";
	else
		return p;
}

gboolean audio_file_is_editable(audio_file *f)
{
	return f->editable;
}

gboolean audio_file_has_changes(audio_file *f)
{
	return f->changed;
}


/*
 * The remaining functions just delegate to the implementation functions
 * registered in the audio_file struct.
 */

void audio_file_delete(audio_file *f)
{
	f->delete(f);
}

const gchar *audio_file_get_desc(audio_file *f)
{
	return f->get_desc(f);
}

const gchar *audio_file_get_info(audio_file *f)
{
	return f->get_info(f);
}

gboolean audio_file_has_tag(audio_file *f)
{
	return f->has_tag(f);
}

void audio_file_create_tag(audio_file *f)
{
	f->create_tag(f);
}

void audio_file_remove_tag(audio_file *f)
{
	f->remove_tag(f);
}

int audio_file_write_changes(audio_file *f)
{
	return f->write_changes(f);
}

int audio_file_set_field(audio_file *f, int field, const char *value)
{
	return f->set_field(f, field, value);
}

int audio_file_get_field(audio_file *f, int field, const char **value)
{
	return f->get_field(f, field, value);
}

void audio_file_dump(audio_file *f)
{
	f->dump(f);
}

void audio_file_edit_load(audio_file *f)
{
	f->edit_load(f);
}

void audio_file_edit_unload(audio_file *f)
{
	f->edit_unload(f);
}


