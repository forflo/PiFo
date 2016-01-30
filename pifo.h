/*
 * LaTeX.h
 * pidgin-latex plugin
 *
 * This a plugin for Pidgin to display LaTeX formula in conversation
 *
 * PLEASE, send any comment, bug report, etc. to the trackers at sourceforge.net
 *
 * Copyright (C) 2006-2011 Benjamin Moll (qjuh@users.sourceforge.net)
 * some portions : Copyright (C) 2004-2006 Nicolas Schoonbroodt (nicolas@ffsa.be)
 *                 Copyright (C) 2004-2006 GRIm@ (thegrima@altern.org).
 *                 Copyright (C) 2004-2006 Eric Betts (bettse@onid.orst.edu).
 *                 Copyright (C) 2008-2009 Martin Keßler (martin@moegger.de).
 * Windows port  : Copyright (C) 2005-2006 Nicolai Stange (nic-stange@t-online.de)
 * Other portions heavily inspired and copied from gaim sources
 * Copyright (C) 1998-2011 Pidgin developers pidgin.im
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; This document is under the scope of
 * the version 2 of the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (see COPYING); if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA, 02110-1301, USA
 *
 */
#ifndef PIDGIN_LATEX
#define PIDGIN_LATEX

#ifndef G_GNUC_NULL_TERMINATED
#  if __GNUC__ >= 4
#    define G_GNUC_NULL_TERMINATED __attribute__((__sentinel__))
#  else
#    define G_GNUC_NULL_TERMINATED
#  endif /* __GNUC__ >= 4 */
#endif /* G_GNUC_NULL_TERMINATED */

#define PURPLE_PLUGINS

#include <pidgin/gtkplugin.h>
#include <libpurple/conversation.h>
#include <libpurple/debug.h>
#include <libpurple/signals.h>
#include <libpurple/imgstore.h>
#include <libpurple/util.h>
#include <libpurple/notify.h>
#include <libpurple/server.h>
#include <libpurple/log.h>
#include <libpurple/version.h>
#include <pidgin/gtksmiley.h>

#define IMG_BEGIN "<img id=\""
#define IMG_END "\">"
#define BEG "[tex]"
#define END "[/tex]"
#define KOPETE_END "}"
#define KOPETE_TEX_BEGIN "\\f{"
#define LISTING_TEX_BEGIN "\\l{"
#define LATEX_PLUGIN_ID "qjuh-LaTeX"
#define WEBSITE "http://sourceforge.net/projects/pidgin-latex/"
#define FILTER_AND "&amp;"
#define FILTER_LT "&lt;"
#define FILTER_GT "&gt;"
#define FILTER_BR "<br>"

#define NB_BLACKLIST (42)
#define BLACKLIST { "\\def", "\\let", "\\futurelet", "\\newcommand", "\\renewcommand", "\\else", "\\fi", "\\write", "\\input", "\\include", "\\chardef", "\\catcode", "\\makeatletter", "\\noexpand", "\\toksdef", "\\every", "\\errhelp", "\\errorstopmode", "\\scrollmode", "\\nonstopmode", "\\batchmode", "\\read", "\\csname", "\\newhelp", "\\relax", "\\afterground", "\\afterassignment", "\\expandafter", "\\noexpand", "\\special", "\\command", "\\loop", "\\repeat", "\\toks", "\\output", "\\line", "\\mathcode", "\\name", "\\item", "\\section", "\\mbox", "\\DeclareRobustCommand" }

static const char *str_replace(const char *orig, const char *rep, const char *with);
static GString *modify_message(const GString *message);
static gboolean is_blacklisted(const char *message);
static void open_log(PurpleConversation *conv);
static gboolean contains_work(const char *message);
static GPtrArray *get_commands(GString *buffer);
static GPtrArray *get_snippets(GString *buffer);
static GString *replace(const GString *original, 
        const GString *command, const GString *snippet, int id);
static int load_imgage(const GString *resulting_png);
static gboolean free_commands(const GPtrArray *commands);
static gboolean free_snippets(const GPtrArray *commands);
static GString *modify_message(const GString *message);
static gboolean pidgin_latex_write(PurpleConversation *conv, 
        const char *nom, const char *message, 
        PurpleMessageFlags messFlag, const char *original);
static void message_send(PurpleConversation *conv, char **buffer);
static gboolean message_receive(PurpleAccount *account, 
        const char *who, const char **buffer, 
        PurpleConversation *conv, PurpleMessageFlags flags);
static gboolean plugin_load(PurplePlugin *plugin);
static void message_send_chat(PurpleAccount *account, 
        const char **buffer, int id);
static void message_send_im(PurpleAccount *account, 
        const char *who, const char **buffer);
static gboolean plugin_unload(PurplePlugin * plugin);

#endif
