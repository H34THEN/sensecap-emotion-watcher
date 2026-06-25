#include "circe_storage.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "circe_buf.h"
#include "circe_index.h"
#include "circe_storage_paths.h"
#include "circe_time.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"

static const char *TAG = "circe_storage";

static bool s_ready = false;
static bool s_probe_passed = false;
static bool s_last_save_non_atomic = false;
static char s_last_error[128] = {0};

void circe_storage_set_last_error(const char *msg)
{
    if (!msg) {
        s_last_error[0] = '\0';
        return;
    }
    strncpy(s_last_error, msg, sizeof(s_last_error) - 1);
    s_last_error[sizeof(s_last_error) - 1] = '\0';
}

const char *circe_storage_get_last_error(void)
{
    return s_last_error;
}

bool circe_storage_last_save_non_atomic(void)
{
    return s_last_save_non_atomic;
}

static bool mkdir_p(const char *path);

static bool file_exists_at(const char *path, bool want_dir)
{
    struct stat st;
    if (stat(path, &st) != 0) {
        return false;
    }
    return want_dir ? S_ISDIR(st.st_mode) : S_ISREG(st.st_mode);
}

static void cleanup_stale_tmp_in_dir(const char *dir_path)
{
    DIR *d = opendir(dir_path);
    if (!d) {
        return;
    }
    struct dirent *ent;
    char path[160];
    while ((ent = readdir(d)) != NULL) {
        size_t n = strlen(ent->d_name);
        if (n < 4 || strcasecmp(ent->d_name + n - 4, ".tmp") != 0) {
            continue;
        }
        if (!circe_storage_path_join(path, sizeof(path), dir_path, ent->d_name)) {
            continue;
        }
        if (unlink(path) == 0) {
            ESP_LOGW(TAG, "removed stale tmp %s", path);
        }
    }
    closedir(d);
}

static void cleanup_stale_tmp_under_entries(void)
{
    const char *entries = circe_storage_path_entries();
    DIR *root = opendir(entries);
    if (!root) {
        return;
    }
    struct dirent *ent;
    char dir_path[128];
    while ((ent = readdir(root)) != NULL) {
        if (ent->d_name[0] == '.') {
            continue;
        }
        if (!circe_storage_path_join(dir_path, sizeof(dir_path), entries, ent->d_name)) {
            continue;
        }
        struct stat st;
        if (stat(dir_path, &st) == 0 && S_ISDIR(st.st_mode)) {
            cleanup_stale_tmp_in_dir(dir_path);
        }
    }
    closedir(root);
}

static bool ensure_entry_dir(const char *dir_path)
{
    struct stat st;
    if (stat(dir_path, &st) == 0 && S_ISDIR(st.st_mode)) {
        ESP_LOGI(TAG, "save_json dir exists %s", dir_path);
        return true;
    }
    ESP_LOGW(TAG, "save_json dir missing, mkdir %s errno=%d", dir_path, errno);
    if (!mkdir_p(dir_path)) {
        ESP_LOGE(TAG, "save_json mkdir failed dir=%s errno=%d", dir_path, errno);
        return false;
    }
    if (stat(dir_path, &st) == 0 && S_ISDIR(st.st_mode)) {
        ESP_LOGI(TAG, "save_json dir created %s", dir_path);
        return true;
    }
    ESP_LOGE(TAG, "save_json dir still missing after mkdir %s", dir_path);
    return false;
}

static bool write_file_payload(const char *path, const char *payload, size_t payload_len)
{
    FILE *f = fopen(path, "w");
    if (!f) {
        ESP_LOGE(TAG, "write open fail path='%s' errno=%d", path, errno);
        return false;
    }
    size_t n = fwrite(payload, 1, payload_len, f);
    fflush(f);
    fsync(fileno(f));
    fclose(f);
    if (n != payload_len) {
        ESP_LOGE(TAG, "write short path='%s' wrote=%u need=%u", path, (unsigned)n, (unsigned)payload_len);
        unlink(path);
        return false;
    }
    return true;
}

static void log_rename_attempt(const char *temp_path, const char *final_path, const char *phase)
{
    bool temp_exists = file_exists_at(temp_path, false);
    bool final_exists = file_exists_at(final_path, false);
    ESP_LOGI(TAG, "commit %s temp='%s' len=%u exists=%d final='%s' len=%u exists=%d", phase, temp_path,
             (unsigned)strlen(temp_path), temp_exists, final_path, (unsigned)strlen(final_path), final_exists);
}

static bool try_rename_temp_to_final(const char *temp_path, const char *final_path, bool remove_final_first)
{
    if (remove_final_first && access(final_path, F_OK) == 0) {
        errno = 0;
        int ur = unlink(final_path);
        if (ur != 0) {
            ESP_LOGE(TAG, "commit FINAL_REMOVE_FAILED final='%s' errno=%d (%s)", final_path, errno, strerror(errno));
            circe_storage_set_last_error("FINAL_REMOVE_FAILED");
            return false;
        }
        ESP_LOGI(TAG, "commit removed existing final %s", final_path);
    }

    log_rename_attempt(temp_path, final_path, remove_final_first ? "rename_after_remove" : "rename");
    errno = 0;
    int rc = rename(temp_path, final_path);
    int e = errno;
    if (rc != 0) {
        ESP_LOGE(TAG, "commit rename FAIL rc=%d errno=%d (%s) temp='%s' final='%s'", rc, e, strerror(e), temp_path,
                 final_path);
        return false;
    }
    ESP_LOGI(TAG, "commit rename OK temp='%s' -> final='%s'", temp_path, final_path);
    return true;
}

static bool direct_write_final_with_verify(const char *final_path, const char *json, size_t json_len)
{
    if (!write_file_payload(final_path, json, json_len)) {
        circe_storage_set_last_error("Failed to write entry file");
        return false;
    }
    if (!file_exists_at(final_path, false)) {
        circe_storage_set_last_error("Failed to finalize entry file");
        return false;
    }
    ESP_LOGI(TAG, "commit direct write OK final='%s' bytes=%u", final_path, (unsigned)json_len);
    return true;
}

typedef enum {
    CIRCE_COMMIT_ATOMIC_RENAME = 0,
    CIRCE_COMMIT_REMOVE_RENAME,
    CIRCE_COMMIT_DIRECT_WRITE,
} circe_commit_mode_t;

static bool commit_json_file(const char *dir_path, const char *temp_path, const char *final_path, const char *json,
                             size_t json_len, circe_commit_mode_t *mode_out)
{
    cleanup_stale_tmp_in_dir(dir_path);

    if (!write_file_payload(temp_path, json, json_len)) {
        circe_storage_set_last_error("Failed to write entry file");
        return false;
    }

    if (try_rename_temp_to_final(temp_path, final_path, false)) {
        if (mode_out) {
            *mode_out = CIRCE_COMMIT_ATOMIC_RENAME;
        }
        return true;
    }

    if (try_rename_temp_to_final(temp_path, final_path, true)) {
        if (mode_out) {
            *mode_out = CIRCE_COMMIT_REMOVE_RENAME;
        }
        return true;
    }

    ESP_LOGW(TAG, "commit rename failed; fallback direct write final='%s'", final_path);
    if (!direct_write_final_with_verify(final_path, json, json_len)) {
        return false;
    }
    unlink(temp_path);
    if (mode_out) {
        *mode_out = CIRCE_COMMIT_DIRECT_WRITE;
    }
    return true;
}

static const char *commit_mode_name(circe_commit_mode_t mode)
{
    switch (mode) {
    case CIRCE_COMMIT_ATOMIC_RENAME:
        return "atomic_rename";
    case CIRCE_COMMIT_REMOVE_RENAME:
        return "remove_rename";
    case CIRCE_COMMIT_DIRECT_WRITE:
        return "direct_write";
    default:
        return "unknown";
    }
}

static bool mkdir_p(const char *path)
{
    char tmp[128];
    strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
                return false;
            }
            *p = '/';
        }
    }
    return mkdir(tmp, 0755) == 0 || errno == EEXIST;
}

static bool sd_mount_exists(void)
{
    struct stat st;
    return stat("/sdcard", &st) == 0 && S_ISDIR(st.st_mode);
}

static bool dir_exists(const char *path)
{
    struct stat st;
    return path && stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static bool is_entry_json_filename(const char *name)
{
    if (!name || name[0] == '.') {
        return false;
    }
    size_t n = strlen(name);
    if (n >= 4 && strcasecmp(name + n - 4, ".tmp") == 0) {
        return false;
    }
    if (n >= 4 && strcasecmp(name + n - 4, ".JSN") == 0) {
        return true;
    }
    if (n >= 5 && strcasecmp(name + n - 5, ".JSON") == 0) {
        return true;
    }
    if (n >= 5 && strcasecmp(name + n - 5, ".json") == 0) {
        return true;
    }
    if (n >= 4 && strcasecmp(name + n - 4, ".DAT") == 0) {
        return true;
    }
    if (strchr(name, '.') == NULL && n >= 4 && n <= CIRCE_MAX_ID) {
        for (size_t i = 0; i < n; i++) {
            if (!isxdigit((unsigned char)name[i])) {
                return false;
            }
        }
        return true;
    }
    return false;
}

static const char *entry_lookup_exts[] = {".JSN", ".JSON", ".json", ".DAT", ""};

static bool build_entry_file_path(const char *date_folder, const char *id, const char *ext, char *out,
                                  size_t out_len)
{
    char dir[128];
    if (!circe_storage_path_join(dir, sizeof(dir), circe_storage_path_entries(), date_folder)) {
        return false;
    }
    char file[64];
    int n = snprintf(file, sizeof(file), "%s%s", id, ext);
    if (n < 0 || (size_t)n >= sizeof(file)) {
        ESP_LOGE(TAG, "PATH_TOO_LONG entry file id='%s' ext='%s'", id, ext);
        return false;
    }
    return circe_storage_path_join(out, out_len, dir, file);
}

static void entry_json_path(const circe_entry_t *entry, char *path, size_t len)
{
    char day[16];
    if (!circe_time_entry_storage_folder(entry, day, sizeof(day))) {
        path[0] = '\0';
        return;
    }
    if (!circe_storage_path_join(path, len, circe_storage_path_entries(), day)) {
        path[0] = '\0';
        return;
    }
    char file[48];
    snprintf(file, sizeof(file), "%s%s", entry->id, circe_storage_path_entry_ext());
    char full[160];
    if (!circe_storage_path_join(full, sizeof(full), path, file)) {
        path[0] = '\0';
        return;
    }
    strncpy(path, full, len - 1);
    path[len - 1] = '\0';
}

void circe_storage_entry_json_path(const circe_entry_t *entry, char *path, size_t len)
{
    entry_json_path(entry, path, len);
}

static void log_fat_ext_probe_case(const char *logs_dir, const char *final_name)
{
    char temp_path[128];
    char final_path[128];
    if (!circe_storage_path_join(temp_path, sizeof(temp_path), logs_dir, "PROBE.TMP") ||
        !circe_storage_path_join(final_path, sizeof(final_path), logs_dir, final_name)) {
        return;
    }

    unlink(temp_path);
    unlink(final_path);

    const char payload[] = "x";

    errno = 0;
    FILE *df = fopen(final_path, "w");
    int de = errno;
    ESP_LOGI(TAG, "fatname '%s' direct_fopen=%s errno=%d (%s)", final_name, df ? "OK" : "FAIL", de,
             df ? "ok" : strerror(de));
    if (df) {
        fputs(payload, df);
        fflush(df);
        fclose(df);
        errno = 0;
        int dr = unlink(final_path);
        ESP_LOGI(TAG, "fatname '%s' direct_delete=%s errno=%d", final_name, dr == 0 ? "OK" : "FAIL",
                 dr == 0 ? 0 : errno);
    }

    errno = 0;
    FILE *tf = fopen(temp_path, "w");
    if (!tf) {
        ESP_LOGI(TAG, "fatname '%s' temp_fopen=FAIL errno=%d (%s)", final_name, errno, strerror(errno));
        unlink(temp_path);
        return;
    }
    fputs(payload, tf);
    fflush(tf);
    fclose(tf);

    errno = 0;
    int rc = rename(temp_path, final_path);
    int re = errno;
    ESP_LOGI(TAG, "fatname '%s' rename=%s rc=%d errno=%d (%s)", final_name, rc == 0 ? "OK" : "FAIL", rc, re,
             rc == 0 ? "ok" : strerror(re));

    if (access(final_path, F_OK) == 0) {
        errno = 0;
        int dr = unlink(final_path);
        ESP_LOGI(TAG, "fatname '%s' rename_delete=%s errno=%d", final_name, dr == 0 ? "OK" : "FAIL",
                 dr == 0 ? 0 : errno);
    }
    unlink(temp_path);
}

static void run_fat_filename_extension_probe(const char *logs_dir)
{
    ESP_LOGI(TAG, "fatname probe start dir='%s'", logs_dir);
    log_fat_ext_probe_case(logs_dir, "PROBE.TMP");
    log_fat_ext_probe_case(logs_dir, "PROBE.JSN");
    log_fat_ext_probe_case(logs_dir, "PROBE.JSON");
    log_fat_ext_probe_case(logs_dir, "PROBE.DAT");
    log_fat_ext_probe_case(logs_dir, "PROBE");
    ESP_LOGI(TAG, "fatname probe done");
}

bool circe_storage_run_probe(circe_storage_probe_result_t *out)
{
    circe_storage_probe_result_t local = {0};
    circe_storage_probe_result_t *r = out ? out : &local;

    const char *base = circe_storage_path_base();
    const char *logs = circe_storage_path_logs_dir();
    const char *probe_path = circe_storage_path_probe_tmp();
    const char *mode_w = "w";
    const char *mode_r = "r";
    const char payload[] = "ok\n";

    ESP_LOGI(TAG, "probe start base='%s' logs='%s' path='%s' len=%u mode_w='%s'", base, logs, probe_path,
             (unsigned)strlen(probe_path), mode_w);

    struct stat st;
    if (stat("/sdcard", &st) != 0 || !S_ISDIR(st.st_mode)) {
        snprintf(r->detail, sizeof(r->detail), "A stat /sdcard fail errno=%d", errno);
        circe_storage_set_last_error(r->detail);
        ESP_LOGE(TAG, "probe %s", r->detail);
        return false;
    }
    ESP_LOGI(TAG, "probe A stat /sdcard OK");

    if (stat(base, &st) != 0 || !S_ISDIR(st.st_mode)) {
        snprintf(r->detail, sizeof(r->detail), "B stat base fail errno=%d", errno);
        circe_storage_set_last_error(r->detail);
        ESP_LOGE(TAG, "probe %s base='%s'", r->detail, base);
        return false;
    }
    ESP_LOGI(TAG, "probe B stat base OK");

    if (stat(logs, &st) != 0 || !S_ISDIR(st.st_mode)) {
        snprintf(r->detail, sizeof(r->detail), "C stat logs fail errno=%d", errno);
        circe_storage_set_last_error(r->detail);
        ESP_LOGE(TAG, "probe %s logs='%s'", r->detail, logs);
        return false;
    }
    ESP_LOGI(TAG, "probe C stat logs OK");

    errno = 0;
    FILE *f = fopen(probe_path, mode_w);
    if (!f) {
        int e1 = errno;
        ESP_LOGE(TAG, "probe D fopen(w) FAIL path='%s' mode='%s' errno=%d", probe_path, mode_w, e1);

        errno = 0;
        int fd = open(probe_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            int e2 = errno;
            if (e2 == 22 || e1 == 22) {
                snprintf(r->detail, sizeof(r->detail), "Storage path invalid. Probe: %s", probe_path);
            } else {
                snprintf(r->detail, sizeof(r->detail), "probe open fail errno=%d path='%s'", e2, probe_path);
            }
            circe_storage_set_last_error(r->detail);
            ESP_LOGE(TAG, "probe D open() FAIL errno=%d (fopen was %d) path='%s'", e2, e1, probe_path);
            return false;
        }
        if (e1 == 22) {
            ESP_LOGW(TAG, "probe: fopen(w) EINVAL but open() succeeded path='%s'", probe_path);
        }
        ssize_t nw = write(fd, payload, strlen(payload));
        fsync(fd);
        close(fd);
        r->write_ok = (nw == (ssize_t)strlen(payload));
        ESP_LOGI(TAG, "probe D/E open+write OK bytes=%d", (int)nw);
    } else {
        ESP_LOGI(TAG, "probe D fopen(w) OK");
        if (fputs(payload, f) < 0) {
            ESP_LOGE(TAG, "probe E fputs FAIL errno=%d", errno);
        } else {
            ESP_LOGI(TAG, "probe E fputs OK");
        }
        fflush(f);
        fsync(fileno(f));
        fclose(f);
        r->write_ok = true;
        ESP_LOGI(TAG, "probe F/G fclose OK");
    }

    char read_buf[16] = {0};
    errno = 0;
    f = fopen(probe_path, mode_r);
    if (!f) {
        int e1 = errno;
        ESP_LOGW(TAG, "probe H fopen(r) FAIL errno=%d, trying open()", e1);
        int fd = open(probe_path, O_RDONLY, 0);
        if (fd < 0) {
            snprintf(r->detail, sizeof(r->detail), "probe read open fail errno=%d", errno);
            circe_storage_set_last_error(r->detail);
            unlink(probe_path);
            return false;
        }
        ssize_t nr = read(fd, read_buf, sizeof(read_buf) - 1);
        close(fd);
        r->read_ok = (nr == (ssize_t)strlen(payload) && strcmp(read_buf, payload) == 0);
        ESP_LOGI(TAG, "probe I read OK bytes=%d", (int)nr);
    } else {
        ESP_LOGI(TAG, "probe H fopen(r) OK");
        size_t nr = fread(read_buf, 1, sizeof(read_buf) - 1, f);
        fclose(f);
        read_buf[nr] = '\0';
        r->read_ok = (nr == strlen(payload) && strcmp(read_buf, payload) == 0);
        ESP_LOGI(TAG, "probe I fread OK bytes=%u", (unsigned)nr);
    }

    r->delete_ok = (unlink(probe_path) == 0);
    ESP_LOGI(TAG, "probe J remove %s", r->delete_ok ? "OK" : "FAIL");

    run_fat_filename_extension_probe(logs);

    r->passed = r->write_ok && r->read_ok && r->delete_ok;
    snprintf(r->detail, sizeof(r->detail), "write=%s read=%s del=%s", r->write_ok ? "OK" : "FAIL",
             r->read_ok ? "OK" : "FAIL", r->delete_ok ? "OK" : "FAIL");
    s_probe_passed = r->passed;
    if (r->passed) {
        circe_storage_set_last_error("");
        ESP_LOGI(TAG, "probe PASS %s path='%s'", r->detail, probe_path);
    } else {
        circe_storage_set_last_error(r->detail);
        ESP_LOGE(TAG, "probe FAIL %s path='%s'", r->detail, probe_path);
    }
    return r->passed;
}

static bool ensure_storage_dirs(void)
{
    circe_storage_paths_resolve();
    if (!mkdir_p(circe_storage_path_base()) || !mkdir_p(circe_storage_path_entries()) ||
        !mkdir_p(circe_storage_path_index_dir()) || !mkdir_p(circe_storage_path_cache_dir()) ||
        !mkdir_p(circe_storage_path_logs_dir())) {
        ESP_LOGE(TAG, "mkdir failed base=%s", circe_storage_path_base());
        circe_storage_set_last_error("Failed to create storage directories");
        return false;
    }
    return true;
}

bool circe_storage_is_ready(void)
{
    return s_ready && s_probe_passed && sd_mount_exists();
}

bool circe_storage_ensure_ready(void)
{
    if (circe_storage_is_ready()) {
        return true;
    }
    ESP_LOGW(TAG, "ensure_ready: reinit (ready=%d probe=%d mount=%d)", s_ready, s_probe_passed,
             sd_mount_exists());
    return circe_storage_reinit();
}

bool circe_storage_init(void)
{
    if (s_ready && s_probe_passed) {
        return true;
    }
    if (!sd_mount_exists()) {
        circe_storage_set_last_error("SD card not mounted");
        ESP_LOGE(TAG, "init fail: SD not mounted");
        s_ready = false;
        s_probe_passed = false;
        return false;
    }
    if (!ensure_storage_dirs()) {
        s_ready = false;
        s_probe_passed = false;
        return false;
    }
    if (!circe_index_open()) {
        circe_storage_set_last_error("Failed to open index");
        s_ready = false;
        s_probe_passed = false;
        return false;
    }
    circe_index_load_dirty_state();
    if (!circe_storage_run_probe(NULL)) {
        ESP_LOGE(TAG, "init fail: probe failed");
        s_ready = false;
        return false;
    }
    s_ready = true;
    circe_storage_set_last_error("");
    cleanup_stale_tmp_under_entries();
    ESP_LOGI(TAG, "storage ready base=%s entries=%s index=%s", circe_storage_path_base(),
             circe_storage_path_entries(), circe_storage_path_index_dir());
    circe_storage_rebuild_index_if_dirty(NULL);
    return true;
}

bool circe_storage_reinit(void)
{
    circe_storage_deinit();
    return circe_storage_init();
}

void circe_storage_deinit(void)
{
    circe_index_close();
    s_ready = false;
    s_probe_passed = false;
}

bool circe_entry_create(circe_entry_t *entry, circe_entry_mode_t mode)
{
    circe_entry_init_defaults(entry, mode);
    return true;
}

bool circe_entry_save_json_atomic(const circe_entry_t *entry)
{
    s_last_save_non_atomic = false;
    if (!entry) {
        circe_storage_set_last_error("Null entry");
        return false;
    }
    if (!s_ready) {
        circe_storage_set_last_error("Storage not ready");
        ESP_LOGE(TAG, "save_json: storage not ready");
        return false;
    }

    char dir_path[96];
    char day_folder[16];
    if (!circe_time_entry_storage_folder(entry, day_folder, sizeof(day_folder))) {
        circe_storage_set_last_error("Missing entry folder");
        ESP_LOGE(TAG, "save_json: folder resolve failed");
        return false;
    }
    if (day_folder[0] == '\0') {
        circe_storage_set_last_error("Missing local_date");
        ESP_LOGE(TAG, "save_json: missing entry folder");
        return false;
    }
    if (!circe_storage_path_join(dir_path, sizeof(dir_path), circe_storage_path_entries(), day_folder)) {
        circe_storage_set_last_error("Entry path too long");
        ESP_LOGE(TAG, "save_json: PATH_TOO_LONG dir for date=%s", entry->local_date);
        return false;
    }
    if (!ensure_entry_dir(dir_path)) {
        circe_storage_set_last_error("Failed to create entry directory");
        return false;
    }

    char final_path[128];
    char temp_path[140];
    entry_json_path(entry, final_path, sizeof(final_path));
    if (final_path[0] == '\0') {
        circe_storage_set_last_error("Entry path too long");
        ESP_LOGE(TAG, "save_json: PATH_TOO_LONG final path id=%s", entry->id);
        return false;
    }
    char temp_name[64];
    int tn = snprintf(temp_name, sizeof(temp_name), "%s.TMP", entry->id);
    if (tn < 0 || (size_t)tn >= sizeof(temp_name)) {
        circe_storage_set_last_error("Entry path too long");
        return false;
    }
    if (!circe_storage_path_join(temp_path, sizeof(temp_path), dir_path, temp_name)) {
        circe_storage_set_last_error("Entry path too long");
        return false;
    }

    char *json = NULL;
    if (!circe_json_buf_alloc(&json, CIRCE_JSON_BUF_SIZE)) {
        circe_storage_set_last_error("MEMORY_ALLOCATION_FAILED");
        ESP_LOGE(TAG, "save_json: alloc failed id=%s", entry->id);
        return false;
    }

    bool ok = false;
    if (!circe_entry_to_json(entry, json, CIRCE_JSON_BUF_SIZE)) {
        circe_storage_set_last_error("JSON serialization failed");
        ESP_LOGE(TAG, "save_json: to_json failed id=%s", entry->id);
        goto done;
    }

    circe_commit_mode_t mode = CIRCE_COMMIT_ATOMIC_RENAME;
    ok = commit_json_file(dir_path, temp_path, final_path, json, strlen(json), &mode);
    if (ok) {
        s_last_save_non_atomic = (mode == CIRCE_COMMIT_DIRECT_WRITE);
        ESP_LOGI(TAG, "save_json commit=%s id=%s final='%s'", commit_mode_name(mode), entry->id, final_path);
        if (s_last_save_non_atomic) {
            circe_storage_set_last_error("JSON_OK_NON_ATOMIC");
        } else {
            circe_storage_set_last_error("");
        }
    } else if (file_exists_at(temp_path, false)) {
        ESP_LOGW(TAG, "save_json leaving temp for recovery %s", temp_path);
    }

done:
    circe_json_buf_free(json);
    return ok;
}

bool circe_entry_update(circe_entry_t *entry)
{
    if (!s_ready || !entry || !entry->id[0]) {
        circe_storage_set_last_error("Invalid entry for update");
        return false;
    }
    entry->revision++;
    circe_entry_touch_updated(entry);
    entry->training_ok = false;
    entry->private_locked = true;
    if (!circe_entry_save_json_atomic(entry)) {
        circe_storage_set_last_error("Update save failed");
        return false;
    }
    circe_entry_index_insert_best_effort(entry);
    circe_storage_set_last_error("");
    return true;
}

bool circe_entry_index_insert_best_effort(const circe_entry_t *entry)
{
    char json_path[128];
    entry_json_path(entry, json_path, sizeof(json_path));
    return circe_index_append_best_effort(entry, json_path);
}

bool circe_entry_index_insert(const circe_entry_t *entry)
{
    char json_path[128];
    entry_json_path(entry, json_path, sizeof(json_path));
    return circe_index_insert(entry, json_path);
}

bool circe_entry_load(const char *id, circe_entry_t *entry)
{
    if (!id || !entry) {
        return false;
    }
    char json_path[128] = {0};
    if (!circe_index_get_json_path(id, json_path, sizeof(json_path))) {
        circe_storage_find_json_path_for_id(id, json_path, sizeof(json_path));
    }
    if (!json_path[0]) {
        return false;
    }
    FILE *f = fopen(json_path, "r");
    if (!f) {
        return false;
    }
    char *json = NULL;
    if (!circe_json_buf_alloc(&json, CIRCE_JSON_BUF_SIZE)) {
        fclose(f);
        circe_storage_set_last_error("MEMORY_ALLOCATION_FAILED");
        return false;
    }
    size_t n = fread(json, 1, CIRCE_JSON_BUF_SIZE - 1, f);
    json[n] = '\0';
    fclose(f);
    bool ok = circe_entry_from_json(json, entry);
    circe_json_buf_free(json);
    if (!ok) {
        return false;
    }
    strncpy(entry->json_path, json_path, sizeof(entry->json_path) - 1);
    entry->json_path[sizeof(entry->json_path) - 1] = '\0';
    return true;
}

bool circe_storage_find_json_path_for_id(const char *id, char *path_out, size_t path_len)
{
    if (!id || !path_out || path_len == 0) {
        return false;
    }
    path_out[0] = '\0';

    DIR *root = opendir(circe_storage_path_entries());
    if (!root) {
        return false;
    }
    struct dirent *date_ent;
    char path[160];
    for (size_t i = 0; i < sizeof(entry_lookup_exts) / sizeof(entry_lookup_exts[0]); i++) {
        while ((date_ent = readdir(root)) != NULL) {
            if (date_ent->d_name[0] == '.') {
                continue;
            }
            if (!build_entry_file_path(date_ent->d_name, id, entry_lookup_exts[i], path, sizeof(path))) {
                continue;
            }
            if (access(path, R_OK) == 0) {
                strncpy(path_out, path, path_len - 1);
                path_out[path_len - 1] = '\0';
                closedir(root);
                return true;
            }
        }
        rewinddir(root);
    }
    closedir(root);
    return false;
}

static bool scan_entries_for_latest(char *id_out, size_t id_len, char *path_out, size_t path_len)
{
    if (!id_out || id_len == 0) {
        return false;
    }
    id_out[0] = '\0';
    if (path_out && path_len > 0) {
        path_out[0] = '\0';
    }

    char best_id[CIRCE_MAX_ID] = {0};
    char best_path[128] = {0};
    char best_at[32] = {0};

    DIR *root = opendir(circe_storage_path_entries());
    if (!root) {
        return false;
    }
    struct dirent *date_ent;
    char date_dir[128];
    while ((date_ent = readdir(root)) != NULL) {
        if (date_ent->d_name[0] == '.') {
            continue;
        }
        snprintf(date_dir, sizeof(date_dir), "%s/%s", circe_storage_path_entries(), date_ent->d_name);
        struct stat st;
        if (stat(date_dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
            continue;
        }
        DIR *day = opendir(date_dir);
        if (!day) {
            continue;
        }
        struct dirent *file_ent;
        char file_path[160];
        char *json = NULL;
        if (!circe_json_buf_alloc(&json, CIRCE_JSON_BUF_SIZE)) {
            closedir(day);
            closedir(root);
            circe_storage_set_last_error("MEMORY_ALLOCATION_FAILED");
            return false;
        }
        while ((file_ent = readdir(day)) != NULL) {
            if (!is_entry_json_filename(file_ent->d_name)) {
                continue;
            }
            snprintf(file_path, sizeof(file_path), "%s/%s", date_dir, file_ent->d_name);
            FILE *f = fopen(file_path, "r");
            if (!f) {
                continue;
            }
            size_t n = fread(json, 1, CIRCE_JSON_BUF_SIZE - 1, f);
            json[n] = '\0';
            fclose(f);

            circe_entry_t entry;
            if (!circe_entry_from_json(json, &entry)) {
                continue;
            }
            if (entry.lifecycle_state != CIRCE_LIFECYCLE_ACTIVE) {
                continue;
            }
            const char *created = entry.created_at[0] ? entry.created_at : entry.updated_at;
            if (!best_at[0] || (created[0] && strcmp(created, best_at) > 0)) {
                strncpy(best_at, created, sizeof(best_at) - 1);
                best_at[sizeof(best_at) - 1] = '\0';
                strncpy(best_id, entry.id, sizeof(best_id) - 1);
                best_id[sizeof(best_id) - 1] = '\0';
                strncpy(best_path, file_path, sizeof(best_path) - 1);
                best_path[sizeof(best_path) - 1] = '\0';
            }
        }
        circe_json_buf_free(json);
        closedir(day);
    }
    closedir(root);

    if (!best_id[0]) {
        return false;
    }
    strncpy(id_out, best_id, id_len - 1);
    id_out[id_len - 1] = '\0';
    if (path_out && path_len > 0) {
        strncpy(path_out, best_path, path_len - 1);
        path_out[path_len - 1] = '\0';
    }
    return true;
}

bool circe_entry_delete_hard(const char *id)
{
    if (!id) {
        return false;
    }
    char json_path[128] = {0};
    if (!circe_index_get_json_path(id, json_path, sizeof(json_path))) {
        circe_storage_find_json_path_for_id(id, json_path, sizeof(json_path));
    }
    if (json_path[0]) {
        unlink(json_path);
    }
    circe_index_delete(id);
    return true;
}

static bool index_one_json_file(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        return false;
    }
    char *json = NULL;
    if (!circe_json_buf_alloc(&json, CIRCE_JSON_BUF_SIZE)) {
        fclose(f);
        return false;
    }
    size_t n = fread(json, 1, CIRCE_JSON_BUF_SIZE - 1, f);
    json[n] = '\0';
    fclose(f);

    circe_entry_t entry;
    bool ok = false;
    if (circe_entry_from_json(json, &entry)) {
        if (entry.lifecycle_state != CIRCE_LIFECYCLE_ACTIVE) {
            ok = true;
        } else {
            ok = circe_entry_index_insert(&entry);
        }
    }
    circe_json_buf_free(json);
    return ok;
}

static void scan_dir_recursive(const char *dir, int *count)
{
    DIR *d = opendir(dir);
    if (!d) {
        return;
    }
    struct dirent *ent;
    char path[256];
    while ((ent = readdir(d)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
        }
        snprintf(path, sizeof(path), "%s/%s", dir, ent->d_name);
        struct stat st;
        if (stat(path, &st) != 0) {
            continue;
        }
        if (S_ISDIR(st.st_mode)) {
            scan_dir_recursive(path, count);
        } else if (S_ISREG(st.st_mode) && is_entry_json_filename(ent->d_name)) {
            if (index_one_json_file(path)) {
                (*count)++;
            }
        }
    }
    closedir(d);
}

bool circe_rebuild_index_from_json(int *out_count)
{
    if (!circe_index_clear()) {
        return false;
    }
    int count = 0;
    scan_dir_recursive(circe_storage_path_entries(), &count);
    if (out_count) {
        *out_count = count;
    }
    circe_index_clear_dirty();
    ESP_LOGI(TAG, "rebuilt index: %d entries", count);
    return true;
}

bool circe_storage_rebuild_index_if_dirty(int *out_count)
{
    if (!circe_index_is_dirty()) {
        return true;
    }
    ESP_LOGW(TAG, "rebuilding dirty index from JSON");
    return circe_rebuild_index_from_json(out_count);
}

bool circe_storage_health_check(circe_storage_health_t *health)
{
    if (!health) {
        return false;
    }
    memset(health, 0, sizeof(*health));
    health->sd_mounted = sd_mount_exists();
    health->storage_ready = circe_storage_is_ready();
    health->probe_passed = s_probe_passed;
    health->writable = s_probe_passed;
    health->entries_dir_ok = dir_exists(circe_storage_path_entries());
    health->index_dir_ok = dir_exists(circe_storage_path_index_dir());
    health->index_dirty = circe_index_is_dirty();
    strncpy(health->base_path, circe_storage_path_base(), sizeof(health->base_path) - 1);
    strncpy(health->entries_path, circe_storage_path_entries(), sizeof(health->entries_path) - 1);
    strncpy(health->index_path, circe_storage_path_index_file(), sizeof(health->index_path) - 1);
    circe_index_count(&health->entry_count);

    uint64_t total_bytes = 0;
    uint64_t free_bytes = 0;
    if (esp_vfs_fat_info("/sdcard", &total_bytes, &free_bytes) == ESP_OK && total_bytes > 0) {
        health->card_size_mb = (long)(total_bytes / (1024ULL * 1024ULL));
    }

    if (!health->sd_mounted) {
        strncpy(health->last_error, "SD card not mounted", sizeof(health->last_error) - 1);
    } else if (!s_probe_passed) {
        strncpy(health->last_error, s_last_error[0] ? s_last_error : "Probe not passed", sizeof(health->last_error) - 1);
    } else if (!s_ready) {
        strncpy(health->last_error, "Storage not initialized", sizeof(health->last_error) - 1);
    } else if (s_last_error[0]) {
        strncpy(health->last_error, s_last_error, sizeof(health->last_error) - 1);
    }
    snprintf(health->probe_detail, sizeof(health->probe_detail), "%s", s_probe_passed ? "PASS" : "FAIL");
    return health->storage_ready;
}

bool circe_storage_today_strand(circe_strand_block_t *blocks, int max_blocks, int *out_count)
{
    if (!blocks || max_blocks <= 0 || !out_count) {
        return false;
    }
    *out_count = 0;
    char today[CIRCE_MAX_DATE];
    time_t now = time(NULL);
    struct tm tm_local;
    localtime_r(&now, &tm_local);
    strftime(today, sizeof(today), "%Y-%m-%d", &tm_local);

    circe_index_row_t rows[32];
    int row_count = 0;
    if (!circe_index_list_for_date(today, rows, 32, &row_count)) {
        return false;
    }
    for (int i = 0; i < row_count && *out_count < max_blocks; i++) {
        circe_entry_t entry;
        if (!circe_entry_load(rows[i].id, &entry)) {
            continue;
        }
        strncpy(blocks[*out_count].color_hex, entry.color_hex, sizeof(blocks[0].color_hex) - 1);
        blocks[*out_count].color_hex[sizeof(blocks[0].color_hex) - 1] = '\0';
        (*out_count)++;
    }
    return true;
}

bool circe_storage_get_latest_entry_id(char *id_out, size_t id_len)
{
    if (!id_out || id_len == 0) {
        return false;
    }
    id_out[0] = '\0';

    if (circe_index_is_dirty()) {
        circe_storage_rebuild_index_if_dirty(NULL);
    }

    if (circe_index_get_latest_id(id_out, id_len) && id_out[0]) {
        char path[128];
        if (circe_index_get_json_path(id_out, path, sizeof(path)) && access(path, R_OK) == 0) {
            return true;
        }
    }

    char path[128];
    if (scan_entries_for_latest(id_out, id_len, path, sizeof(path))) {
        ESP_LOGW(TAG, "review fallback scan found latest id=%s", id_out);
        return true;
    }
    return false;
}

static bool self_test_read_contains_id(const char *json_path, const char *id)
{
    FILE *f = fopen(json_path, "r");
    if (!f) {
        return false;
    }
    char *json = NULL;
    if (!circe_json_buf_alloc(&json, CIRCE_JSON_BUF_SIZE)) {
        fclose(f);
        circe_storage_set_last_error("MEMORY_ALLOCATION_FAILED");
        return false;
    }
    size_t n = fread(json, 1, CIRCE_JSON_BUF_SIZE - 1, f);
    json[n] = '\0';
    fclose(f);
    bool ok = id && id[0] && strstr(json, id) != NULL;
    circe_json_buf_free(json);
    return ok;
}

bool circe_storage_run_save_self_test(circe_save_self_test_result_t *out)
{
    circe_save_self_test_result_t local = {0};
    if (out) {
        *out = local;
    }
    circe_save_self_test_result_t *r = out ? out : &local;

    if (!circe_storage_ensure_ready()) {
        snprintf(r->summary, sizeof(r->summary), "JSON FAIL  STORAGE_NOT_READY");
        circe_storage_set_last_error("Storage not ready");
        ESP_LOGE(TAG, "save self-test: storage not ready");
        return false;
    }

    circe_entry_t e;
    circe_entry_create(&e, CIRCE_ENTRY_MODE_QUICK);
    snprintf(e.color_hex, sizeof(e.color_hex), "#808080");

    char json_path[128];
    circe_storage_entry_json_path(&e, json_path, sizeof(json_path));
    ESP_LOGI(TAG, "save self-test A minimal entry id=%s path=%s", e.id, json_path);

    ESP_LOGI(TAG, "save self-test B write JSON");
    r->json_ok = circe_entry_save_json_atomic(&e);
    if (!r->json_ok) {
        const char *err = circe_storage_get_last_error();
        if (err && strstr(err, "MEMORY_ALLOCATION_FAILED")) {
            snprintf(r->summary, sizeof(r->summary), "MEMORY_ALLOCATION_FAILED");
        } else {
            snprintf(r->summary, sizeof(r->summary), "JSON FAIL  INDEX -  LOAD -  DEL -");
        }
        ESP_LOGE(TAG, "save self-test: %s", r->summary);
        return false;
    }

    ESP_LOGI(TAG, "save self-test C read back");
    r->load_ok = self_test_read_contains_id(json_path, e.id);

    ESP_LOGI(TAG, "save self-test D index optional");
    r->index_ok = circe_entry_index_insert_best_effort(&e);

    ESP_LOGI(TAG, "save self-test E delete file");
    r->delete_ok = (unlink(json_path) == 0);

    const char *json_tag = circe_storage_last_save_non_atomic() ? "OK_NON_ATOMIC" : "OK";
    snprintf(r->summary, sizeof(r->summary), "JSON %s  INDEX %s  LOAD %s  DEL %s", json_tag,
             r->index_ok ? "OK" : "WARN", r->load_ok ? "OK" : "FAIL", r->delete_ok ? "OK" : "FAIL");
    ESP_LOGI(TAG, "save self-test: %s", r->summary);
    return r->json_ok && r->load_ok && r->delete_ok;
}

bool circe_storage_run_self_test(void)
{
    circe_save_self_test_result_t r;
    return circe_storage_run_save_self_test(&r);
}
