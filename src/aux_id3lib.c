
/*
 * NOTICE  NOTICE  NOTICE  NOTICE  NOTICE  NOTICE  NOTICE  NOTICE  NOTICE 
 *
 *  This code wast adapted from ID3Lib 3.8.3, to work around a bug:
 *  The C wrapper defines a bunch of ID3FrameInfo_* functions, but the 
 *  implementation is actually missing from the library.  As soon as 
 *  that bug is fixed this file can be removed.
 *
 * NOTICE  NOTICE  NOTICE  NOTICE  NOTICE  NOTICE  NOTICE  NOTICE  NOTICE 
 */

#include "config.h"

#ifdef LIBID3_MISSING_ID3FRAMEINFO

#include <id3.h>


typedef struct
{
	ID3_FieldID   _id;
	ID3_FieldType _type;
	size_t        _fixed_size;
	ID3_V2Spec    _spec_begin;
	ID3_V2Spec    _spec_end;
	flags_t       _flags;
	ID3_FieldID   _linked_field;
	//static const ID3_FieldDef* DEFAULT;
} ID3_FieldDef;

typedef struct
{
	ID3_FrameID   eID;
	char          sShortTextID[3 + 1];
	char          sLongTextID[4 + 1];
	bool          bTagDiscard;
	bool          bFileDiscard;
	const ID3_FieldDef* aeFieldDefs;
	const char *  sDescription;
} ID3_FrameDef;


//----------------------------------------------------------------------


// This is used for unimplemented frames so that their data is preserved when
// parsing and rendering
static ID3_FieldDef ID3FD_Unimplemented[] =
{
  {
    ID3FN_DATA,                         // FIELD NAME
    ID3FTY_BINARY,                      // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  { ID3FN_NOFIELD }
};

static ID3_FieldDef ID3FD_URL[] =
{
  {
    ID3FN_URL,                          // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  { ID3FN_NOFIELD }
};

static ID3_FieldDef ID3FD_UserURL[] =
{
  {
    ID3FN_TEXTENC,                      // FIELD NAME
    ID3FTY_INTEGER,                     // FIELD TYPE
    1,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_DESCRIPTION,                  // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_CSTR | ID3FF_ENCODABLE,       // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_URL,                          // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  { ID3FN_NOFIELD }
};

static ID3_FieldDef ID3FD_Text[] =
{
  {
    ID3FN_TEXTENC,                      // FIELD NAME
    ID3FTY_INTEGER,                     // FIELD TYPE
    1,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_TEXT,                         // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_ENCODABLE,                    // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  { ID3FN_NOFIELD }
};


static ID3_FieldDef ID3FD_UserText[] =
{
  {
    ID3FN_TEXTENC,                      // FIELD NAME
    ID3FTY_INTEGER,                     // FIELD TYPE
    1,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_DESCRIPTION,                  // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_CSTR | ID3FF_ENCODABLE,       // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_TEXT,                         // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_ENCODABLE,                    // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  { ID3FN_NOFIELD }
};


static ID3_FieldDef ID3FD_GeneralText[] =
{
  {
    ID3FN_TEXTENC,                      // FIELD NAME
    ID3FTY_INTEGER,                     // FIELD TYPE
    1,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_LANGUAGE,                     // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    3,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_DESCRIPTION,                  // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_CSTR | ID3FF_ENCODABLE,       // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_TEXT,                         // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_ENCODABLE,                    // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  { ID3FN_NOFIELD }
};

static ID3_FieldDef ID3FD_TermsOfUse[] =
{
  {
    ID3FN_TEXTENC,                      // FIELD NAME
    ID3FTY_INTEGER,                     // FIELD TYPE
    1,                                  // FIXED LEN
    ID3V2_3_0,                          // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_LANGUAGE,                     // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    3,                                  // FIXED LEN
    ID3V2_3_0,                          // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_TEXT,                         // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_3_0,                          // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_ENCODABLE,                    // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  { ID3FN_NOFIELD }
};

static ID3_FieldDef ID3FD_LinkedInfo[] =
{
  {
    ID3FN_ID,                           // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    3,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_2_1,                          // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_ID,                           // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    4,                                  // FIXED LEN
    ID3V2_3_0,                          // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_URL,                          // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_CSTR,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_TEXT,                         // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  { ID3FN_NOFIELD }
};

static ID3_FieldDef ID3FD_Picture[] =
{
  {
    ID3FN_TEXTENC,                      // FIELD NAME
    ID3FTY_INTEGER,                     // FIELD TYPE
    1,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_IMAGEFORMAT,                  // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    3,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_2_1,                          // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_MIMETYPE,                     // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_3_0,                          // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_CSTR,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_PICTURETYPE,                  // FIELD NAME
    ID3FTY_INTEGER,                     // FIELD TYPE
    1,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_DESCRIPTION,                  // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_CSTR | ID3FF_ENCODABLE,       // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_DATA,                         // FIELD NAME
    ID3FTY_BINARY,                      // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  { ID3FN_NOFIELD }
};

static ID3_FieldDef ID3FD_GEO[] =
{
  {
    ID3FN_TEXTENC,                      // FIELD NAME
    ID3FTY_INTEGER,                     // FIELD TYPE
    1,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_MIMETYPE,                     // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_CSTR,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_FILENAME,                     // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_CSTR | ID3FF_ENCODABLE,       // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_DESCRIPTION,                  // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_CSTR | ID3FF_ENCODABLE,       // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_DATA,                         // FIELD NAME
    ID3FTY_BINARY,                      // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  { ID3FN_NOFIELD }
};

static ID3_FieldDef ID3FD_UFI[] =
{
  {
    ID3FN_OWNER,                        // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_CSTR,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_DATA,                         // FIELD NAME
    ID3FTY_BINARY,                      // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  { ID3FN_NOFIELD }
};

static ID3_FieldDef ID3FD_PlayCounter[] =
{
  {
    ID3FN_COUNTER,                      // FIELD NAME
    ID3FTY_INTEGER,                     // FIELD TYPE
    4,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  { ID3FN_NOFIELD }
};

static ID3_FieldDef ID3FD_Popularimeter[] =
{
  {
    ID3FN_EMAIL,                        // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_CSTR,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_RATING,                       // FIELD NAME
    ID3FTY_INTEGER,                     // FIELD TYPE
    1,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_COUNTER,                      // FIELD NAME
    ID3FTY_INTEGER,                     // FIELD TYPE
    4,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  { ID3FN_NOFIELD }
};

static ID3_FieldDef ID3FD_Private[] =
{
  {
    ID3FN_OWNER,                        // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_3_0,                          // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_CSTR,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_DATA,                         // FIELD NAME
    ID3FTY_BINARY,                      // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_3_0,                          // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  { ID3FN_NOFIELD }
};


static ID3_FieldDef ID3FD_Registration[] =
{
  {
    ID3FN_OWNER,                        // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_3_0,                          // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_CSTR,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_ID,                           // FIELD NAME
    ID3FTY_INTEGER,                     // FIELD TYPE
    1,                                  // FIXED LEN
    ID3V2_3_0,                          // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_DATA,                         // FIELD NAME
    ID3FTY_BINARY,                      // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_3_0,                          // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  { ID3FN_NOFIELD }
};

static ID3_FieldDef ID3FD_InvolvedPeople[] =
{
  {
    ID3FN_TEXTENC,                      // FIELD NAME
    ID3FTY_INTEGER,                     // FIELD TYPE
    1,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_TEXT,                         // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_TEXTLIST,                     // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  { ID3FN_NOFIELD }
};

static ID3_FieldDef ID3FD_CDM[] =
{
  {
    ID3FN_DATA,                         // FIELD NAME
    ID3FTY_BINARY,                      // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_2_1,                          // INITIAL SPEC
    ID3V2_2_1,                          // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  }
};

static ID3_FieldDef ID3FD_SyncLyrics[] =
{
  {
    ID3FN_TEXTENC,                      // FIELD NAME
    ID3FTY_INTEGER,                     // FIELD TYPE
    1,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_LANGUAGE,                     // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    3,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_TIMESTAMPFORMAT,              // FIELD NAME
    ID3FTY_INTEGER,                     // FIELD TYPE
    1,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_CONTENTTYPE,                  // FIELD NAME
    ID3FTY_INTEGER,                     // FIELD TYPE
    1,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_DESCRIPTION,                  // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_CSTR | ID3FF_ENCODABLE,       // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_DATA,                         // FIELD NAME
    ID3FTY_BINARY,                      // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  { ID3FN_NOFIELD }
};


static  ID3_FrameDef ID3_FrameDefs[] =
{
  //                          short  long   tag    file
  // frame id                 id     id     discrd discrd field defs           description
  {ID3FID_AUDIOCRYPTO,       "CRA", "AENC", false, false, ID3FD_Unimplemented, "Audio encryption"},
  {ID3FID_PICTURE,           "PIC", "APIC", false, false, ID3FD_Picture,       "Attached picture"},
  {ID3FID_COMMENT,           "COM", "COMM", false, false, ID3FD_GeneralText,   "Comments"},
  {ID3FID_COMMERCIAL,        ""   , "COMR", false, false, ID3FD_Unimplemented, "Commercial"},
  {ID3FID_CRYPTOREG,         ""   , "ENCR", false, false, ID3FD_Registration,  "Encryption method registration"},
  {ID3FID_EQUALIZATION,      "EQU", "EQUA", false, true,  ID3FD_Unimplemented, "Equalization"},
  {ID3FID_EVENTTIMING,       "ETC", "ETCO", false, true,  ID3FD_Unimplemented, "Event timing codes"},
  {ID3FID_GENERALOBJECT,     "GEO", "GEOB", false, false, ID3FD_GEO,           "General encapsulated object"},
  {ID3FID_GROUPINGREG,       ""   , "GRID", false, false, ID3FD_Registration,  "Group identification registration"},
  {ID3FID_INVOLVEDPEOPLE,    "IPL", "IPLS", false, false, ID3FD_InvolvedPeople,"Involved people list"},
  {ID3FID_LINKEDINFO,        "LNK", "LINK", false, false, ID3FD_LinkedInfo,    "Linked information"},
  {ID3FID_CDID,              "MCI", "MCDI", false, false, ID3FD_Unimplemented, "Music CD identifier"},
  {ID3FID_MPEGLOOKUP,        "MLL", "MLLT", false, true,  ID3FD_Unimplemented, "MPEG location lookup table"},
  {ID3FID_OWNERSHIP,         ""   , "OWNE", false, false, ID3FD_Unimplemented, "Ownership frame"},
  {ID3FID_PRIVATE,           ""   , "PRIV", false, false, ID3FD_Private,       "Private frame"},
  {ID3FID_PLAYCOUNTER,       "CNT", "PCNT", false, false, ID3FD_PlayCounter,   "Play counter"},
  {ID3FID_POPULARIMETER,     "POP", "POPM", false, false, ID3FD_Popularimeter, "Popularimeter"},
  {ID3FID_POSITIONSYNC,      ""   , "POSS", false, true,  ID3FD_Unimplemented, "Position synchronisation frame"},
  {ID3FID_BUFFERSIZE,        "BUF", "RBUF", false, false, ID3FD_Unimplemented, "Recommended buffer size"},
  {ID3FID_VOLUMEADJ,         "RVA", "RVAD", false, true,  ID3FD_Unimplemented, "Relative volume adjustment"},
  {ID3FID_REVERB,            "REV", "RVRB", false, false, ID3FD_Unimplemented, "Reverb"},
  {ID3FID_SYNCEDLYRICS,      "SLT", "SYLT", false, false, ID3FD_SyncLyrics,    "Synchronized lyric/text"},
  {ID3FID_SYNCEDTEMPO,       "STC", "SYTC", false, true,  ID3FD_Unimplemented, "Synchronized tempo codes"},
  {ID3FID_ALBUM,             "TAL", "TALB", false, false, ID3FD_Text,          "Album/Movie/Show title"},
  {ID3FID_BPM,               "TBP", "TBPM", false, false, ID3FD_Text,          "BPM (beats per minute)"},
  {ID3FID_COMPOSER,          "TCM", "TCOM", false, false, ID3FD_Text,          "Composer"},
  {ID3FID_CONTENTTYPE,       "TCO", "TCON", false, false, ID3FD_Text,          "Content type"},
  {ID3FID_COPYRIGHT,         "TCR", "TCOP", false, false, ID3FD_Text,          "Copyright message"},
  {ID3FID_DATE,              "TDA", "TDAT", false, false, ID3FD_Text,          "Date"},
  {ID3FID_PLAYLISTDELAY,     "TDY", "TDLY", false, false, ID3FD_Text,          "Playlist delay"},
  {ID3FID_ENCODEDBY,         "TEN", "TENC", false, true,  ID3FD_Text,          "Encoded by"},
  {ID3FID_LYRICIST,          "TXT", "TEXT", false, false, ID3FD_Text,          "Lyricist/Text writer"},
  {ID3FID_FILETYPE,          "TFT", "TFLT", false, false, ID3FD_Text,          "File type"},
  {ID3FID_TIME,              "TIM", "TIME", false, false, ID3FD_Text,          "Time"},
  {ID3FID_CONTENTGROUP,      "TT1", "TIT1", false, false, ID3FD_Text,          "Content group description"},
  {ID3FID_TITLE,             "TT2", "TIT2", false, false, ID3FD_Text,          "Title/songname/content description"},
  {ID3FID_SUBTITLE,          "TT3", "TIT3", false, false, ID3FD_Text,          "Subtitle/Description refinement"},
  {ID3FID_INITIALKEY,        "TKE", "TKEY", false, false, ID3FD_Text,          "Initial key"},
  {ID3FID_LANGUAGE,          "TLA", "TLAN", false, false, ID3FD_Text,          "Language(s)"},
  {ID3FID_SONGLEN,           "TLE", "TLEN", false, true,  ID3FD_Text,          "Length"},
  {ID3FID_MEDIATYPE,         "TMT", "TMED", false, false, ID3FD_Text,          "Media type"},
  {ID3FID_ORIGALBUM,         "TOT", "TOAL", false, false, ID3FD_Text,          "Original album/movie/show title"},
  {ID3FID_ORIGFILENAME,      "TOF", "TOFN", false, false, ID3FD_Text,          "Original filename"},
  {ID3FID_ORIGLYRICIST,      "TOL", "TOLY", false, false, ID3FD_Text,          "Original lyricist(s)/text writer(s)"},
  {ID3FID_ORIGARTIST,        "TOA", "TOPE", false, false, ID3FD_Text,          "Original artist(s)/performer(s)"},
  {ID3FID_ORIGYEAR,          "TOR", "TORY", false, false, ID3FD_Text,          "Original release year"},
  {ID3FID_FILEOWNER,         ""   , "TOWN", false, false, ID3FD_Text,          "File owner/licensee"},
  {ID3FID_LEADARTIST,        "TP1", "TPE1", false, false, ID3FD_Text,          "Lead performer(s)/Soloist(s)"},
  {ID3FID_BAND,              "TP2", "TPE2", false, false, ID3FD_Text,          "Band/orchestra/accompaniment"},
  {ID3FID_CONDUCTOR,         "TP3", "TPE3", false, false, ID3FD_Text,          "Conductor/performer refinement"},
  {ID3FID_MIXARTIST,         "TP4", "TPE4", false, false, ID3FD_Text,          "Interpreted, remixed, or otherwise modified by"},
  {ID3FID_PARTINSET,         "TPA", "TPOS", false, false, ID3FD_Text,          "Part of a set"},
  {ID3FID_PUBLISHER,         "TPB", "TPUB", false, false, ID3FD_Text,          "Publisher"},
  {ID3FID_TRACKNUM,          "TRK", "TRCK", false, false, ID3FD_Text,          "Track number/Position in set"},
  {ID3FID_RECORDINGDATES,    "TRD", "TRDA", false, false, ID3FD_Text,          "Recording dates"},
  {ID3FID_NETRADIOSTATION,   "TRN", "TRSN", false, false, ID3FD_Text,          "Internet radio station name"},
  {ID3FID_NETRADIOOWNER,     "TRO", "TRSO", false, false, ID3FD_Text,          "Internet radio station owner"},
  {ID3FID_SIZE,              "TSI", "TSIZ", false, true,  ID3FD_Text,          "Size"},
  {ID3FID_ISRC,              "TRC", "TSRC", false, false, ID3FD_Text,          "ISRC (international standard recording code)"},
  {ID3FID_ENCODERSETTINGS,   "TSS", "TSSE", false, false, ID3FD_Text,          "Software/Hardware and settings used for encoding"},
  {ID3FID_USERTEXT,          "TXX", "TXXX", false, false, ID3FD_UserText,      "User defined text information"},
  {ID3FID_YEAR,              "TYE", "TYER", false, false, ID3FD_Text,          "Year"},
  {ID3FID_UNIQUEFILEID,      "UFI", "UFID", false, false, ID3FD_UFI,           "Unique file identifier"},
  {ID3FID_TERMSOFUSE,        ""   , "USER", false, false, ID3FD_TermsOfUse,    "Terms of use"},
  {ID3FID_UNSYNCEDLYRICS,    "ULT", "USLT", false, false, ID3FD_GeneralText,   "Unsynchronized lyric/text transcription"},
  {ID3FID_WWWCOMMERCIALINFO, "WCM", "WCOM", false, false, ID3FD_URL,           "Commercial information"},
  {ID3FID_WWWCOPYRIGHT,      "WCP", "WCOP", false, false, ID3FD_URL,           "Copyright/Legal infromation"},
  {ID3FID_WWWAUDIOFILE,      "WAF", "WOAF", false, false, ID3FD_URL,           "Official audio file webpage"},
  {ID3FID_WWWARTIST,         "WAR", "WOAR", false, false, ID3FD_URL,           "Official artist/performer webpage"},
  {ID3FID_WWWAUDIOSOURCE,    "WAS", "WOAS", false, false, ID3FD_URL,           "Official audio source webpage"},
  {ID3FID_WWWRADIOPAGE,      "WRA", "WORS", false, false, ID3FD_URL,           "Official internet radio station homepage"},
  {ID3FID_WWWPAYMENT,        "WPY", "WPAY", false, false, ID3FD_URL,           "Payment"},
  {ID3FID_WWWPUBLISHER,      "WPB", "WPUB", false, false, ID3FD_URL,           "Official publisher webpage"},
  {ID3FID_WWWUSER,           "WXX", "WXXX", false, false, ID3FD_UserURL,       "User defined URL link"},
  {ID3FID_METACRYPTO,        "CRM", ""    , false, false, ID3FD_Unimplemented, "Encrypted meta frame"},
  {ID3FID_METACOMPRESSION,   "CDM", ""    , false, false, ID3FD_CDM,           "Compressed data meta frame"},
  {ID3FID_NOFRAME}
};


static ID3_FrameDef* ID3_FindFrameDef(ID3_FrameID id)
{
	ID3_FrameDef* info = NULL;

	size_t cur;
	for (cur = 0; ID3_FrameDefs[cur].eID != ID3FID_NOFRAME; ++cur) {
		if (ID3_FrameDefs[cur].eID == id) {
			info = &ID3_FrameDefs[cur];
			break;
		}
	}

	return info;
}


//----------------------------------------------------------------------


char* ID3FrameInfo_ShortName(ID3_FrameID frameid)
{
	ID3_FrameDef *pFD = ID3_FindFrameDef(frameid);
	if (pFD!=NULL)
		return pFD->sShortTextID;
	else
		return NULL;
}

char* ID3FrameInfo_LongName(ID3_FrameID frameid)
{
	ID3_FrameDef *pFD = ID3_FindFrameDef(frameid);
	if (pFD!=NULL)
		return pFD->sLongTextID;
	else
		return NULL;
}

const char* ID3FrameInfo_Description(ID3_FrameID frameid)
{
	ID3_FrameDef *pFD = ID3_FindFrameDef(frameid);
	if (pFD!=NULL)
		return pFD->sDescription;
	else
		return NULL;
}

int ID3FrameInfo_MaxFrameID()
{
	return ID3FID_LASTFRAMEID-1;
}

int ID3FrameInfo_NumFields(ID3_FrameID frameid)
{
	int fieldnum = 0;
	ID3_FrameDef *pFD = ID3_FindFrameDef(frameid);
	if (pFD!=NULL) {
		while (pFD->aeFieldDefs[fieldnum]._id != ID3FN_NOFIELD) {
			++fieldnum;
		}
	}
	return fieldnum;
}

ID3_FieldType ID3FrameInfo_FieldType(ID3_FrameID frameid, int fieldnum)
{
	ID3_FrameDef *pFD = ID3_FindFrameDef(frameid);
	if (pFD!=NULL)
		return (pFD->aeFieldDefs[fieldnum]._type);
	else
		return ID3FTY_NONE;
}

size_t ID3FrameInfo_FieldSize(ID3_FrameID frameid, int fieldnum)
{
	ID3_FrameDef *pFD = ID3_FindFrameDef(frameid);
	if (pFD!=NULL)
		return (pFD->aeFieldDefs[fieldnum]._fixed_size);
	else
		return 0;
}

flags_t ID3FrameInfo_FieldFlags (ID3_FrameID frameid, int fieldnum)
{
	ID3_FrameDef *pFD = ID3_FindFrameDef(frameid);
	if (pFD!=NULL)
		return (pFD->aeFieldDefs[fieldnum]._flags);
	else
		return 0;
}


#endif /* LIBID3_MISSING_ID3FRAMEINFO */
