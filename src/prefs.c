#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "elist.h"
#include "file_util.h"
#include "prefs_versions.h"
#include "prefs.h"


#define PREFS_FILE ".tagtoolrc"
#define DTD_FILE   "preferences.dtd"


#define IS_LIST(t)	(t & PREF_LIST)

typedef struct {
	int type;
	void *data;
} pref_entry;

static GHashTable *prefs = NULL;


/*** private functions ******************************************************/

/*
 * Returns the full path name of the preferences file.
 */
static const char *prefs_file_name()
{
	static char *name = NULL;

	if (name == NULL)
		name = fu_join_path(g_get_home_dir(), PREFS_FILE);

	return name;
}

/*
 * Returns the full path name of the preferences DTD file.
 */
static const char *dtd_file_name()
{
	return DATADIR "/" DTD_FILE;
}


/*
 * Returns a type given its string representation.
 */
static int type_from_str(char *name)
{
	int type;

	if (strstr(name, "int") == name) type = PREF_INT;
	else if (strstr(name, "uint") == name) type = PREF_UINT;
	else if (strstr(name, "real") == name) type = PREF_REAL;
	else if (strstr(name, "string") == name) type = PREF_STRING;
	else if (strstr(name, "boolean") == name) type = PREF_BOOLEAN;
	else return PREF_INVALID;

	if (strstr(name, "list") != NULL) type |= PREF_LIST;

	return type;
}

/*
 * Returns the string representation of a type. The returned pointer will 
 * remain valid until the next call to type_to_str().
 */
static const char *type_to_str(int type)
{
	static char name[16];

	switch (PREF_TYPE_MASK & type) {
		case PREF_INT:     strcpy(name, "int"); break;
		case PREF_UINT:    strcpy(name, "uint"); break;
		case PREF_REAL:    strcpy(name, "real"); break;
		case PREF_STRING:  strcpy(name, "string"); break;
		case PREF_BOOLEAN: strcpy(name, "boolean"); break;
		default: return NULL;
	}
	if (IS_LIST(type)) strcpy(&name[strlen(name)], " list");

	return name;
}

/*
 * Returns TRUE if the given type is valid.
 */
static gboolean type_is_valid(int type)
{
	switch (PREF_TYPE_MASK & type) {
		case PREF_INT:
		case PREF_UINT:
		case PREF_REAL:
		case PREF_STRING:
		case PREF_BOOLEAN:
			break;
		default:
			return FALSE;
	}
	switch (~PREF_TYPE_MASK & type) {
	case 0:
		case PREF_LIST:
			break;
		default:
			return FALSE;
	}
	return TRUE;
}


/*
 * Used by copy_value()
 */
static void *copy_single_value(int type, void *data)
{
	void *copy;
	size_t size;

	switch (PREF_TYPE_MASK & type) {
		case PREF_INT:
		case PREF_UINT:
			size = sizeof(long);
			break;
		case PREF_REAL:
			size = sizeof(double);
			break;
		case PREF_STRING:
			size = strlen(data)+1;
			break;
		case PREF_BOOLEAN:
			size = sizeof(int);
			break;
		default: 
			return NULL;
	}

	copy = malloc(size);
	memcpy(copy, data, size);
	return copy;
}

/*
 * Used by copy_value()
 */
static void *copy_list_value(int type, void *data)
{
	GEList *list;
	GList *iter;

	list = g_elist_new();
	for (iter = g_elist_first((GEList*)data) ; iter ; iter = iter->next)
		g_elist_append(list, copy_single_value(type, iter->data));
	return list;
}

/*
 * Allocates a new copy of the given value.
 */
static void *copy_value(int type, void *data)
{
	if (IS_LIST(type))
		return copy_list_value(type, data);
	else
		return copy_single_value(type, data);
}


/*
 * Frees the given value.
 */
static void free_value(int type, void *data)
{
	if (IS_LIST(type))
		g_elist_free_data((GEList*)data);
	else 
		free(data);
}


/*
 * Used by read_value()
 */
static void *read_single_value(xmlDocPtr doc, xmlNodePtr node, int type)
{
	char *str;
	void *ret;

	str = xmlNodeListGetString(doc, node->xmlChildrenNode, 1);

	switch (PREF_TYPE_MASK & type) {
		case PREF_INT:
			ret = malloc(sizeof(long));
			*(long *)ret = strtol(str, NULL, 10);
			free(str);
			break;
		case PREF_UINT:
			ret = malloc(sizeof(long));
			*(unsigned long *)ret = strtoul(str, NULL, 10);
			free(str);
			break;
		case PREF_REAL:
			ret = malloc(sizeof(double));
			*(double *)ret = strtod(str, NULL);
			free(str);
			break;
		case PREF_STRING:
			ret = (str ? str : strdup(""));
			break;
		case PREF_BOOLEAN:
			ret = malloc(sizeof(int));
			if (strcasecmp(str,"false")==0)
				*(int *)ret = 0;
			else if (strcasecmp(str,"true")==0)
				*(int *)ret = 1;
			else
				*(int *)ret = (atoi(str) ? 1 : 0);
			free(str);
			break;
		default:
			ret = NULL;
			free(str);
			break;
	}

	return ret;
}

/*
 * Used by read_value()
 */
static void *read_list_value(xmlDocPtr doc, xmlNodePtr node, int type)
{
	GEList *list;
	xmlNodePtr curr_node;

	list = g_elist_new();
	for (curr_node = node ; curr_node ; curr_node = curr_node->next) {
		if (strcmp(curr_node->name, "value") != 0) {
			g_warning("unexpected element: %s", curr_node->name);
			continue;
		}
		g_elist_append(list, read_single_value(doc, curr_node, type));
	}

	return list;
}

/*
 * Returns a newly allocated value, read from the xml document.
 * <type> is required to determine the expected format of the document, and
 * to allocate the correct amount of memory for the value.
 */
static void *read_value(xmlDocPtr doc, xmlNodePtr node, int type)
{
	if (IS_LIST(type))
		return read_list_value(doc, node, type);
	else
		return read_single_value(doc, node, type);
}


/*
 * Used by write_value()
 */
static void write_single_value(xmlNodePtr node, int type, void *data)
{
	char buf[32];
	char *str;
	gboolean free_str;

	switch (PREF_TYPE_MASK & type) {
		case PREF_INT:
			snprintf(buf, 32, "%li", *(long *)data);
			str = buf;
			free_str = FALSE;
			break;
		case PREF_UINT:
			snprintf(buf, 32, "%lu", *(unsigned long *)data);
			str = buf;
			free_str = FALSE;
			break;
		case PREF_REAL:
			snprintf(buf, 32, "%.10g", *(double *)data);
			str = buf;
			free_str = FALSE;
			break;
		case PREF_STRING:
			str = xmlEncodeEntitiesReentrant(node->doc, data);
			free_str = TRUE;
			break;
		case PREF_BOOLEAN:
			strcpy(buf, (*(int*)data ? "true" : "false"));
			str = buf;
			free_str = FALSE;
			break;
		default: 

			return;
	}

	xmlNewChild(node, NULL, "value", str);

	if (free_str)
		free(str);
}

/*
 * Used by write_value()
 */
static void write_list_value(xmlNodePtr node, int type, void *data)
{
	GList *iter;
	for (iter = g_elist_first((GEList*)data) ; iter ; iter = iter->next)
		write_single_value(node, type, iter->data);
}

/*
 * Writes a value to the xml document.
 * <type> is required to write the correct format to the document.
 */
static void write_value(xmlNodePtr node, int type, void *data)
{
	if (IS_LIST(type))
		write_list_value(node, type, data);
	else
		write_single_value(node, type, data);
}

/*
 * Passed to g_hash_table_foreach(), writes all the entries to file.
 * The parent node is passed in user_data.
 */
static void write_entry(gpointer key, gpointer value, gpointer user_data)
{
	xmlNodePtr node;
	pref_entry *entry;

	entry = (pref_entry *)g_hash_table_lookup(prefs, key);

	node = xmlNewChild((xmlNodePtr)user_data, NULL, "attr", NULL);
	xmlSetProp(node, "name", key);
	xmlSetProp(node, "type", type_to_str(entry->type));
	write_value(node, entry->type, entry->data);
}


/*
 * Passed to g_hash_table_foreach_remove(), frees all the entries.
 */
static gboolean table_free_entry(gpointer key, gpointer value, gpointer user_data)
{
	pref_entry *entry = (pref_entry *)value;
	free_value(entry->type, entry->data);

	free(key);
	free(value);
	
	return TRUE;
}

/* 
 * Frees the prefs table and all the entries it contains.
 */
static void table_free()
{
	g_hash_table_foreach_remove(prefs, table_free_entry, NULL);
	g_hash_table_destroy(prefs);
	prefs = NULL;
}

/* 
 * Allocates a new prefs table.
 */
static void table_new()
{
	if (prefs != NULL)
		table_free();
	prefs = g_hash_table_new(g_str_hash, g_str_equal);
}


/* 
 * Inserts a value into the prefs table.
 * If <copy> is TRUE a new copy of the value is allocated and stored, 
 * otherwise the pointer is stored directly.
 * Returns a pointer to the stored value (if <copy> is FALSE this will 
 * be the same as <data>)
 */
static void *table_insert(const char *name, int type, void *data, gboolean copy)
{
	pref_entry *entry;

	entry = (pref_entry *)g_hash_table_lookup(prefs, name);
	if (!entry) {
		entry = malloc(sizeof(pref_entry));
		g_hash_table_insert(prefs, strdup(name), entry);
	} else {
		free_value(entry->type, entry->data);
	}

	entry->type = type;
	if (copy)
		entry->data = copy_value(type, data);
	else
		entry->data = data;

	return entry->data;
}

/* 
 * Retrieves a value from the prefs table.
 * If <copy> is TRUE a new copy of the value is allocated and returned,
 * otherwise a direct pointer to the stored data is returned.
 * If the lookup is successful the function will return TRUE and the 
 * output parameters <type> and <data> will be set.
 */
static gboolean table_lookup(const char *name, int *type, void **data, gboolean copy)
{
	pref_entry *entry;

	entry = (pref_entry *)g_hash_table_lookup(prefs, name);
	if (!entry)
		return FALSE;

	*type = entry->type;
	if (copy)
		*data = copy_value(entry->type, entry->data);
	else
		*data = entry->data;

	return TRUE;
}


/*
 * Validates the preferences file against the DTD.
 */
static gboolean validate_dtd(xmlDocPtr doc)
{
	xmlDtdPtr dtd;
	xmlValidCtxtPtr cvp;
	gboolean valid;

	dtd = xmlParseDTD(NULL, dtd_file_name());
	if (dtd == NULL) {
		g_warning("Could not parse DTD file \"%s\".\n", dtd_file_name());
		return FALSE;
	}

	cvp = xmlNewValidCtxt();
	if (cvp == NULL) {
		g_warning("Could not allocate validation context.\n");
		xmlFreeDtd(dtd);
		return FALSE;
	}
	cvp->userData = (void *) stderr;
	cvp->error    = (xmlValidityErrorFunc) fprintf;
	cvp->warning  = (xmlValidityWarningFunc) fprintf;

	valid = xmlValidateDtd(cvp, doc, dtd);
	
	xmlFreeValidCtxt(cvp);
	xmlFreeDtd(dtd);
	return valid;
}


/*** public functions *******************************************************/

gboolean pref_read_file()
{
	xmlDocPtr doc;
	xmlNodePtr root, node;
	xmlChar *ver, *name, *type, *str;
	int t;
	void *value;

	/* allocate the prefs hash table */
	table_new();


	xmlKeepBlanksDefault(0);

	doc = xmlParseFile(prefs_file_name());
	if (!doc)
		return FALSE;
	if (!validate_dtd(doc))
		return FALSE;


	root = xmlDocGetRootElement(doc);

	str = xmlGetProp(root, "app");
	if (strcmp(str, PACKAGE_NAME) != 0) {
		free(str);
		return FALSE;
	}
	free(str);

	ver = xmlGetProp(root, "ver");
	if (!prefs_versions_allow(ver)) {
		free(ver);
		return FALSE;
	}

	for (node = root->xmlChildrenNode ; node ; node = node->next) {
		name = xmlGetProp(node, "name");
		type = xmlGetProp(node, "type");

		t = type_from_str(type);
		if (t == PREF_INVALID) {
			g_warning("invalid attr type: %s", type);
			goto _continue;
		}

		value = read_value(doc, node->xmlChildrenNode, t);
		if (!value) {
			g_warning("failed to read attr \"%s\"\n", name);
			goto _continue;
		}

		table_insert(name, t, value, FALSE);

	  _continue:
		free(name);
		free(type);
	}

	xmlFreeDoc(doc);

	prefs_versions_convert(ver);
	free(ver);

	return TRUE;
}


gboolean pref_write_file()
{
	xmlDocPtr doc;
	int res;

	doc = xmlNewDoc("1.0");
	doc->xmlRootNode = xmlNewDocNode(doc, NULL, "preferences", NULL);
	xmlSetProp(doc->xmlRootNode, "app", PACKAGE_NAME);
	xmlSetProp(doc->xmlRootNode, "ver", PACKAGE_VERSION);

	g_hash_table_foreach(prefs, write_entry, doc->xmlRootNode);

	res = xmlSaveFormatFileEnc(prefs_file_name(), doc, "UTF-8", 1);
	if (res <= 0)
		g_warning("Couldn't save prefs to file: %s", prefs_file_name());

	xmlFreeDoc(doc);
	return (res > 0);
}


void *pref_set(const char *name, int type, void *value)
{
	if (type_is_valid(type))
		return table_insert(name, type, value, TRUE);
	else
		return NULL;
}

void *pref_get_or_set(const char *name, int type, void *value)
{
	int temptype;
	void *data;

	if (table_lookup(name, &temptype, &data, FALSE)) {
		if (type == temptype)
			return data;
		else
			return NULL;
	} else {
		if (type_is_valid(type))
			return table_insert(name, type, value, TRUE);
		else
			return NULL;
	}
}

void *pref_get_ref(const char *name)
{
	int type;
	void *data;

	if (table_lookup(name, &type, &data, FALSE))
		return data;
	else
		return NULL;
}


void pref_unset(const char *name)
{
	char *key;
	pref_entry *entry;
	gboolean found;

	found = g_hash_table_lookup_extended(prefs, name, (gpointer*)&key, (gpointer*)&entry);
	if (found) {
		g_hash_table_remove(prefs, name);

		free_value(entry->type, entry->data);
		free(entry);
		free(key);
	}
}


