#ifndef MESSAGE_BOX_H
#define MESSAGE_BOX_H


/* 
 * Creates a modal message box and waits for the user to close it. 
 * Returns an integer indicating which button was clicked.
 * NOTE: Only one message_box can be visible at any given time.
 *
 * <transient>	If not NULL, the dialog is made transient for this window.
 * <title>	Dialog title.
 * <text>	Dialog message text.
 * <buttons>	Gtk Message dialog buttons.
 * <defbutton>	Default button (or < 0 for none).
 *
 * return	The index of the button that was clicked, or < 0 if the dialog 
 *		was closed by other means.
 */
int message_box(GtkWindow *transientfor, char *title, char* text, 
	GtkButtonsType buttons, int defbutton);


#endif
