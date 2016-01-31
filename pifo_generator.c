#include <string.h>

#include "pifo_generator.h"
#include "pifo_util.h"
#include "pifo.h"

#define DEBUG

extern PurplePlugin *me;

/* Commandstring -> Function mapping */
static const struct mapping commandmap[] = {
    /* source highlighting commands */
    {"ada", (gboolean (*)(const GString *c, const GString *s, void **r)) 
        generate_latex_listing},
    {"haskell", (gboolean (*)(const GString *c, const GString *s, void **r)) 
        generate_latex_listing},
    {"bash", (gboolean (*)(const GString *c, const GString *s, void **r)) 
        generate_latex_listing},
    {"awk", (gboolean (*)(const GString *c, const GString *s, void **r)) 
        generate_latex_listing},
    {"c", (gboolean (*)(const GString *c, const GString *s, void **r)) 
        generate_latex_listing},
    {"cpluscplus", (gboolean (*)(const GString *c, const GString *s, void **r)) 
        generate_latex_listing},
    {"html", (gboolean (*)(const GString *c, const GString *s, void **r)) 
        generate_latex_listing},
    {"lua", (gboolean (*)(const GString *c, const GString *s, void **r)) 
        generate_latex_listing},
    {"make", (gboolean (*)(const GString *c, const GString *s, void **r)) 
        generate_latex_listing},
    {"octave", (gboolean (*)(const GString *c, const GString *s, void **r)) 
        generate_latex_listing},
    {"perl", (gboolean (*)(const GString *c, const GString *s, void **r)) 
        generate_latex_listing},
    {"python", (gboolean (*)(const GString *c, const GString *s, void **r)) 
        generate_latex_listing},
    {"ruby", (gboolean (*)(const GString *c, const GString *s, void **r)) 
        generate_latex_listing},
    {"vhdl", (gboolean (*)(const GString *c, const GString *s, void **r)) 
        generate_latex_listing},
    {"verilo", (gboolean (*)(const GString *c, const GString *s, void **r)) 
        generate_latex_listing},
    {"xml", (gboolean (*)(const GString *c, const GString *s, void **r)) 
        generate_latex_listing},
    {"latex", (gboolean (*)(const GString *c, const GString *s, void **r)) 
        generate_latex_listing},

    /* graphviz command */
    {"dot", (gboolean (*)(const GString *c, const GString *s, void **r)) 
        generate_graphviz_png},

    /* formula typesetting */
    {"formula", (gboolean (*)(const GString *c, const GString *s, void **r)) 
        generate_latex_formula},
};


GString *fgcolor_as_string(void){
    GString *result = g_string_new(NULL);
	int rgb;
	char const *pidgin_fgcolor;
    /* Gather some information about the current pidgin settings
     * so that we can populate the latex template file appropriately */
	if (!strcmp(purple_prefs_get_string("/pidgin/conversations/fgcolor"), "")) {
        g_string_append(result, "0,0,0");
	} else {
		pidgin_fgcolor = purple_prefs_get_string("/pidgin/conversations/fgcolor");
		rgb = strtol(pidgin_fgcolor + 1, NULL, 16);
		g_string_append_printf(result, "%d,%d,%d", 
                rgb >> 16, (rgb >> 8) & 0xff, rgb & 0xff);
	}

    return result;
}

GString *bgcolor_as_string(){
    GString *result = g_string_new(NULL);
	int rgb;
	char const *pidgin_bgcolor;

	if (!strcmp(purple_prefs_get_string("/pidgin/conversations/bgcolor"), "")) {
        g_string_append(result, "255,255,255");
	} else {
		pidgin_bgcolor = purple_prefs_get_string("/pidgin/conversations/bgcolor");
		rgb = strtol(pidgin_bgcolor + 1, NULL, 16);
		g_string_append_printf(result, "%d,%d,%d", 
                rgb >> 16, (rgb >> 8) & 0xff, rgb & 0xff);
	}

    return result;
}

/* Used to parse the command and trigger appropriate compilier runs */
GString *dispatch_command(const GString *command, const GString *snippet){
    GString *result;
    int i;

    for (i=0; i<sizeof(commandmap)/sizeof(struct mapping); i++){
        if (!strcmp(command->str, commandmap[i].command)){
            purple_debug_info("LaTeX", 
                    "Running backend for [%s]\n",
                    command->str);

            if (commandmap[i].handler(snippet, command, (void **) &result)){
                return result;
            } else {
                return NULL;
            }
        } 
    }

    return NULL;
}

gboolean generate_latex_listing(const GString *listing, 
        const GString *language, GString **filename){

    FILE *transcript_file;
    char *listing_temp = listing->str;
    gboolean returnval = TRUE;

    GString *fgcolor = fgcolor_as_string(),
            *bgcolor = bgcolor_as_string();
    GString *texfilepath, *dvifilepath, 
            *pngfilepath, *auxfilepath, *logfilepath;

    setup_files(&texfilepath, &dvifilepath, 
                &pngfilepath, &auxfilepath, &logfilepath);

    purple_debug_info("LaTex", 
            "Using [%s] as latex file\n", 
            texfilepath->str);

    if (!chtempdir(texfilepath)){
        returnval = FALSE;
        goto out;
    } 

	if (!(transcript_file = fopen(texfilepath->str, "w"))){
        returnval = FALSE;
        goto out;
    }

	purple_debug_info("LaTeX", 
            "Using [%s] as foreground and [%s] as background\n",
            fgcolor->str, bgcolor->str);

    /* temporary cut off { and } */
    listing_temp[listing->len - 1] = '\0';
    listing_temp++;

#ifdef DEBUG
	printf("transcript_file: " LATEX_LST_TEMPLATE "\n", 
            fgcolor->str, bgcolor->str, 
            "none", "5", "none", language->str, 
            listing_temp);
#endif
    /* Generate latex template file */
	fprintf(transcript_file, LATEX_LST_TEMPLATE, 
            fgcolor->str, bgcolor->str, 
            "none", "5", "none", language->str, 
            listing_temp);
	fclose(transcript_file);

    listing_temp--;
    listing_temp[listing->len - 1] = '}';

    if (render_latex(pngfilepath, texfilepath, dvifilepath) == TRUE){
        *filename = pngfilepath;
    } else {
	    purple_debug_info("LaTeX", 
                "Image creation exited with failure\n");
        returnval = FALSE;
        g_string_free(pngfilepath, TRUE);
        *filename = NULL;
    }

out:
    unlink(texfilepath->str);
    unlink(dvifilepath->str);
    unlink(auxfilepath->str);
    unlink(logfilepath->str);

    g_string_free(texfilepath, TRUE);
    g_string_free(auxfilepath, TRUE);
    g_string_free(logfilepath, TRUE);
    g_string_free(dvifilepath, TRUE);

    return returnval;
}

/* Currently not implemented */
gboolean generate_graphviz_png(const GString *command, 
        const GString *dotcode,
        GString **filename){
    *filename = NULL;
    return FALSE;
}

gboolean setup_files(GString **tex, 
        GString **dvi, GString **png,
        GString **aux, GString **log){

    GString *tmpfilepath;
    tmpfilepath = get_unique_tmppath();

    *tex = g_string_new(tmpfilepath->str);
    *dvi = g_string_new(tmpfilepath->str);
    *png = g_string_new(tmpfilepath->str);
    *aux = g_string_new(tmpfilepath->str);
    *log = g_string_new(tmpfilepath->str);

    g_string_append(*tex, ".tex");
    g_string_append(*dvi, ".dvi");
    g_string_append(*png, ".png");
    g_string_append(*log, ".log");
    g_string_append(*aux, ".aux");

    return TRUE;
}

gboolean chtempdir(const GString *path){
    char *dirname_temp = getdirname(path->str);
	if (dirname_temp == NULL){
        return FALSE;
	}

    if (chdir(dirname_temp) == -1){
        free(dirname_temp);
        return FALSE;
    }

    return TRUE;
}

gboolean render_latex(const GString *pngfilepath, 
        const GString *texfilepath, const GString *dvifilepath){
    gboolean exec_ok;
    /* Make sure that latex cannot do shell escape, even
     * if the local default config says so! */
	char * const latexopts[] = { 
        "latex", 
        "--no-shell-escape", 
        "--interaction=nonstopmode", 
        texfilepath->str, NULL
    };

	char * const dvipngopts[] = { 
        "dvipng", "-Q", "10", "-T", 
        "tight", "--follow", "-o", 
        pngfilepath->str, dvifilepath->str, NULL
    };

    /* Start Latex and dvipng */
	exec_ok = (execute("latex", latexopts) == 0) &&
              (execute("dvipng", dvipngopts) == 0);

    if (!exec_ok){
        purple_debug_info("LaTeX",
                "Could not render latex string!\n");
        return FALSE;
    }

    return TRUE;
}

gboolean generate_latex_formula(const GString *formula, 
        const GString *command,
        GString **filename_png){
    FILE *transcript_file;
    gboolean returnval = TRUE;

    GString *fgcolor = fgcolor_as_string(),
            *bgcolor = bgcolor_as_string();

    GString *texfilepath, *dvifilepath, 
            *pngfilepath, *auxfilepath, *logfilepath;

    setup_files(&texfilepath, &dvifilepath, 
                &pngfilepath, &auxfilepath, &logfilepath);

    if (!chtempdir(texfilepath)){
        returnval = FALSE;
        goto out;
    } 

	if (!(transcript_file = fopen(texfilepath->str, "w"))){
		purple_notify_error(me, "LaTeX", 
                "Error while trying to transcript LaTeX!", 
                "Error opening file!");
        returnval = FALSE;
        goto out;
    }

    /* Generate latex template file */
	fprintf(transcript_file, LATEX_MATH_TEMPLATE, 
            fgcolor->str, bgcolor->str, formula->str);
	fclose(transcript_file);


    if (render_latex(pngfilepath, texfilepath, dvifilepath) == TRUE){
        *filename_png = pngfilepath;
    } else {
	    purple_debug_info("LaTeX", 
                "Image creation exited with failure status\n");
        returnval = FALSE;
        g_string_free(pngfilepath, TRUE);
    }

    goto out;

out:
//    unlink(texfilepath->str);
    unlink(dvifilepath->str);
    unlink(auxfilepath->str);
    unlink(logfilepath->str);

    g_string_free(texfilepath, TRUE);
    g_string_free(auxfilepath, TRUE);
    g_string_free(logfilepath, TRUE);
    g_string_free(dvifilepath, TRUE);

    return returnval;
}
