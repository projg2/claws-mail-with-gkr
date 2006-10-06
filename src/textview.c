/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2006 Hiroyuki Yamamoto and the Sylpheed-Claws team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "defs.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtksignal.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#if HAVE_LIBCOMPFACE
#  include <compface.h>
#endif

#if HAVE_LIBCOMPFACE
#define XPM_XFACE_HEIGHT	(HEIGHT + 3)  /* 3 = 1 header + 2 colors */
#endif

#include "main.h"
#include "summaryview.h"
#include "procheader.h"
#include "prefs_common.h"
#include "codeconv.h"
#include "utils.h"
#include "gtkutils.h"
#include "procmime.h"
#include "html.h"
#include "enriched.h"
#include "compose.h"
#include "addressbook.h"
#include "displayheader.h"
#include "account.h"
#include "mimeview.h"
#include "alertpanel.h"
#include "menu.h"
#include "image_viewer.h"
#include "filesel.h"
#include "base64.h"
#include "inputdialog.h"
#include "timing.h"

struct _ClickableText
{
	gchar *uri;

	gchar *filename;

	gpointer data;

	guint start;
	guint end;
	
	gboolean is_quote;
	gint quote_level;
	gboolean q_expanded;
	gchar *fg_color;
};

gint previousquotelevel = -1;

static GdkColor quote_colors[3] = {
	{(gulong)0, (gushort)0, (gushort)0, (gushort)0},
	{(gulong)0, (gushort)0, (gushort)0, (gushort)0},
	{(gulong)0, (gushort)0, (gushort)0, (gushort)0}
};

static GdkColor quote_bgcolors[3] = {
	{(gulong)0, (gushort)0, (gushort)0, (gushort)0},
	{(gulong)0, (gushort)0, (gushort)0, (gushort)0},
	{(gulong)0, (gushort)0, (gushort)0, (gushort)0}
};
static GdkColor signature_color = {
	(gulong)0,
	(gushort)0x7fff,
	(gushort)0x7fff,
	(gushort)0x7fff
};
	
static GdkColor uri_color = {
	(gulong)0,
	(gushort)0,
	(gushort)0,
	(gushort)0
};

static GdkColor emphasis_color = {
	(gulong)0,
	(gushort)0,
	(gushort)0,
	(gushort)0xcfff
};

static GdkCursor *hand_cursor = NULL;
static GdkCursor *text_cursor = NULL;
static GdkCursor *watch_cursor= NULL;

#define TEXTVIEW_STATUSBAR_PUSH(textview, str)				    \
{	if (textview->messageview->statusbar)				    \
	gtk_statusbar_push(GTK_STATUSBAR(textview->messageview->statusbar), \
			   textview->messageview->statusbar_cid, str);	    \
}

#define TEXTVIEW_STATUSBAR_POP(textview)				   \
{	if (textview->messageview->statusbar)				   \
	gtk_statusbar_pop(GTK_STATUSBAR(textview->messageview->statusbar), \
			  textview->messageview->statusbar_cid);	   \
}

static void textview_show_ertf		(TextView	*textview,
					 FILE		*fp,
					 CodeConverter	*conv);
static void textview_add_part		(TextView	*textview,
					 MimeInfo	*mimeinfo);
static void textview_add_parts		(TextView	*textview,
					 MimeInfo	*mimeinfo);
static void textview_write_body		(TextView	*textview,
					 MimeInfo	*mimeinfo);
static void textview_show_html		(TextView	*textview,
					 FILE		*fp,
					 CodeConverter	*conv);

static void textview_write_line		(TextView	*textview,
					 const gchar	*str,
					 CodeConverter	*conv,
					 gboolean	 do_quote_folding);
static void textview_write_link		(TextView	*textview,
					 const gchar	*str,
					 const gchar	*uri,
					 CodeConverter	*conv);

static GPtrArray *textview_scan_header	(TextView	*textview,
					 FILE		*fp);
static void textview_show_header	(TextView	*textview,
					 GPtrArray	*headers);

static gint textview_key_pressed		(GtkWidget	*widget,
						 GdkEventKey	*event,
						 TextView	*textview);
static gboolean textview_motion_notify		(GtkWidget	*widget,
						 GdkEventMotion	*motion,
						 TextView	*textview);
static gboolean textview_leave_notify		(GtkWidget	  *widget,
						 GdkEventCrossing *event,
						 TextView	  *textview);
static gboolean textview_visibility_notify	(GtkWidget	*widget,
						 GdkEventVisibility *event,
						 TextView	*textview);
static void textview_uri_update			(TextView	*textview,
						 gint		x,
						 gint		y);
static gboolean textview_get_uri_range		(TextView	*textview,
						 GtkTextIter	*iter,
						 GtkTextTag	*tag,
						 GtkTextIter	*start_iter,
						 GtkTextIter	*end_iter);
static ClickableText *textview_get_uri_from_range	(TextView	*textview,
						 GtkTextIter	*iter,
						 GtkTextTag	*tag,
						 GtkTextIter	*start_iter,
						 GtkTextIter	*end_iter);
static ClickableText *textview_get_uri		(TextView	*textview,
						 GtkTextIter	*iter,
						 GtkTextTag	*tag);
static gboolean textview_uri_button_pressed	(GtkTextTag 	*tag,
						 GObject 	*obj,
						 GdkEvent 	*event,
						 GtkTextIter	*iter,
						 TextView 	*textview);

static gboolean textview_uri_security_check	(TextView	*textview,
						 ClickableText	*uri);
static void textview_uri_list_remove_all	(GSList		*uri_list);

static void textview_toggle_quote		(TextView 	*textview, 
						 ClickableText 	*uri,
						 gboolean	 expand_only);

static void open_uri_cb				(TextView 	*textview,
						 guint		 action,
						 void		*data);
static void copy_uri_cb				(TextView 	*textview,
						 guint		 action,
						 void		*data);
static void add_uri_to_addrbook_cb 		(TextView 	*textview, 
						 guint 		 action, 
						 void 		*data);
static void mail_to_uri_cb 			(TextView 	*textview, 
						 guint 		 action, 
						 void 		*data);
static void copy_mail_to_uri_cb			(TextView 	*textview,
						 guint		 action,
						 void		*data);
static void save_file_cb			(TextView 	*textview,
						 guint		 action,
						 void		*data);
static void open_image_cb			(TextView 	*textview,
						 guint		 action,
						 void		*data);
static void textview_show_icon(TextView *textview, const gchar *stock_id);

static GtkItemFactoryEntry textview_link_popup_entries[] = 
{
	{N_("/_Open with Web browser"),	NULL, open_uri_cb, 0, NULL},
	{N_("/Copy this _link"),	NULL, copy_uri_cb, 0, NULL},
};

static GtkItemFactoryEntry textview_mail_popup_entries[] = 
{
	{N_("/Compose _new message"),	NULL, mail_to_uri_cb, 0, NULL},
	{N_("/Add to _address book"),	NULL, add_uri_to_addrbook_cb, 0, NULL},
	{N_("/Copy this add_ress"),	NULL, copy_mail_to_uri_cb, 0, NULL},
};

static GtkItemFactoryEntry textview_file_popup_entries[] = 
{
	{N_("/_Open image"),		NULL, open_image_cb, 0, NULL},
	{N_("/_Save image..."),		NULL, save_file_cb, 0, NULL},
};

static void scrolled_cb (GtkAdjustment *adj, TextView *textview)
{
#ifndef WIDTH
#  define WIDTH 48
#  define HEIGHT 48
#endif
	if (textview->image) {
		gint x, y, x1;
		x1 = textview->text->allocation.width - WIDTH - 5;
		gtk_text_view_buffer_to_window_coords(
			GTK_TEXT_VIEW(textview->text),
			GTK_TEXT_WINDOW_TEXT, x1, 5, &x, &y);
		gtk_text_view_move_child(GTK_TEXT_VIEW(textview->text), 
			textview->image, x1, y);
	}
}

static void textview_size_allocate_cb	(GtkWidget	*widget,
					 GtkAllocation	*allocation,
					 gpointer	 data)
{
	scrolled_cb(NULL, (TextView *)data);
}

TextView *textview_create(void)
{
	TextView *textview;
	GtkWidget *vbox;
	GtkWidget *scrolledwin;
	GtkWidget *text;
	GtkTextBuffer *buffer;
	GtkClipboard *clipboard;
	GtkItemFactory *link_popupfactory, *mail_popupfactory, *file_popupfactory;
	GtkWidget *link_popupmenu, *mail_popupmenu, *file_popupmenu;
	GtkAdjustment *adj;
	gint n_entries;

	debug_print("Creating text view...\n");
	textview = g_new0(TextView, 1);

	scrolledwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwin),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledwin),
					    GTK_SHADOW_IN);
	gtk_widget_set_size_request
		(scrolledwin, prefs_common.mainview_width, -1);

	/* create GtkSText widgets for single-byte and multi-byte character */
	text = gtk_text_view_new();
	gtk_widget_add_events(text, GDK_LEAVE_NOTIFY_MASK);
	gtk_widget_show(text);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text), GTK_WRAP_WORD_CHAR);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text), FALSE);
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(text), 6);
	gtk_text_view_set_right_margin(GTK_TEXT_VIEW(text), 6);

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
	clipboard = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
	gtk_text_buffer_add_selection_clipboard(buffer, clipboard);

	gtk_widget_ensure_style(text);

	gtk_widget_ref(scrolledwin);

	gtk_container_add(GTK_CONTAINER(scrolledwin), text);

	g_signal_connect(G_OBJECT(text), "key-press-event",
			 G_CALLBACK(textview_key_pressed), textview);
	g_signal_connect(G_OBJECT(text), "motion-notify-event",
			 G_CALLBACK(textview_motion_notify), textview);
	g_signal_connect(G_OBJECT(text), "leave-notify-event",
			 G_CALLBACK(textview_leave_notify), textview);
	g_signal_connect(G_OBJECT(text), "visibility-notify-event",
			 G_CALLBACK(textview_visibility_notify), textview);
	adj = gtk_scrolled_window_get_vadjustment(
		GTK_SCROLLED_WINDOW(scrolledwin));
	g_signal_connect(G_OBJECT(adj), "value-changed",
			 G_CALLBACK(scrolled_cb), textview);
	g_signal_connect(G_OBJECT(text), "size_allocate",
			 G_CALLBACK(textview_size_allocate_cb),
			 textview);


	gtk_widget_show(scrolledwin);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), scrolledwin, TRUE, TRUE, 0);

	gtk_widget_show(vbox);

	n_entries = sizeof(textview_link_popup_entries) /
		sizeof(textview_link_popup_entries[0]);
	link_popupmenu = menu_create_items(textview_link_popup_entries, n_entries,
				      "<UriPopupMenu>", &link_popupfactory,
				      textview);

	n_entries = sizeof(textview_mail_popup_entries) /
		sizeof(textview_mail_popup_entries[0]);
	mail_popupmenu = menu_create_items(textview_mail_popup_entries, n_entries,
				      "<UriPopupMenu>", &mail_popupfactory,
				      textview);

	n_entries = sizeof(textview_file_popup_entries) /
		sizeof(textview_file_popup_entries[0]);
	file_popupmenu = menu_create_items(textview_file_popup_entries, n_entries,
				      "<FilePopupMenu>", &file_popupfactory,
				      textview);

	textview->vbox               = vbox;
	textview->scrolledwin        = scrolledwin;
	textview->text               = text;
	textview->uri_list           = NULL;
	textview->body_pos           = 0;
	textview->show_all_headers   = FALSE;
	textview->last_buttonpress   = GDK_NOTHING;
	textview->link_popup_menu    = link_popupmenu;
	textview->link_popup_factory = link_popupfactory;
	textview->mail_popup_menu    = mail_popupmenu;
	textview->mail_popup_factory = mail_popupfactory;
	textview->file_popup_menu    = file_popupmenu;
	textview->file_popup_factory = file_popupfactory;
	textview->image		     = NULL;
	return textview;
}

static void textview_create_tags(GtkTextView *text, TextView *textview)
{
	GtkTextBuffer *buffer;
	GtkTextTag *tag, *qtag;
	static PangoFontDescription *font_desc, *bold_font_desc;
	
	if (!font_desc)
		font_desc = pango_font_description_from_string
			(NORMAL_FONT);

	if (!bold_font_desc) {
		bold_font_desc = pango_font_description_from_string
			(NORMAL_FONT);
		pango_font_description_set_weight
			(bold_font_desc, PANGO_WEIGHT_BOLD);
	}

	buffer = gtk_text_view_get_buffer(text);

	gtk_text_buffer_create_tag(buffer, "header",
				   "pixels-above-lines", 0,
				   "pixels-above-lines-set", TRUE,
				   "pixels-below-lines", 0,
				   "pixels-below-lines-set", TRUE,
				   "font-desc", font_desc,
				   "left-margin", 3,
				   "left-margin-set", TRUE,
				   NULL);
	gtk_text_buffer_create_tag(buffer, "header_title",
				   "font-desc", bold_font_desc,
				   NULL);
	tag = gtk_text_buffer_create_tag(buffer, "hlink",
				   "pixels-above-lines", 0,
				   "pixels-above-lines-set", TRUE,
				   "pixels-below-lines", 0,
				   "pixels-below-lines-set", TRUE,
				   "font-desc", font_desc,
				   "left-margin", 3,
				   "left-margin-set", TRUE,
				   "foreground-gdk", &uri_color,
				   NULL);
	g_signal_connect(G_OBJECT(tag), "event",
                         G_CALLBACK(textview_uri_button_pressed), textview);
	if (prefs_common.enable_bgcolor) {
		gtk_text_buffer_create_tag(buffer, "quote0",
				"foreground-gdk", &quote_colors[0],
				"paragraph-background-gdk", &quote_bgcolors[0],
				NULL);
		gtk_text_buffer_create_tag(buffer, "quote1",
				"foreground-gdk", &quote_colors[1],
				"paragraph-background-gdk", &quote_bgcolors[1],
				NULL);
		gtk_text_buffer_create_tag(buffer, "quote2",
				"foreground-gdk", &quote_colors[2],
				"paragraph-background-gdk", &quote_bgcolors[2],
				NULL);
	} else {
		gtk_text_buffer_create_tag(buffer, "quote0",
				"foreground-gdk", &quote_colors[0],
				NULL);
		gtk_text_buffer_create_tag(buffer, "quote1",
				"foreground-gdk", &quote_colors[1],
				NULL);
		gtk_text_buffer_create_tag(buffer, "quote2",
				"foreground-gdk", &quote_colors[2],
				NULL);
	}
	gtk_text_buffer_create_tag(buffer, "emphasis",
			"foreground-gdk", &emphasis_color,
			NULL);
	gtk_text_buffer_create_tag(buffer, "signature",
			"foreground-gdk", &signature_color,
			NULL);
	tag = gtk_text_buffer_create_tag(buffer, "link",
			"foreground-gdk", &uri_color,
			NULL);
	qtag = gtk_text_buffer_create_tag(buffer, "qlink",
			NULL);
	gtk_text_buffer_create_tag(buffer, "link-hover",
			"underline", PANGO_UNDERLINE_SINGLE,
			NULL);
	g_signal_connect(G_OBJECT(qtag), "event",
                         G_CALLBACK(textview_uri_button_pressed), textview);
	g_signal_connect(G_OBJECT(tag), "event",
                         G_CALLBACK(textview_uri_button_pressed), textview);
 }

void textview_init(TextView *textview)
{
	if (!hand_cursor)
		hand_cursor = gdk_cursor_new(GDK_HAND2);
	if (!text_cursor)
		text_cursor = gdk_cursor_new(GDK_XTERM);
	if (!watch_cursor)
		watch_cursor = gdk_cursor_new(GDK_WATCH);

	textview_reflect_prefs(textview);
	textview_set_all_headers(textview, FALSE);
	textview_set_font(textview, NULL);
	textview_create_tags(GTK_TEXT_VIEW(textview->text), textview);
}

#if GTK_CHECK_VERSION(2, 8, 0)
 #define CHANGE_TAG_COLOR(tagname, colorfg, colorbg) { \
	tag = gtk_text_tag_table_lookup(tags, tagname); \
	if (tag) \
		g_object_set(G_OBJECT(tag), "foreground-gdk", colorfg, "paragraph-background-gdk", colorbg, NULL); \
 }
#else
 #define CHANGE_TAG_COLOR(tagname, colorfg, colorbg) { \
	tag = gtk_text_tag_table_lookup(tags, tagname); \
	if (tag) \
		g_object_set(G_OBJECT(tag), "foreground-gdk", colorfg, NULL); \
 }
#endif

static void textview_update_message_colors(TextView *textview)
{
	GdkColor black = {0, 0, 0, 0};
	GdkColor colored_emphasis = {0, 0, 0, 0xcfff};
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview->text));

	GtkTextTagTable *tags = gtk_text_buffer_get_tag_table(buffer);
	GtkTextTag *tag = NULL;

	quote_bgcolors[0] = quote_bgcolors[1] = quote_bgcolors[2] = black;
	quote_colors[0] = quote_colors[1] = quote_colors[2] = 
		uri_color = emphasis_color = signature_color = black;

	if (prefs_common.enable_color) {
		/* grab the quote colors, converting from an int to a GdkColor */
		gtkut_convert_int_to_gdk_color(prefs_common.quote_level1_col,
					       &quote_colors[0]);
		gtkut_convert_int_to_gdk_color(prefs_common.quote_level2_col,
					       &quote_colors[1]);
		gtkut_convert_int_to_gdk_color(prefs_common.quote_level3_col,
					       &quote_colors[2]);
		gtkut_convert_int_to_gdk_color(prefs_common.uri_col,
					       &uri_color);
		gtkut_convert_int_to_gdk_color(prefs_common.signature_col,
					       &signature_color);
		emphasis_color = colored_emphasis;
	}
	if (prefs_common.enable_color && prefs_common.enable_bgcolor) {
		gtkut_convert_int_to_gdk_color(prefs_common.quote_level1_bgcol,
						   &quote_bgcolors[0]);
		gtkut_convert_int_to_gdk_color(prefs_common.quote_level2_bgcol,
						   &quote_bgcolors[1]);
		gtkut_convert_int_to_gdk_color(prefs_common.quote_level3_bgcol,
						   &quote_bgcolors[2]);
		CHANGE_TAG_COLOR("quote0", &quote_colors[0], &quote_bgcolors[0]);
		CHANGE_TAG_COLOR("quote1", &quote_colors[1], &quote_bgcolors[1]);
		CHANGE_TAG_COLOR("quote2", &quote_colors[2], &quote_bgcolors[2]);
	} else {
		CHANGE_TAG_COLOR("quote0", &quote_colors[0], NULL);
		CHANGE_TAG_COLOR("quote1", &quote_colors[1], NULL);
		CHANGE_TAG_COLOR("quote2", &quote_colors[2], NULL);
	}

	CHANGE_TAG_COLOR("emphasis", &emphasis_color, NULL);
	CHANGE_TAG_COLOR("signature", &signature_color, NULL);
	CHANGE_TAG_COLOR("link", &uri_color, NULL);
	CHANGE_TAG_COLOR("link-hover", &uri_color, NULL);
}
#undef CHANGE_TAG_COLOR

void textview_reflect_prefs(TextView *textview)
{
	textview_set_font(textview, NULL);
	textview_update_message_colors(textview);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(textview->text),
					 prefs_common.textview_cursor_visible);
}

void textview_show_message(TextView *textview, MimeInfo *mimeinfo,
			   const gchar *file)
{
	textview->loading = TRUE;
	textview->stop_loading = FALSE;

	textview_clear(textview);

	textview_add_parts(textview, mimeinfo);

	textview_set_position(textview, 0);

	textview->loading = FALSE;
	textview->stop_loading = FALSE;
}

void textview_show_part(TextView *textview, MimeInfo *mimeinfo, FILE *fp)
{
	START_TIMING("");
	g_return_if_fail(mimeinfo != NULL);
	g_return_if_fail(fp != NULL);

	if ((mimeinfo->type == MIMETYPE_MULTIPART) ||
	    ((mimeinfo->type == MIMETYPE_MESSAGE) && !g_ascii_strcasecmp(mimeinfo->subtype, "rfc822"))) {
		textview->loading = TRUE;
		textview->stop_loading = FALSE;
		
		textview_clear(textview);
		textview_add_parts(textview, mimeinfo);

		textview->loading = FALSE;
		textview->stop_loading = FALSE;
		END_TIMING();
		return;
	}
	textview->loading = TRUE;
	textview->stop_loading = FALSE;

	if (fseek(fp, mimeinfo->offset, SEEK_SET) < 0)
		perror("fseek");

	textview_clear(textview);

	if (mimeinfo->type == MIMETYPE_MULTIPART)
		textview_add_parts(textview, mimeinfo);
	else
		textview_write_body(textview, mimeinfo);

	textview->loading = FALSE;
	textview->stop_loading = FALSE;
	END_TIMING();
}

#define TEXT_INSERT(str) \
	gtk_text_buffer_insert_with_tags_by_name \
				(buffer, &iter, str, -1,\
				 "header", NULL)

#define TEXT_INSERT_LINK(str, fname, udata) {				\
	ClickableText *uri;							\
	uri = g_new0(ClickableText, 1);					\
	uri->uri = g_strdup("");					\
	uri->start = gtk_text_iter_get_offset(&iter);			\
	gtk_text_buffer_insert_with_tags_by_name 			\
				(buffer, &iter, str, -1,		\
				 "link", "header_title", "header", 	\
				 NULL); 				\
	uri->end = gtk_text_iter_get_offset(&iter);			\
	uri->filename = fname?g_strdup(fname):NULL;			\
	uri->data = udata;						\
	textview->uri_list =						\
		g_slist_append(textview->uri_list, uri);		\
}

static void textview_add_part(TextView *textview, MimeInfo *mimeinfo)
{
	GtkTextView *text;
	GtkTextBuffer *buffer;
	GtkTextIter iter, start_iter;
	gchar buf[BUFFSIZE];
	GPtrArray *headers = NULL;
	const gchar *name;
	gchar *content_type;
	gint charcount;
	START_TIMING("");

	g_return_if_fail(mimeinfo != NULL);
	text = GTK_TEXT_VIEW(textview->text);
	buffer = gtk_text_view_get_buffer(text);
	charcount = gtk_text_buffer_get_char_count(buffer);
	gtk_text_buffer_get_end_iter(buffer, &iter);

	if (textview->stop_loading) {
		return;
	}
	if (mimeinfo->type == MIMETYPE_MULTIPART) {
		END_TIMING();
		return;
	}

	if ((mimeinfo->type == MIMETYPE_MESSAGE) && !g_ascii_strcasecmp(mimeinfo->subtype, "rfc822")) {
		FILE *fp;

		fp = g_fopen(mimeinfo->data.filename, "rb");
		fseek(fp, mimeinfo->offset, SEEK_SET);
		headers = textview_scan_header(textview, fp);
		if (headers) {
			if (charcount > 0)
				gtk_text_buffer_insert(buffer, &iter, "\n", 1);
			textview_show_header(textview, headers);
			procheader_header_array_destroy(headers);
		}
		fclose(fp);
		END_TIMING();
		return;
	}

	name = procmime_mimeinfo_get_parameter(mimeinfo, "filename");
	content_type = procmime_get_content_type_str(mimeinfo->type,
						     mimeinfo->subtype);
	if (name == NULL)
		name = procmime_mimeinfo_get_parameter(mimeinfo, "name");
	if (name != NULL)
		g_snprintf(buf, sizeof(buf), _("[%s  %s (%d bytes)]"),
			   name, content_type, mimeinfo->length);
	else
		g_snprintf(buf, sizeof(buf), _("[%s (%d bytes)]"),
			   content_type, mimeinfo->length);

	g_free(content_type);			   

	if (mimeinfo->disposition == DISPOSITIONTYPE_ATTACHMENT
	|| (mimeinfo->disposition == DISPOSITIONTYPE_INLINE && 
	    mimeinfo->type != MIMETYPE_TEXT)) {
		gtk_text_buffer_insert(buffer, &iter, "\n", 1);
		TEXT_INSERT_LINK(buf, "sc://select_attachment", mimeinfo);
		gtk_text_buffer_insert(buffer, &iter, " \n", -1);
		if (mimeinfo->type == MIMETYPE_IMAGE  &&
		    prefs_common.inline_img ) {
			GdkPixbuf *pixbuf;
			GError *error = NULL;
			gchar *filename;
			ClickableText *uri;
			gchar *uri_str;
			START_TIMING("inserting image");

			filename = procmime_get_tmp_file_name(mimeinfo);

			if (procmime_get_part(filename, mimeinfo) < 0) {
				g_warning("Can't get the image file.");
				g_free(filename);
				END_TIMING();
				return;
			}

			if (!prefs_common.resize_img) {
				pixbuf = gdk_pixbuf_new_from_file(filename, &error);
			} else {
				gint w, h;
				gdk_pixbuf_get_file_info(filename, &w, &h);
				if (w > textview->scrolledwin->allocation.width - 100)
					pixbuf = gdk_pixbuf_new_from_file_at_scale(filename, 
						textview->scrolledwin->allocation.width - 100, 
						-1, TRUE, &error);
				else
					pixbuf = gdk_pixbuf_new_from_file(filename, &error);
			}
			if (error != NULL) {
				g_warning("%s\n", error->message);
				g_error_free(error);
			}
			if (!pixbuf) {
				g_warning("Can't load the image.");
				g_free(filename);
				END_TIMING();
				return;
			}

			uri_str = g_filename_to_uri(filename, NULL, NULL);
			if (uri_str) {
				uri = g_new0(ClickableText, 1);
				uri->uri = uri_str;
				uri->start = gtk_text_iter_get_offset(&iter);
				
				gtk_text_buffer_insert_pixbuf(buffer, &iter, pixbuf);
				
				uri->end = uri->start + 1;
				uri->filename = procmime_get_part_file_name(mimeinfo);
				textview->uri_list =
					g_slist_append(textview->uri_list, uri);
				
				gtk_text_buffer_insert(buffer, &iter, " ", 1);
				gtk_text_buffer_get_iter_at_offset(buffer, &start_iter, uri->start);	
				gtk_text_buffer_apply_tag_by_name(buffer, "link", 
						&start_iter, &iter);
			} else {
				gtk_text_buffer_insert_pixbuf(buffer, &iter, pixbuf);
				gtk_text_buffer_insert(buffer, &iter, " ", 1);
			}

			g_object_unref(pixbuf);
			g_free(filename);
			END_TIMING();
			GTK_EVENTS_FLUSH();
		}
	} else if (mimeinfo->type == MIMETYPE_TEXT) {
		if (prefs_common.display_header && (charcount > 0))
			gtk_text_buffer_insert(buffer, &iter, "\n", 1);

		textview_write_body(textview, mimeinfo);
	}
	END_TIMING();
}

static void recursive_add_parts(TextView *textview, GNode *node)
{
        GNode * iter;
	MimeInfo *mimeinfo;
        START_TIMING("");

        mimeinfo = (MimeInfo *) node->data;
        
        textview_add_part(textview, mimeinfo);
        
        if ((mimeinfo->type != MIMETYPE_MULTIPART) &&
            (mimeinfo->type != MIMETYPE_MESSAGE)) {
	    	END_TIMING();
                return;
        }
        if (g_ascii_strcasecmp(mimeinfo->subtype, "alternative") == 0) {
                GNode * prefered_body;
                int prefered_score;
                
                /*
                  text/plain : score 3
                  text/ *    : score 2
                  other      : score 1
                */
                prefered_body = NULL;
                prefered_score = 0;
                
                for (iter = g_node_first_child(node) ; iter != NULL ;
                     iter = g_node_next_sibling(iter)) {
                        int score;
                        MimeInfo * submime;
                        
                        score = 1;
                        submime = (MimeInfo *) iter->data;
                        if (submime->type == MIMETYPE_TEXT)
                                score = 2;
                        
                        if (submime->subtype != NULL) {
                                if (g_ascii_strcasecmp(submime->subtype, "plain") == 0)
                                        score = 3;
                        }
                        
                        if (score > prefered_score) {
                                prefered_score = score;
                                prefered_body = iter;
                        }
                }
                
                if (prefered_body != NULL) {
                        recursive_add_parts(textview, prefered_body);
                }
        }
        else {
                for (iter = g_node_first_child(node) ; iter != NULL ;
                     iter = g_node_next_sibling(iter)) {
                        recursive_add_parts(textview, iter);
                }
        }
	END_TIMING();
}

static void textview_add_parts(TextView *textview, MimeInfo *mimeinfo)
{
	g_return_if_fail(mimeinfo != NULL);
        
        recursive_add_parts(textview, mimeinfo->node);
}

void textview_show_error(TextView *textview)
{
	GtkTextView *text;
	GtkTextBuffer *buffer;
	GtkTextIter iter;

	textview_set_font(textview, NULL);
	textview_clear(textview);

	text = GTK_TEXT_VIEW(textview->text);
	buffer = gtk_text_view_get_buffer(text);
	gtk_text_buffer_get_start_iter(buffer, &iter);

	TEXT_INSERT(_("\n"
		      "  This message can't be displayed.\n"
		      "  This is probably due to a network error.\n"
		      "\n"
		      "  Use "));
	TEXT_INSERT_LINK(_("'View Log'"), "sc://view_log", NULL);
	TEXT_INSERT(_(" in the Tools menu for more information."));
	textview_show_icon(textview, GTK_STOCK_DIALOG_ERROR);

}

void textview_show_mime_part(TextView *textview, MimeInfo *partinfo)
{
	GtkTextView *text;
	GtkTextBuffer *buffer;
	GtkTextIter iter;

	if (!partinfo) return;

	textview_set_font(textview, NULL);
	textview_clear(textview);

	text = GTK_TEXT_VIEW(textview->text);
	buffer = gtk_text_view_get_buffer(text);
	gtk_text_buffer_get_start_iter(buffer, &iter);

	TEXT_INSERT("\n");
	TEXT_INSERT(_("  The following can be performed on this part by\n"));
	TEXT_INSERT(_("  right-clicking the icon or list item:\n"));

	TEXT_INSERT(_("     - To save, select "));
	TEXT_INSERT_LINK(_("'Save as...'"), "sc://save_as", NULL);
	TEXT_INSERT(_(" (Shortcut key: 'y')\n"));
	TEXT_INSERT(_("     - To display as text, select "));
	TEXT_INSERT_LINK(_("'Display as text'"), "sc://display_as_text", NULL);
	TEXT_INSERT(_(" (Shortcut key: 't')\n"));
	TEXT_INSERT(_("     - To open with an external program, select "));
	TEXT_INSERT_LINK(_("'Open'"), "sc://open", NULL);
	TEXT_INSERT(_(" (Shortcut key: 'l')\n"));
	TEXT_INSERT(_("       (alternately double-click, or click the middle "));
	TEXT_INSERT(_("mouse button)\n"));
	TEXT_INSERT(_("     - Or use "));
	TEXT_INSERT_LINK(_("'Open with...'"), "sc://open_with", NULL);
	TEXT_INSERT(_(" (Shortcut key: 'o')\n"));
	textview_show_icon(textview, GTK_STOCK_DIALOG_INFO);
}

#undef TEXT_INSERT
#undef TEXT_INSERT_LINK

static void textview_write_body(TextView *textview, MimeInfo *mimeinfo)
{
	FILE *tmpfp;
	gchar buf[BUFFSIZE];
	CodeConverter *conv;
	const gchar *charset, *p, *cmd;
	GSList *cur;
	int lines = 0;
	
	if (textview->messageview->forced_charset)
		charset = textview->messageview->forced_charset;
	else
		charset = procmime_mimeinfo_get_parameter(mimeinfo, "charset");

	textview_set_font(textview, charset);

	conv = conv_code_converter_new(charset);

	procmime_force_encoding(textview->messageview->forced_encoding);
	
	textview->is_in_signature = FALSE;

	procmime_decode_content(mimeinfo);

	if (!g_ascii_strcasecmp(mimeinfo->subtype, "html") &&
	    prefs_common.render_html) {
		gchar *filename;
		
		filename = procmime_get_tmp_file_name(mimeinfo);
		if (procmime_get_part(filename, mimeinfo) == 0) {
			tmpfp = g_fopen(filename, "rb");
			textview_show_html(textview, tmpfp, conv);
			fclose(tmpfp);
			g_unlink(filename);
		}
		g_free(filename);
	} else if (!g_ascii_strcasecmp(mimeinfo->subtype, "enriched")) {
		gchar *filename;
		
		filename = procmime_get_tmp_file_name(mimeinfo);
		if (procmime_get_part(filename, mimeinfo) == 0) {
			tmpfp = g_fopen(filename, "rb");
			textview_show_ertf(textview, tmpfp, conv);
			fclose(tmpfp);
			g_unlink(filename);
		}
		g_free(filename);
#ifndef G_OS_WIN32
	} else if ( g_ascii_strcasecmp(mimeinfo->subtype, "plain") &&
		   (cmd = prefs_common.mime_textviewer) && *cmd &&
		   (p = strchr(cmd, '%')) && *(p + 1) == 's') {
		int pid, pfd[2];
		const gchar *fname;

		fname  = procmime_get_tmp_file_name(mimeinfo);
		if (procmime_get_part(fname, mimeinfo)) goto textview_default;

		g_snprintf(buf, sizeof(buf), cmd, fname);
		debug_print("Viewing text content of type: %s (length: %d) "
			"using %s\n", mimeinfo->subtype, mimeinfo->length, buf);

		if (pipe(pfd) < 0) {
			g_snprintf(buf, sizeof(buf),
				"pipe failed for textview\n\n%s\n", strerror(errno));
			textview_write_line(textview, buf, conv, TRUE);
			goto textview_default;
		}
		pid = fork();
		if (pid < 0) {
			g_snprintf(buf, sizeof(buf),
				"fork failed for textview\n\n%s\n", strerror(errno));
			textview_write_line(textview, buf, conv, TRUE);
			close(pfd[0]);
			close(pfd[1]);
			goto textview_default;
		}
		if (pid == 0) { /* child */
			int rc;
			gchar **argv;
			argv = strsplit_with_quote(buf, " ", 0);
			close(1);
			close(pfd[0]);
			dup(pfd[1]);
			rc = execvp(argv[0], argv);
			close(pfd[1]);
			printf (_("The command to view attachment "
			        "as text failed:\n"
			        "    %s\n"
			        "Exit code %d\n"), buf, rc);
			exit(255);
		}
		close(pfd[1]);
		tmpfp = fdopen(pfd[0], "rb");
		while (fgets(buf, sizeof(buf), tmpfp)) {
			textview_write_line(textview, buf, conv, TRUE);
			
			lines++;
			if (lines % 500 == 0)
				GTK_EVENTS_FLUSH();
			if (textview->stop_loading) {
				fclose(tmpfp);
				waitpid(pid, pfd, 0);
				unlink(fname);
				return;
			}
		}

		fclose(tmpfp);
		waitpid(pid, pfd, 0);
		unlink(fname);
#endif
	} else {
textview_default:
		lines = 0;
		tmpfp = g_fopen(mimeinfo->data.filename, "rb");
		fseek(tmpfp, mimeinfo->offset, SEEK_SET);
		debug_print("Viewing text content of type: %s (length: %d)\n", mimeinfo->subtype, mimeinfo->length);
		while ((ftell(tmpfp) < mimeinfo->offset + mimeinfo->length) &&
		       (fgets(buf, sizeof(buf), tmpfp) != NULL)) {
			textview_write_line(textview, buf, conv, TRUE);
			lines++;
			if (lines % 500 == 0)
				GTK_EVENTS_FLUSH();
			if (textview->stop_loading) {
				fclose(tmpfp);
				return;
			}
		}
		fclose(tmpfp);
	}

	conv_code_converter_destroy(conv);
	procmime_force_encoding(0);

	lines = 0;
	for (cur = textview->uri_list; cur; cur = cur->next) {
		ClickableText *uri = (ClickableText *)cur->data;
		if (!uri->is_quote)
			continue;
		if (!prefs_common.hide_quotes ||
		    uri->quote_level+1 < prefs_common.hide_quotes) {
			textview_toggle_quote(textview, uri, TRUE);
			lines++;
			if (lines % 500 == 0)
				GTK_EVENTS_FLUSH();
			if (textview->stop_loading) {
				return;
			}
		}
	}
}

static void textview_show_html(TextView *textview, FILE *fp,
			       CodeConverter *conv)
{
	SC_HTMLParser *parser;
	gchar *str;
	gint lines = 0;

	parser = sc_html_parser_new(fp, conv);
	g_return_if_fail(parser != NULL);

	while ((str = sc_html_parse(parser)) != NULL) {
	        if (parser->state == SC_HTML_HREF) {
		        /* first time : get and copy the URL */
		        if (parser->href == NULL) {
				/* ALF - the sylpheed html parser returns an empty string,
				 * if still inside an <a>, but already parsed past HREF */
				str = strtok(str, " ");
				if (str) {
					while (str && *str && g_ascii_isspace(*str))
						str++; 
					parser->href = g_strdup(str);
					/* the URL may (or not) be followed by the
					 * referenced text */
					str = strtok(NULL, "");
				}	
		        }
		        if (str != NULL)
			        textview_write_link(textview, str, parser->href, NULL);
	        } else
		        textview_write_line(textview, str, NULL, FALSE);
		lines++;
		if (lines % 500 == 0)
			GTK_EVENTS_FLUSH();
		if (textview->stop_loading) {
			return;
		}
	}
	textview_write_line(textview, "\n", NULL, FALSE);
	sc_html_parser_destroy(parser);
}

static void textview_show_ertf(TextView *textview, FILE *fp,
			       CodeConverter *conv)
{
	ERTFParser *parser;
	gchar *str;
	gint lines = 0;

	parser = ertf_parser_new(fp, conv);
	g_return_if_fail(parser != NULL);

	while ((str = ertf_parse(parser)) != NULL) {
		textview_write_line(textview, str, NULL, FALSE);
		lines++;
		if (lines % 500 == 0)
			GTK_EVENTS_FLUSH();
		if (textview->stop_loading) {
			return;
		}
	}
	
	ertf_parser_destroy(parser);
}

#define ADD_TXT_POS(bp_, ep_, pti_) \
	if ((last->next = alloca(sizeof(struct txtpos))) != NULL) { \
		last = last->next; \
		last->bp = (bp_); last->ep = (ep_); last->pti = (pti_); \
		last->next = NULL; \
	} else { \
		g_warning("alloc error scanning URIs\n"); \
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, \
							 linebuf, -1, \
							 fg_tag, NULL); \
		return; \
	}

#define ADD_TXT_POS_LATER(bp_, ep_, pti_) \
	if ((last->next = alloca(sizeof(struct txtpos))) != NULL) { \
		last = last->next; \
		last->bp = (bp_); last->ep = (ep_); last->pti = (pti_); \
		last->next = NULL; \
	} else { \
		g_warning("alloc error scanning URIs\n"); \
	}

/* textview_make_clickable_parts() - colorizes clickable parts */
static void textview_make_clickable_parts(TextView *textview,
					  const gchar *fg_tag,
					  const gchar *uri_tag,
					  const gchar *linebuf,
					  gboolean hdr)
{
	GtkTextView *text = GTK_TEXT_VIEW(textview->text);
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(text);
	GtkTextIter iter;
	gchar *mybuf = g_strdup(linebuf);
	
	/* parse table - in order of priority */
	struct table {
		const gchar *needle; /* token */

		/* token search function */
		gchar    *(*search)	(const gchar *haystack,
					 const gchar *needle);
		/* part parsing function */
		gboolean  (*parse)	(const gchar *start,
					 const gchar *scanpos,
					 const gchar **bp_,
					 const gchar **ep_,
					 gboolean hdr);
		/* part to URI function */
		gchar    *(*build_uri)	(const gchar *bp,
					 const gchar *ep);
	};

	static struct table parser[] = {
		{"http://",  strcasestr, get_uri_part,   make_uri_string},
		{"https://", strcasestr, get_uri_part,   make_uri_string},
		{"ftp://",   strcasestr, get_uri_part,   make_uri_string},
		{"sftp://",  strcasestr, get_uri_part,   make_uri_string},
		{"www.",     strcasestr, get_uri_part,   make_http_string},
		{"mailto:",  strcasestr, get_uri_part,   make_uri_string},
		{"@",        strcasestr, get_email_part, make_email_string}
	};
	const gint PARSE_ELEMS = sizeof parser / sizeof parser[0];

	gint  n;
	const gchar *walk, *bp, *ep;

	struct txtpos {
		const gchar	*bp, *ep;	/* text position */
		gint		 pti;		/* index in parse table */
		struct txtpos	*next;		/* next */
	} head = {NULL, NULL, 0,  NULL}, *last = &head;

	if (!g_utf8_validate(linebuf, -1, NULL)) {
		mybuf = g_malloc(strlen(linebuf)*2 +1);
		conv_localetodisp(mybuf, strlen(linebuf)*2 +1, linebuf);
	}

	gtk_text_buffer_get_end_iter(buffer, &iter);

	/* parse for clickable parts, and build a list of begin and end positions  */
	for (walk = mybuf, n = 0;;) {
		gint last_index = PARSE_ELEMS;
		gchar *scanpos = NULL;

		/* FIXME: this looks phony. scanning for anything in the parse table */
		for (n = 0; n < PARSE_ELEMS; n++) {
			gchar *tmp;

			tmp = parser[n].search(walk, parser[n].needle);
			if (tmp) {
				if (scanpos == NULL || tmp < scanpos) {
					scanpos = tmp;
					last_index = n;
				}
			}					
		}

		if (scanpos) {
			/* check if URI can be parsed */
			if (parser[last_index].parse(walk, scanpos, &bp, &ep, hdr)
			    && (size_t) (ep - bp - 1) > strlen(parser[last_index].needle)) {
					ADD_TXT_POS(bp, ep, last_index);
					walk = ep;
			} else
				walk = scanpos +
					strlen(parser[last_index].needle);
		} else
			break;
	}

	/* colorize this line */
	if (head.next) {
		const gchar *normal_text = mybuf;

		/* insert URIs */
		for (last = head.next; last != NULL;
		     normal_text = last->ep, last = last->next) {
			ClickableText *uri;
			uri = g_new0(ClickableText, 1);
			if (last->bp - normal_text > 0)
				gtk_text_buffer_insert_with_tags_by_name
					(buffer, &iter,
					 normal_text,
					 last->bp - normal_text,
					 fg_tag, NULL);
			uri->uri = parser[last->pti].build_uri(last->bp,
							       last->ep);
			uri->start = gtk_text_iter_get_offset(&iter);
			gtk_text_buffer_insert_with_tags_by_name
				(buffer, &iter, last->bp, last->ep - last->bp,
				 uri_tag, fg_tag, NULL);
			uri->end = gtk_text_iter_get_offset(&iter);
			uri->filename = NULL;
			textview->uri_list =
				g_slist_append(textview->uri_list, uri);
		}

		if (*normal_text)
			gtk_text_buffer_insert_with_tags_by_name
				(buffer, &iter, normal_text, -1, fg_tag, NULL);
	} else {
		gtk_text_buffer_insert_with_tags_by_name
			(buffer, &iter, mybuf, -1, fg_tag, NULL);
	}
	g_free(mybuf);
}

/* textview_make_clickable_parts() - colorizes clickable parts */
static void textview_make_clickable_parts_later(TextView *textview,
					  gint start, gint end)
{
	GtkTextView *text = GTK_TEXT_VIEW(textview->text);
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(text);
	GtkTextIter start_iter, end_iter;
	gchar *mybuf;
	gint offset = 0;
	/* parse table - in order of priority */
	struct table {
		const gchar *needle; /* token */

		/* token search function */
		gchar    *(*search)	(const gchar *haystack,
					 const gchar *needle);
		/* part parsing function */
		gboolean  (*parse)	(const gchar *start,
					 const gchar *scanpos,
					 const gchar **bp_,
					 const gchar **ep_,
					 gboolean hdr);
		/* part to URI function */
		gchar    *(*build_uri)	(const gchar *bp,
					 const gchar *ep);
	};

	static struct table parser[] = {
		{"http://",  strcasestr, get_uri_part,   make_uri_string},
		{"https://", strcasestr, get_uri_part,   make_uri_string},
		{"ftp://",   strcasestr, get_uri_part,   make_uri_string},
		{"sftp://",  strcasestr, get_uri_part,   make_uri_string},
		{"www.",     strcasestr, get_uri_part,   make_http_string},
		{"mailto:",  strcasestr, get_uri_part,   make_uri_string},
		{"@",        strcasestr, get_email_part, make_email_string}
	};
	const gint PARSE_ELEMS = sizeof parser / sizeof parser[0];

	gint  n;
	const gchar *walk, *bp, *ep;

	struct txtpos {
		const gchar	*bp, *ep;	/* text position */
		gint		 pti;		/* index in parse table */
		struct txtpos	*next;		/* next */
	} head = {NULL, NULL, 0,  NULL}, *last = &head;

	gtk_text_buffer_get_iter_at_offset(buffer, &start_iter, start);
	gtk_text_buffer_get_iter_at_offset(buffer, &end_iter, end);
	mybuf = gtk_text_buffer_get_text(buffer, &start_iter, &end_iter, FALSE);
	offset = gtk_text_iter_get_offset(&start_iter);

	/* parse for clickable parts, and build a list of begin and end positions  */
	for (walk = mybuf, n = 0;;) {
		gint last_index = PARSE_ELEMS;
		gchar *scanpos = NULL;

		/* FIXME: this looks phony. scanning for anything in the parse table */
		for (n = 0; n < PARSE_ELEMS; n++) {
			gchar *tmp;

			tmp = parser[n].search(walk, parser[n].needle);
			if (tmp) {
				if (scanpos == NULL || tmp < scanpos) {
					scanpos = tmp;
					last_index = n;
				}
			}					
		}

		if (scanpos) {
			/* check if URI can be parsed */
			if (parser[last_index].parse(walk, scanpos, &bp, &ep, FALSE)
			    && (size_t) (ep - bp - 1) > strlen(parser[last_index].needle)) {
					ADD_TXT_POS_LATER(bp, ep, last_index);
					walk = ep;
			} else
				walk = scanpos +
					strlen(parser[last_index].needle);
		} else
			break;
	}

	/* colorize this line */
	if (head.next) {
		/* insert URIs */
		for (last = head.next; last != NULL; last = last->next) {
			ClickableText *uri;
			gint start_offset, end_offset;
			gchar *tmp_str;
			gchar old_char;
			uri = g_new0(ClickableText, 1);
			uri->uri = parser[last->pti].build_uri(last->bp,
							       last->ep);
			
			tmp_str = mybuf;
			old_char = tmp_str[last->ep - mybuf];
			tmp_str[last->ep - mybuf] = '\0';				       
			end_offset = g_utf8_strlen(tmp_str, -1);
			tmp_str[last->ep - mybuf] = old_char;
			
			old_char = tmp_str[last->bp - mybuf];
			tmp_str[last->bp - mybuf] = '\0';				       
			start_offset = g_utf8_strlen(tmp_str, -1);
			tmp_str[last->bp - mybuf] = old_char;
			
			gtk_text_buffer_get_iter_at_offset(buffer, &start_iter, start_offset + offset);
			gtk_text_buffer_get_iter_at_offset(buffer, &end_iter, end_offset + offset);
			
			uri->start = gtk_text_iter_get_offset(&start_iter);
			
			gtk_text_buffer_apply_tag_by_name(buffer, "link", &start_iter, &end_iter);

			uri->end = gtk_text_iter_get_offset(&end_iter);
			uri->filename = NULL;
			textview->uri_list =
				g_slist_append(textview->uri_list, uri);
		}
	} 

	g_free(mybuf);
}

#undef ADD_TXT_POS

static void textview_write_line(TextView *textview, const gchar *str,
				CodeConverter *conv, gboolean do_quote_folding)
{
	GtkTextView *text;
	GtkTextBuffer *buffer;
	GtkTextIter iter;
	gchar buf[BUFFSIZE];
	gchar *fg_color;
	gint quotelevel = -1, real_quotelevel = -1;
	gchar quote_tag_str[10];

	text = GTK_TEXT_VIEW(textview->text);
	buffer = gtk_text_view_get_buffer(text);
	gtk_text_buffer_get_end_iter(buffer, &iter);

	if (!conv)
		strncpy2(buf, str, sizeof(buf));
	else if (conv_convert(conv, buf, sizeof(buf), str) < 0)
		conv_localetodisp(buf, sizeof(buf), str);
		
	strcrchomp(buf);
	fg_color = NULL;

	/* change color of quotation
	   >, foo>, _> ... ok, <foo>, foo bar>, foo-> ... ng
	   Up to 3 levels of quotations are detected, and each
	   level is colored using a different color. */
	if (prefs_common.enable_color 
	    && line_has_quote_char(buf, prefs_common.quote_chars)) {
		real_quotelevel = get_quote_level(buf, prefs_common.quote_chars);
		quotelevel = real_quotelevel;
		/* set up the correct foreground color */
		if (quotelevel > 2) {
			/* recycle colors */
			if (prefs_common.recycle_quote_colors)
				quotelevel %= 3;
			else
				quotelevel = 2;
		}
	}

	if (quotelevel == -1)
		fg_color = NULL;
	else {
		g_snprintf(quote_tag_str, sizeof(quote_tag_str),
			   "quote%d", quotelevel);
		fg_color = quote_tag_str;
	}

	if (prefs_common.enable_color && (strcmp(buf,"-- \n") == 0 || textview->is_in_signature)) {
		fg_color = "signature";
		textview->is_in_signature = TRUE;
	}

	if (real_quotelevel > -1 && do_quote_folding) {
		if ( previousquotelevel != real_quotelevel ) {
			ClickableText *uri;
			uri = g_new0(ClickableText, 1);
			uri->uri = g_strdup("");
			uri->data = g_strdup(buf);
			uri->start = gtk_text_iter_get_offset(&iter);
			gtk_text_buffer_insert_with_tags_by_name
						(buffer, &iter, " [...]", -1,
						 "qlink", fg_color, NULL);
			uri->end = gtk_text_iter_get_offset(&iter);
			uri->filename = NULL;
			uri->fg_color = g_strdup(fg_color);
			uri->is_quote = TRUE;
			uri->quote_level = real_quotelevel;
			textview->uri_list =
				g_slist_append(textview->uri_list, uri);
			gtk_text_buffer_insert(buffer, &iter, "  \n", -1);
		
			previousquotelevel = real_quotelevel;
		} else {
			GSList *last = g_slist_last(textview->uri_list);
			ClickableText *lasturi = (ClickableText *)last->data;
 			gint e_len = lasturi->data ? strlen(lasturi->data):0;
  			gint n_len = strlen(buf);
  			lasturi->data = g_realloc((gchar *)lasturi->data, e_len + n_len + 1);
  			strcpy((gchar *)lasturi->data + e_len, buf);
		}
	} else {
		textview_make_clickable_parts(textview, fg_color, "link", buf, FALSE);
		previousquotelevel = -1;
	}
}

void textview_write_link(TextView *textview, const gchar *str,
			 const gchar *uri, CodeConverter *conv)
{
    	GdkColor *link_color = NULL;
	GtkTextView *text;
	GtkTextBuffer *buffer;
	GtkTextIter iter;
	gchar buf[BUFFSIZE];
	gchar *bufp;
	ClickableText *r_uri;

	if (!str || *str == '\0')
		return;
	if (!uri)
		return;

	while (uri && *uri && g_ascii_isspace(*uri))
		uri++;
		
	text = GTK_TEXT_VIEW(textview->text);
	buffer = gtk_text_view_get_buffer(text);
	gtk_text_buffer_get_end_iter(buffer, &iter);

	if (!conv)
		strncpy2(buf, str, sizeof(buf));
	else if (conv_convert(conv, buf, sizeof(buf), str) < 0)
		conv_utf8todisp(buf, sizeof(buf), str);

	if (g_utf8_validate(buf, -1, NULL) == FALSE)
		return;

	strcrchomp(buf);

	gtk_text_buffer_get_end_iter(buffer, &iter);
	for (bufp = buf; *bufp != '\0'; bufp = g_utf8_next_char(bufp)) {
		gunichar ch;

		ch = g_utf8_get_char(bufp);
		if (!g_unichar_isspace(ch))
			break;
	}
	if (bufp > buf)
		gtk_text_buffer_insert(buffer, &iter, buf, bufp - buf);

    	if (prefs_common.enable_color) {
		link_color = &uri_color;
    	}
	r_uri = g_new0(ClickableText, 1);
	r_uri->uri = g_strdup(uri);
	r_uri->start = gtk_text_iter_get_offset(&iter);
	gtk_text_buffer_insert_with_tags_by_name
		(buffer, &iter, bufp, -1, "link", NULL);
	r_uri->end = gtk_text_iter_get_offset(&iter);
	r_uri->filename = NULL;
	textview->uri_list = g_slist_append(textview->uri_list, r_uri);
}

static void textview_set_cursor(GdkWindow *window, GdkCursor *cursor)
{
	if (GDK_IS_WINDOW(window))
		gdk_window_set_cursor(window, cursor);
}
void textview_clear(TextView *textview)
{
	GtkTextView *text = GTK_TEXT_VIEW(textview->text);
	GtkTextBuffer *buffer;
	GdkWindow *window = gtk_text_view_get_window(text,
				GTK_TEXT_WINDOW_TEXT);

	buffer = gtk_text_view_get_buffer(text);
	gtk_text_buffer_set_text(buffer, "", -1);

	TEXTVIEW_STATUSBAR_POP(textview);
	textview_uri_list_remove_all(textview->uri_list);
	textview->uri_list = NULL;
	textview->uri_hover = NULL;

	textview->body_pos = 0;
	if (textview->image) 
		gtk_widget_destroy(textview->image);
	textview->image = NULL;

	if (textview->messageview->mainwin->cursor_count == 0) {
		textview_set_cursor(window, text_cursor);
	} else {
		textview_set_cursor(window, watch_cursor);
	}
}

void textview_destroy(TextView *textview)
{
	GtkTextBuffer *buffer;
	GtkClipboard *clipboard;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview->text));
	clipboard = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
	gtk_text_buffer_remove_selection_clipboard(buffer, clipboard);

	textview_uri_list_remove_all(textview->uri_list);
	textview->uri_list = NULL;

	g_free(textview);
}

void textview_set_all_headers(TextView *textview, gboolean all_headers)
{
	textview->show_all_headers = all_headers;
}

#define CHANGE_TAG_FONT(tagname, font) { \
	tag = gtk_text_tag_table_lookup(tags, tagname); \
	if (tag) \
		g_object_set(G_OBJECT(tag), "font-desc", font, NULL); \
}

void textview_set_font(TextView *textview, const gchar *codeset)
{
	GtkTextTag *tag;
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview->text));
	GtkTextTagTable *tags = gtk_text_buffer_get_tag_table(buffer);
	
	if (NORMAL_FONT) {
		PangoFontDescription *font_desc, *bold_font_desc;
		font_desc = pango_font_description_from_string
						(NORMAL_FONT);
		bold_font_desc = pango_font_description_from_string
						(NORMAL_FONT);
		if (font_desc) {
			gtk_widget_modify_font(textview->text, font_desc);
			CHANGE_TAG_FONT("header", font_desc);
			pango_font_description_free(font_desc);
		}
		if (bold_font_desc) {
			pango_font_description_set_weight
				(bold_font_desc, PANGO_WEIGHT_BOLD);
			CHANGE_TAG_FONT("header_title", bold_font_desc);
			pango_font_description_free(bold_font_desc);
		}
	}

	if (prefs_common.textfont) {
		PangoFontDescription *font_desc;

		font_desc = pango_font_description_from_string
						(prefs_common.textfont);
		if (font_desc) {
			gtk_widget_modify_font(textview->text, font_desc);
			pango_font_description_free(font_desc);
		}
	}
	gtk_text_view_set_pixels_above_lines(GTK_TEXT_VIEW(textview->text),
					     prefs_common.line_space / 2);
	gtk_text_view_set_pixels_below_lines(GTK_TEXT_VIEW(textview->text),
					     prefs_common.line_space / 2);
}

void textview_set_text(TextView *textview, const gchar *text)
{
	GtkTextView *view;
	GtkTextBuffer *buffer;

	g_return_if_fail(textview != NULL);
	g_return_if_fail(text != NULL);

	textview_clear(textview);

	view = GTK_TEXT_VIEW(textview->text);
	buffer = gtk_text_view_get_buffer(view);
	gtk_text_buffer_set_text(buffer, text, strlen(text));
}

enum
{
	H_DATE		= 0,
	H_FROM		= 1,
	H_TO		= 2,
	H_NEWSGROUPS	= 3,
	H_SUBJECT	= 4,
	H_CC		= 5,
	H_REPLY_TO	= 6,
	H_FOLLOWUP_TO	= 7,
	H_X_MAILER	= 8,
	H_X_NEWSREADER	= 9,
	H_USER_AGENT	= 10,
	H_ORGANIZATION	= 11,
};

void textview_set_position(TextView *textview, gint pos)
{
	GtkTextView *text = GTK_TEXT_VIEW(textview->text);

	gtkut_text_view_set_position(text, pos);
}

static gboolean header_is_internal(Header *header)
{
	const gchar *internal_hdrs[] = 
		{"AF:", "NF:", "PS:", "SRH:", "SFN:", "DSR:", "MID:", 
		 "CFG:", "PT:", "S:", "RQ:", "SSV:", "NSV:", "SSH:", 
		 "R:", "MAID:", "SCF:", "RMID:", "FMID:", "NAID:", 
		 "X-Sylpheed-Account-Id:", "X-Sylpheed-Sign:", "X-Sylpheed-Encrypt:", 
		 "X-Sylpheed-Privacy-System:", "X-Sylpheed-End-Special-Headers:",
		 NULL};
	int i;
	
	for (i = 0; internal_hdrs[i]; i++) {
		if (!strcmp(header->name, internal_hdrs[i]))
			return TRUE;
	}
	return FALSE;
}

static GPtrArray *textview_scan_header(TextView *textview, FILE *fp)
{
	gchar buf[BUFFSIZE];
	GPtrArray *headers, *sorted_headers;
	GSList *disphdr_list;
	Header *header;
	gint i;

	g_return_val_if_fail(fp != NULL, NULL);

	if (textview->show_all_headers) {
		headers = procheader_get_header_array_asis(fp);
		sorted_headers = g_ptr_array_new();
		for (i = 0; i < headers->len; i++) {
			header = g_ptr_array_index(headers, i);
			if (!header_is_internal(header))
				g_ptr_array_add(sorted_headers, header);
			else
				procheader_header_free(header);
		}
		g_ptr_array_free(headers, TRUE);
		return sorted_headers;
	}

	if (!prefs_common.display_header) {
		while (fgets(buf, sizeof(buf), fp) != NULL)
			if (buf[0] == '\r' || buf[0] == '\n') break;
		return NULL;
	}

	headers = procheader_get_header_array_asis(fp);

	sorted_headers = g_ptr_array_new();

	for (disphdr_list = prefs_common.disphdr_list; disphdr_list != NULL;
	     disphdr_list = disphdr_list->next) {
		DisplayHeaderProp *dp =
			(DisplayHeaderProp *)disphdr_list->data;

		for (i = 0; i < headers->len; i++) {
			header = g_ptr_array_index(headers, i);

			if (procheader_headername_equal(header->name,
							dp->name)) {
				if (dp->hidden)
					procheader_header_free(header);
				else
					g_ptr_array_add(sorted_headers, header);

				g_ptr_array_remove_index(headers, i);
				i--;
			}
		}
	}

	if (prefs_common.show_other_header) {
		for (i = 0; i < headers->len; i++) {
			header = g_ptr_array_index(headers, i);
			if (!header_is_internal(header)) {
				g_ptr_array_add(sorted_headers, header);
			} else {
				procheader_header_free(header);
			}
		}
		g_ptr_array_free(headers, TRUE);
	} else
		procheader_header_array_destroy(headers);


	return sorted_headers;
}

static void textview_show_face(TextView *textview)
{
	GtkTextView *text = GTK_TEXT_VIEW(textview->text);
	MsgInfo *msginfo = textview->messageview->msginfo;
	int x = 0;
	
	if (prefs_common.display_header_pane
	||  !prefs_common.display_xface)
		goto bail;
	
	if (!msginfo->extradata || !msginfo->extradata->face) {
		goto bail;
	}

	if (textview->image) 
		gtk_widget_destroy(textview->image);
	
	textview->image = face_get_from_header(msginfo->extradata->face);
	g_return_if_fail(textview->image != NULL);

	gtk_widget_show(textview->image);
	
	x = textview->text->allocation.width - WIDTH -5;

	gtk_text_view_add_child_in_window(text, textview->image, 
		GTK_TEXT_WINDOW_TEXT, x, 5);

	gtk_widget_show_all(textview->text);
	

	return;
bail:
	if (textview->image) 
		gtk_widget_destroy(textview->image);
	textview->image = NULL;	
}

static void textview_show_icon(TextView *textview, const gchar *stock_id)
{
	GtkTextView *text = GTK_TEXT_VIEW(textview->text);
	int x = 0;
	
	if (textview->image) 
		gtk_widget_destroy(textview->image);
	
	textview->image = gtk_image_new_from_stock(stock_id, GTK_ICON_SIZE_DIALOG);
	g_return_if_fail(textview->image != NULL);

	gtk_widget_show(textview->image);
	
	x = textview->text->allocation.width - WIDTH -5;

	gtk_text_view_add_child_in_window(text, textview->image, 
		GTK_TEXT_WINDOW_TEXT, x, 5);

	gtk_widget_show_all(textview->text);
	

	return;
}

#if HAVE_LIBCOMPFACE
static void textview_show_xface(TextView *textview)
{
	MsgInfo *msginfo = textview->messageview->msginfo;
	GtkTextView *text = GTK_TEXT_VIEW(textview->text);
	int x = 0;

	if (prefs_common.display_header_pane
	||  !prefs_common.display_xface)
		goto bail;
	
	if (!msginfo || !msginfo->extradata)
		goto bail;

	if (msginfo->extradata->face)
		return;
	
	if (!msginfo->extradata->xface || strlen(msginfo->extradata->xface) < 5) {
		goto bail;
	}

	if (textview->image) 
		gtk_widget_destroy(textview->image);
	
	textview->image = xface_get_from_header(msginfo->extradata->xface,
				&textview->text->style->white,
				textview->text->window);
	g_return_if_fail(textview->image != NULL);

	gtk_widget_show(textview->image);
	
	x = textview->text->allocation.width - WIDTH -5;

	gtk_text_view_add_child_in_window(text, textview->image, 
		GTK_TEXT_WINDOW_TEXT, x, 5);

	gtk_widget_show_all(textview->text);
	
	return;
bail:
	if (textview->image) 
		gtk_widget_destroy(textview->image);
	textview->image = NULL;
	
}
#endif

static void textview_show_header(TextView *textview, GPtrArray *headers)
{
	GtkTextView *text = GTK_TEXT_VIEW(textview->text);
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(text);
	GtkTextIter iter;
	Header *header;
	gint i;

	g_return_if_fail(headers != NULL);

	for (i = 0; i < headers->len; i++) {
		header = g_ptr_array_index(headers, i);
		g_return_if_fail(header->name != NULL);

		gtk_text_buffer_get_end_iter (buffer, &iter);
		if(prefs_common.trans_hdr == TRUE) {
			gchar *hdr = g_strndup(header->name, strlen(header->name) - 1);
			gchar *trans_hdr = gettext(hdr);
			gtk_text_buffer_insert_with_tags_by_name(buffer,
				&iter, trans_hdr, -1,
				"header_title", "header", NULL);
			gtk_text_buffer_insert_with_tags_by_name(buffer,
				&iter, ":", 1, "header_title", "header", NULL);
			g_free(hdr);
		} else {
			gtk_text_buffer_insert_with_tags_by_name(buffer,
				&iter, header->name,
				-1, "header_title", "header", NULL);
		}
		if (header->name[strlen(header->name) - 1] != ' ')
		gtk_text_buffer_insert_with_tags_by_name
				(buffer, &iter, " ", 1,
				 "header_title", "header", NULL);

		if (procheader_headername_equal(header->name, "Subject") ||
		    procheader_headername_equal(header->name, "From")    ||
		    procheader_headername_equal(header->name, "To")      ||
		    procheader_headername_equal(header->name, "Cc"))
			unfold_line(header->body);

		if ((procheader_headername_equal(header->name, "X-Mailer") ||
		     procheader_headername_equal(header->name,
						 "X-Newsreader")) &&
		    strstr(header->body, "Sylpheed-Claws") != NULL) {
			gtk_text_buffer_get_end_iter (buffer, &iter);
			gtk_text_buffer_insert_with_tags_by_name
				(buffer, &iter, header->body, -1,
				 "header", "emphasis", NULL);
		} else {
			gboolean hdr = 
			  procheader_headername_equal(header->name, "From") ||
			  procheader_headername_equal(header->name, "To") ||
			  procheader_headername_equal(header->name, "Cc") ||
			  procheader_headername_equal(header->name, "Bcc") ||
			  procheader_headername_equal(header->name, "Reply-To") ||
			  procheader_headername_equal(header->name, "Sender");
			textview_make_clickable_parts(textview, "header", 
						      "hlink", header->body, 
						      hdr);
		}
		gtk_text_buffer_get_end_iter (buffer, &iter);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "\n", 1,
							 "header", NULL);
	}
	
	textview_show_face(textview);
#if HAVE_LIBCOMPFACE
	textview_show_xface(textview);
#endif
}

gboolean textview_search_string(TextView *textview, const gchar *str,
				gboolean case_sens)
{
	GtkTextView *text = GTK_TEXT_VIEW(textview->text);

	return gtkut_text_view_search_string(text, str, case_sens);
}

gboolean textview_search_string_backward(TextView *textview, const gchar *str,
					 gboolean case_sens)
{
	GtkTextView *text = GTK_TEXT_VIEW(textview->text);

	return gtkut_text_view_search_string_backward(text, str, case_sens);
}

void textview_scroll_one_line(TextView *textview, gboolean up)
{
	GtkTextView *text = GTK_TEXT_VIEW(textview->text);
	GtkAdjustment *vadj = text->vadjustment;

	gtkutils_scroll_one_line(GTK_WIDGET(text), vadj, up);
}

gboolean textview_scroll_page(TextView *textview, gboolean up)
{
	GtkTextView *text = GTK_TEXT_VIEW(textview->text);
	GtkAdjustment *vadj = text->vadjustment;

	return gtkutils_scroll_page(GTK_WIDGET(text), vadj, up);
}

#define KEY_PRESS_EVENT_STOP() \
	g_signal_stop_emission_by_name(G_OBJECT(widget), \
				       "key_press_event");

static gint textview_key_pressed(GtkWidget *widget, GdkEventKey *event,
				 TextView *textview)
{
	SummaryView *summaryview = NULL;
	MessageView *messageview = textview->messageview;

	if (!event) return FALSE;
	if (messageview->mainwin)
		summaryview = messageview->mainwin->summaryview;

	switch (event->keyval) {
	case GDK_Tab:
	case GDK_Home:
	case GDK_Left:
	case GDK_Up:
	case GDK_Right:
	case GDK_Down:
	case GDK_Page_Up:
	case GDK_Page_Down:
	case GDK_End:
	case GDK_Control_L:
	case GDK_Control_R:
		return FALSE;
	case GDK_space:
		if (summaryview)
			summary_pass_key_press_event(summaryview, event);
		else
			mimeview_scroll_page
				(messageview->mimeview,
				 (event->state &
				  (GDK_SHIFT_MASK|GDK_MOD1_MASK)) != 0);
		break;
	case GDK_BackSpace:
		mimeview_scroll_page(messageview->mimeview, TRUE);
		break;
	case GDK_Return:
		mimeview_scroll_one_line
			(messageview->mimeview, (event->state &
				    (GDK_SHIFT_MASK|GDK_MOD1_MASK)) != 0);
		break;
	case GDK_Delete:
		if (summaryview)
			summary_pass_key_press_event(summaryview, event);
		break;
	case GDK_y:
	case GDK_t:
	case GDK_l:
	case GDK_c:
		if ((event->state & (GDK_MOD1_MASK|GDK_CONTROL_MASK)) == 0) {
			KEY_PRESS_EVENT_STOP();
			mimeview_pass_key_press_event(messageview->mimeview,
						      event);
			break;
		}
		/* possible fall through */
	default:
		if (summaryview &&
		    event->window != messageview->mainwin->window->window) {
			GdkEventKey tmpev = *event;

			tmpev.window = messageview->mainwin->window->window;
			KEY_PRESS_EVENT_STOP();
			gtk_widget_event(messageview->mainwin->window,
					 (GdkEvent *)&tmpev);
		}
		break;
	}

	return TRUE;
}

static gboolean textview_motion_notify(GtkWidget *widget,
				       GdkEventMotion *event,
				       TextView *textview)
{
	if (textview->loading)
		return FALSE;
	textview_uri_update(textview, event->x, event->y);
	gdk_window_get_pointer(widget->window, NULL, NULL, NULL);

	return FALSE;
}

static gboolean textview_leave_notify(GtkWidget *widget,
				      GdkEventCrossing *event,
				      TextView *textview)
{
	if (textview->loading)
		return FALSE;
	textview_uri_update(textview, -1, -1);

	return FALSE;
}

static gboolean textview_visibility_notify(GtkWidget *widget,
					   GdkEventVisibility *event,
					   TextView *textview)
{
	gint wx, wy;
	GdkWindow *window;

	if (textview->loading)
		return FALSE;

	window = gtk_text_view_get_window(GTK_TEXT_VIEW(widget),
					  GTK_TEXT_WINDOW_TEXT);

	/* check if occurred for the text window part */
	if (window != event->window)
		return FALSE;
	
	gdk_window_get_pointer(widget->window, &wx, &wy, NULL);
	textview_uri_update(textview, wx, wy);

	return FALSE;
}

void textview_cursor_wait(TextView *textview)
{
	GdkWindow *window = gtk_text_view_get_window(
			GTK_TEXT_VIEW(textview->text),
			GTK_TEXT_WINDOW_TEXT);
	textview_set_cursor(window, watch_cursor);
}

void textview_cursor_normal(TextView *textview)
{
	GdkWindow *window = gtk_text_view_get_window(
			GTK_TEXT_VIEW(textview->text),
			GTK_TEXT_WINDOW_TEXT);
	textview_set_cursor(window, text_cursor);
}

static void textview_uri_update(TextView *textview, gint x, gint y)
{
	GtkTextBuffer *buffer;
	GtkTextIter start_iter, end_iter;
	ClickableText *uri = NULL;
	
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview->text));

	if (x != -1 && y != -1) {
		gint bx, by;
		GtkTextIter iter;
		GSList *tags;
		GSList *cur;
		
		gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(textview->text), 
						      GTK_TEXT_WINDOW_WIDGET,
						      x, y, &bx, &by);
		gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(textview->text),
						   &iter, bx, by);

		tags = gtk_text_iter_get_tags(&iter);
		for (cur = tags; cur != NULL; cur = cur->next) {
			GtkTextTag *tag = cur->data;
			char *name;

			g_object_get(G_OBJECT(tag), "name", &name, NULL);

			if ((!strcmp(name, "link") || !strcmp(name, "hlink"))
			    && textview_get_uri_range(textview, &iter, tag,
						      &start_iter, &end_iter)) {

				uri = textview_get_uri_from_range(textview,
								  &iter, tag,
								  &start_iter,
								  &end_iter);
			}
			g_free(name);
			if (uri)
				break;
		}
		g_slist_free(tags);
	}
	
	if (uri != textview->uri_hover) {
		GdkWindow *window;

		if (textview->uri_hover)
			gtk_text_buffer_remove_tag_by_name(buffer,
							   "link-hover",
							   &textview->uri_hover_start_iter,
							   &textview->uri_hover_end_iter);
		    
		textview->uri_hover = uri;
		if (uri) {
			textview->uri_hover_start_iter = start_iter;
			textview->uri_hover_end_iter = end_iter;
		}
		
		window = gtk_text_view_get_window(GTK_TEXT_VIEW(textview->text),
						  GTK_TEXT_WINDOW_TEXT);
		if (textview->messageview->mainwin->cursor_count == 0) {
			textview_set_cursor(window, uri ? hand_cursor : text_cursor);
		} else {
			textview_set_cursor(window, watch_cursor);
		}

		TEXTVIEW_STATUSBAR_POP(textview);

		if (uri) {
			char *trimmed_uri;

			if (!uri->is_quote)
				gtk_text_buffer_apply_tag_by_name(buffer,
							  "link-hover",
							  &start_iter,
							  &end_iter);

			trimmed_uri = trim_string(uri->uri, 60);
			TEXTVIEW_STATUSBAR_PUSH(textview, trimmed_uri);
			g_free(trimmed_uri);
		}
	}
}

static gboolean textview_get_uri_range(TextView *textview,
				       GtkTextIter *iter,
				       GtkTextTag *tag,
				       GtkTextIter *start_iter,
				       GtkTextIter *end_iter)
{
	return get_tag_range(iter, tag, start_iter, end_iter);
}

static ClickableText *textview_get_uri_from_range(TextView *textview,
					      GtkTextIter *iter,
					      GtkTextTag *tag,
					      GtkTextIter *start_iter,
					      GtkTextIter *end_iter)
{
	gint start_pos, end_pos, cur_pos;
	ClickableText *uri = NULL;
	GSList *cur;

	start_pos = gtk_text_iter_get_offset(start_iter);
	end_pos = gtk_text_iter_get_offset(end_iter);
	cur_pos = gtk_text_iter_get_offset(iter);

	for (cur = textview->uri_list; cur != NULL; cur = cur->next) {
		ClickableText *uri_ = (ClickableText *)cur->data;
		if (start_pos == uri_->start &&
		    end_pos ==  uri_->end) {
			uri = uri_;
			break;
		} 
	}
	for (cur = textview->uri_list; uri == NULL && cur != NULL; cur = cur->next) {
		ClickableText *uri_ = (ClickableText *)cur->data;
		if (start_pos == uri_->start ||
			   end_pos == uri_->end) {
			/* in case of contiguous links, textview_get_uri_range
			 * returns a broader range (start of 1st link to end
			 * of last link).
			 * In that case, correct link is the one covering
			 * current iter.
			 */
			if (uri_->start <= cur_pos && cur_pos <= uri_->end) {
				uri = uri_;
				break;
			}
		} 
	}

	return uri;
}

static ClickableText *textview_get_uri(TextView *textview,
				   GtkTextIter *iter,
				   GtkTextTag *tag)
{
	GtkTextIter start_iter, end_iter;
	ClickableText *uri = NULL;

	if (textview_get_uri_range(textview, iter, tag, &start_iter,
				   &end_iter))
		uri = textview_get_uri_from_range(textview, iter, tag,
						  &start_iter, &end_iter);

	return uri;
}

static void textview_shift_uris_after(TextView *textview, gint start, gint shift)
{
	GSList *cur;
	for (cur = textview->uri_list; cur; cur = cur->next) {
		ClickableText *uri = (ClickableText *)cur->data;
		if (uri->start <= start)
			continue;
		uri->start += shift;
		uri->end += shift;
	}
}

static void textview_remove_uris_in(TextView *textview, gint start, gint end)
{
	GSList *cur;
	for (cur = textview->uri_list; cur; ) {
		ClickableText *uri = (ClickableText *)cur->data;
		if (uri->start > start && uri->end < end) {
			cur = cur->next;
			textview->uri_list = g_slist_remove(textview->uri_list, uri);
			g_free(uri->uri);
			g_free(uri->filename);
			if (uri->is_quote) {
				g_free(uri->fg_color);
				g_free(uri->data); 
				/* (only free data in quotes uris) */
			}
			g_free(uri);
		} else {
			cur = cur->next;
		}
		
	}
}

static void textview_toggle_quote(TextView *textview, ClickableText *uri, gboolean expand_only)
{
	GtkTextIter start, end;
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview->text));
	
	if (!uri->is_quote)
		return;
	
	if (uri->q_expanded && expand_only)
		return;

	gtk_text_buffer_get_iter_at_offset(buffer, &start, uri->start);
	gtk_text_buffer_get_iter_at_offset(buffer, &end,   uri->end);
	if (textview->uri_hover)
		gtk_text_buffer_remove_tag_by_name(buffer,
						   "link-hover",
						   &textview->uri_hover_start_iter,
						   &textview->uri_hover_end_iter);
	textview->uri_hover = NULL;
	gtk_text_buffer_remove_tag_by_name(buffer,
					   "qlink",
					   &start,
					   &end);
	/* when shifting URIs start and end, we have to do it per-UTF8-char
	 * so use g_utf8_strlen(). OTOH, when inserting in the text buffer, 
	 * we have to pass a number of bytes, so use strlen(). disturbing. */
	 
	if (!uri->q_expanded) {
		gtk_text_buffer_get_iter_at_offset(buffer, &start, uri->start);
		gtk_text_buffer_get_iter_at_offset(buffer, &end,   uri->end);
		textview_shift_uris_after(textview, uri->start, 
			g_utf8_strlen((gchar *)uri->data, -1)-strlen(" [...]\n"));
		gtk_text_buffer_delete(buffer, &start, &end);
		gtk_text_buffer_get_iter_at_offset(buffer, &start, uri->start);
		gtk_text_buffer_insert_with_tags_by_name
				(buffer, &start, (gchar *)uri->data, 
				 strlen((gchar *)uri->data)-1,
				 "qlink", (gchar *)uri->fg_color, NULL);
		uri->end = gtk_text_iter_get_offset(&start);
		textview_make_clickable_parts_later(textview,
					  uri->start, uri->end);
		uri->q_expanded = TRUE;
	} else {
		gtk_text_buffer_get_iter_at_offset(buffer, &start, uri->start);
		gtk_text_buffer_get_iter_at_offset(buffer, &end,   uri->end);
		textview_remove_uris_in(textview, uri->start, uri->end);
		textview_shift_uris_after(textview, uri->start, 
			strlen(" [...]\n")-g_utf8_strlen((gchar *)uri->data, -1));
		gtk_text_buffer_delete(buffer, &start, &end);
		gtk_text_buffer_get_iter_at_offset(buffer, &start, uri->start);
		gtk_text_buffer_insert_with_tags_by_name
				(buffer, &start, " [...]", -1,
				 "qlink", (gchar *)uri->fg_color, NULL);
		uri->end = gtk_text_iter_get_offset(&start);
		uri->q_expanded = FALSE;
	}
	if (textview->messageview->mainwin->cursor_count == 0) {
		textview_cursor_normal(textview);
	} else {
		textview_cursor_wait(textview);
	}
}
static gboolean textview_uri_button_pressed(GtkTextTag *tag, GObject *obj,
					    GdkEvent *event, GtkTextIter *iter,
					    TextView *textview)
{
	GdkEventButton *bevent;
	ClickableText *uri = NULL;
	char *tagname;
	gboolean qlink = FALSE;

	if (!event)
		return FALSE;

	if (event->type != GDK_BUTTON_PRESS && event->type != GDK_2BUTTON_PRESS
		&& event->type != GDK_MOTION_NOTIFY)
		return FALSE;

	uri = textview_get_uri(textview, iter, tag);
	if (!uri)
		return FALSE;

	g_object_get(G_OBJECT(tag), "name", &tagname, NULL);
	
	if (!strcmp(tagname, "qlink"))
		qlink = TRUE;

	g_free(tagname);
	
	bevent = (GdkEventButton *) event;
	
	/* doubleclick: open compose / add address / browser */
	if (qlink && event->type == GDK_BUTTON_PRESS && bevent->button != 1) {
		/* pass rightclick through */
		return FALSE;
	} else if ((event->type == (qlink ? GDK_2BUTTON_PRESS:GDK_BUTTON_PRESS) && bevent->button == 1) ||
		bevent->button == 2 || bevent->button == 3) {
		if (uri->filename && !g_ascii_strncasecmp(uri->filename, "sc://", 5)) {
			MimeView *mimeview = 
				(textview->messageview)?
					textview->messageview->mimeview:NULL;
			if (mimeview && bevent->button == 1) {
				mimeview_handle_cmd(mimeview, uri->filename, NULL, uri->data);
			} else if (mimeview && bevent->button == 2 && 
				!g_ascii_strcasecmp(uri->filename, "sc://select_attachment")) {
				mimeview_handle_cmd(mimeview, "sc://open_attachment", NULL, uri->data);
			} else if (mimeview && bevent->button == 3 && 
				!g_ascii_strcasecmp(uri->filename, "sc://select_attachment")) {
				mimeview_handle_cmd(mimeview, "sc://menu_attachment", bevent, uri->data);
			} 
			return TRUE;
		} else if (qlink && bevent->button == 1) {
			textview_toggle_quote(textview, uri, FALSE);
			return TRUE;
		} else if (!g_ascii_strncasecmp(uri->uri, "mailto:", 7)) {
			if (bevent->button == 3) {
				g_object_set_data(
					G_OBJECT(textview->mail_popup_menu),
					"menu_button", uri);
				gtk_menu_popup(GTK_MENU(textview->mail_popup_menu), 
					       NULL, NULL, NULL, NULL, 
					       bevent->button, bevent->time);
			} else {
				PrefsAccount *account = NULL;

				if (textview->messageview && textview->messageview->msginfo &&
				    textview->messageview->msginfo->folder) {
					FolderItem   *folder_item;

					folder_item = textview->messageview->msginfo->folder;
					if (folder_item->prefs && folder_item->prefs->enable_default_account)
						account = account_find_from_id(folder_item->prefs->default_account);
				}
				compose_new(account, uri->uri + 7, NULL);
			}
			return TRUE;
		} else if (g_ascii_strncasecmp(uri->uri, "file:", 5)) {
			if (bevent->button == 1 &&
			    textview_uri_security_check(textview, uri) == TRUE) 
					open_uri(uri->uri,
						 prefs_common.uri_cmd);
			else if (bevent->button == 3 && !qlink) {
				g_object_set_data(
					G_OBJECT(textview->link_popup_menu),
					"menu_button", uri);
				gtk_menu_popup(GTK_MENU(textview->link_popup_menu), 
					       NULL, NULL, NULL, NULL, 
					       bevent->button, bevent->time);
			}
			return TRUE;
		} else {
			if (bevent->button == 3 && !qlink) {
				g_object_set_data(
					G_OBJECT(textview->file_popup_menu),
					"menu_button", uri);
				gtk_menu_popup(GTK_MENU(textview->file_popup_menu), 
					       NULL, NULL, NULL, NULL, 
					       bevent->button, bevent->time);
				return TRUE;
			}
		}
	}

	return FALSE;
}

/*!
 *\brief    Check to see if a web URL has been disguised as a different
 *          URL (possible with HTML email).
 *
 *\param    uri The uri to check
 *
 *\param    textview The TextView the URL is contained in
 *
 *\return   gboolean TRUE if the URL is ok, or if the user chose to open
 *          it anyway, otherwise FALSE          
 */
static gboolean textview_uri_security_check(TextView *textview, ClickableText *uri)
{
	gchar *visible_str;
	gboolean retval = TRUE;
	GtkTextBuffer *buffer;
	GtkTextIter start, end;

	if (is_uri_string(uri->uri) == FALSE)
		return TRUE;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview->text));

	gtk_text_buffer_get_iter_at_offset(buffer, &start, uri->start);
	gtk_text_buffer_get_iter_at_offset(buffer, &end,   uri->end);

	visible_str = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

	if (visible_str == NULL)
		return TRUE;

	if (strcmp(visible_str, uri->uri) != 0 && is_uri_string(visible_str)) {
		gchar *uri_path;
		gchar *visible_uri_path;

		uri_path = get_uri_path(uri->uri);
		visible_uri_path = get_uri_path(visible_str);
		if (path_cmp(uri_path, visible_uri_path) != 0)
			retval = FALSE;
	}

	if (retval == FALSE) {
		gchar *msg;
		AlertValue aval;

		msg = g_markup_printf_escaped(_("The real URL is different from "
						"the displayed URL.\n"
						"\n"
						"<b>Displayed URL:</b> %s\n"
						"\n"
						"<b>Real URL:</b> %s\n"
						"\n"
						"Open it anyway?"),
				       	       visible_str,uri->uri);
		aval = alertpanel_full(_("Phishing attempt warning"), msg,
				       GTK_STOCK_CANCEL, _("_Open URL"), NULL, FALSE,
				       NULL, ALERT_WARNING, G_ALERTDEFAULT);
		g_free(msg);
		if (aval == G_ALERTALTERNATE)
			retval = TRUE;
	}

	g_free(visible_str);

	return retval;
}

static void textview_uri_list_remove_all(GSList *uri_list)
{
	GSList *cur;

	for (cur = uri_list; cur != NULL; cur = cur->next) {
		if (cur->data) {
			g_free(((ClickableText *)cur->data)->uri);
			g_free(((ClickableText *)cur->data)->filename);
			if (((ClickableText *)cur->data)->is_quote) {
				g_free(((ClickableText *)cur->data)->fg_color);
				g_free(((ClickableText *)cur->data)->data); 
				/* (only free data in quotes uris) */
			}
			g_free(cur->data);
		}
	}

	g_slist_free(uri_list);
}

static void open_uri_cb (TextView *textview, guint action, void *data)
{
	ClickableText *uri = g_object_get_data(G_OBJECT(textview->link_popup_menu),
					   "menu_button");
	if (uri == NULL)
		return;

	if (textview_uri_security_check(textview, uri) == TRUE) 
		open_uri(uri->uri,
			 prefs_common.uri_cmd);
	g_object_set_data(G_OBJECT(textview->link_popup_menu), "menu_button",
			  NULL);
}

static void open_image_cb (TextView *textview, guint action, void *data)
{
	ClickableText *uri = g_object_get_data(G_OBJECT(textview->file_popup_menu),
					   "menu_button");

	gchar *cmd = NULL;
	gchar buf[1024];
	const gchar *p;
	gchar *filename = NULL;
	gchar *tmp_filename = NULL;

	if (uri == NULL)
		return;

	if (uri->filename == NULL)
		return;
	
	filename = g_strdup(uri->filename);
	
	if (!g_utf8_validate(filename, -1, NULL)) {
		gchar *tmp = conv_filename_to_utf8(filename);
		g_free(filename);
		filename = tmp;
	}

	subst_for_filename(filename);

	tmp_filename = g_filename_from_uri(uri->uri, NULL, NULL);
	copy_file(tmp_filename, filename, FALSE);
	g_free(tmp_filename);

	cmd = mailcap_get_command_for_type("image/jpeg", filename);
	if (cmd == NULL) {
		gboolean remember = FALSE;
		cmd = input_dialog_combo_remember
			(_("Open with"),
			 _("Enter the command line to open file:\n"
			   "('%s' will be replaced with file name)"),
			 prefs_common.mime_open_cmd,
			 prefs_common.mime_open_cmd_history,
			 TRUE, &remember);
		if (cmd && remember) {
			mailcap_update_default("image/jpeg", cmd);
		}
	}
	if (cmd && (p = strchr(cmd, '%')) && *(p + 1) == 's' &&
	    !strchr(p + 2, '%'))
		g_snprintf(buf, sizeof(buf), cmd, filename);
	else {
		g_warning("Image viewer command line is invalid: '%s'", cmd);
		return;
	}

	execute_command_line(buf, TRUE);

	g_free(filename);
	g_free(cmd);

	g_object_set_data(G_OBJECT(textview->file_popup_menu), "menu_button",
			  NULL);
}

static void save_file_cb (TextView *textview, guint action, void *data)
{
	ClickableText *uri = g_object_get_data(G_OBJECT(textview->file_popup_menu),
					   "menu_button");
	gchar *filename = NULL;
	gchar *filepath = NULL;
	gchar *filedir = NULL;
	gchar *tmp_filename = NULL;
	if (uri == NULL)
		return;

	if (uri->filename == NULL)
		return;
	
	filename = g_strdup(uri->filename);
	
	if (!g_utf8_validate(filename, -1, NULL)) {
		gchar *tmp = conv_filename_to_utf8(filename);
		g_free(filename);
		filename = tmp;
	}

	subst_for_filename(filename);
	
	if (prefs_common.attach_save_dir)
		filepath = g_strconcat(prefs_common.attach_save_dir,
				       G_DIR_SEPARATOR_S, filename, NULL);
	else
		filepath = g_strdup(filename);

	g_free(filename);

	filename = filesel_select_file_save(_("Save as"), filepath);
	if (!filename) {
		g_free(filepath);
		return;
	}

	if (is_file_exist(filename)) {
		AlertValue aval;
		gchar *res;
		
		res = g_strdup_printf(_("Overwrite existing file '%s'?"),
				      filename);
		aval = alertpanel(_("Overwrite"), res, GTK_STOCK_CANCEL, 
				  GTK_STOCK_OK, NULL);
		g_free(res);					  
		if (G_ALERTALTERNATE != aval)
			return;
	}

	tmp_filename = g_filename_from_uri(uri->uri, NULL, NULL);
	copy_file(tmp_filename, filename, FALSE);
	g_free(tmp_filename);
	
	filedir = g_path_get_dirname(filename);
	if (filedir && strcmp(filedir, ".")) {
		g_free(prefs_common.attach_save_dir);
		prefs_common.attach_save_dir = g_strdup(filedir);
	}

	g_free(filedir);
	g_free(filepath);

	g_object_set_data(G_OBJECT(textview->file_popup_menu), "menu_button",
			  NULL);
}

static void copy_uri_cb	(TextView *textview, guint action, void *data)
{
	ClickableText *uri = g_object_get_data(G_OBJECT(textview->link_popup_menu),
					   "menu_button");
	if (uri == NULL)
		return;

	gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY), uri->uri, -1);
	gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), uri->uri, -1);
	g_object_set_data(G_OBJECT(textview->link_popup_menu), "menu_button",
			  NULL);
}

static void add_uri_to_addrbook_cb (TextView *textview, guint action, void *data)
{
	gchar *fromname, *fromaddress;
	ClickableText *uri = g_object_get_data(G_OBJECT(textview->mail_popup_menu),
					   "menu_button");
	if (uri == NULL)
		return;

	/* extract url */
	fromaddress = g_strdup(uri->uri + 7);
	/* Hiroyuki: please put this function in utils.c! */
	fromname = procheader_get_fromname(fromaddress);
	extract_address(fromaddress);
	g_message("adding from textview %s <%s>", fromname, fromaddress);
	/* Add to address book - Match */
	addressbook_add_contact( fromname, fromaddress, NULL );

	g_free(fromaddress);
	g_free(fromname);
}

static void mail_to_uri_cb (TextView *textview, guint action, void *data)
{
	PrefsAccount *account = NULL;
	ClickableText *uri = g_object_get_data(G_OBJECT(textview->mail_popup_menu),
					   "menu_button");
	if (uri == NULL)
		return;

	if (textview->messageview && textview->messageview->msginfo &&
	    textview->messageview->msginfo->folder) {
		FolderItem   *folder_item;

		folder_item = textview->messageview->msginfo->folder;
		if (folder_item->prefs && folder_item->prefs->enable_default_account)
			account = account_find_from_id(folder_item->prefs->default_account);
	}
	compose_new(account, uri->uri + 7, NULL);
}

static void copy_mail_to_uri_cb	(TextView *textview, guint action, void *data)
{
	ClickableText *uri = g_object_get_data(G_OBJECT(textview->mail_popup_menu),
					   "menu_button");
	if (uri == NULL)
		return;

	gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY), uri->uri +7, -1);
	gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), uri->uri +7, -1);
	g_object_set_data(G_OBJECT(textview->mail_popup_menu), "menu_button",
			  NULL);
}

