#include "circe_buf.h"

#include <stdlib.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "circe_buf";

void *circe_buf_alloc(size_t size)
{
    void *p = malloc(size);
    if (!p) {
        ESP_LOGE(TAG, "alloc failed size=%u heap_free=%u", (unsigned)size, (unsigned)esp_get_free_heap_size());
    }
    return p;
}

void circe_buf_free(void *ptr)
{
    free(ptr);
}

bool circe_json_buf_alloc(char **out, size_t size)
{
    if (!out) {
        return false;
    }
    *out = circe_buf_alloc(size);
    return *out != NULL;
}

void circe_json_buf_free(char *buf)
{
    circe_buf_free(buf);
}

size_t circe_buf_free_heap(void)
{
    return esp_get_free_heap_size();
}
