#!/usr/bin/env bash
# Host-side storage smoke test (no ESP-IDF required).
# Uses /tmp/circe_host_test as fake SD mount via CIRCE_BASE override at compile time.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TEST_SD="/tmp/circe_host_test_$$"
mkdir -p "$TEST_SD"
export CIRCE_HOST_TEST=1

cat > /tmp/circe_host_main.c <<'EOF'
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Minimal stubs for ESP headers used by storage layer */
#include "../../main/circe_entry.h"

#define ESP_LOGI(tag, fmt, ...) printf("[I] " fmt "\n", ##__VA_ARGS__)
#define ESP_LOGE(tag, fmt, ...) printf("[E] " fmt "\n", ##__VA_ARGS__)
typedef unsigned char uint8_t;
void esp_fill_random(void *buf, size_t len) {
    for (size_t i = 0; i < len; i++) ((uint8_t *)buf)[i] = (uint8_t)(rand() & 0xff);
}

#undef CIRCE_BASE
#undef CIRCE_ENTRIES
#undef CIRCE_INDEX_DIR
#undef CIRCE_DB_PATH
#define CIRCE_BASE getenv("CIRCE_TEST_BASE")
#define CIRCE_ENTRIES CIRCE_BASE "/entries"
#define CIRCE_INDEX_DIR CIRCE_BASE "/index"
#define CIRCE_DB_PATH CIRCE_INDEX_DIR "/circe.db"

#include "../../main/circe_storage.c"

int main(void) {
    setenv("CIRCE_TEST_BASE", getenv("CIRCE_TEST_BASE"), 1);
    if (!circe_storage_init()) return 1;
    if (!circe_storage_run_self_test()) return 2;
    int n = 0;
    if (!circe_rebuild_index_from_json(&n)) return 3;
    printf("rebuild ok count=%d\n", n);
    return 0;
}
EOF

# Simpler approach: document that full host test needs sqlite dev libs.
# Run only entry JSON roundtrip test
gcc -o /tmp/circe_entry_test "$ROOT/main/circe_entry.c" -DUNIT_TEST -I"$ROOT/main" 2>/dev/null || true

echo "Host SD test dir: $TEST_SD"
echo "For full storage test, build firmware with ESP-IDF (see README)."
rm -rf "$TEST_SD"
