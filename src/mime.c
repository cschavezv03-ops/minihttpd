#include <string.h>
#include <stdio.h>
#include <strings.h>
#include "../include/mime.h"

static const MimeEntry types[] = {
    {".html", "text/html"},
    {".css", "text/css"},
    {".js", "application/javascript"},
    {".png", "image/png"},
    {".jpg",  "image/jpeg"},
    {".jpeg", "image/jpeg"}
};

const char *get_mime_type(const char *path){
    if (path == NULL) {
        return "application/octet-stream";
    }
    const char *filename = strrchr(path, '/');
    filename = (filename != NULL) ? filename + 1 : path;
    const char *extension = strrchr(filename, '.');
    if(extension == NULL){
        return "application/octet-stream";
    }
	for (int i = 0; i < 6; i++) {
        if (strcasecmp(extension, types[i].extension) == 0) {
            return  types[i].mime_type;
        }
	}
	return "application/octet-stream";
}