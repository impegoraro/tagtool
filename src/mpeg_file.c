#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>

#include "math_util.h"
#include "str_convert.h"
#include "genre.h"
#include "prefs_dlg.h"
#include "mpeg_file.h"


/*** private functions dealing with MPEG headers ****************************/

static gint lookup_bit_rate(guint8 major_ver, guint8 minor_ver, guint8 layer, gint id)
{
	static gint v1[3][15] = {
		{0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448},	/* layer 1 */
		{0, 32, 48, 56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384},	/* layer 2 */
		{0, 32, 40, 48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320}	/* layer 3 */
	};
	static gint v2[3][15] = {
		{0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256},	/* layer 1 */
		{0,  8, 16, 24, 32, 40, 48,  56,  64,  80,  96, 112, 128, 144, 160},	/* layer 2 */
		{0,  8, 16, 24, 32, 40, 48,  56,  64,  80,  96, 112, 128, 144, 160}	/* layer 3 */
	};

	if (id == 0xF) return -1;

	if (major_ver == 1 && minor_ver == 0)
		return v1[layer-1][id];
	else if (major_ver == 2 && (minor_ver == 0 || minor_ver == 5))
		return v2[layer-1][id];

	return -1;
}

static gint lookup_sample_rate(guint8 major_ver, guint8 minor_ver, gint id)
{
	static gint v1[] = { 44100, 48000, 32000 };
	static gint v2[] = { 22050, 24000, 16000 };
	static gint v2_5[] = { 11025, 12000, 8000 };

	if (id == 0x3) return -1;

	if (major_ver == 1 && minor_ver == 0) {
		return v1[id];
	} else if (major_ver == 2) {
		if (minor_ver == 0) return v2[id];
		if (minor_ver == 5) return v2_5[id];
	}
	
	return -1;
}

static char *lookup_channel_mode(gint id)
{
	static char *modes[] = {
		"Stereo",
		"Joint Stereo",
		"Dual Channel",
		"Mono" 
	};

	return modes[id];
}


/* Reads the first valid mpeg header found in the given file */
static gboolean read_header(FILE *file, size_t offset, frame_header *header)
{
	/*
	  mpeg audio frame header (32 bits):

	  AAAAAAAA AAABBCCD EEEEFFGH IIJJKLMM

	  A  Frame sync (all bits set)
	  B  MPEG Audio version ID
	  C  Layer number
	  D  CRC bit (if set 16 bit CRC folows header)
	  E  Bitrate ID
	  F  Sampling rate ID
	  G  Padding bit (if set frame is padded with one extra slot)
	  H  Private bit (free for application specific use)
	  I  Channel Mode
	  J  Mode extension (only if Joint Stereo)
	  K  Copyright bit
	  L  Original bit
	  M  Emphasis
	*/

	/* TODO:
	   - This needs to be more robust. Any chunk of random binary data
	     will seem to contain valid frame headers.
	     Checking for 2 or 3 valid frames in a row should do the trick.
	   - Calculate the playing time and include it in the info.
	     Requires reading the whole file (because ov VBR)
	*/
	
	guint8 hdr[4];

	fseek(file, offset, SEEK_SET);

	while (fread(&hdr, 1, 4, file) == 4) {
		/* first 11 bits should be set (frame synchronization mark) */
		if (hdr[0] != 0xFF || (hdr[1] & 0xE0) != 0xE0) {
			/* frame sync not found */
			fseek(file, -3, SEEK_CUR);
			continue;
		}

		/* get mpeg audio version */
		switch (hdr[1]>>3 & 0x03) {
		case 3:
			header->major_version = 1;
			header->minor_version = 0;
			break;
		case 2:
			header->major_version = 2;
			header->minor_version = 0;
			break;
		case 0:
			header->major_version = 2;
			header->minor_version = 5;
			break;
		default:
			/* invalid version */
			fseek(file, -3, SEEK_CUR);
			continue;
		}

		/* get layer number */
		switch (hdr[1]>>1 & 0x03) {
		case 3:
			header->layer = 1;
			break;
		case 2:
			header->layer = 2;
			break;
		case 1:
			header->layer = 3;
			break;
		default:
			/* invalid layer */
			fseek(file, -3, SEEK_CUR);
			continue;
		}

		/* get bit rate */
		header->bit_rate = lookup_bit_rate(header->major_version, header->minor_version, 
						   header->layer, hdr[2]>>4 & 0x0F);
		if (header->bit_rate == -1) {
			/* invalid bitrate */
			fseek(file, -3, SEEK_CUR);
			continue;
		}

		/* get sample rate */
		header->sample_rate = lookup_sample_rate(header->major_version, header->minor_version, 
							 hdr[2]>>2 & 0x03);
		if (header->sample_rate == -1) {
			/* invalid sample rate */
			fseek(file, -3, SEEK_CUR);
			continue;
		}

		/* get channel mode */
		header->channel_mode = lookup_channel_mode(hdr[3]>>6 & 0x03);

		/* get copyright flag */
		header->copyright = hdr[3]>>3 & 0x01;

		/* get original flag */
		header->original = hdr[3]>>2 & 0x01;

		/* keep the offset */
		header->offset = ftell(file)-4;

		/* keep the raw header */
		memcpy(header->raw, hdr, 4);

		return TRUE;
	}

	/* eof or error, give up */
	return FALSE;
}


/*** private functions dealing with ID3 tags ********************************/

/* Gets the ID3v2 frame id that corresponds to the given field */
static int frame_id_from_field(int field)
{
	switch (field) {
		case AF_TITLE:
			return ID3FID_TITLE;		// TIT2
		case AF_ARTIST:
			return ID3FID_LEADARTIST;	// TPE1
		case AF_ALBUM:
			return ID3FID_ALBUM;		// TALB
		case AF_YEAR:
			return ID3FID_YEAR;		// TYER
		case AF_GENRE:
			return ID3FID_CONTENTTYPE;	// TCON
		case AF_TRACK:
			return ID3FID_TRACKNUM;		// TRCK
		case AF_COMMENT:
			return ID3FID_COMMENT;		// COMM
		default:
			return ID3FID_NOFRAME;
	}
}


/*
 * Returns the string stored in a frame's text or url field.  If the frame does 
 * not have a text or url field, returns NULL.
 */
static const char *get_text_field(ID3Frame* frame)
{
	static char *convbuf = NULL;
	static char *readbuf = NULL;
	static size_t readbuf_size = 128;
	ID3Field *field;
	int encoding;

	if (ID3Frame_GetID(frame) == ID3FID_NOFRAME)
		return "[ unknown frame ]";

	field = ID3Frame_GetField(frame, ID3FN_TEXTENC);
	if (field != NULL)
		encoding = ID3Field_GetINT(field);
	else
		encoding = ID3TE_ISO8859_1;

	field = ID3Frame_GetField(frame, ID3FN_TEXT);
	if (field == NULL)
		field = ID3Frame_GetField(frame, ID3FN_URL);
	if (field == NULL)
		return NULL;

	if (readbuf == NULL)
		readbuf = malloc(readbuf_size);

	while(1) {
		size_t read;

		if (ID3TE_IS_SINGLE_BYTE_ENC(encoding))
			read = ID3Field_GetASCII(field, readbuf, readbuf_size);
		else
			read = ID3Field_GetUNICODE(field, (unicode_t*)readbuf, readbuf_size / 2);

		if (read < readbuf_size)
			break;

		/* grow buffer when needed */
		free(readbuf);
		readbuf_size *= 2;
		readbuf = malloc(readbuf_size);
	}

	free(convbuf);
	if (ID3TE_IS_SINGLE_BYTE_ENC(encoding)) {
		convbuf = str_convert_encoding(ISO_8859_1, UTF_8, readbuf);
	}
	else {
		/*
		 * NOTE
		 * Returned UTF-16 string does not hava a BOM, but seems to be big-endian.
		 * No mention of this in the docs though, so can we rely on it???
		 */
		convbuf = str_convert_encoding(UTF_16BE, UTF_8, readbuf);
	}

	return convbuf;
}

/*
 * Sets the string stored in a frame's text or url field.  If the frame does 
 * not have a text or url field, has no effect.
 */
static void set_text_field(ID3Frame* frame, const char *text)
{
	ID3Field *field;
	char *convbuf;
	int encoding;

	field = ID3Frame_GetField(frame, ID3FN_TEXTENC);
	if (field != NULL) {

		// XXX make configurable
		
		ID3Field_SetINT(field, ID3TE_ISO8859_1);
		encoding = ISO_8859_1;
	}
	else {
		encoding = ISO_8859_1;
	}

	field = ID3Frame_GetField(frame, ID3FN_TEXT);
	if (field == NULL)
		field = ID3Frame_GetField(frame, ID3FN_URL);
	if (field == NULL)
		return;

	convbuf = str_convert_encoding(UTF_8, encoding, text);
	if (encoding == ISO_8859_1)
		ID3Field_SetASCII(field, convbuf);
	else
		ID3Field_SetUNICODE(field, (unicode_t*)convbuf);

	free(convbuf);
}


/*
 * Expand the genre notation used in ID3v2 TCON frames
 * NOTE: This function ignores anything past the first genre reference. 
 *       (The ID3v2 spec pointlessly complicates the definition of the TCON frame.
 *       In fact the whole spec is an exercise in pointless complication. Sigh.)
 */
static const char *parse_v2_content_type(const char *text)
{
	gulong id;
	char *end;

	if (text[0] != '(')
		return text;

	if (text[1] == '(')
		return text+1;

	id = strtoul(text+1, &end, 10);
	if (end == text || *end != ')') {
		return text;
	}
	else {
		const char *genre = genre_get_name((int)id);
		return (genre == NULL ? "" : genre);
	}
}

/* Encode the genre with the notation used in ID3v2 TCON frames */
static const char *encode_v2_content_type(const char *text)
{
	static char buf[10];
	
	int id = genre_get_id(text);
	if (id < 0) {
		return "";
	}
	else {
		snprintf(buf, 10, "(%i)", id);
		return buf;
	}
}


/* Copy frames from one tag version to another */
static void copy_frames(int from_v, int to_v)
{
	// XXX - this can't be done because the C wrapper is missing the 
	//       copy constructors and Field iterator.

	/*
	ID3Tag *from_tag, *to_tag;
	ID3Frame *from_frame, *to_frame;
	ID3TagIterator *i;

	from_tag = (from_v == ID3TT_ID3V1) ? f->v1_tag : f->v2_tag;
	to_tag = (to_v == ID3TT_ID3V1) ? f->v1_tag : f->v2_tag;

	i = ID3Tag_CreateIterator(from_tag);
	while ( (from_frame = ID3TagIterator_GetNext(i)) ) {


	}

	ID3TagIterator_Delete(i);
	*/
}


/* Remove all frames from a tag */
static void remove_all_frames(ID3Tag *tag)
{
	ID3Frame *frame;
	ID3TagIterator *i = ID3Tag_CreateIterator(tag);

	while ( (frame = ID3TagIterator_GetNext(i)) ) {
		ID3Tag_RemoveFrame(tag, frame);
		ID3Frame_Delete(frame);
	}

	ID3TagIterator_Delete(i);
}


/* Returns TRUE if the tag can be saved to ID3v1 without loss of information */
static gboolean tag_fits_id3v1(ID3Tag *tag)
{
	gboolean result = TRUE;

	ID3Frame *frame;
	ID3TagIterator *i;
	const char *text;
	const char *p;

	i = ID3Tag_CreateIterator(tag);
	while ( (frame = ID3TagIterator_GetNext(i)) && result ) {
		int id = ID3Frame_GetID(frame);

		switch (id) {
		case ID3FID_TITLE:
		case ID3FID_LEADARTIST:
		case ID3FID_ALBUM:
		case ID3FID_COMMENT:
			text = get_text_field(frame);
			result = strlen(text) <= 30;
			break;
		case ID3FID_YEAR:
			text = get_text_field(frame);
			result = strlen(text) <= 4;
			break;
		case ID3FID_CONTENTTYPE:
			text = parse_v2_content_type(get_text_field(frame));
			result = genre_get_id(text) >= 0;
			break;
		case ID3FID_TRACKNUM:
			text = get_text_field(frame);
			for (p = text; *p && result; p++)
				result = isdigit(*p);
			break;
		default:
			result = FALSE;
			break;
		}
	}
	ID3TagIterator_Delete(i);

	return result;
}


/* Dump tag contents to stdout */
static void dump_tag(ID3Tag *tag)
{
	ID3Frame *frame;
	ID3TagIterator *i;
	const char *text;

	i = ID3Tag_CreateIterator(tag);
	while ( (frame = ID3TagIterator_GetNext(i)) ) {
		int id = ID3Frame_GetID(frame);

		if (id != ID3FID_NOFRAME)
			printf("%s (%s)\n", ID3FrameInfo_LongName(id), ID3FrameInfo_Description(id));
		else
			printf("**** UNKNOWN FRAME\n");
			
		text = get_text_field(frame);
		if (text != NULL)
			printf("\t%s\n", text);
		else
			printf("\t[ non-text frame ]\n");
	}
	ID3TagIterator_Delete(i);
}


/*
 * This wrapper is meant to be called from audio_file_write_changes()
 * It enforces the tag version preferences
 */
int mpeg_file_write_changes_wrapper(mpeg_file *f)
{
	id3_prefs prefs = prefs_get_id3_prefs();

	if (prefs.write_id3_version == WRITE_V1 && (!f->orig_v2_tag || !prefs.preserve_existing)) {
		mpeg_file_remove_tag_v(f, ID3TT_ID3V2);
	}
	else if (prefs.write_id3_version == WRITE_V2 && (!f->orig_v1_tag || !prefs.preserve_existing)) {
		mpeg_file_remove_tag_v(f, ID3TT_ID3V1);
	}
	else if (prefs.write_id3_version == WRITE_AUTO && (!f->orig_v2_tag || !prefs.preserve_existing)) {
		if (tag_fits_id3v1(f->v2_tag))
			mpeg_file_remove_tag_v(f, ID3TT_ID3V2);
	}

	return mpeg_file_write_changes(f);
}


/*** public functions *******************************************************/

/* 
 * MPEG implementation of the standard audio_file functions
 */

int mpeg_file_new(mpeg_file **f, const char *filename, gboolean editable)
{
	mpeg_file newfile;
	int v2_disk_size;
	gboolean res;

	*f = NULL;
	
	newfile.file = fopen(filename, (editable ? "r+" : "r"));
	if (!newfile.file)
		return AF_ERR_FILE;

	/* read v1 tag */
	newfile.v1_tag = ID3Tag_New();
	ID3Tag_LinkWithFlags(newfile.v1_tag, filename, ID3TT_ID3V1);
	newfile.has_v1_tag = ID3Tag_HasTagType(newfile.v1_tag, ID3TT_ID3V1);
	newfile.orig_v1_tag = newfile.has_v1_tag;

	/* read v2 tag */
	newfile.v2_tag = ID3Tag_New();
	v2_disk_size = ID3Tag_LinkWithFlags(newfile.v2_tag, filename, ID3TT_ID3V2);
	newfile.has_v2_tag = ID3Tag_HasTagType(newfile.v2_tag, ID3TT_ID3V2);;
	newfile.orig_v2_tag = newfile.has_v2_tag;

	/* read 1st frame header */
	res = read_header(newfile.file, v2_disk_size, &newfile.header);
	fclose(newfile.file);
	newfile.file = NULL;
	if (!res) {
		ID3Tag_Delete(newfile.v1_tag);
		ID3Tag_Delete(newfile.v2_tag);
		return AF_ERR_FORMAT;
	}


	newfile.type = AF_MPEG;
	newfile.name = strdup(filename);
	newfile.editable = editable;
	newfile.changed = FALSE;

	newfile.delete = (af_delete_func) mpeg_file_delete;
	newfile.get_desc = (af_get_desc_func) mpeg_file_get_desc;
	newfile.get_info = (af_get_info_func) mpeg_file_get_info;
	newfile.has_tag = (af_has_tag_func) mpeg_file_has_tag;
	newfile.create_tag = (af_create_tag_func) mpeg_file_create_tag;
	newfile.remove_tag = (af_remove_tag_func) mpeg_file_remove_tag;
	newfile.write_changes = (af_write_changes_func) mpeg_file_write_changes_wrapper;
	newfile.set_field = (af_set_field_func) mpeg_file_set_field;
	newfile.get_field = (af_get_field_func) mpeg_file_get_field;
	newfile.dump = (af_dump_func) mpeg_file_dump;
	newfile.edit_load = (af_edit_load_func) mpeg_file_edit_load;
	newfile.edit_unload = (af_edit_unload_func) mpeg_file_edit_unload;

	*f = malloc(sizeof(mpeg_file));
	memcpy(*f, &newfile, sizeof(mpeg_file));

	return AF_OK;
}

void mpeg_file_delete(mpeg_file *f)
{
	if (f) {
		if (f->file) fclose(f->file);

		if (f->v1_tag != NULL)
			ID3Tag_Delete(f->v1_tag);
		if (f->v2_tag != NULL)
			ID3Tag_Delete(f->v2_tag);

		free(f->name);
		free(f);
	}
}


const char *mpeg_file_get_desc(mpeg_file *f)
{
	static char buf[32];

	snprintf(buf, 32, "MPEG version %d.%d, layer %d", 
		 f->header.major_version, f->header.minor_version, f->header.layer);
	return buf;
}

const char *mpeg_file_get_info(mpeg_file *f)
{
	static char buf[256];

	snprintf(buf, sizeof(buf), 
		 _("Bit Rate\n%d kbps\n"
		   "Sample Rate\n%d Hz\n"
		   "Channel Mode\n%s\n"
		   "Copyrighted\n%s\n"
		   "Original\n%s\n"),
		 f->header.bit_rate, f->header.sample_rate, f->header.channel_mode, 
		 (f->header.copyright ? _("Yes") : _("No")), 
		 (f->header.original ? _("Yes") : _("No")));
	return buf;
}


gboolean mpeg_file_has_tag(mpeg_file *f)
{
	return f->has_v1_tag || f->has_v2_tag;
}


void mpeg_file_create_tag(mpeg_file *f)
{
	mpeg_file_create_tag_v(f, ID3TT_ID3V1);
	mpeg_file_create_tag_v(f, ID3TT_ID3V2);
}


void mpeg_file_remove_tag(mpeg_file *f)
{
	mpeg_file_remove_tag_v(f, ID3TT_ID3V1);
	mpeg_file_remove_tag_v(f, ID3TT_ID3V2);
}


int mpeg_file_write_changes(mpeg_file *f)
{
	ID3_Err res;

	if (!f->changed)
		return AF_OK;

	if (!f->editable)
		return AF_ERR_READONLY;


	if (f->has_v1_tag)
		res = ID3Tag_UpdateByTagType(f->v1_tag, ID3TT_ID3V1);
	else {
		/*
		 * NOTE BUG WORKAROUND:
		 * Reloading the tag here is needed because if the other tag was 
		 * deleted in the meantime, id3lib gets all confused about the 
		 * size of the file and ends up leaving garbage at the end.
		 */
		ID3Tag_LinkWithFlags(f->v1_tag, f->name, ID3TT_ID3V1);

		res = ID3Tag_Strip(f->v1_tag, ID3TT_ID3V1);

		remove_all_frames(f->v1_tag);
	}
	if (res != ID3E_NoError)
		return AF_ERR_FILE;
	
	if (f->has_v2_tag)
		res = ID3Tag_UpdateByTagType(f->v2_tag, ID3TT_ID3V2);
	else {
		/* NOTE BUG WORKAROUND: same as above */
		ID3Tag_LinkWithFlags(f->v2_tag, f->name, ID3TT_ID3V2);

		res = ID3Tag_Strip(f->v2_tag, ID3TT_ID3V2);

		remove_all_frames(f->v2_tag);
	}
	if (res != ID3E_NoError)
		return AF_ERR_FILE;


	f->changed = FALSE;

	return AF_OK;
}


int mpeg_file_set_field(mpeg_file *f, int field, const char *value)
{
	if (f->has_v1_tag) {
		int res = mpeg_file_set_field_v(f, ID3TT_ID3V1, field, value);
		if (res != AF_OK)
			return res;
	}
	if (f->has_v2_tag) {
		int res = mpeg_file_set_field_v(f, ID3TT_ID3V2, field, value);
		if (res != AF_OK)
			return res;
	}

	return AF_OK;
}

int mpeg_file_get_field(mpeg_file *f, int field, const char **value)
{
	int version;

	/* If both v1 and v2 tags are present v2 has priority */
	if (f->has_v2_tag)
		version = ID3TT_ID3V2;
	else if (f->has_v1_tag)
		version = ID3TT_ID3V1;
	else
		return AF_ERR_NO_TAG;

	return mpeg_file_get_field_v(f, version, field, value);
}


void mpeg_file_dump(mpeg_file *f)
{
	printf("\nFile type: MPEG Audio\n");

	printf("\nFirst frame found at 0x%08X (header: 0x%02X%02X%02X%02X):\n",
	       f->header.offset, f->header.raw[0], f->header.raw[1], 
	       f->header.raw[2], f->header.raw[3]);
	printf("MPEG type   : Version %d.%d, Layer %d\n"
	       "Bit rate    : %d kbps\n"
	       "Sample rate : %d Hz\n"
	       "Channel mode: %s\n"
	       "Copyrighted : %s\n"
	       "Original    : %s\n",
	       f->header.major_version, f->header.minor_version, f->header.layer, 
	       f->header.bit_rate, f->header.sample_rate, f->header.channel_mode, 
	       (f->header.copyright ? "Yes" : "No"), 
	       (f->header.original ? "Yes" : "No"));

	if (f->has_v1_tag) {
		printf("\n---- ID3 v1 ----\n\n");
		dump_tag(f->v1_tag);
	}
	if (f->has_v2_tag) {
		printf("\n---- ID3 v2 ----\n\n");
		dump_tag(f->v2_tag);
	}
	if (!(f->has_v1_tag || f->has_v2_tag))
		printf("\nNo ID3 tags.\n");

	printf("\n");
}


extern void mpeg_edit_load(mpeg_file *f);

void mpeg_file_edit_load(mpeg_file *f)
{
	mpeg_edit_load(f);
}


extern void mpeg_edit_unload();

void mpeg_file_edit_unload(mpeg_file *f)
{
	mpeg_edit_unload();
}


/*
 * MPEG specific functions
 */

gboolean mpeg_file_has_tag_v(mpeg_file *f, int version)
{
	switch (version) {
		case ID3TT_ID3V1:
			return f->has_v1_tag;
		case ID3TT_ID3V2:
			return f->has_v2_tag;
		default:
			return FALSE;
	}
}

void mpeg_file_create_tag_v(mpeg_file *f, int version)
{
	switch (version) {
		case ID3TT_ID3V1:
			if (!f->has_v1_tag) {
				f->has_v1_tag = TRUE;
				f->changed = TRUE;
				if (!f->has_v2_tag)
					copy_frames(ID3TT_ID3V2, ID3TT_ID3V1);
			}
			break;
		case ID3TT_ID3V2:
			if (!f->has_v2_tag) {
				f->has_v2_tag = TRUE;
				f->changed = TRUE;
				if (!f->has_v1_tag)
					copy_frames(ID3TT_ID3V1, ID3TT_ID3V2);
			}
			break;
	}
}

void mpeg_file_remove_tag_v(mpeg_file *f, int version)
{
	switch (version) {
		case ID3TT_ID3V1:
			if (f->has_v1_tag) {
				remove_all_frames(f->v1_tag);
				f->has_v1_tag = FALSE;
				f->changed = TRUE;
			}
			break;
		case ID3TT_ID3V2:
			if (f->has_v2_tag) {
				remove_all_frames(f->v2_tag);
				f->has_v2_tag = FALSE;
				f->changed = TRUE;
			}
			break;
	}
}


int mpeg_file_set_field_v(mpeg_file *f, int version, int field, const char *value)
{
	ID3Tag* tag = NULL;
	ID3Frame *frame = NULL;
	int frame_id = frame_id_from_field(field);

	if (version == ID3TT_ID3V1)
		tag = f->v1_tag;
	else if (version == ID3TT_ID3V2)
		tag = f->v2_tag;
	else
		return AF_ERR_NO_TAG;

	frame = ID3Tag_FindFrameWithID(tag, frame_id);

	if (strcmp(value, "") == 0) {
		if (frame != NULL) {
			ID3Tag_RemoveFrame(tag, frame);
			ID3Frame_Delete(frame);
		}
		return AF_OK;
	}

	if (frame == NULL) {
		frame = ID3Frame_NewID(frame_id);
		ID3Tag_AttachFrame(tag, frame);
	}
	return mpeg_file_set_frame_text(f, version, frame, value);
}

int mpeg_file_get_field_v(mpeg_file *f, int version, int field, const char **value)
{
	ID3Tag* tag = NULL;
	ID3Frame *frame = NULL;

	*value = NULL;

	if (version == ID3TT_ID3V1)
		tag = f->v1_tag;
	else if (version == ID3TT_ID3V2)
		tag = f->v2_tag;
	else
		return AF_ERR_NO_TAG;
	
	frame = ID3Tag_FindFrameWithID(tag, frame_id_from_field(field));
	if (frame != NULL) {
		return mpeg_file_get_frame_text(f, version, frame, value);
	}
	else {
		*value = "";
		return AF_OK;
	}
}


int mpeg_file_set_frame_text(mpeg_file *f, int version, ID3Frame *frame, const char *value)
{
	int frame_id = ID3Frame_GetID(frame);

	if (frame_id == ID3FID_NOFRAME)
		return AF_ERR_NO_FIELD;


	if (version == ID3TT_ID3V1 && frame_id == ID3FID_CONTENTTYPE) {
		/* when saving v1 tags, genre must be encoded numerically */
		value = encode_v2_content_type(value);
	}

	set_text_field(frame, value);
	f->changed = TRUE;

	return AF_OK;
}

int mpeg_file_get_frame_text(mpeg_file *f, int version, ID3Frame *frame, const char **value)
{
	int frame_id = ID3Frame_GetID(frame);
	
	*value = NULL;

	if (frame_id == ID3FID_NOFRAME)
		return AF_ERR_NO_FIELD;

	
	if (frame != NULL)
		*value = get_text_field(frame);

	if (frame_id == ID3FID_CONTENTTYPE)
		*value = parse_v2_content_type(*value);

	return AF_OK;
}


int mpeg_file_remove_frame(mpeg_file *f, int version, ID3Frame *frame)
{
	ID3Tag* tag = NULL;

	if (version == ID3TT_ID3V1)
		tag = f->v1_tag;
	else if (version == ID3TT_ID3V2)
		tag = f->v2_tag;
	else
		return AF_ERR_NO_TAG;

	ID3Tag_RemoveFrame(tag, frame);
	ID3Frame_Delete(frame);

	return AF_OK;
}


int mpeg_file_get_frames(mpeg_file *f, int version, ID3Frame ***frames)
{
	ID3Tag *tag;
	ID3Frame *frame;
	ID3TagIterator *iter;
	int num_frames;
	int i;

	if (version == ID3TT_ID3V1)
		tag = f->v1_tag;
	else if (version == ID3TT_ID3V2)
		tag = f->v2_tag;
	else
		return 0;

	num_frames = ID3Tag_NumFrames(tag);
	*frames = malloc(sizeof(ID3Frame*) * num_frames);

	i = 0;
	iter = ID3Tag_CreateIterator(tag);
	while ( (frame = ID3TagIterator_GetNext(iter)) )
		(*frames)[i++] = frame;
	ID3TagIterator_Delete(iter);

	return num_frames;
}


void mpeg_file_get_frame_info(int frame_id, const char **name, const char** desc, gboolean *editable, gboolean *simple)
{
	if (frame_id == ID3FID_NOFRAME) {
		if (name)
			*name = "????";
		if (desc)
			*desc = "UNKNOWN FRAME";
		if (editable)
			*editable = FALSE;
		if (simple)
			*simple = FALSE;
	}
	else {
		if (name)
			*name = ID3FrameInfo_LongName(frame_id);
		if (desc)
			*desc = ID3FrameInfo_Description(frame_id);
		if (editable) {
			char *aux = ID3FrameInfo_LongName(frame_id);
			
			if (aux == NULL || strlen(aux) != 4)
				*editable = FALSE;
			else if (aux[0] == 'T' && strcmp(aux, "TXXX") != 0)
				*editable = TRUE;
			else if (aux[0] == 'W' && strcmp(aux, "WXXX") != 0)
				*editable = TRUE;
			else if (strcmp(aux, "COMM") == 0)
				*editable = TRUE;
			else
				*editable = FALSE;
		}
		if (simple) {
			switch (frame_id) {
			case ID3FID_TITLE:
			case ID3FID_LEADARTIST:
			case ID3FID_ALBUM:
			case ID3FID_YEAR:
			case ID3FID_CONTENTTYPE:
			case ID3FID_TRACKNUM:
			case ID3FID_COMMENT:
				*simple = TRUE;
				break;
			default:
				*simple = FALSE;
				break;
			}
		}
	}
}


int mpeg_file_get_editable_frame_ids(int **ids)
{
	static int editable_frame_ids[] =
	{
		/* COMM */ ID3FID_COMMENT,
		/* TALB */ ID3FID_ALBUM,
		/* TBPM */ ID3FID_BPM,
		/* TCOM */ ID3FID_COMPOSER,
		/* TCON */ ID3FID_CONTENTTYPE,
		/* TCOP */ ID3FID_COPYRIGHT,
		/* TDAT */ ID3FID_DATE,
		/* TDEN */ //ID3FID_ENCODINGTIME,
		/* TDLY */ ID3FID_PLAYLISTDELAY,
		/* TDOR */ //ID3FID_ORIGRELEASETIME,
		/* TDRC */ //ID3FID_RECORDINGTIME,
		/* TDRL */ //ID3FID_RELEASETIME,
		/* TDTG */ //ID3FID_TAGGINGTIME,
		/* TIPL */ //ID3FID_INVOLVEDPEOPLE2,
		/* TENC */ ID3FID_ENCODEDBY,
		/* TEXT */ ID3FID_LYRICIST,
		/* TFLT */ ID3FID_FILETYPE,
		/* TIME */ ID3FID_TIME,
		/* TIT1 */ ID3FID_CONTENTGROUP,
		/* TIT2 */ ID3FID_TITLE,
		/* TIT3 */ ID3FID_SUBTITLE,
		/* TKEY */ ID3FID_INITIALKEY,
		/* TLAN */ ID3FID_LANGUAGE,
		/* TLEN */ ID3FID_SONGLEN,
		/* TMCL */ //ID3FID_MUSICIANCREDITLIST,
		/* TMED */ ID3FID_MEDIATYPE,
		/* TMOO */ //ID3FID_MOOD,
		/* TOAL */ ID3FID_ORIGALBUM,
		/* TOFN */ ID3FID_ORIGFILENAME,
		/* TOLY */ ID3FID_ORIGLYRICIST,
		/* TOPE */ ID3FID_ORIGARTIST,
		/* TORY */ ID3FID_ORIGYEAR,
		/* TOWN */ ID3FID_FILEOWNER,
		/* TPE1 */ ID3FID_LEADARTIST,
		/* TPE2 */ ID3FID_BAND,
		/* TPE3 */ ID3FID_CONDUCTOR,
		/* TPE4 */ ID3FID_MIXARTIST,
		/* TPOS */ ID3FID_PARTINSET,
		/* TPRO */ //ID3FID_PRODUCEDNOTICE,
		/* TPUB */ ID3FID_PUBLISHER,
		/* TRCK */ ID3FID_TRACKNUM,
		/* TRDA */ ID3FID_RECORDINGDATES,
		/* TRSN */ ID3FID_NETRADIOSTATION,
		/* TRSO */ ID3FID_NETRADIOOWNER,
		/* TSIZ */ ID3FID_SIZE,
		/* TSOA */ //ID3FID_ALBUMSORTORDER,
		/* TSOP */ //ID3FID_PERFORMERSORTORDER,
		/* TSOT */ //ID3FID_TITLESORTORDER,
		/* TSRC */ ID3FID_ISRC,
		/* TSSE */ ID3FID_ENCODERSETTINGS,
		/* TSST */ //ID3FID_SETSUBTITLE,
		/* TYER */ ID3FID_YEAR,
		/* WCOM */ ID3FID_WWWCOMMERCIALINFO,
		/* WCOP */ ID3FID_WWWCOPYRIGHT,
		/* WOAF */ ID3FID_WWWAUDIOFILE,
		/* WOAR */ ID3FID_WWWARTIST,
		/* WOAS */ ID3FID_WWWAUDIOSOURCE,
		/* WORS */ ID3FID_WWWRADIOPAGE,
		/* WPAY */ ID3FID_WWWPAYMENT,
		/* WPUB */ ID3FID_WWWPUBLISHER
	};

	*ids = editable_frame_ids;
	return sizeof(editable_frame_ids) / sizeof(int);
}

