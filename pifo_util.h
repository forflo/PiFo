#ifndef PIFUTIL
#define PIFUTIL

static GString *get_unique_tmppath(void);
static int execute(const char *prog, char * const cmd[]);
static char* getfilename(const char const *file);
static char* getdirname(const char const *file);

#endif
