/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 2004 Hiroyuki Yamamoto & the Sylpheed-Claws team
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

typedef struct GPGConfig GPGConfig;

struct GPGConfig
{
	gboolean auto_check_signatures;
	gboolean store_passphrase;
	gint store_passphrase_timeout;
	gboolean passphrase_grab;
	gboolean gpg_warning;
};

void prefs_gpg_init(void);
void prefs_gpg_done(void);
void prefs_gpg_save_config(void);
struct GPGConfig *prefs_gpg_get_config();
