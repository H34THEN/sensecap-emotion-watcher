#pragma once

#include <stddef.h>
#include <stdbool.h>

#define CIRCE_JSON_BUF_SIZE 4096
#define CIRCE_INDEX_LINE_SIZE 640

void *circe_buf_alloc(size_t size);
void circe_buf_free(void *ptr);

bool circe_json_buf_alloc(char **out, size_t size);
void circe_json_buf_free(char *buf);

size_t circe_buf_free_heap(void);
