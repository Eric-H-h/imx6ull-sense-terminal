#ifndef STATUS_JSON_H
#define STATUS_JSON_H

#include "sense.h"

#include <stddef.h>

int status_json_format(const StatusSnapshot *snapshot,
                       char *destination,
                       size_t capacity);

#endif
