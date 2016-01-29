#include "pifo_generator.h"
#include "pifo.h"

static const char *graphviz_command = "dot";
static const char *formula_command = "formula";
static const char *allowed_languages[] = {
    "ada", "haskell",
    "bash", "awk",
    "c", "cplusplus",
    "html", "java",
    "lisp", "lua",
    "make", "octave",
    "perl", "python",
    "ruby", "vhdl",
    "verilo", "xml",
    "latex"
}

static GString *fgcolor_as_string(){
    GString *result = g_string_new(NULL);
	int rgb;
	char const *pidgin_fgcolor;
    /* Gather some information about the current pidgin settings
     * so that we can populate the latex template file appropriately */
	if (!strcmp(purple_prefs_get_string("/pidgin/conversations/fgcolor"), "")) {
        g_string_append("0,0,0");
	} else {
		pidgin_fgcolor = purple_prefs_get_string("/pidgin/conversations/fgcolor");
		rgb = strtol(pidgin_fgcolor + 1, NULL, 16);
		purple_debug_info("LaTeX", "Numerical: %d\n", rgb);
		purple_debug_info("LaTeX", "Found foregroundcolor '%s'\n", pidgin_fgcolor);
		g_string_append_printf(result, "%d,%d,%d", 
                rgb >> 16, (rgb >> 8) & 0xff, rgb & 0xff);
	}
	purple_debug_info("LaTeX", "Using '%s' for foreground\n", fgcolor);

    return result;
}

static GString *fgcolor_as_string(){
    GString *result = g_string_new(NULL);
	int rgb;
	char const *pidgin_bgcolor;

	if (!strcmp(purple_prefs_get_string("/pidgin/conversations/bgcolor"), "")) {
        g_string_append(result, "255,255,255");
	} else {
		pidgin_bgcolor = purple_prefs_get_string("/pidgin/conversations/bgcolor");
		rgb = strtol(pidgin_bgcolor + 1, NULL, 16);
		purple_debug_info("LaTeX", "Numerical: %d\n", rgb);
		purple_debug_info("LaTeX", "Found backgroundcolor '%s'\n", pidgin_bgcolor);
		g_string_append_printf(result, "%d,%d,%d", 
                rgb >> 16, (rgb >> 8) & 0xff, rgb & 0xff);
	}
	purple_debug_info("LaTeX", "Using '%s' for background\n", bgcolor);

    return result;
}

/* Used to parse the command and trigger appropriate compilier runs */
static GString *dispatch_command(GString *command, GString *snippet){
    GString *result;
    int i;

    for (i=0; i<sizeof(allowed_languages); i++){
        if (!strcmp(command->str, allowed_languages[i])){
            if (generate_latex_listing(snippet, 
                        g_new_string(allowed_languages[i]), &result));
                return result;
            else 
                return NULL;
        } 
    }

    if (!strcmp(command->str, graphviz_command)){
        if (generate_graphviz_png(snippet, &result))
            return result;
        else 
            return NULL;
    }

    if (!strcmp(command->str, formula_command)){
        if (generate_latex_formula(snippet, &result))
            return result;
        else
            return NULL;
    }

    return NULL;
}


static gboolean generate_latex_listing(GString *listing, 
        GString *language, GString **filename){

    FILE *transcript_file;
    gboolean returnval = TRUE;

    GString *fgcolor = fgcolor_as_string();
            *bgcolor = bgcolor_as_string();
    GString *texfilepath, *dvifilepath, 
            *pngfilepath, *auxfilepath, *logfilepath

    setup_files(&texfilepath, &dvifilepath, 
                &pngfilepath, &auxfilepath, &logfilepath);

    if (!chtempdir(texfilepath->str)){
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
	fprintf(transcript_file, LATEX_LST_TEMPLATE(%s,%s,%s,%s), 
            language->str, fgcolor->str, bgcolor->str, listing->str);
	fclose(transcript_file);

    if (exec_latex(pngfilepath, texfilepath, dvifilepath) == FALSE){
        *filename_png = pngfilepath;
    } else {
	    purple_debug_info("LaTeX", 
                "Image creation exited with status '%d'\n", 
                !exec_ok);
        returnval = FALSE;
        g_string_free(pngfilepath);
    }

out:
    unlink(texfilepath->str);
    unlink(dvifilepath->str);
    unlink(auxfilepath->str);
    unlink(logfilepath->str);

    g_string_free(texfilepath);
    g_string_free(auxfilepath);
    g_string_free(logfilepath);
    g_string_free(dvifilepath);
    g_string_free(tmpfilepath);

    return returnval;
}

static gboolean generate_graphviz_png(GString *dotcode,
        GString **filename);

static gboolean generate_latex_formula(GString *formula, 
        GString **filename);

static gboolean setup_files(GString **tex, 
        GString **dvi, GString **png,
        GString **aux, GString **log){

    GString *tmpfilepath;
    tmpfilepath = get_unique_tmppath();

    *tex = g_string_new(tmpfilepath->str);
    *dvi = g_string_new(tmpfilepath->str);
    *png = g_string_new(tmpfilepath->str);
    *aux = g_string_new(tmpfilepath->str);
    *log = g_string_new(tmpfilepath->str);

    g_string_append(texfilepath, ".tex");
    g_string_append(dvifilepath, ".dvi");
    g_string_append(pngfilepath, ".png");
    g_string_append(logfilepath, ".log");
    g_string_append(auxfilepath, ".aux");

    return TRUE;
}

static gboolean chtempdir(GString *path){
    char *dirname_temp = getdirname(path->str);
	if (dirname_temp == NULL){
		purple_notify_error(me, "LaTeX", 
                "Error while trying to transcript LaTeX!", 
                "Couldnt allocate memory for path");

        return FALSE;
	}

    if (chdir(dirname_temp) == -1){
		purple_notify_error(me, "LaTeX", 
                "Error while trying to transcript LaTeX!", 
                "Couldn't cange to temporary directory");
         
        free(dirname_temp);
        return FALSE;
    }

    return TRUE;
}

static gboolean exec_latex(GString *pngfilepath, 
        GString *texfilepath, GString *dvifilepath){
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

    if (!exec_ok)
        return FALSE;

    return TRUE;
}

static gboolean generate_latex_formula(GString *formula, 
        GString **filename_png){
    FILE *transcript_file;
    gboolean returnval = TRUE;

    GString *fgcolor = fgcolor_as_string();
            *bgcolor = bgcolor_as_string();

    GString *texfilepath, *dvifilepath, 
            *pngfilepath, *auxfilepath, *logfilepath

    setup_files(&texfilepath, &dvifilepath, 
                &pngfilepath, &auxfilepath, &logfilepath);

    if (!chtempdir(texfilepath->str)){
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
	fprintf(transcript_file, LATEX_MATH_TEMPLATE(%s,%s,%s), 
            formula->str, fgcolor->str, bgcolor->str);
	fclose(transcript_file);


    if (exec_latex(pngfilepath, texfilepath, dvifilepath) == FALSE){
        *filename_png = pngfilepath;
    } else {
	    purple_debug_info("LaTeX", 
                "Image creation exited with status '%d'\n", 
                !exec_ok);
        returnval = FALSE;
        g_string_free(pngfilepath);
    }

out:
    unlink(texfilepath->str);
    unlink(dvifilepath->str);
    unlink(auxfilepath->str);
    unlink(logfilepath->str);

    g_string_free(texfilepath);
    g_string_free(auxfilepath);
    g_string_free(logfilepath);
    g_string_free(dvifilepath);
    g_string_free(tmpfilepath);

    return returnval;
}
