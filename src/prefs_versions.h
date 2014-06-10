#ifndef PREFS_VERSIONS_H
#define PREFS_VERSIONS_H


/*
 * Returns TRUE if the preferences file version is compatible with the 
 * current program version
 */
gboolean prefs_versions_allow(char *file_ver);

/*
 * Tries to convert prefererences from a different version. 
 * Don't call this unless prefs_versions_allow returns TRUE.
 */
void prefs_versions_convert(char *file_ver);


#endif

