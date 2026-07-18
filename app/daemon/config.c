#define _POSIX_C_SOURCE 200809L

#include "sense.h"

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONFIG_MAX_BYTES (64U * 1024U)

typedef struct {
    const char *current;
    const char *end;
} JsonCursor;

static void set_error(char *error, size_t error_size, const char *format, ...)
{
    va_list arguments;

    if (error_size == 0) {
        return;
    }

    va_start(arguments, format);
    vsnprintf(error, error_size, format, arguments);
    va_end(arguments);
}

static void skip_space(JsonCursor *cursor)
{
    while ((cursor->current < cursor->end) &&
           isspace((unsigned char)*cursor->current)) {
        ++cursor->current;
    }
}

static int consume(JsonCursor *cursor, char expected)
{
    skip_space(cursor);
    if ((cursor->current >= cursor->end) ||
        (*cursor->current != expected)) {
        return -1;
    }
    ++cursor->current;
    return 0;
}

static int parse_string(JsonCursor *cursor, char *output, size_t output_size)
{
    size_t length = 0;

    skip_space(cursor);
    if ((cursor->current >= cursor->end) || (*cursor->current != '"')) {
        return -1;
    }
    ++cursor->current;

    while (cursor->current < cursor->end) {
        char value = *cursor->current++;

        if (value == '"') {
            output[length] = '\0';
            return 0;
        }

        if (value == '\\') {
            if (cursor->current >= cursor->end) {
                return -1;
            }
            value = *cursor->current++;
            switch (value) {
            case '"':
            case '\\':
            case '/':
                break;
            case 'b':
                value = '\b';
                break;
            case 'f':
                value = '\f';
                break;
            case 'n':
                value = '\n';
                break;
            case 'r':
                value = '\r';
                break;
            case 't':
                value = '\t';
                break;
            default:
                return -1;
            }
        }

        if ((output_size == 0) || (length + 1 >= output_size)) {
            return -1;
        }
        output[length++] = value;
    }

    return -1;
}

static int parse_integer(JsonCursor *cursor, long *value)
{
    char *number_end;

    skip_space(cursor);
    if (cursor->current >= cursor->end) {
        return -1;
    }

    errno = 0;
    *value = strtol(cursor->current, &number_end, 10);
    if ((errno != 0) || (number_end == cursor->current) ||
        (number_end > cursor->end)) {
        return -1;
    }
    cursor->current = number_end;
    return 0;
}

static int skip_value(JsonCursor *cursor)
{
    char scratch[512];
    long number;
    static const char *const literals[] = {"true", "false", "null"};

    skip_space(cursor);
    if (cursor->current >= cursor->end) {
        return -1;
    }

    if (*cursor->current == '"') {
        return parse_string(cursor, scratch, sizeof(scratch));
    }

    if ((*cursor->current == '-') || isdigit((unsigned char)*cursor->current)) {
        return parse_integer(cursor, &number);
    }

    for (size_t i = 0; i < sizeof(literals) / sizeof(literals[0]); ++i) {
        size_t length = strlen(literals[i]);
        if (((size_t)(cursor->end - cursor->current) >= length) &&
            (strncmp(cursor->current, literals[i], length) == 0)) {
            cursor->current += length;
            return 0;
        }
    }

    return -1;
}

static int set_number(SenseConfig *config, const char *key, long value)
{
    if (strcmp(key, "width") == 0) {
        config->width = (unsigned int)value;
    } else if (strcmp(key, "height") == 0) {
        config->height = (unsigned int)value;
    } else if (strcmp(key, "fps_limit") == 0) {
        config->fps_limit = (unsigned int)value;
    } else if (strcmp(key, "jpeg_quality") == 0) {
        config->jpeg_quality = (unsigned int)value;
    } else if (strcmp(key, "http_port") == 0) {
        config->http_port = (int)value;
    } else if (strcmp(key, "motion_threshold") == 0) {
        config->motion_threshold = (unsigned int)value;
    } else if (strcmp(key, "motion_cooldown_ms") == 0) {
        config->motion_cooldown_ms = (unsigned int)value;
    } else {
        return 0;
    }
    return 1;
}

static int parse_config_json(const char *text, size_t length,
                             SenseConfig *config,
                             char *error, size_t error_size)
{
    JsonCursor cursor = {text, text + length};
    char key[64];

    if (consume(&cursor, '{') < 0) {
        set_error(error, error_size, "configuration must be a JSON object");
        return -1;
    }

    skip_space(&cursor);
    if ((cursor.current < cursor.end) && (*cursor.current == '}')) {
        ++cursor.current;
    } else {
        for (;;) {
            long number;

            if ((parse_string(&cursor, key, sizeof(key)) < 0) ||
                (consume(&cursor, ':') < 0)) {
                set_error(error, error_size, "invalid JSON key/value pair");
                return -1;
            }

            if (strcmp(key, "device") == 0) {
                if (parse_string(&cursor, config->device,
                                 sizeof(config->device)) < 0) {
                    set_error(error, error_size, "device must be a string");
                    return -1;
                }
            } else if (strcmp(key, "event_log") == 0) {
                if (parse_string(&cursor, config->event_log,
                                 sizeof(config->event_log)) < 0) {
                    set_error(error, error_size, "event_log must be a string");
                    return -1;
                }
            } else if ((strcmp(key, "width") == 0) ||
                       (strcmp(key, "height") == 0) ||
                       (strcmp(key, "fps_limit") == 0) ||
                       (strcmp(key, "jpeg_quality") == 0) ||
                       (strcmp(key, "http_port") == 0) ||
                       (strcmp(key, "motion_threshold") == 0) ||
                       (strcmp(key, "motion_cooldown_ms") == 0)) {
                if (parse_integer(&cursor, &number) < 0) {
                    set_error(error, error_size, "%s must be an integer", key);
                    return -1;
                }
                set_number(config, key, number);
            } else if (skip_value(&cursor) < 0) {
                set_error(error, error_size, "unsupported value for key %s", key);
                return -1;
            }

            skip_space(&cursor);
            if ((cursor.current < cursor.end) && (*cursor.current == ',')) {
                ++cursor.current;
                continue;
            }
            if ((cursor.current < cursor.end) && (*cursor.current == '}')) {
                ++cursor.current;
                break;
            }

            set_error(error, error_size, "expected ',' or '}'");
            return -1;
        }
    }

    skip_space(&cursor);
    if (cursor.current != cursor.end) {
        set_error(error, error_size, "trailing data after JSON object");
        return -1;
    }
    return 0;
}

static int validate_config(const SenseConfig *config,
                           char *error, size_t error_size)
{
    if (config->device[0] == '\0') {
        set_error(error, error_size, "device cannot be empty");
    } else if ((config->width == 0) || (config->width > 4096)) {
        set_error(error, error_size, "width must be between 1 and 4096");
    } else if ((config->height == 0) || (config->height > 2160)) {
        set_error(error, error_size, "height must be between 1 and 2160");
    } else if ((config->fps_limit == 0) || (config->fps_limit > 120)) {
        set_error(error, error_size, "fps_limit must be between 1 and 120");
    } else if ((config->jpeg_quality == 0) || (config->jpeg_quality > 100)) {
        set_error(error, error_size, "jpeg_quality must be between 1 and 100");
    } else if ((config->http_port <= 0) || (config->http_port > 65535)) {
        set_error(error, error_size, "http_port must be between 1 and 65535");
    } else {
        return 0;
    }
    return -1;
}

int config_load(const char *path, SenseConfig *config,
                char *error, size_t error_size)
{
    FILE *file;
    char *text;
    long file_size;
    size_t bytes_read;
    int result;

    memset(config, 0, sizeof(*config));
    snprintf(config->device, sizeof(config->device), "auto");
    config->width = 640;
    config->height = 480;
    config->fps_limit = 30;
    config->jpeg_quality = 75;
    config->http_port = 8080;
    snprintf(config->event_log, sizeof(config->event_log),
             "/var/log/imx6ull-sense/events.jsonl");
    config->motion_threshold = 12000;
    config->motion_cooldown_ms = 1500;

    file = fopen(path, "rb");
    if (file == NULL) {
        set_error(error, error_size, "cannot open %s: %s", path, strerror(errno));
        return -1;
    }

    if ((fseek(file, 0, SEEK_END) < 0) ||
        ((file_size = ftell(file)) < 0) ||
        ((unsigned long)file_size > CONFIG_MAX_BYTES) ||
        (fseek(file, 0, SEEK_SET) < 0)) {
        set_error(error, error_size, "cannot read configuration size");
        fclose(file);
        return -1;
    }

    text = malloc((size_t)file_size + 1);
    if (text == NULL) {
        set_error(error, error_size, "out of memory loading configuration");
        fclose(file);
        return -1;
    }

    bytes_read = fread(text, 1, (size_t)file_size, file);
    fclose(file);
    if (bytes_read != (size_t)file_size) {
        set_error(error, error_size, "short read loading configuration");
        free(text);
        return -1;
    }
    text[bytes_read] = '\0';

    result = parse_config_json(text, bytes_read, config, error, error_size);
    free(text);
    if (result < 0) {
        return -1;
    }
    return validate_config(config, error, error_size);
}
