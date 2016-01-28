
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

/* Used to parse the command and trigger appropriate compilier runs */
static GString *dispatch_command(GString *command, GString *snippet);

static GString *generate_latex_lstlisting(GString *listing, 

        GString *language, const char *filename);

/* returns filename containing pic of formula*/
static GString generate_latex_formula(GString *formula, 
        const char *filename);



/* old */
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

    /* EDIT: FG color is missing! */

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
