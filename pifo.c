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
#define DEBUG

#include "pifo.h"
#include "pifo_util.h"
#include "pifo_generator.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <regex.h>
#include <sys/types.h>

#ifndef MAX
#   define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

PurplePlugin *me;

gboolean contains_work(const char *message){
    if (strstr(message, INTRO))
        return TRUE;
    return FALSE;
}

void open_log(PurpleConversation *conv) {
	conv->logs = g_list_append(NULL, 
            purple_log_new(conv->type == PURPLE_CONV_TYPE_CHAT ? PURPLE_LOG_CHAT :
                PURPLE_LOG_IM, conv->name, conv->account,
                conv, time(NULL), NULL));
}

gboolean is_blacklisted(const char *message){
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

GPtrArray *get_commands(const GString *buffer){
    GPtrArray *commands = g_ptr_array_new();
    GString *command = NULL;
    int i, stack = 0;

    printf("In get commands! Buffer: %s\n", buffer->str);

    for (i=0; i<buffer->len; i++){
        if (buffer->str[i] == INTROC && stack == 0) {
            command = g_string_new(NULL);
            i++;
            while (g_ascii_isalnum(buffer->str[i])){
                g_string_append_c(command, buffer->str[i++]);
            }

            g_ptr_array_add(commands, command);
        }

        if (buffer->str[i] == '{'){
            stack++;
        }

        if (buffer->str[i] == '}' && stack > 0){
            stack--;
        }
    }

#ifdef DEBUG
    for (i=0; i<commands->len; i++){
        command = g_ptr_array_index(commands, i);

        printf("Salvaged command #%i = [%s]\n", i, command->str);
    }
#endif

    return commands;
}

GPtrArray *get_snippets(const GString *buffer){
    GPtrArray *snippets = g_ptr_array_new();
    GString *snippet = NULL;
    int i;
    int stackcnt = 0;
    char current;

    for (i=0; i<buffer->len; i++){
        current = buffer->str[i];

        if (current == '{' && stackcnt == 0){
            snippet = g_string_new(NULL);
            stackcnt++;
            g_string_append_c(snippet, current);
            continue;
        }

        if (stackcnt > 0) {
            g_string_append_c(snippet, current);

            if (current == '{'){
                stackcnt++;
            }

            if (current == '}'){
                stackcnt--;
                if (stackcnt == 0)
                    g_ptr_array_add(snippets, snippet);
            }
        }
    }

#ifdef DEBUG
    for (i=0; i<snippets->len; i++){
        snippet = g_ptr_array_index(snippets, i);

        printf("Salvaged snippet #%i = [%s]\n", i, snippet->str);
    }
#endif

    return snippets;
}

GString *replace(const GString *original, 
        const GString *command, const GString *snippet, int id){

    GString *replacer = g_string_new(IMG_BEG);
    GString *to_replace = g_string_new(INTRO);
    char idbuffer[10];
    char *new_msg;
    GString *result;

    g_string_append(to_replace, command->str);
    g_string_append(to_replace, snippet->str);

	sprintf(idbuffer, "%d\0", id);
    g_string_append(replacer, idbuffer);
    g_string_append(replacer, IMG_END);

#ifdef DEBUG
    printf("replace: %s with %s\n", to_replace->str, replacer->str);
#endif

    new_msg = str_replace(original->str, to_replace->str, replacer->str);

    g_string_free(replacer, TRUE);
    g_string_free(to_replace, TRUE);

    result = g_string_new(new_msg);
    free(new_msg);

#ifdef DEBUG
    printf("result: %s\n", result->str);
#endif 

    return result;
}


/* Credit to http://stackoverflow.com/questions/779875/
 * what-is-the-function-to-replace-string-in-c*/
char *str_replace(const char *orig, const char *rep, const char *with) {
    const char *ins;    // the next insert point
    char *result; // the return string
    char *tmp;    // varies
    int len_rep;  // length of rep
    int len_with; // length of with
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements

    if (!orig || !rep || !with)
        return NULL;

    len_rep = strlen(rep);
    len_with = strlen(with);

    /* count occurences of rep in orig */
    ins = orig;
    for (count = 0; tmp = strstr(ins, rep); ++count) {
        ins = tmp + len_rep;
    }

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}

int load_image(const GString *resulting_png){
    int img_id = 0;
    gchar *filedata;
    gsize size;
    GError *error;

	if (!g_file_get_contents(resulting_png->str, &filedata, &size, &error)) {
		purple_notify_error(me, "LaTeX", 
                "Error while reading the rendered markup [%s]", 
                error->message);
		g_error_free(error);

		return -1;
	}

	img_id = purple_imgstore_add_with_id(filedata, 
            MAX(1024, size), getfilename(resulting_png->str));

	if (img_id == 0) {
		purple_notify_error(me, "LaTeX", 
                "Error while reading the generated image!", 
                "Failed to store image.");
		return -1;
	}

    return img_id;
}

gboolean free_commands(const GPtrArray *commands){
    int i;
    GString *command;
    for (i=0; i<commands->len; i++){
        command = g_ptr_array_index(commands, i);
        g_string_free(command, TRUE);
    }

    return TRUE;
}

gboolean free_snippets(const GPtrArray *snippets){
    int i;
    GString *snippet;
    for (i=0; i<snippets->len; i++){
        snippet = g_ptr_array_index(snippets, i);
        g_string_free(snippet, TRUE);
    }

    return TRUE;
}

GString *modify_message(const GString *message){
    int image_id;
    int i;

    GString *snippet;
    GString *command;
    GString *picpath;
    GString *old = g_string_new(message->str);
    GString *new;

    GPtrArray *snippets = get_snippets(message);
    GPtrArray *commands = get_commands(message);

    if (snippets->len != commands->len){
		purple_notify_error(me, "LaTeX", 
                "Error while analysing the message!", 
                "Different amounts of snippets and commands!");
        return NULL;
    }

    for (i=0; i<commands->len; i++){
        command = g_ptr_array_index(commands, i);
        snippet = g_ptr_array_index(snippets, i);

        picpath = dispatch_command(command, snippet);
        if (picpath == NULL){
            purple_debug_info("LaTeX", 
                    "Could not dispatch command: [%s(%s,%s)]\n",
                    "dispatch_command", command->str, snippet->str);
            return NULL;
        }

        image_id = load_image(picpath);

        if(image_id == -1){
            return NULL;  
        }

        new = replace(old, command, snippet, image_id);

        unlink(picpath->str);
        g_string_free(old, TRUE);
        g_string_free(picpath, TRUE);

        old = new;
    }

    purple_debug_info("LaTeX",
            "Changed message from [%s] to [%s]\n",
            message->str, new->str);

#ifdef DEBUG
    printf("LaTeX",
            "Changed message from [%s] to [%s]\n",
            message->str, new->str);
#endif 

    free_snippets(snippets);
    free_commands(commands);
    g_ptr_array_free(snippets, TRUE);
    g_ptr_array_free(commands, TRUE);

    return new;
}

//TODO: Check sanity of this function!
gboolean pidgin_latex_write(PurpleConversation *conv, 
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

void message_send_chat(PurpleAccount *account, 
        const char **buffer, int id){
	PurpleConnection *conn = purple_account_get_connection(account);
	message_send(purple_find_chat(conn, id), buffer);
}

void message_send_im(PurpleAccount *account, 
        const char *who, const char **buffer){
	message_send(purple_find_conversation_with_account(
                PURPLE_CONV_TYPE_IM, who, account), buffer);
}

gboolean message_receive(PurpleAccount *account, 
        const char *who, const char **buffer, 
        PurpleConversation *conv, PurpleMessageFlags flags){
    gchar *unescaped = purple_unescape_html(*buffer);

#ifdef DEBUG
    printf("Message_received!\n");
#endif
    GString *wrapper = g_string_new(unescaped);
    GString *modified;
    g_free(unescaped);

	purple_debug_info("LaTeX", 
            "[message_receive()] Writing Message: %s\n", *buffer);

	if (!contains_work(*buffer)){
        g_string_free(wrapper, TRUE);
		return FALSE;
	}

	if (is_blacklisted(*buffer)) {
		purple_debug_info("LaTeX", 
                "Message not analysed, because it contained blacklisted code.\n");
        g_string_free(wrapper, TRUE);
		return FALSE;
	}


	purple_debug_info("LaTeX", 
            "[message_receive()] Analyse: %s\n", *buffer);

    modified = modify_message(wrapper);
    if (modified == NULL){
        purple_debug_info("LaTeX", 
                "Message could not be modified: %s",
                "modify_message()");
        return FALSE;
    }

	pidgin_latex_write(conv, who, modified->str, flags, *buffer);

    g_string_free(modified, TRUE);
    g_string_free(wrapper, TRUE);

	return TRUE;
}

void message_send(PurpleConversation *conv, const char **buffer){
    gchar *unescaped = purple_unescape_html(*buffer);
    GString *wrapper = g_string_new(unescaped);
	GString *modified;
	gboolean smileys;
    g_free(unescaped);

#ifdef DEBUG
    printf("Message_send!\n");
#endif

	purple_debug_info("LaTeX", 
            "[message_send()] Sending Message: %s\n", *buffer);

	if (!contains_work(*buffer)){
		return;
	}

	if (is_blacklisted(*buffer)) {
		purple_debug_info("LaTeX", 
                "Message not analysed, because it contained blacklisted code.\n");
		return;
	}

    modified = modify_message(wrapper);
    *buffer = g_string_free(modified, FALSE);
    g_string_free(wrapper, TRUE);

    return;
}


gboolean plugin_load(PurplePlugin *plugin){
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

gboolean plugin_unload(PurplePlugin * plugin){
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

PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,                 /**< type           */
	PIDGIN_PLUGIN_TYPE,                     /**< ui_requirement */
	0,                                      /**< flags          */
	NULL,                                   /**< dependencies   */
	PURPLE_PRIORITY_DEFAULT,                /**< priority       */

	LATEX_PLUGIN_ID,                        /**< id             */
	"PiFo",                                /**< name           */
	"1.0",                                  /**< version        */
	/**  summary        */
	"To display various rendered markup codes into Pidgin conversation.",
	/**  description    */
	"Put Markup-code between \\markupcmd{...} to have it displayed as " 
    "Picture in your conversation.\nRemember that your contact needs "
    "an similar plugin or else he will just see the pure Markup-code\nYou "
    "must have LaTeX, Graphviz and dvipng installed (in your PATH)",
	"Florian Mayer <mayer.florian@web.de> (complete rewrite)\n"
	"Benjamin Moll <qjuh@users.sourceforge.net>, Nicolas Schoonbroodt\n"
    "<nicolas@ffsa.be> and Nicolai Stange <nic-stange@t-online.de>\n" 
    "provided the valuable base sources (legacy pidgin-latex)", /**< author       */
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

 void init_plugin(PurplePlugin *plugin){
    return;
} 

PURPLE_INIT_PLUGIN(pifo, init_plugin, info)
