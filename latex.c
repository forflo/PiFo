/*
 * LaTeX.c
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
#include "latex.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <regex.h>
#include <sys/types.h>

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

static PurplePlugin *me;

static void open_log(PurpleConversation *conv)
{
	conv->logs = g_list_append(NULL, 
            purple_log_new(conv->type == PURPLE_CONV_TYPE_CHAT ? PURPLE_LOG_CHAT :
                PURPLE_LOG_IM, conv->name, conv->account,
                conv, time(NULL), NULL));
}

/* Cuts off the file name in file leaving you with just the path.
 * The function also makes a new copy of the string on the heap. 
 */
static char* getdirname(const char const *file){
	char *s = NULL;
	char *r = NULL;
	s = strrchr(file, G_DIR_SEPARATOR);

    /* Is file just a pure filename without dir? */
	if (!s) { 			
        /* Here is no standard-, but GNU-bahaviour of getcwd assumed.
		   Note that msdn.microsoft.com defines the same as GNU. */
	    return getcwd(NULL, 0);
	}

    /* Get the G_DIR_SEPARATOR at the end of directory-string */
	s += 1; 	
    r = malloc(s - file + sizeof(char));
	if (r){
		memcpy(r, file, s - file);
		r[(s - file) / sizeof(char)] = '\0';
	}
	return r;
}

/* Cuts off the path part of file leaving you with just the
 * filename. The Function also generates a new string on the heap.
 */
static char* getfilename(const char const *file){
	char *s = NULL;
	char *r = NULL;
	s = strrchr(file, G_DIR_SEPARATOR);

    /* Is file just a pure filename without dir? */
	if (!s) {
		r = malloc((strlen(file) + 1) * sizeof(char));
		strcpy(r, file);
		return r;
	}

	s += 1;
	r = malloc((strlen(file) + 1) * sizeof(char) + file - s);
	if (r) {
		memcpy(r, s, strlen(file) * sizeof(char) + file - s);
		r[strlen(file) + (file - s) / sizeof(char)] = '\0';
	}
	return r;
}

/* Helper function for command execution */
static int execute(const char *prog, char * const cmd[]){
	int i = 0;
	int exitcode = -1, exitstatus;
	pid_t child_id = 0;

	purple_debug_misc("[execute()] LaTeX", "'%s' started\n", cmd[0]);

    child_id = fork();
	switch (child_id) {
        case 0: 
            /* In child */
		    exitcode = execvp(prog, cmd);
		    exit(exitcode);
            break;
        case -1:
            purple_debug_error("LaTeX", 
                    "[execute()] Error while executing '%s'", 
                    "Could not fork");

            return exitcode;
            break;
        default:
            /* In parent. Nothing to do */
            break;
	}

	if (wait(&exitstatus) > 0) {
		if (WIFEXITED(exitstatus)) {
			exitcode = WEXITSTATUS(exitstatus);
			purple_debug_info("LaTeX", 
                    "[execute()] '%s' ended normally with exitcode '%d'\n", 
                    prog, exitcode);
		} else {
			purple_debug_error("LaTeX", 
                    "[execute()] '%s' ended abnormally via the signal '%d'\n", 
                    prog, WTERMSIG(exitstatus));
        }
	} else {
		purple_debug_error("LaTeX", 
                "[execute()] While executing '%s' the following error occured: '%s'(%d)\n", 
                prog, strerror(errno), errno);
	}

	return exitcode;
}

static gboolean is_blacklisted(const char *message){
	char *not_secure[NB_BLACKLIST] = BLACKLIST;
	int reti;
	int i;

	for (i = 0; i < NB_BLACKLIST; i++) {
		regex_t regex;
		char *begin_not_secure = 
            malloc((strlen(not_secure[i]) + 18) * sizeof(char));
		strcpy(begin_not_secure, "\\\\begin\\W*{\\W*");
		strcat(begin_not_secure, not_secure[i] + 0x01);
		strcat(begin_not_secure, "\\W*}");
		reti = regcomp(&regex, begin_not_secure, 0);
		reti = regexec(&regex, message, 0, NULL, 0);
		regfree(&regex);
		if (strstr(message, not_secure[i]) != NULL || 
                reti != REG_NOMATCH) return TRUE;

        free(begin_not_secure);
	}

	return FALSE;
}

static gboolean latex_to_image(const char *latex_expression, 
        char **filename_png, enum format format){
    FILE *transcript_file;
    FILE *temp;

    char *filename_temp = NULL;
    char *dirname_temp = NULL;

    char *file_tex = NULL;
    char *file_dvi = NULL;

    char fgcolor[16], bgcolor[16];
    gboolean exec_ok;
    
    /* The following creats temporary files */
    temp = purple_mkstemp(&filename_temp,TRUE);
    unlink(filename_temp);
    fclose(temp);

    file_tex = malloc((strlen(filename_temp) + 5) * sizeof(char));
    file_dvi = malloc((strlen(filename_temp) + 5) * sizeof(char));
    *filename_png = malloc((strlen(filename_temp) + 5) * sizeof(char));

    if(!(filename_temp && file_tex && file_dvi && *filename_png))
    {
        purple_notify_error(me, "LaTeX", 
                "Error while running LaTeX!", 
                "Couldn't create temporary files.");
        goto error;
    }
    
    /* Create filenames based on filename_temp */
    strcpy(file_tex, filename_temp);
    strcat(file_tex, ".tex");
    strcpy(file_dvi, filename_temp);
    strcat(file_dvi, ".dvi");
    strcpy(*filename_png, filename_temp);
    strcat(*filename_png, ".png");

    /* Gather some information about the current pidgin settings
     * so that we can populate the latex template file appropriately */
	if (!strcmp(purple_prefs_get_string("/pidgin/conversations/fgcolor"), "")) {
		strcpy(fgcolor, "0,0,0");
	} else {
		int rgb;
		char const *pidgin_fgcolor;
		pidgin_fgcolor = purple_prefs_get_string("/pidgin/conversations/fgcolor");
		purple_debug_info("LaTeX", "found foregroundcolor '%s'\n", pidgin_fgcolor);
		rgb = strtol(pidgin_fgcolor + 1, NULL, 16);
		purple_debug_info("LaTeX", "numerical: %d\n", rgb);
		sprintf(fgcolor, "%d,%d,%d", rgb >> 16, (rgb >> 8) & 0xff, rgb & 0xff);
	}
	purple_debug_info("LaTeX", "Using '%s' for foreground\n", fgcolor);

	if (!strcmp(purple_prefs_get_string("/pidgin/conversations/bgcolor"), "")) {
		strcpy(bgcolor, "255,255,255");
	} else {
		int rgb;
		char const *pidgin_bgcolor;
		pidgin_bgcolor = purple_prefs_get_string("/pidgin/conversations/bgcolor");
		purple_debug_info("LaTeX", "found backgroundcolor '%s'\n", pidgin_bgcolor);
		rgb = strtol(pidgin_bgcolor + 1, NULL, 16);
		purple_debug_info("LaTeX", "numerical: %d\n", rgb);
		sprintf(bgcolor, "%d,%d,%d", rgb >> 16, (rgb >> 8) & 0xff, rgb & 0xff);
	}
	purple_debug_info("LaTeX", "Using '%s' for background\n", bgcolor);

	if (!(transcript_file = fopen(file_tex, "w"))) 
        goto error;

    /* Generate latex template file */
    switch (format) {
        case FORMULA:
	        fprintf(transcript_file, HEADER HEADER_FCOLOR "{%s}" 
                    HEADER_BCOLOR "{%s}" HEADER_DOC BEG_MATH "%s" END_MATH
                    FOOTER, fgcolor, bgcolor, latex_expression);
            break;
        case LISTING:
	        fprintf(transcript_file, HEADER HEADER_FCOLOR "{%s}" 
                    HEADER_BCOLOR "{%s}" HEADER_DOC BEG_LISTING "\n%s\n" END_LISTING
                    FOOTER, fgcolor, bgcolor, latex_expression);
            break;
        default:
            goto error;
            break;
    }
	fclose(transcript_file);

	dirname_temp = getdirname(file_tex);

	if (!dirname_temp || chdir(dirname_temp)) {
		if (dirname_temp) 
            free(dirname_temp);

		purple_notify_error(me, "LaTeX", 
                "Error while trying to transcript LaTeX!", 
                "Couldn't cange to temporary directory");

        goto error;
	}

    /* Make sure that latex cannot do shell escape, even
     * if the local default config says so! */
	char * const latexopts[] = { 
        "latex", "--no-shell-escape", "--interaction=nonstopmode", 
        file_tex, NULL
    };

	char * const dvipngopts[] = { 
        "dvipng", "-Q", "10", "-T", 
        "tight", "--follow", "-o", 
        *filename_png, file_dvi, NULL
    };

    /* Start Latex and dvipng */
	exec_ok = !(execute("latex", latexopts) || 
            execute("dvipng", dvipngopts));

	purple_debug_info("LaTeX", 
            "Image creation exited with status '%d'\n", 
            !exec_ok);

    goto out;

error:
    unlink(file_tex);
    unlink(file_dvi);
    unlink(*filename_png);

    if (file_tex) free(file_tex);
    if (file_dvi) free(file_dvi);
    if (*filename_png) free(*filename_png);
    if (filename_temp) free(filename_temp);
    if (dirname_temp) free(dirname_temp);

    *filename_png = NULL;

    return FALSE;

out:
    unlink(file_tex);
    unlink(file_dvi);

    /* Remove latex intermediate files */
	file_tex[strlen(file_tex) - 4] = '\0';
	strcat(file_tex, ".aux");
	unlink(file_tex);
	file_tex[strlen(file_tex) - 4] = '\0';
	strcat(file_tex, ".log");
	unlink(file_tex);

    if (file_tex) free(file_tex);
    if (file_dvi) free(file_dvi);
    if (filename_temp) free(filename_temp);
    if (dirname_temp) free(dirname_temp);

    return TRUE;
}

static enum format get_format(const char *message){
    if (strstr(message, LISTING_TEX_BEGIN) != NULL)
        return LISTING;
    if (strstr(message, KOPETE_TEX_BEGIN) != NULL)
        return FORMULA;
    return NONE;
}

static const char *get_format_string(enum format fmt){
    return format_table[fmt];
}

//TODO check for mem leaks
static gboolean analyse(char **tmp2){
    enum format format = get_format(*tmp2);
    const char *startdelim = get_format_string(format);
    const char *enddelim = KOPETE_END;

	int pos1, pos2, idimg, formulas = 0;
	char *ptr1, *ptr2;
	char *file_png = NULL;

	ptr1 = strstr(*tmp2, startdelim);

	while (ptr1 != NULL) {
		char *tex, *tex2, *message, *filter, *idstring, *shortcut;
		size_t size;
		gchar *filedata;
		GError *error = NULL;

		pos1 = strlen(*tmp2) - strlen(ptr1);

		/* Have to ignore the first 2 char ("$$") --> & [2] */
		ptr2 = strstr(&ptr1[strlen(startdelim)], enddelim);
		if (ptr2 == NULL)
			return FALSE;

		pos2 = strlen(*tmp2) - strlen(ptr2) + strlen(enddelim);

		tex = malloc(pos2 - pos1 - strlen(enddelim) - strlen(startdelim) + 1);
		if (tex == NULL) {
			purple_notify_error(me, "LaTeX", 
                    "Error while analysing the message!", 
                    "Out of memory!");
			return FALSE;
		}

		strncpy(tex, &ptr1[strlen(startdelim)], 
                pos2 - pos1 - strlen(startdelim) - strlen(enddelim));
		tex[pos2 - pos1 - strlen(startdelim) - strlen(enddelim)] = '\0';

		tex2 = malloc((strlen(tex) + 1) * sizeof(char));
		strcpy(tex2, tex);
		tex = purple_unescape_html(tex2);

		purple_debug_misc("LaTeX", "Found LaTeX-Code: %s\n", tex);

		shortcut = malloc((strlen(tex2) + 5) * sizeof(char));
		strcpy(shortcut, "$$");
		strcat(shortcut, tex2);
		strcat(shortcut, "$$");
		free(tex2);

		/* Creates the image in file_png */
		if (!latex_to_image(tex, &file_png, format)) {
			free(tex);
			free(shortcut);
			return FALSE;
		}

		free(tex);

        /* Loading image */
		if (!g_file_get_contents(file_png, &filedata, &size, &error)) {
			purple_notify_error(me, "LaTeX", 
                    "Error while reading the generated image!", error->message);
			g_error_free(error);
			free(shortcut);
			return FALSE;
		}

		idimg = purple_imgstore_add_with_id(filedata, 
                MAX(1024, size), getfilename(file_png));


		unlink(file_png);
		free(file_png);

		if (idimg == 0) {
			purple_notify_error(me, "LaTeX", 
                    "Error while reading the generated image!", 
                    "Failed to store image.");
			free(shortcut);
			return FALSE;
		}

		free(shortcut);
		idstring = malloc(10);
		sprintf(idstring, "%d\0", idimg);

		/* making new message */
		message = malloc(strlen(*tmp2) - pos2 + pos1 + 
                strlen(idstring) + strlen(IMG_BEGIN) + strlen(IMG_END) + 1);
		if (message  == NULL) {
			purple_notify_error(me, "LaTeX", 
                    "Error while composing the message!", 
                    "Couldn't make the message.");
			free(idstring);
			return FALSE;
		}

		if (pos1 > 0) {
			strncpy(message, *tmp2, pos1);
			message[pos1] = '\0';
			strcat(message, IMG_BEGIN);
		} else {
			strcpy(message, IMG_BEGIN);
        }

		strcat(message, idstring);
		strcat(message, IMG_END);
		free(idstring);

		if (pos2 < strlen(*tmp2))
			strcat(message, &ptr2[strlen(enddelim)]);

		free(*tmp2);
		if ((*tmp2 = malloc(strlen(message) + 1)) == NULL) {
			purple_notify_error(me, "LaTeX", 
                    "Error while composing the message!", 
                    "Couldn't split the message.");
			return FALSE;
		}

		strcpy(*tmp2, message);
		free(message);

		ptr1 = strstr(&(*tmp2)[pos2], startdelim);
	}

    return TRUE;
}

static gboolean pidgin_latex_write(PurpleConversation *conv, 
        const char *nom, const char *message, 
        PurpleMessageFlags messFlag, const char *original){

	gboolean logflag = purple_conversation_is_logging(conv);

	if (logflag) {
		GList *log;

		if (conv->logs == NULL)
			open_log(conv);

		log = conv->logs;
		while (log != NULL) {
			purple_log_write((PurpleLog*)log->data, 
                    messFlag, nom, time(NULL), original);
			log = log->next;
		}
		purple_conversation_set_logging(conv, FALSE);
	}

    /* Write trimmed message. */
	if (purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_CHAT){
		purple_conv_chat_write(PURPLE_CONV_CHAT(conv), 
                nom, message, messFlag, time(NULL));
    } else if (purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_IM) {
		purple_conv_im_write(PURPLE_CONV_IM(conv), 
                nom, message, messFlag, time(NULL));
    }

	if (logflag){
		purple_conversation_set_logging(conv, TRUE);
    }

	return TRUE;
}

static void message_send(PurpleConversation *conv, const char **buffer){
	char *temp_buffer;
	gboolean smileys;

	purple_debug_info("LaTeX", "(message_send()) Sending Message: %s\n", *buffer);

	if (strstr(*buffer, KOPETE_TEX_BEGIN) == NULL &&
        strstr(*buffer, LISTING_TEX_BEGIN) == NULL) {
		return;
	}

	if (is_blacklisted(*buffer)) {
		purple_debug_info("LaTeX", 
                "Message not analysed, because it contained blacklisted code.\n");
		return;
	}

	temp_buffer = malloc(strlen(*buffer) + 1);
	if (temp_buffer == NULL) {
		purple_notify_error(me, "LaTeX", 
                "Error while analysing the message!", 
                "Out of memory!");
		return;
	}

	strcpy(temp_buffer, *buffer);
	if (analyse(&temp_buffer)) {
		*buffer = temp_buffer;
	} else {
        free(temp_buffer);
    }
}

static void message_send_chat(PurpleAccount *account, const char **buffer, int id){
	PurpleConnection *conn = purple_account_get_connection(account);
	message_send(purple_find_chat(conn, id), buffer);
}

static void message_send_im(PurpleAccount *account, const char *who, const char **buffer){
	message_send(purple_find_conversation_with_account(
                PURPLE_CONV_TYPE_IM, who, account), buffer);
}

static gboolean message_receive(PurpleAccount *account, 
        const char *who, const char **buffer, 
        PurpleConversation *conv, PurpleMessageFlags flags){

	char *temp_buffer;
	purple_debug_info("LaTeX", "[message_receive()] Writing Message: %s\n", *buffer);

	if (strstr(*buffer, KOPETE_TEX_BEGIN) == NULL &&
        strstr(*buffer, LISTING_TEX_BEGIN) == NULL) {
		return FALSE;
	}

	if (is_blacklisted(*buffer)) {
		purple_debug_info("LaTeX", 
                "Message not analysed, because it contained blacklisted code.\n");
		return FALSE;
	}

    temp_buffer = malloc(strlen(*buffer) + 1);
	if (temp_buffer == NULL) {
		purple_notify_error(me, "LaTeX", 
                "Error while analysing the message!", 
                "Out of memory!");
		return FALSE;
	}

	strcpy(temp_buffer, *buffer);

	purple_debug_info("LaTeX", "[message_receive()] Analyse: %s\n", temp_buffer);
	if (analyse(&temp_buffer)) {
		pidgin_latex_write(conv, who, temp_buffer, flags, *buffer);
		free(temp_buffer);
		return TRUE;
	}

	free(temp_buffer);
	return FALSE;
}

static gboolean plugin_load(PurplePlugin *plugin){
	void *conv_handle = purple_conversations_get_handle();

	me = plugin;
	purple_signal_connect(conv_handle, "sending-im-msg",
			      plugin, PURPLE_CALLBACK(message_send_im), NULL);

	purple_signal_connect(conv_handle, "sending-chat-msg",
			      plugin, PURPLE_CALLBACK(message_send_chat), NULL);

	purple_signal_connect(conv_handle, "writing-im-msg",
			      plugin, PURPLE_CALLBACK(message_receive), NULL);

	purple_signal_connect(conv_handle, "writing-chat-msg",
			      plugin, PURPLE_CALLBACK(message_receive), NULL);

	purple_debug_info("LaTeX", "LaTeX loaded\n");

	return TRUE;
}

static gboolean plugin_unload(PurplePlugin * plugin)
{
	void *conv_handle = purple_conversations_get_handle(); 
	purple_signal_disconnect(conv_handle, 
            "sending-im-msg", plugin, 
            PURPLE_CALLBACK(message_send_im));
	purple_signal_disconnect(conv_handle, 
            "sending-chat-msg", plugin, 
            PURPLE_CALLBACK(message_send_chat));
	purple_signal_disconnect(conv_handle, 
            "writing-im-msg", plugin, 
            PURPLE_CALLBACK(message_receive));
	purple_signal_disconnect(conv_handle, 
            "writing-chat-msg", plugin, 
            PURPLE_CALLBACK(message_receive));

	me = NULL;
	purple_debug_info("LaTeX", "LaTeX unloaded\n");

	return TRUE;
}

static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,                 /**< type           */
	PIDGIN_PLUGIN_TYPE,                     /**< ui_requirement */
	0,                                      /**< flags          */
	NULL,                                   /**< dependencies   */
	PURPLE_PRIORITY_DEFAULT,                /**< priority       */

	LATEX_PLUGIN_ID,                        /**< id             */
	"LaTeX",                                /**< name           */
	"1.4",                                  /**< version        */
	/**  summary        */
	"To display LaTeX formula into Pidgin conversation.",
	/**  description    */
	"Put LaTeX-code between $$ ... $$ markup to have it displayed as " 
    "Picture in your conversation.\nRemember that your contact needs "
    "an similar plugin or else he will just see the pure LaTeX-code\nYou "
    "must have LaTeX and dvipng installed (in your PATH)",
	"Benjamin Moll <qjuh@users.sourceforge.net>\nNicolas Schoonbroodt "
    "<nicolas@ffsa.be>\nNicolai Stange <nic-stange@t-online.de>", /**< author       */
	WEBSITE,                                /**< homepage       */
	plugin_load,                            /**< load           */
	plugin_unload,                          /**< unload         */
	NULL,                                   /**< destroy        */
	NULL,                                   /**< ui_info        */
	NULL,                                   /**< extra_info     */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static void
init_plugin(PurplePlugin *plugin) { }

PURPLE_INIT_PLUGIN(LaTeX, init_plugin, info)
