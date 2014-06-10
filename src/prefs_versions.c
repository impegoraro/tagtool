#include <config.h>

#include <string.h>
#include <stdlib.h>
#include <glib.h>

#include "elist.h"
#include "prefs.h"
#include "prefs_versions.h"


/*** private functions ******************************************************/

static int compare_version(char *v1, char *v2)
{
	char *p1, *p2;
	int n1, n2;

	p1 = v1;
	p2 = v2;

	while (*p1 && *p2) {
		n1 = atoi(p1);
		n2 = atoi(p2);

		if (n1 < n2) return -1;
		if (n1 > n2) return 1;

		p1 = strchr(p1, '.');
		p2 = strchr(p2, '.');

		if (!p1 && p2) return -1;
		if (p1 && !p2) return 1;
		if (!p1 && !p2) return 0;

		p1++;
		p2++;
	}

	return 0;
}


/*** public functions *******************************************************/

gboolean prefs_versions_allow(char *file_ver)
{
	/* allow interval from 0.5 to current version */
	if (compare_version(file_ver, "0.5") >= 0 && compare_version(file_ver, PACKAGE_VERSION) <= 0)
		return TRUE;

	g_warning("Preferences file version is incompatible (%s)", file_ver);
	return FALSE;
}


void prefs_versions_convert(char *file_ver)
{
	gboolean conv = FALSE;

	/* changes in 0.6 */
	if (compare_version(file_ver, "0.6") < 0) {

		/* file name templates changed, delete old ones */
		pref_unset("tt:format_mru");
		pref_unset("rt:format_mru");

		conv = TRUE;
	}

	/* changes in 0.9 */
	if (compare_version(file_ver, "0.9") < 0) {

		/* "vorbis_edit:genre_mru" was renamed to "common_edit:genre_mru" */
		GEList *genre_list = pref_get_ref("vorbis_edit:genre_mru");
		if (genre_list) {
			pref_set("common_edit:genre_mru", PREF_STRING | PREF_LIST, genre_list);
			pref_unset("vorbis_edit:genre_mru");
		}
		
		conv = TRUE;
	}

	if (conv)
		g_warning("Converted old preferences file (%s)", file_ver);
}


