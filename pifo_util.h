#ifndef PIFUTIL
#define PIFUTIL

#include "pifo.h"

GString *get_unique_tmppath(void);
int execute(const char *prog, char * const cmd[]);
char* getfilename(const char const *file);
char* getdirname(const char const *file);

#endif
