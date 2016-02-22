#include <string.h>

#include "pifo_generator.h"
#include "pifo_util.h"
#include "pifo.h"

#define DEBUG
#define FOO "fnord";


extern PurplePlugin *me;

/* commandstring -> function mapping */
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

    /* markdown support per pandoc */
    {"markdown", (gboolean (*)(const GString *c, const GString *s, void **r))
     generate_markdown},

    {"tikz", (gboolean (*)(const GString *c, const GString *s, void **r))
     generate_tikz_png},

    {"svg", (gboolean (*)(const GString *c, const GString *s, void **r))
     generate_svg_png}
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

gboolean generate_graphviz_png(const GString *dotcode,
                               const GString *command,
                               GString **filename){
    *filename = NULL;

    FILE *dotfile;
    gboolean returnval = TRUE;
    gboolean exec;
    GString *tmpfile = get_unique_tmppath();
    GString *pngfile = g_string_new(tmpfile->str);
    g_string_append(pngfile, ".png");

    if (!chtempdir(tmpfile)){
        returnval = FALSE;
        goto out;
    }

    if (!(dotfile = fopen(tmpfile->str, "w"))){
        purple_notify_error(me, "PiFo",
                            "Error while trying to render graphviz file",
                            "Error opening file!");

        return FALSE;
    }

    fprintf(dotfile, "%s", dotcode->str);
    fclose(dotfile);

    char * const dotopts[] = {
        "dot", "-O",
        "-T", "png",
        tmpfile->str, NULL
    };

#ifdef DEBUG
    printf("generate_graphivz_png(): tmpfile: [%s]\npngfile: [%s]\n",
           tmpfile->str, pngfile->str);
#endif

    exec = execute("dot", dotopts);

    if (exec != 0){
        purple_debug_info("PiFo",
                          "Could not render dot code!\n");
        *filename = NULL;
        returnval = FALSE;
        goto out;
    }

    *filename = pngfile;
    returnval = TRUE;
    goto out;

 out:
    //unlink(tmpfile->str);
    //g_string_free(tmpfile, TRUE);

    return returnval;
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

    //TODO!!
//    g_string_free(tmpfilepath, TRUE);

    return TRUE;
}

gboolean chtempdir(const GString *path){
    g_assert (path != NULL);

    char *dirname_temp = getdirname(path->str);
    if (dirname_temp == NULL){
        return FALSE;
    }

    if (chdir(dirname_temp) == -1){
        g_free(dirname_temp);
        return FALSE;
    }

    g_free(dirname_temp);

    return TRUE;
}

gboolean render_latex_pdf_to_png(const GString *pngfilepath,
        const GString *texfilepath, const GString *epsfilepath,
        const GString *pdffilepath){

    int exec;

    char * const pdflatex[] = {
        "pdflatex", "--no-shell-escape",
        "--interaction=nonstopmode",
        texfilepath->str, NULL
    };

    char * const pdftops[] = {
        "pdftops", "-eps",
        pdffilepath->str, NULL
    };

    char * const convert[] = {
        "convert", "-trim",
        "-density", "300",
        epsfilepath->str, pngfilepath->str, NULL
    };

    exec = (execute("pdflatex", pdflatex) == 0) &&
           (execute("pdftops", pdftops) == 0) &&
           (execute("convert", convert) == 0);

    if (!exec){
        purple_debug_info("PiFo",
                "Could not render file [%s]\n",
                texfilepath->str);
        return FALSE;
    }

    return TRUE;
}

gboolean render_svg_to_png(const GString *pngfile, const GString *svgfile){
    int exec;

    char * const convert[] = {
        "convert", "-trim",
        "-density", "300",
        svgfile->str,
        pngfile->str, NULL
    };

    /* Disgusting hack because fucking libpurple
       automatically linkifies every "http://somewhat"
       string that gets into the conversation window. Html unescape
       OF COURSE does not take care of that...
       I hate libpurple! */
    char * const sed[] = {
         "sed", "-i"
         "-e",
         "s/\\\"<A HREF=\\\".*\\\">\\(http:\\/\\/.*\\)<\\/A>\\\"/\\\"\\1\\\"/g",
         svgfile->str,
	 NULL
    };

    #ifdef DEBUG
    printf("Sed Argument: [%s]\n", sed[3]);
    #endif

    exec = (execute("sed", sed) == 0) &&
	   (execute("convert", convert) == 0);

    if (!exec){
        purple_debug_info("PiFo",
                "Could not render file [%s]\n",
                svgfile->str);
        return FALSE;
    }

    return TRUE;
}

gboolean generate_svg_png(const GString *svg_code,
        const GString *command,
        GString **filename_png){

    FILE *svgfile;
    gboolean returnval = TRUE;

    GString *svgfilepath, *pngfilepath;

    GString *tmpfilepath;
    tmpfilepath = get_unique_tmppath();

    pngfilepath = g_string_new(tmpfilepath->str);
    svgfilepath = g_string_new(tmpfilepath->str);

    g_string_free(tmpfilepath, TRUE);

    g_string_append(pngfilepath, ".png");
    g_string_append(svgfilepath, ".svg");

    purple_debug_info("PiFo",
                      "Using [%s] as svg file\n",
                      svgfilepath->str);

    if (!chtempdir(svgfilepath)){
        returnval = FALSE;
        goto out;
    }

    if (!(svgfile = fopen(svgfilepath->str, "w"))){
        returnval = FALSE;
        goto out;
    }

#ifdef DEBUG
    printf("Transcript file: %s\n", svgfilepath->str);
#endif

    /* Generate svg file */
    fprintf(svgfile, "%s", svg_code->str);
    fclose(svgfile);

    if (render_svg_to_png(pngfilepath, svgfilepath) == TRUE){
        *filename_png = pngfilepath;
    } else {
        purple_debug_info("PiFo",
                          "Image creation exited with failure\n");
        returnval = FALSE;
        g_string_free(pngfilepath, TRUE);
        *filename_png = NULL;
    }

out:
    // unlink(svgfilepath->str);

    g_string_free(svgfilepath, TRUE);

    return returnval;
    return TRUE;
}

gboolean generate_tikz_png(const GString *tikz_code,
        const GString *command,
        GString **filename_png){

    FILE *tikz_texfile;
    gboolean returnval = TRUE;

    GString *texfilepath, *pdffilepath,
            *auxfilepath, *logfilepath,
            *epsfilepath, *pngfilepath;

    GString *tmpfilepath;
    tmpfilepath = get_unique_tmppath();

    epsfilepath = g_string_new(tmpfilepath->str);
    texfilepath = g_string_new(tmpfilepath->str);
    pdffilepath = g_string_new(tmpfilepath->str);
    pngfilepath = g_string_new(tmpfilepath->str);
    auxfilepath = g_string_new(tmpfilepath->str);
    logfilepath = g_string_new(tmpfilepath->str);

    g_string_free(tmpfilepath, TRUE);

    g_string_append(epsfilepath, ".eps");
    g_string_append(texfilepath, ".tex");
    g_string_append(pdffilepath, ".pdf");
    g_string_append(pngfilepath, ".png");
    g_string_append(logfilepath, ".log");
    g_string_append(auxfilepath, ".aux");

    purple_debug_info("PiFo",
                      "Using [%s] as latex-tikz file\n",
                      texfilepath->str);

    if (!chtempdir(texfilepath)){
        returnval = FALSE;
        goto out;
    }

    if (!(tikz_texfile = fopen(texfilepath->str, "w"))){
        returnval = FALSE;
        goto out;
    }

#ifdef DEBUG
    printf("Transcript_file: " LATEX_TIKZ_TEMPLATE "\n",
            tikz_code->str);
#endif

    /* Generate latex template file */
    fprintf(tikz_texfile, LATEX_TIKZ_TEMPLATE, tikz_code->str);
   fclose(tikz_texfile);

   if (render_latex_pdf_to_png(pngfilepath,
               texfilepath, epsfilepath,
               pdffilepath) == TRUE){
       *filename_png = pngfilepath;
   } else {
       purple_debug_info("PiFo",
                         "Image creation exited with failure\n");
       returnval = FALSE;
       g_string_free(pngfilepath, TRUE);
       *filename_png = NULL;
   }

out:
   unlink(texfilepath->str);
   unlink(pdffilepath->str);
   unlink(auxfilepath->str);
   unlink(logfilepath->str);
   unlink(epsfilepath->str);

   g_string_free(epsfilepath, TRUE);
   g_string_free(texfilepath, TRUE);
   g_string_free(auxfilepath, TRUE);
   g_string_free(logfilepath, TRUE);
   g_string_free(pdffilepath, TRUE);

   return returnval;
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

gboolean render_markdown (const GString *markdownfilepath,
                          const GString *texfilepath){
    g_assert (markdownfilepath != NULL);
    g_assert (texfilepath != NULL);

    //  pandoc finds the right in/output types
    //  based on the file suffix
    char * const pandoc_options[] = {
        "pandoc",
        "--self-contained",
        markdownfilepath->str,
        "-o",
        texfilepath->str,
        NULL
    };

    return execute("pandoc", pandoc_options) == 0;
}

gboolean generate_markdown(const GString *markdown_text,
                          const GString *command,
                          GString **filename_png){
    g_assert (markdown_text != NULL);
    g_assert (command != NULL);
    g_assert (*filename_png != NULL);

    // file to hold markdown_text
    FILE *markdownfile = NULL;

    gboolean everything_ok = TRUE;
    gboolean conversion_to_tex_worked = FALSE;
    gboolean conversion_to_png_worked = FALSE;

    // latex files
    GString *texfilepath, *dvifilepath,
        *pngfilepath, *auxfilepath, *logfilepath,
        // pandoc files
        *tmpfilepath, *markdownfilepath = NULL;

    // TODO: should preapre a list of paths
    setup_files(&texfilepath, &dvifilepath,
                &pngfilepath, &auxfilepath, &logfilepath);

    tmpfilepath = get_unique_tmppath();
    markdownfilepath = g_string_new(tmpfilepath->str);
    g_string_append(markdownfilepath, ".md");

    #ifdef DEBUG
    printf ("pandoc temp files: %s %s %s %s %s %s %s",
            texfilepath->str, dvifilepath->str, pngfilepath->str,
            auxfilepath->str, logfilepath->str,
            markdownfilepath->str);
    #endif

    if (!chtempdir(texfilepath)){
        everything_ok = FALSE;
        goto cleanup;
    }

    if (!(markdownfile = fopen(markdownfilepath->str, "w"))){
        purple_notify_error(me, "Pandoc",
                            "Error while trying to convert via pandoc!",
                            "Error opening file!");
        everything_ok = FALSE;
        goto cleanup;
    }

    /* Generate markdown file */
    // WORKAROUND: append this to surpress page numbering
    //  because the resulting png would be huge
    fprintf(markdownfile, "\\pagenumbering{gobble}\n");
    fprintf(markdownfile, markdown_text->str);
    fclose(markdownfile);

    conversion_to_tex_worked = render_markdown (markdownfilepath, texfilepath);

    if (conversion_to_tex_worked){
        conversion_to_png_worked =
            render_latex(pngfilepath, texfilepath, dvifilepath);
    } else {
        everything_ok = FALSE;
        goto cleanup;
    }

    if (conversion_to_png_worked){
        *filename_png = pngfilepath;
    } else {
        purple_debug_info("Pandoc",
                          "Image creation exited with failure status\n");
        everything_ok = FALSE;
        g_string_free(pngfilepath, TRUE);
        goto cleanup;
    }

 cleanup:
    unlink(texfilepath->str);
    unlink(dvifilepath->str);
    unlink(auxfilepath->str);
    unlink(logfilepath->str);
    unlink(markdownfilepath->str);

    g_string_free(texfilepath, TRUE);
    g_string_free(auxfilepath, TRUE);
    g_string_free(logfilepath, TRUE);
    g_string_free(dvifilepath, TRUE);
    g_string_free(markdownfilepath, TRUE);

    return everything_ok;
}
