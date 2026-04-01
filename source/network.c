/**
 * 3DS CloudSaver - Network Module Implementation
 * ───────────────────────────────────────────────
 * Provides SFTP/SMB connectivity via raw TCP sockets.
 *
 * NOTE: The 3DS does not have a full POSIX socket stack by default.
 * This uses the SOC service from libctru. Full SFTP would require
 * porting libssh2 or a minimal SSH/SFTP implementation.
 *
 * For a first version, this implements a simple custom TCP protocol
 * that talks to a companion server application running on the user's
 * machine. The companion server exposes a minimal file-transfer API.
 *
 * Protocol (CloudSaver Transfer Protocol - CSTP):
 *   CMD  | Payload
 *   PING | (none) -> PONG
 *   LIST | path   -> file list (newline-separated)
 *   MKDR | path   -> OK/ERR
 *   UPLD | path + size + data -> OK/ERR
 *   DNLD | path   -> size + data / ERR
 *   DELE | path   -> OK/ERR
 *   EXST | path   -> YES/NO/ERR
 */

#include "network.h"

#include <malloc.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

/*═══════════════════════════════════════════════════════════════*
 *  Constants
 *═══════════════════════════════════════════════════════════════*/
#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x80000   /* 512 KB (reduced for stability) */
#define NET_TIMEOUT_MS  10000
#define RECV_BUF_SIZE   (256 * 1024)

/* CSTP command IDs */
#define CMD_PING  "PING"
#define CMD_LIST  "LIST"
#define CMD_MKDR  "MKDR"
#define CMD_UPLD  "UPLD"
#define CMD_DNLD  "DNLD"
#define CMD_DELE  "DELE"
#define CMD_EXST  "EXST"

#define RSP_OK    "OK"
#define RSP_ERR   "ERR"
#define RSP_PONG  "PONG"
#define RSP_YES   "YES"
#define RSP_NO    "NO"

/*═══════════════════════════════════════════════════════════════*
 *  State
 *═══════════════════════════════════════════════════════════════*/
static u32   *soc_buffer   = NULL;
static int    sock_fd      = -1;
static bool   connected    = false;
static char   last_error[256] = {0};
static char   server_base_path[MAX_PATH_LEN] = {0};

/*═══════════════════════════════════════════════════════════════*
 *  Helpers
 *═══════════════════════════════════════════════════════════════*/
static void set_error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(last_error, sizeof(last_error), fmt, ap);
    va_end(ap);
}

/** Send exactly `len` bytes. Returns true on success. */
static bool send_all(const void *data, size_t len)
{
    const u8 *p = (const u8 *)data;
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(sock_fd, p + sent, len - sent, 0);
        if (n <= 0) {
            set_error("send failed");
            return false;
        }
        sent += (size_t)n;
    }
    return true;
}

/** Receive exactly `len` bytes. Returns true on success. */
static bool recv_all(void *data, size_t len)
{
    u8 *p = (u8 *)data;
    size_t got = 0;
    while (got < len) {
        ssize_t n = recv(sock_fd, p + got, len - got, 0);
        if (n <= 0) {
            set_error("recv failed");
            return false;
        }
        got += (size_t)n;
    }
    return true;
}

/** Send a CSTP command: [4-byte cmd][4-byte payload_len][payload] */
static bool send_cmd(const char *cmd, const void *payload, u32 payload_len)
{
    if (!send_all(cmd, 4)) return false;
    u32 net_len = __builtin_bswap32(payload_len);
    if (!send_all(&net_len, 4)) return false;
    if (payload_len > 0 && payload) {
        if (!send_all(payload, payload_len)) return false;
    }
    return true;
}

/** Receive a CSTP response: [4-byte status][4-byte payload_len][payload]
 *  Caller must free *out_data if out_data is not NULL and *out_len > 0.
 */
static bool recv_response(char *status_out, void **out_data, u32 *out_len)
{
    char status[5] = {0};
    if (!recv_all(status, 4)) return false;
    if (status_out) memcpy(status_out, status, 4);

    u32 net_len = 0;
    if (!recv_all(&net_len, 4)) return false;
    u32 payload_len = __builtin_bswap32(net_len);

    if (out_len) *out_len = payload_len;

    if (payload_len > 0 && out_data) {
        *out_data = malloc(payload_len + 1); /* +1 for null term */
        if (!*out_data) {
            set_error("out of memory");
            return false;
        }
        if (!recv_all(*out_data, payload_len)) {
            free(*out_data);
            *out_data = NULL;
            return false;
        }
        ((char *)*out_data)[payload_len] = '\0';
    } else if (payload_len > 0) {
        /* Drain the payload */
        u8 drain[1024];
        u32 remaining = payload_len;
        while (remaining > 0) {
            u32 chunk = MIN(remaining, sizeof(drain));
            if (!recv_all(drain, chunk)) return false;
            remaining -= chunk;
        }
    }

    return true;
}

/** Build a full remote path from base + relative. */
static void build_full_path(char *out, size_t max, const char *relative)
{
    if (relative[0] == '/')
        snprintf(out, max, "%s%s", server_base_path, relative);
    else
        snprintf(out, max, "%s/%s", server_base_path, relative);
}

/*═══════════════════════════════════════════════════════════════*
 *  Public API
 *═══════════════════════════════════════════════════════════════*/

bool net_init(void)
{
    if (soc_buffer) return true; /* Already init */

    soc_buffer = (u32 *)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
    if (!soc_buffer) {
        set_error("Failed to allocate SOC buffer");
        return false;
    }

    Result res = socInit(soc_buffer, SOC_BUFFERSIZE);
    if (R_FAILED(res)) {
        free(soc_buffer);
        soc_buffer = NULL;
        set_error("socInit failed: 0x%08lX", (unsigned long)res);
        return false;
    }

    return true;
}

void net_exit(void)
{
    net_disconnect();
    socExit();
    if (soc_buffer) {
        free(soc_buffer);
        soc_buffer = NULL;
    }
}

bool net_connect(const ServerConfig *cfg)
{
    if (!cfg || cfg->type == CONN_NONE) {
        set_error("No server configured");
        return false;
    }

    net_disconnect();

    strncpy(server_base_path, cfg->remote_path, MAX_PATH_LEN - 1);

    /* Resolve hostname */
    struct hostent *host = gethostbyname(cfg->host);
    if (!host) {
        set_error("Cannot resolve host: %s", cfg->host);
        return false;
    }

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        set_error("socket() failed");
        return false;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons((u16)cfg->port);
    memcpy(&addr.sin_addr, host->h_addr_list[0], (size_t)host->h_length);

    if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        set_error("connect() failed to %s:%d", cfg->host, cfg->port);
        close(sock_fd);
        sock_fd = -1;
        return false;
    }

    /* Send PING to verify connection */
    if (!send_cmd(CMD_PING, NULL, 0)) {
        close(sock_fd);
        sock_fd = -1;
        return false;
    }

    char status[5] = {0};
    if (!recv_response(status, NULL, NULL) ||
        memcmp(status, RSP_PONG, 4) != 0)
    {
        set_error("Server did not respond with PONG");
        close(sock_fd);
        sock_fd = -1;
        return false;
    }

    connected = true;
    return true;
}

void net_disconnect(void)
{
    if (sock_fd >= 0) {
        close(sock_fd);
        sock_fd = -1;
    }
    connected = false;
}

bool net_is_connected(void)
{
    return connected && sock_fd >= 0;
}

bool net_upload(const char *local_path, const char *remote_path)
{
    if (!net_is_connected()) {
        set_error("Not connected");
        return false;
    }

    FILE *f = fopen(local_path, "rb");
    if (!f) {
        set_error("Cannot open local file: %s", local_path);
        return false;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    /* Build full remote path */
    char full_path[MAX_PATH_LEN];
    build_full_path(full_path, sizeof(full_path), remote_path);

    /* Payload: [path_len(4)][path][file_size(4)][file_data] */
    u32 path_len = (u32)strlen(full_path);
    u32 total_payload = 4 + path_len + 4 + (u32)file_size;

    u8 *payload = (u8 *)malloc(total_payload);
    if (!payload) {
        fclose(f);
        set_error("Out of memory");
        return false;
    }

    u32 offset = 0;
    u32 net_path_len = __builtin_bswap32(path_len);
    memcpy(payload + offset, &net_path_len, 4); offset += 4;
    memcpy(payload + offset, full_path, path_len); offset += path_len;

    u32 net_file_size = __builtin_bswap32((u32)file_size);
    memcpy(payload + offset, &net_file_size, 4); offset += 4;
    fread(payload + offset, 1, (size_t)file_size, f);
    fclose(f);

    bool ok = send_cmd(CMD_UPLD, payload, total_payload);
    free(payload);

    if (!ok) return false;

    char status[5] = {0};
    if (!recv_response(status, NULL, NULL)) return false;

    if (memcmp(status, RSP_OK, 2) != 0) {
        set_error("Upload failed: server returned %s", status);
        return false;
    }

    return true;
}

bool net_download(const char *remote_path, const char *local_path)
{
    if (!net_is_connected()) {
        set_error("Not connected");
        return false;
    }

    char full_path[MAX_PATH_LEN];
    build_full_path(full_path, sizeof(full_path), remote_path);

    u32 path_len = (u32)strlen(full_path);
    if (!send_cmd(CMD_DNLD, full_path, path_len)) return false;

    char status[5] = {0};
    void *data = NULL;
    u32 data_len = 0;

    if (!recv_response(status, &data, &data_len)) return false;

    if (memcmp(status, RSP_OK, 2) != 0 || !data) {
        if (data) free(data);
        set_error("Download failed");
        return false;
    }

    FILE *f = fopen(local_path, "wb");
    if (!f) {
        free(data);
        set_error("Cannot create local file: %s", local_path);
        return false;
    }

    fwrite(data, 1, data_len, f);
    fclose(f);
    free(data);

    return true;
}

int net_list_dir(const char *remote_path, char ***out_names)
{
    if (!net_is_connected()) {
        set_error("Not connected");
        return -1;
    }

    char full_path[MAX_PATH_LEN];
    build_full_path(full_path, sizeof(full_path), remote_path);

    u32 path_len = (u32)strlen(full_path);
    if (!send_cmd(CMD_LIST, full_path, path_len)) return -1;

    char status[5] = {0};
    void *data = NULL;
    u32 data_len = 0;

    if (!recv_response(status, &data, &data_len)) return -1;
    if (memcmp(status, RSP_OK, 2) != 0 || !data) {
        if (data) free(data);
        return -1;
    }

    /* Parse newline-separated file list */
    int count = 0;
    char *str = (char *)data;
    for (u32 i = 0; i < data_len; i++) {
        if (str[i] == '\n') count++;
    }
    if (data_len > 0 && str[data_len - 1] != '\n') count++;

    if (count == 0) {
        free(data);
        return 0;
    }

    *out_names = (char **)malloc(count * sizeof(char *));
    if (!*out_names) {
        free(data);
        return -1;
    }

    int idx = 0;
    char *token = strtok(str, "\n");
    while (token && idx < count) {
        (*out_names)[idx] = strdup(token);
        idx++;
        token = strtok(NULL, "\n");
    }

    free(data);
    return idx;
}

bool net_mkdir(const char *remote_path)
{
    if (!net_is_connected()) return false;

    char full_path[MAX_PATH_LEN];
    build_full_path(full_path, sizeof(full_path), remote_path);

    if (!send_cmd(CMD_MKDR, full_path, (u32)strlen(full_path)))
        return false;

    char status[5] = {0};
    if (!recv_response(status, NULL, NULL)) return false;
    return memcmp(status, RSP_OK, 2) == 0;
}

bool net_exists(const char *remote_path)
{
    if (!net_is_connected()) return false;

    char full_path[MAX_PATH_LEN];
    build_full_path(full_path, sizeof(full_path), remote_path);

    if (!send_cmd(CMD_EXST, full_path, (u32)strlen(full_path)))
        return false;

    char status[5] = {0};
    if (!recv_response(status, NULL, NULL)) return false;
    return memcmp(status, RSP_YES, 3) == 0;
}

bool net_delete(const char *remote_path)
{
    if (!net_is_connected()) return false;

    char full_path[MAX_PATH_LEN];
    build_full_path(full_path, sizeof(full_path), remote_path);

    if (!send_cmd(CMD_DELE, full_path, (u32)strlen(full_path)))
        return false;

    char status[5] = {0};
    if (!recv_response(status, NULL, NULL)) return false;
    return memcmp(status, RSP_OK, 2) == 0;
}

const char *net_last_error(void)
{
    return last_error;
}

void net_free_names(char **names, int count)
{
    if (!names) return;
    for (int i = 0; i < count; i++) {
        if (names[i]) free(names[i]);
    }
    free(names);
}
