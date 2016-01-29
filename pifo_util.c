#include "pifo.h"

static gboolean latex_compile_png(const char *filename);

static GString *get_unique_tmppath(void){
    FILE *temp;
    char *filename_temp;
    GString *result = g_string_new(NULL);

    temp = purple_mkstemp(&filename_temp,TRUE);
    fclose(temp);
    unlink(filename_temp);

    return result;
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

