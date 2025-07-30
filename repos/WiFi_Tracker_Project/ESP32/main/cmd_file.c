#include "cmd_file.h"

/*	VARIABLES	*/
const char *TAGH = "File";

int file_check(const char *path){
	ESP_LOGI(TAGH,"[+] Checking if file '%s' within the directory.\n", path);
	FILE *f = fopen(path, "r");
	if (f == NULL)
		return -1;
	else
		fclose(f);
	return 0;
}

int file_delete(const char *path){
	ESP_LOGI(TAGH,"[!] Deleting the file specified in path: '%s'.\n", path);
	int succes;
	succes = remove(path);
	if(succes == 0) {
	   return 0;
	}  else {
	   return -1;
	}
}

int file_read_full(const char *path) {
	ESP_LOGI(TAGH,"[+] Reading the file '%s'.\n\n", path);
	FILE *fp = fopen(path, "r");
	size_t len = 255;
	if (fp == NULL) {
		return -1;
	}
	// need malloc memory for line, if not, segmentation fault error will occurred.
	char *line = malloc(sizeof(char) * len);
	while(fgets(line, len, fp) != NULL) {
		printf("%s\n", line);
	}
	ESP_LOGI(TAGH,"END-------------.\n");
	free(line);
	return 0;
}

int printDirContent(const char *path){
	ESP_LOGI(TAGH,"[!] Printing file in the selected directory('%s')...\n",path);
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir (path)) != NULL) {
		/* print all the files and directories within directory */
	  while ((ent = readdir (dir)) != NULL) {
		  ESP_LOGI(TAGH,"%s\n", ent->d_name);
	  }
	  closedir (dir);
	  return 0;
	} else {
	  return -1;
	}
}
