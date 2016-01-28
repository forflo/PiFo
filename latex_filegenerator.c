
	      //  fprintf(transcript_file, HEADER HEADER_FCOLOR "{%s}" 
          //          HEADER_BCOLOR "{%s}" HEADER_DOC BEG_MATH "%s" END_MATH
          //          FOOTER, fgcolor, bgcolor, latex_expression);

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

static GString *fgcolor_as_string();
static GString *bgcolor_as_string();

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
		g_string_append_printf(result, "%d,%d,%d", rgb >> 16, (rgb >> 8) & 0xff, rgb & 0xff);
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
		g_string_append_printf(result, "%d,%d,%d", rgb >> 16, (rgb >> 8) & 0xff, rgb & 0xff);
	}
	purple_debug_info("LaTeX", "Using '%s' for background\n", bgcolor);

    return result;
}

#define LATEX_MATH_TEMPLATE (FORMULA,FCOLOR,BCOLOR) \
    "\\documentclass[12pt]{article}\\usepackage{color}"  \
    "\\usepackage[dvips]{graphicx}\\usepackage{amsmath}" \
    "\\usepackage{amssymb}\\usepackage[utf8]{inputenc}"  \
    "\\pagestyle{empty}" \
    "\\definecolor{fgcolor}{RGB}" "{" #FCOLOR "}" \
    "\\definecolor{bgcolor}{RGB}" "{" #BCOLOR "}" \
    "\\begin{document}\\pagecolor{bgcolor}\\color{fgcolor}" \
    "\\begin{gather*}" \
        #FORMULA \
    "\\end{gather*}" \
    "\\end{document}"

#define LATEX_LST_TEMPLATE (LANGUAGE,FCOLOR,BCOLOR,LISTING) \
    "\\documentclass[12pt]{article}\\usepackage{color}"  \
    "\\usepackage[dvips]{graphicx}\\usepackage{amsmath}" \
    "\\usepackage{amssymb}\\usepackage[utf8]{inputenc}"  \
    "\\usepackage{listings}\\pagestyle{empty}" \
    "\\definecolor{fgcolor}{RGB}" "{" #FCOLOR "}" \
    "\\definecolor{bgcolor}{RGB}" "{" #BCOLOR "}" \
    "\\lstset{numbers=none,numberstyle=\\small{"\
    "\\ttfamily{}},stepnumber=1,numbersep=4pt} \\lstset{tabsize=4}"\
    "\\lstset{breaklines=true,breakatwhitespace=true}"\
    "\\lstset{frame=none}\\lstset{language=" #LANGUAGE "}" \
    "\\begin{document}\\pagecolor{bgcolor}\\color{fgcolor}" \
    "\\begin{lstlisting}"\
        #LISTING \
    "\\end{lstlisting}" \
    "\\end{document}"

static const char *graphviz_command = "dot";
static const char *formula_command = "formula";
static const char *allowed_languages[] = {
    "ada",
    "haskell",
    "bash",
    "awk",
    "c",
    "cplusplus",
    "html",
    "java",
    "lisp",
    "lua",
    "make",
    "octave",
    "perl",
    "python",
    "ruby",
    "vhdl",
    "verilo",
    "xml",
    "latex"
}


/* Used to parse the command and trigger appropriate compilier runs */
static GString *dispatch_command(GString *command, GString *snippet);

static GString *generate_latex_lstlisting(GString *listing, 
        GString *language, const char *filename);

/* returns filename containing pic of formula*/
static GString generate_latex_formula(GString *formula, 
        const char *filename);

static GString *get_unique_tmppath(void){
    FILE *temp;
    char *filename_temp;
    GString *result = g_string_new(NULL);

    temp = purple_mkstemp(&filename_temp,TRUE);
    fclose(temp);
    unlink(filename_temp);

    return result;
}

static gboolean generate_latex_formula(GString *formula, 
        char **filename_png){
    FILE *transcript_file;
    FILE *temp;
    char *dirname_temp = NULL;
    gboolean exec_ok;

    GString *fgcolor = fgcolor_as_string, 
            *bgcolor = bgcolor_as_string;

    GString *tmpfilepath = get_unique_tmppath();
    GString *texfilepath = g_string_new(tmpfilepath->str);
    GString *dvifilepath = g_string_new(tmpfilepath->str);
    GString *pngfilepath = g_string_new(tmpfilepath->str);

    g_string_append(texfilepath, ".tex");
    g_string_append(dvifilepath, ".dvi");
    g_string_append(pngfilepath, ".png");

	dirname_temp = getdirname(textfilepath->str);
    
	if (!(transcript_file = fopen(texfilepath->str, "w"))) 
        goto error;

    /* Generate latex template file */
	fprintf(transcript_file, LATEX_MATH_TEMPLATE(%s,%s,%s), 
            formula->str, fgcolor, bgcolor);

	fclose(transcript_file);

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
