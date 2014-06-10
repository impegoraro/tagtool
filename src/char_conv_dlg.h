#ifndef CHAR_CONV_DLG_H
#define CHAR_CONV_DLG_H


enum {
	CHCONV_TAG = 0,
	CHCONV_RENAME = 1
};


typedef struct {
	char* space_conv;	// NULL == no conversion
	int   case_conv;
} chconv_tag_options;

typedef struct {
	char* space_conv;	// NULL == no conversion
	char* invalid_conv;	// NULL == omit
	int   case_conv;
} chconv_rename_options;


chconv_tag_options chconv_get_tag_options();

chconv_rename_options chconv_get_rename_options();

void chconv_display(int tab);

void chconv_init(GladeXML *xml);


#endif
