#include <gio/gio.h>
#include <glib-object.h>


#include "mplayer.h"

static char* DBUS_NAME = "org.mpris.MediaPlayer2.amarok";
static char* DBUS_PLAYER_PATH = "/org/mpris/MediaPlayer2";
static char* DBUS_TRACKLIST_PATH = "/TrackList";


GDBusConnection *dbusConn = NULL;
GDBusProxy *playerProxy = NULL;
GDBusProxy *trackProxy = NULL;

void mp_init()
{
	GError *error = NULL;

	// initialize the type sub system

	dbusConn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
	if(error != NULL) {
		g_print("(EE) Could not obtain connection to dbus server, Message: %s\n", error->message);
		g_clear_error(&error);
		return;
	}

	playerProxy = g_dbus_proxy_new_sync(dbusConn, G_DBUS_PROXY_FLAGS_NONE, NULL, 
		DBUS_NAME, DBUS_PLAYER_PATH, "org.mpris.MediaPlayer2.Player", NULL, &error);
	if(error != NULL) {
		g_print("(EE) Could not obtain connection to dbus server, Message: %s\n", error->message);
		g_clear_error(&error);
		return;
	}

	trackProxy = g_dbus_proxy_new_sync(dbusConn, G_DBUS_PROXY_FLAGS_NONE, NULL, 
		DBUS_NAME, DBUS_TRACKLIST_PATH, "org.mpris.MediaPlayer2.TrackList", NULL, &error);
	if(error != NULL) {
		g_print("(EE) Could not obtain connection to dbus server, Message: %s\n", error->message);
		g_clear_error(&error);
		return;
	}
}


void mp_add_track(const gchar* trackPath)
{
	GError *error = NULL;

	g_dbus_proxy_call_sync(trackProxy, "org.freedesktop.MediaPlayer.AddTrack", 
			g_variant_new("(sb)", trackPath, TRUE),  G_DBUS_CALL_FLAGS_NONE,
           -1,
           NULL,
           &error);

	if(error != NULL) {
		g_print("Error: %s\n", error->message);
		g_clear_error(&error);
	}
}

void mp_stop()
{
	GError *error = NULL;

	g_dbus_proxy_call_sync(playerProxy, "org.mpris.MediaPlayer2.Player.PlayPause", 
			NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

	if(error != NULL) {
		g_print("Error: %s\n", error->message);
		g_clear_error(&error);
	}
}