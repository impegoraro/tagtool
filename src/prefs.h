#ifndef PREFS_H
#define PREFS_H

/****************************************************************************
 *	API for setting and getting preference values, and saving them
 *	to file.
 ****************************************************************************/


#define PREF_INVALID	0x0000
#define PREF_INT	0x0001
#define PREF_UINT	0x0002
#define PREF_REAL	0x0003
#define PREF_STRING	0x0004
#define PREF_BOOLEAN	0x0005
#define PREF_TYPE_MASK	0x00FF
#define PREF_LIST	0x0100


/* 
 * Reads preferences from file.
 * Returns TRUE if the operation was successful.
 */
gboolean pref_read_file();


/* 
 * Writes preferences to file.
 * Returns TRUE if the operation was successful.
 */
gboolean pref_write_file();


/*
 * Sets a preference value. The 'value' will be interpreted according to the 
 * 'type' passed to the function:
 *	PREF_INT	long*
 *	PREF_UINT	unsigned long*
 *	PREF_REAL	double*
 *	PREF_STRING	char* (NULL terminated)
 *	PREF_BOOLEAN	int*
 *	PREF_LIST	GEList*
 * PREF_LIST is meant to be combined with one of the other types, to indicate
 * a list of that type.
 * The value will be copied and stored.
 *
 * <name>	Name of the preference variable.
 * <type>	Type of the preference variable.
 * <value>	New value.
 *
 * return	A pointer to the stored value, or NULL if the type is not 
 *		valid.
 */
void *pref_set(const char *name, int type, void *value);


/*
 * Gets a pointer to a preference value. The pointer will be valid until
 * the next time this preference is changed (with pref_set or pref_unset).
 *
 * <name>	Name of the preference variable
 *
 * return	A pointer to the stored value, or NULL if no preference was 
 *		found with that name.
 */
void *pref_get_ref(const char *name);


/*
 * If the requested preference exists, does a pref_get(), otherwise does
 * a pref_set() with the given default value.
 *
 * <name>	Name of the preference variable.
 * <type>	Type of the preference variable.
 * <value>	The default value.
 *
 * return	A pointer to the stored value, or NULL if the type is not 
 *		valid or is inconsitent with stored type.
 */
void *pref_get_or_set(const char *name, int type, void *value);


/*
 * Unsets (removes) a preference value.
 *
 * <name>	Name of the preference variable.
 */
void pref_unset(const char *name);


#endif
