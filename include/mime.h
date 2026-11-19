#ifndef MIME_H
#define MIME_H

const char *get_mime_type(const char *path);

typedef struct {
    const char *extension;
    const char *mime_type;
}MimeEntry;

#endif