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
#include "LaTeX.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <regex.h>
#include <sys/types.h>

#ifndef _WIN32
#include <sys/wait.h>
#endif

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



#ifdef _WIN32
void win32_purple_notify_error(char *prep)
{
	char *errmsg = NULL;
	char *finalmsg = NULL;
	if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                FORMAT_MESSAGE_FROM_SYSTEM, NULL, 
                GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
                (void*)&errmsg, 0, NULL)) {

		purple_notify_error(me, 
            "LaTeX", 
            "Can't display error message.", 
            "PidginLaTeX was unable to get the actual error message from Windows.");
		return;
	}

	if (prep) {
		finalmsg = malloc((strlen(errmsg) + strlen(prep) + 3) * sizeof(char));
		if (!finalmsg) {
			purple_notify_error(me, 
                "LaTeX", 
                "Can't display error message.", 
                "Not enough memory.");
            /* we can't do anything more for you */
			LocalFree(errmsg); 			
            return;
		}

		strcpy(finalmsg, prep);
		strcat(finalmsg, ": ");
		strcat(finalmsg, errmsg);
		LocalFree(errmsg);

	} else {
		finalmsg = malloc((strlen(errmsg) + 1) * sizeof(char));
		if (!finalmsg) {
			purple_notify_error(me, 
                    "LaTeX", 
                    "Can't display error message.", 
                    "Not enough memory.");
            /* we can't do anything more for you */
			LocalFree(errmsg); 			
            return;
		}
		strcpy(finalmsg, errmsg);
		LocalFree(errmsg);
	}
	purple_notify_error(me, "LaTeX", "Error in execution of PidginLaTex!", finalmsg);
	free(finalmsg);
	return;
}
#endif

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

char* makeString(const char const *string){
	char *result = NULL;
	result = malloc((strlen(string) + 1) * sizeof(char));
	if (result)
		strcpy(result, string);

	return result;
}

/* Helper function for command execution */
static int execute(char *cmd, char *opts[], int copts){
	int i = 0;
	int exitcode = -1, exitstatus;
	char **opt = NULL;
	char *message = NULL;
	pid_t child_id = 0;

	opt = malloc((copts + 2) * sizeof(char*));

	opt[0] = cmd;
	for (i = 0; i < copts; i++) {
		opt[i + 1] = *(opts + i);
	}
	opt[copts + 1] = NULL;

	purple_debug_misc("LaTeX", "'%s' started\n", cmd);

	child_id = fork();
	switch (child_id) {
        case 0: 
            /* In child */
		    exitcode = execvp(cmd, opt);
		    free(opt);
		    exit(exitcode);
            break;
        case -1:
            purple_debug_error("LaTeX", 
                    "Error while executing '%s'", 
                    "Could not fork");

            return exitcode;
            break;
        default:
            /* In parent. Nothing to do */
            break;
	}

	if (wait(&exitstatus) > 0) {
		free(opt);
		if (WIFEXITED(exitstatus)) {
			exitcode = WEXITSTATUS(exitstatus);
			purple_debug_info("LaTeX", 
                    "'%s' ended normally with exitcode '%d'\n", 
                    cmd, exitcode);
		} else {
			purple_debug_error("LaTeX", 
                    "'%s' ended abnormally via the signal '%d'\n", 
                    cmd, WTERMSIG(exitstatus));
        }
	} else {
		purple_debug_error("LaTeX", 
                "While executing '%s' the following error occured: '%s'(%d)\n", 
                cmd, strerror(errno), errno);
	}

	return exitcode;
}

static char* get_latex_cmd(void){
	return makeString("latex");
}

static char* get_dvips_cmd(void){
	return makeString("dvips");
}

static char* get_dvipng_cmd(void){
	return makeString("dvipng");
}

static char* get_convert_cmd(void){
	return makeString("convert");
}

static gboolean is_blacklisted(char *message){
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
	}
	return FALSE;
}

static gboolean latex_to_image(char *latex, char **filename_png){
    FILE *transcript_file;
    FILE *temp;

    char *filename_temp = NULL;
    char *dirname_temp = NULL;

    char *file_tex;
    char *file_dvi;

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
    
    strcpy(file_tex, filename_temp);
    strcat(file_tex, ".tex");
    strcpy(file_dvi, filename_temp);
    strcat(file_dvi, ".dvi");
    strcpy(*filename_png, filename_temp);
    strcat(*filename_png, ".png");
    free(filename_temp);

    /* Gather some information about the current pidgin settings
     * so that we can populate the latex template file appropriately */
	if (!strcmp(purple_prefs_get_string("/pidgin/conversations/fgcolor"), "")) {
		strcpy(fgcolor, "0,0,0");
	} else {
		char const *pidgin_fgcolor;
		int rgb;
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
		char const *pidgin_bgcolor;
		int rgb;
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
	fprintf(transcript_file, HEADER "%s" 
            HEADER_COLOR "%s" HEADER_MATH "%s" 
            FOOTER_MATH FOOTER, fgcolor, bgcolor, latex);

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
	char *latexopts[2] = { 
        "--no-shell-escape --interaction=nonstopmode", 
        file_tex 
    };

	char *dvipngopts[8] = { 
        "-Q", "10", "-T", 
        "tight", "--follow", "-o", 
        *filename_png, file_dvi 
    };

    /* Start Latex and dvipng */
	exec_ok = !(execute(get_latex_cmd(), latexopts, 2) || 
            execute(get_dvipng_cmd(), dvipngopts, 8));

	purple_debug_info("LaTeX", 
            "Image creation exited with status '%d'\n", 
            !exec_ok);

    goto out;

error:
    unlink(file_tex);
    unlink(file_dvi);
    unlink(*filename_png);

    free(file_tex);
    free(file_dvi);
    free(*filename_png);

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

    free(file_tex);
    free(file_dvi);
    free(dirname_temp);

    return TRUE;
}

static gboolean analyse(char **tmp2, char *startdelim, char *enddelim, gboolean smileys)
{
	int pos1, pos2, idimg, formulas = 0;
	char *ptr1, *ptr2;
	char *file_png = NULL;
	PurpleSmiley *the_smiley;

	purple_debug_misc("LaTeX", "starting to analyse the message: %s\n", *tmp2);

	ptr1 = strstr(*tmp2, startdelim);
	while (ptr1 != NULL) {
		char *tex, *tex2, *message, *filter, *idstring, *shortcut;
		gchar *filedata;
		size_t size;
		GError *error = NULL;

		pos1 = strlen(*tmp2) - strlen(ptr1);

		// Have to ignore the first 2 char ("$$") --> & [2]
		ptr2 = strstr(&ptr1[strlen(startdelim)], enddelim);
		if (ptr2 == NULL)
			return FALSE;

		pos2 = strlen(*tmp2) - strlen(ptr2) + strlen(enddelim);

		if ((tex = malloc(pos2 - pos1 - strlen(enddelim) - strlen(startdelim) + 1)) == NULL) {
			// Report the error
			purple_notify_error(me, "LaTeX", "Error while analysing the message!", "Out of memory!");
			return FALSE;
		}

		strncpy(tex, &ptr1[strlen(startdelim)], pos2 - pos1 - strlen(startdelim) - strlen(enddelim));
		tex[pos2 - pos1 - strlen(startdelim) - strlen(enddelim)] = '\0';

		// Replaced. Using official libpurple-function instead
		/*/ Pidgin transforms & to &amp; and I make the inverse transformation
		   while ( (filter = strstr(tex, FILTER_AND) ) != NULL)
		   strcpy(&tex[strlen(tex) - strlen(filter) + 1], &filter[5]);

		   // Pidgin transforms < to &lt
		   while ( (filter = strstr(tex, FILTER_LT) ) != NULL)
		   {
		   strcpy(&tex[strlen(tex) - strlen(filter)], "<");
		   strcpy(&tex[strlen(tex) - strlen(filter) + 1], &filter[4]);
		   }

		   // Pidgin transforms > to &gt
		   while ( (filter = strstr(tex, FILTER_GT) ) != NULL)
		   {
		   strcpy(&tex[strlen(tex) - strlen(filter)], ">");
		   strcpy(&tex[strlen(tex) - strlen(filter) + 1], &filter[4]);
		   }

		   // <br> filter
		   while ( (filter = strstr(tex, FILTER_BR) ) != NULL)
		   strcpy(&tex[strlen(tex) - strlen(filter)], &filter[4]);//*/

		tex2 = (char*)malloc((strlen(tex) + 1) * sizeof(char));
		strcpy(tex2, tex);
		tex = purple_unescape_html(tex2);

		purple_debug_misc("LaTeX", "Found LaTeX-Code: %s\n", tex);

		shortcut = (char*)malloc((strlen(tex2) + 5) * sizeof(char));
		strcpy(shortcut, "$$");
		strcat(shortcut, tex2);
		strcat(shortcut, "$$");
		free(tex2);

		if (smileys && purple_smileys_find_by_shortcut(shortcut) != NULL) {
			free(tex);
			free(shortcut);
			ptr1 = strstr(&(*tmp2)[pos2], startdelim);
			continue;
		}

		// Creates the image in file_png
		if (!latex_to_image(tex, &file_png)) {
			free(tex);
			free(shortcut);
			return FALSE;
		}

		free(tex);

		// loading image
		if (!g_file_get_contents(file_png, &filedata, &size, &error)) {
			purple_notify_error(me, "LaTeX", "Error while reading the generated image!", error->message);
			g_error_free(error);
			free(shortcut);
			return FALSE;
		}

		unlink(file_png);

		idimg = purple_imgstore_add_with_id(filedata, MAX(1024, size), getfilename(file_png));
		filedata = NULL;
		free(file_png);

		if (idimg == 0) {
			purple_notify_error(me, "LaTeX", "Error while reading the generated image!", "Failed to store image.");
			free(shortcut);
			return FALSE;
		}

		if (smileys) {
			the_smiley = purple_smiley_new(purple_imgstore_find_by_id(idimg), shortcut);
			pidgin_smiley_add_to_list(the_smiley);
			free(shortcut);

			// making new message
			if ((message = malloc(strlen(*tmp2) + 1)) == NULL) {
				purple_notify_error(me, "LaTeX", "Error while composing the message!", "couldn't make the message.");
				return FALSE;
			}

			ptr1 = strstr(&(*tmp2)[pos2], startdelim);
			continue;
		}else  {
			free(shortcut);
			idstring = malloc(10);
			sprintf(idstring, "%d\0", idimg);

			// making new message
			if ((message = malloc(strlen(*tmp2) - pos2 + pos1 + strlen(idstring) + strlen(IMG_BEGIN) + strlen(IMG_END) + 1)) == NULL) {
				purple_notify_error(me, "LaTeX", "Error while composing the message!", "couldn't make the message.");
				free(idstring);
				return FALSE;
			}

			if (pos1 > 0) {
				strncpy(message, *tmp2, pos1);
				message[pos1] = '\0';
				strcat(message, IMG_BEGIN);
			} else
				strcpy(message, IMG_BEGIN);

			strcat(message, idstring);
			strcat(message, IMG_END);
			free(idstring);
		}

		if (pos2 < strlen(*tmp2))
			strcat(message, &ptr2[strlen(enddelim)]);

		free(*tmp2);
		if ((*tmp2 = malloc(strlen(message) + 1)) == NULL) {
			purple_notify_error(me, "LaTeX", "Error while composing the message!", "couldn't split the message.");
			return FALSE;
		}

		strcpy(*tmp2, message);
		free(message);

		formulas++;

		ptr1 = strstr(&(*tmp2)[pos2], startdelim);
	}
	if (!smileys && formulas > 0)
		return TRUE;
	else
		return FALSE;
}

static gboolean pidgin_latex_write(PurpleConversation *conv, const char *nom, char *message, PurpleMessageFlags messFlag, char *original)
{
	gboolean logflag;

	// writing log
	logflag = purple_conversation_is_logging(conv);

	if (logflag) {
		GList *log;

		if (conv->logs == NULL)
			open_log(conv);

		log = conv->logs;
		while (log != NULL) {
			if (strcmp(purple_prefs_get_string("/purple/logging/format"), "html") == 0)
				purple_log_write((PurpleLog*)log->data, messFlag, nom, time(NULL), message);
			else
				purple_log_write((PurpleLog*)log->data, messFlag, nom, time(NULL), original);
			log = log->next;
		}
		purple_conversation_set_logging(conv, FALSE);
	}

	if (purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_CHAT)
		purple_conv_chat_write(PURPLE_CONV_CHAT(conv), nom, message, messFlag, time(NULL));
	else if (purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_IM)
		purple_conv_im_write(PURPLE_CONV_IM(conv), nom, message, messFlag, time(NULL));

	if (logflag)
		purple_conversation_set_logging(conv, TRUE);

	return FALSE;
}

static void message_send(PurpleConversation *conv, char **buffer)
{
	char *tmp2;
	gboolean smileys;

	purple_debug_info("LaTeX", "Sending Message: %s\n", *buffer);

	// if nothing to do
	if (strstr(*buffer, KOPETE_TEX) == NULL) {
		return;
	}

	if (is_blacklisted(*buffer)) {
		purple_debug_info("LaTeX", "message not analysed, because it contained blacklisted code.\n");
		return;
	}

	if ((tmp2 = malloc(strlen(*buffer) + 1)) == NULL) {
		// Notify Error
		purple_notify_error(me, "LaTeX", "Error while analysing the message!", "Out of memory!");
		return;
	}

	smileys = purple_conversation_get_features(conv) & PURPLE_CONNECTION_ALLOW_CUSTOM_SMILEY;

	purple_debug_misc("LaTeX", "smiley support: %s\n", smileys ? "yes" : "no");

	if (smileys) {
		strcpy(tmp2, *buffer);

		if (analyse(&tmp2, KOPETE_TEX, KOPETE_TEX, smileys)) {
			free(*buffer);
			*buffer = tmp2;
		}else  {
			free(tmp2);
		}
	}
}

static void message_send_chat(PurpleAccount *account, char **buffer, int id)
{
	PurpleConnection *conn = purple_account_get_connection(account);
	message_send(purple_find_chat(conn, id), buffer);
}

static void message_send_im(PurpleAccount *account, const char *who, char **buffer)
{
	message_send(purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM, who, account), buffer);
}

static gboolean message_receive(PurpleAccount *account, const char *who, char **buffer, PurpleConversation *conv, PurpleMessageFlags flags)
{
	char *tmp2;
//  gboolean smileys;

	purple_debug_info("LaTeX", "Writing Message: %s\n", *buffer);

	// if nothing to do
	if (strstr(*buffer, KOPETE_TEX) == NULL) {
		return FALSE;
	}

	if (is_blacklisted(*buffer)) {
		purple_debug_info("LaTeX", "message not analysed, because it contained blacklisted code.\n");
		return FALSE;
	}

	if ((tmp2 = malloc(strlen(*buffer) + 1)) == NULL) {
		// Notify Error
		purple_notify_error(me, "LaTeX", "Error while analysing the message!", "Out of memory!");
		return FALSE;
	}

	strcpy(tmp2, *buffer);

//  smileys = purple_conversation_get_features(conv) & PURPLE_CONNECTION_ALLOW_CUSTOM_SMILEY;

//  purple_debug_misc("LaTeX", "smiley support: %s\n", smileys ? "yes" : "no");

	if (analyse(&tmp2, KOPETE_TEX, KOPETE_TEX, FALSE)) {
		pidgin_latex_write(conv, who, tmp2, flags, *buffer);

		free(tmp2);
		return TRUE;
	}

	free(tmp2);

	return FALSE;
}

static gboolean plugin_load(PurplePlugin *plugin)
{
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

	purple_signal_disconnect(conv_handle, "sending-im-msg", plugin, PURPLE_CALLBACK(message_send_im));
	purple_signal_disconnect(conv_handle, "sending-chat-msg", plugin, PURPLE_CALLBACK(message_send_chat));
	purple_signal_disconnect(conv_handle, "writing-im-msg", plugin, PURPLE_CALLBACK(message_receive));
	purple_signal_disconnect(conv_handle, "writing-chat-msg", plugin, PURPLE_CALLBACK(message_receive));

	me = NULL;

	purple_debug_info("LaTeX", "LaTeX unloaded\n");

	return TRUE;
}


static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,                       /**< type           */
	PIDGIN_PLUGIN_TYPE,                                       /**< ui_requirement */
	0,                                          /**< flags          */
	NULL,                                       /**< dependencies   */
	PURPLE_PRIORITY_DEFAULT,                      /**< priority       */

	LATEX_PLUGIN_ID,                            /**< id             */
	"LaTeX",                                /**< name           */
	"1.4",                                  /**< version        */
	/**  summary        */
	"To display LaTeX formula into Pidgin conversation.",
	/**  description    */
	"Put LaTeX-code between $$ ... $$ markup to have it displayed as Picture in your conversation.\nRemember that your contact needs an similar plugin or else he will just see the pure LaTeX-code\nYou must have LaTeX and dvipng installed (in your PATH)",
	"Benjamin Moll <qjuh@users.sourceforge.net>\nNicolas Schoonbroodt <nicolas@ffsa.be>\nNicolai Stange <nic-stange@t-online.de>", /**< author       */
	WEBSITE,                                    /**< homepage       */
	plugin_load,                                /**< load           */
	plugin_unload,                              /**< unload         */
	NULL,                                       /**< destroy        */
	NULL,                                       /**< ui_info        */
	NULL,                                       /**< extra_info     */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};



static void
init_plugin(PurplePlugin *plugin)
{
}

PURPLE_INIT_PLUGIN(LaTeX, init_plugin, info)
