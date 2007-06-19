/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/***************************************************************************
 *
 * polkit-check-caller.c : check if a caller is privileged
 *
 * Copyright (C) 2007 David Zeuthen, <david@fubar.dk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 *
 **************************************************************************/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <errno.h>

#include <polkit-dbus/polkit-dbus.h>

#include <glib.h>

static void
usage (int argc, char *argv[])
{
	fprintf (stderr,
                 "\n"
                 "usage : polkit-check-caller\n"
                 "          --caller <dbus-name> --action <action>\n"
                 "          [--version] [--help]\n");
	fprintf (stderr,
                 "\n"
                 "        --caller         Unique name of caller on the system bus\n"
                 "        --action         Requested action\n"
                 "        --version        Show version and exit\n"
                 "        --help           Show this information and exit\n"
                 "\n"
                 "Determine if a given caller can do a given action. If access is \n"
                 "allowed, this program exits with exit code 0. If no access is allowed\n"
                 "or an error occurs, the program exits with a non-zero exit code.\n");
}

int
main (int argc, char *argv[])
{
        char *action_id = NULL;
        char *dbus_name = NULL;
        gboolean is_version = FALSE;
        DBusConnection *bus;
	DBusError error;
        PolKitContext *pol_ctx;
        PolKitCaller *caller;
        PolKitAction *action;
        gboolean allowed;
        PolKitError *p_error;
        
	if (argc <= 1) {
		usage (argc, argv);
		return 1;
	}

	while (1) {
		int c;
		int option_index = 0;
		const char *opt;
		static struct option long_options[] = {
			{"action", 1, NULL, 0},
			{"caller", 1, NULL, 0},
			{"version", 0, NULL, 0},
			{"help", 0, NULL, 0},
			{NULL, 0, NULL, 0}
		};

		c = getopt_long (argc, argv, "",
				 long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 0:
			opt = long_options[option_index].name;

			if (strcmp (opt, "help") == 0) {
				usage (argc, argv);
				return 0;
			} else if (strcmp (opt, "version") == 0) {
				is_version = TRUE;
			} else if (strcmp (opt, "action") == 0) {
				action_id = strdup (optarg);
			} else if (strcmp (opt, "caller") == 0) {
				dbus_name = strdup (optarg);
			}
			break;

		default:
			usage (argc, argv);
			return 1;
			break;
		}
	}

	if (is_version) {
		printf ("polkit-check-caller " PACKAGE_VERSION "\n");
		return 0;
	}

	if (action_id == NULL || dbus_name == NULL) {
		usage (argc, argv);
		return 1;
	}

        dbus_error_init (&error);
        bus = dbus_bus_get (DBUS_BUS_SYSTEM, &error);
        if (bus == NULL) {
		fprintf (stderr, "error: dbus_bus_get(): %s: %s\n", error.name, error.message);
		return 1;
	}

        p_error = NULL;
        pol_ctx = polkit_context_new ();
        if (!polkit_context_init (pol_ctx, &p_error)) {
		fprintf (stderr, "error: polkit_context_init: %s\n", polkit_error_get_error_message (p_error));
                polkit_error_free (p_error);
                return 1;
        }

        action = polkit_action_new ();
        polkit_action_set_action_id (action, action_id);

        caller = polkit_caller_new_from_dbus_name (bus, dbus_name, &error);
        if (caller == NULL) {
                if (dbus_error_is_set (&error)) {
                        fprintf (stderr, "error: polkit_caller_new_from_dbus_name(): %s: %s\n", 
                                 error.name, error.message);
                        return 1;
                }
        }

        allowed = polkit_context_can_caller_do_action (pol_ctx, action, caller);

        if (allowed)
                return 0;
        else
                return 1;
}
