#include <gtk/gtk.h>
#include <gio/gio.h>
#include <glib-object.h>
#include <glib/gi18n.h>

#include "mplayer.h"

/* Widgets definitions */
static GtkButton* btn_play_pause = NULL;
static GtkButton* btn_previous = NULL;
static GtkButton* btn_next = NULL;
static GtkLabel* lbl_current_song = NULL;
static gboolean player_status;
#define DBUS_NAME "org.mpris.MediaPlayer2.amarok"
#define DBUS_PLAYER_PATH "/Player"
#define DBUS_PLAYER_IFACE "org.freedesktop.MediaPlayer"
#define DBUS_TRACKLIST_PATH "/TrackList"

GDBusConnection *dbusConn = NULL;
GDBusProxy *playerProxy = NULL;
GDBusProxy *trackProxy = NULL;

typedef enum  {
	PLAYER_STATUS_PLAYING,
	PLAYER_STATUS_PAUSED
} PlayerStatus;


// private functions
static void mp_status_changed(GDBusConnection *conn, const gchar *sender_name, const gchar *object_path, const gchar *interface_name, const gchar *signal_name,
                        GVariant *param, gpointer user_data);

static void mp_track_changed_changed(GDBusConnection *conn, const gchar *sender_name, const gchar *object_path, const gchar *interface_name, const gchar *signal_name,
                        GVariant *param, gpointer user_data);

#define MP_PARSE_GET_STATUS(resp) { \
	if(g_variant_is_container(resp) && g_variant_n_children(resp) >= 1) { \
			GVariant* array = g_variant_get_child_value(resp, 0); \
			gint32 playStat = g_variant_get_int32(g_variant_get_child_value(array, 0)); \
			player_status = playStat == 1;\
			switch(playStat) {\
				case 0:\
					mp_btn_player_change(PLAYER_STATUS_PAUSED);\
				break;\
				case 1:\
					mp_btn_player_change(PLAYER_STATUS_PLAYING);\
				break;\
				case 2:\
					mp_btn_player_change(PLAYER_STATUS_PLAYING);\
					gtk_label_set_text(lbl_current_song, N_("No music is currently playing"));\
				break;\
			}\
		}\
	}

#define MP_PARSE_METADATA(metadata) {\
	const gchar *songArtist = NULL;\
	const gchar *songName = NULL;\
	if(g_variant_is_container(metadata) && g_variant_n_children(metadata) >= 1) {\
		GVariant *dict = g_variant_get_child_value(metadata, 0);\
		int i;\
		for(i = 0; i < g_variant_n_children(dict); i++) {\
			GVariant *tmp = g_variant_get_child_value(dict, i);\
			gsize strSize;\
			const gchar *strKey = g_variant_get_string(g_variant_get_child_value(tmp, 0), &strSize);\
			if(g_variant_n_children(tmp) >= 2 && !g_strcmp0(strKey, "title")) {\
				GVariant *vsong = g_variant_get_child_value(tmp, 1);\
				songName = g_variant_get_string(g_variant_get_variant(vsong), &strSize);\
			} else if(g_variant_n_children(tmp) >= 2 && !g_strcmp0(strKey, "artist")) {\
				GVariant *vsong = g_variant_get_child_value(tmp, 1);\
				songArtist = g_variant_get_string(g_variant_get_variant(vsong), &strSize);\
			}\
		}\
	}\
	if(songName != NULL && songArtist != NULL) {\
		GString *str = g_string_new("");\
		g_string_printf(str, "%s - %s", (!g_strcmp0(songArtist, "") ? N_("Unknown artists") : songArtist), (!g_strcmp0(songName, "") ? N_("Unknown song") : songName));\
		gtk_label_set_text(lbl_current_song, str->str);\
		g_string_free(str, TRUE);\
	}\
	else {\
		if(player_status)\
			gtk_label_set_text(lbl_current_song, N_("No music is currently playing"));\
		else {\
			GString *str = g_string_new("");\
			g_string_printf(str, "%s - %s", (!g_strcmp0(songArtist, "") ? N_("Unknown artists") : songArtist), (!g_strcmp0(songName, "") ? N_("Unknown song") : songName));\
			gtk_label_set_text(lbl_current_song, str->str);\
			g_string_free(str, TRUE);\
		}\
	}\
	}

static void mp_btn_player_change(PlayerStatus stat)
{
	GtkImage *btnimg;
	if((btnimg = GTK_IMAGE(gtk_button_get_image(btn_play_pause)))) {

		switch(stat) {
			case PLAYER_STATUS_PLAYING:
				gtk_button_set_label(btn_play_pause, "_Play");
				gtk_image_set_from_icon_name(btnimg, "media-playback-start", GTK_ICON_SIZE_BUTTON);
			break;
			case PLAYER_STATUS_PAUSED:
				gtk_button_set_label(btn_play_pause, "P_ause");
				gtk_image_set_from_icon_name(btnimg, "media-playback-pause", GTK_ICON_SIZE_BUTTON);
			break;
		}
	}
}

static void mp_btn_play_pause_clicked(GtkWidget* btn, gpointer user_data)
{
	mp_stop();
}

static void mp_btn_previous_clicked(GtkWidget* btn, gpointer user_data)
{
	GError *error = NULL;

	g_dbus_proxy_call_sync(playerProxy, DBUS_PLAYER_IFACE".Prev", 
			NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

	if(error != NULL) {
		g_print("Error: %s\n", error->message);
		g_clear_error(&error);
	}
}

static void mp_btn_next_clicked(GtkWidget* btn, gpointer user_data)
{
	GError *error = NULL;

	g_dbus_proxy_call_sync(playerProxy, DBUS_PLAYER_IFACE".Next", 
			NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

	if(error != NULL) {
		g_print("Error: %s\n", error->message);
		g_clear_error(&error);
	}
}

static void mp_get_status()
{
	GError *error = NULL;

	GVariant *resp = g_dbus_proxy_call_sync(playerProxy, DBUS_PLAYER_IFACE".GetStatus", 
			NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

	if(error != NULL) {
		g_print("Error: %s\n", error->message);
		g_clear_error(&error);
	} else {
		MP_PARSE_GET_STATUS(resp)
	}
}

static void mp_get_metadata()
{
	GError *error = NULL;

	GVariant *resp = g_dbus_proxy_call_sync(playerProxy, DBUS_PLAYER_IFACE".GetMetadata", 
			NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

	if(error != NULL) {
		g_print("Error: %s\n", error->message);
		g_clear_error(&error);
	} else {
		MP_PARSE_METADATA(resp)
	}
}

void mp_init(GtkBuilder *builder)
{
	GError *error = NULL;

	btn_play_pause = GTK_BUTTON(gtk_builder_get_object(builder, "btn_play_pause"));
	btn_previous = GTK_BUTTON(gtk_builder_get_object(builder, "btn_previous"));
	btn_next = GTK_BUTTON(gtk_builder_get_object(builder, "btn_next"));
	lbl_current_song = GTK_LABEL(gtk_builder_get_object(builder, "lbl_current_song"));

	// initialize the type sub system

	dbusConn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
	if(error != NULL) {
		g_print(N_("(EE) Could not obtain connection to dbus server, Message: %s\n"), error->message);
		g_clear_error(&error);
		return;
	}

	playerProxy = g_dbus_proxy_new_sync(dbusConn, G_DBUS_PROXY_FLAGS_NONE, NULL, 
		DBUS_NAME, DBUS_PLAYER_PATH, DBUS_PLAYER_IFACE, NULL, &error);
	if(error != NULL) {
		g_print(N_("(EE) Could not obtain connection to dbus server, Message: %s\n"), error->message);
		g_clear_error(&error);
		return;
	}

	trackProxy = g_dbus_proxy_new_sync(dbusConn, G_DBUS_PROXY_FLAGS_NONE, NULL, 
		DBUS_NAME, DBUS_TRACKLIST_PATH, "org.mpris.MediaPlayer2.TrackList", NULL, &error);
	if(error != NULL) {
		g_print(N_("(EE) Could not obtain connection to dbus server, Message: %s\n"), error->message);
		g_clear_error(&error);
		return;
	}

	g_signal_connect(G_OBJECT(btn_play_pause), "clicked", G_CALLBACK(mp_btn_play_pause_clicked), NULL);
	g_signal_connect(G_OBJECT(btn_previous), "clicked", G_CALLBACK(mp_btn_previous_clicked), NULL);
	g_signal_connect(G_OBJECT(btn_next), "clicked", G_CALLBACK(mp_btn_next_clicked), NULL);
	g_dbus_connection_signal_subscribe(dbusConn, DBUS_NAME, DBUS_PLAYER_IFACE, "StatusChange", DBUS_PLAYER_PATH, NULL, 0, mp_status_changed, NULL, NULL);
	g_dbus_connection_signal_subscribe(dbusConn, DBUS_NAME, DBUS_PLAYER_IFACE, "TrackChange", DBUS_PLAYER_PATH, NULL, 0, mp_track_changed_changed, NULL, NULL);
	mp_get_status();
	mp_get_metadata();
}

void mp_status_changed(GDBusConnection *conn, const gchar *sender_name, const gchar *object_path, const gchar *interface_name, const gchar *signal_name,
						GVariant *param, gpointer user_data)
{
	MP_PARSE_GET_STATUS(param)
}

void mp_track_changed_changed(GDBusConnection *conn, const gchar *sender_name, const gchar *object_path, const gchar *interface_name, const gchar *signal_name,
						GVariant *param, gpointer user_data)
{
	MP_PARSE_METADATA(param)
}

void mp_add_track(const gchar* trackPath)
{
	GError *error = NULL;

	g_dbus_proxy_call_sync(trackProxy, DBUS_PLAYER_IFACE".AddTrack", 
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

	g_dbus_proxy_call_sync(playerProxy, DBUS_PLAYER_IFACE".PlayPause", 
			NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

	if(error != NULL) {
		g_print("Error: %s\n", error->message);
		g_clear_error(&error);
	}
}