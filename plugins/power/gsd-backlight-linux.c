/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2010-2011 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU General Public License Version 2
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdlib.h>

#include "config.h"

#include "gsd-backlight-linux.h"

#include <stdlib.h>

#ifdef HAVE_GUDEV
#include <gio/gio.h>
#include <gudev/gudev.h>

#define GSD_POWER_SETTINGS_SCHEMA               "org.gnome.settings-daemon.plugins.power"

static gchar *
gsd_backlight_helper_get_name (GList *devices, const gchar *name, GsdBacklightType *type)
{
	const gchar *name_tmp, *type_tmp;
	GList *d;

	for (d = devices; d != NULL; d = d->next) {
		name_tmp = g_udev_device_get_name (d->data);
		if (g_strcmp0 (name_tmp, name) == 0) {
			if (type != NULL) {
				type_tmp = g_udev_device_get_sysfs_attr (d->data, "type");
				if (g_strcmp0 (type_tmp, "firmware") == 0)
					*type = GSD_BACKLIGHT_TYPE_FIRMWARE;
				else if (g_strcmp0 (type_tmp, "platform") == 0)
					*type = GSD_BACKLIGHT_TYPE_PLATFORM;
				else
					*type = GSD_BACKLIGHT_TYPE_RAW;
			}
			return g_strdup (g_udev_device_get_sysfs_path (d->data));
		}
	}
	return NULL;
}

static gchar *
gsd_backlight_helper_get_type (GList *devices, const gchar *type)
{
	const gchar *type_tmp;
	GList *d;

	for (d = devices; d != NULL; d = d->next) {
		type_tmp = g_udev_device_get_sysfs_attr (d->data, "type");
		if (g_strcmp0 (type_tmp, type) == 0)
			return g_strdup (g_udev_device_get_sysfs_path (d->data));
	}
	return NULL;
}
#endif /* HAVE_GUDEV */

char *
gsd_backlight_helper_get_best_backlight (GsdBacklightType *type)
{
#ifdef HAVE_GUDEV
	gchar *name = NULL;
	gchar *path = NULL;
	GList *devices;
	GUdevClient *client;
	GSettings *settings;

	settings = g_settings_new(GSD_POWER_SETTINGS_SCHEMA);
	name = g_settings_get_string(settings, "preferred-brightness-device");

	client = g_udev_client_new (NULL);
	devices = g_udev_client_query_by_subsystem (client, "backlight");
	if (devices == NULL)
		goto out;

	/* search the backlight devices and prefer the name:
	 * preferred-brightness-device */
	if (name != NULL && g_strcmp0 (name, "") != 0) {
		path = gsd_backlight_helper_get_name (devices, name, type);
		if (path != NULL)
			goto out;
	}

	/* search the backlight devices and prefer the types:
	 * firmware -> platform -> raw */
	path = gsd_backlight_helper_get_type (devices, "firmware");
	if (path != NULL) {
		if (type)
			*type = GSD_BACKLIGHT_TYPE_FIRMWARE;
		goto out;
	}
	path = gsd_backlight_helper_get_type (devices, "platform");
	if (path != NULL) {
		if (type)
			*type = GSD_BACKLIGHT_TYPE_PLATFORM;
		goto out;
	}
	path = gsd_backlight_helper_get_type (devices, "raw");
	if (path != NULL) {
		if (type)
			*type = GSD_BACKLIGHT_TYPE_RAW;
		goto out;
	}
out:
	g_free(name);
	g_object_unref (client);
	g_list_foreach (devices, (GFunc) g_object_unref, NULL);
	g_list_free (devices);
	return path;
#endif /* HAVE_GUDEV */

	return NULL;
}
