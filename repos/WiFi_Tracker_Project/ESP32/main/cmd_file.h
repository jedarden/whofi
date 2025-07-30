#include "esp_log.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>

/*	METHODS	*/
int file_check(const char *path);
int file_delete(const char *path);
int file_read_full(const char *path);
int printDirContent(const char *path);
