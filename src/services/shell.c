#include "nova_runtime.h"
#include "nova_ui.h"
#include "nova_shell_protocol.h"
#include "nova_desktop_protocol.h"
#include "nova_app_protocol.h"
#include "nova_app_catalog_protocol.h"
#include "nova_app_metadata_protocol.h"
#include "nova_asset_protocol.h"
#include "nova_sha256.h"

#define SHELL_STATE "/run/nova/shell.state"

struct shell_desktop_state {
    unsigned long reconnects;
    unsigned long task_events;
    unsigned long commands;
    unsigned long activates;
    unsigned long closes;
    unsigned long launches;
    unsigned long task_model_syncs;
    unsigned long task_model_rejected;
    unsigned int task_model_entries;
    unsigned long long task_model_generation;
    unsigned int last_client_id;
    unsigned int last_action;
};

struct shell_lifecycle_state {
    unsigned long reconnects;
    unsigned long requests;
    unsigned long statuses;
    unsigned long launch_spawned;
    unsigned long launch_resident_fallbacks;
    unsigned long stop_intents;
    unsigned long stop_commands;
    unsigned long stop_not_running;
    unsigned long rejected;
    unsigned int last_request_id;
    unsigned int last_result;
    unsigned int last_state;
    unsigned long long last_generation;
    long long last_pid;
    char last_app_id[NOVA_APP_ID_BYTES];
};

struct shell_catalog_entry {
    unsigned int client_id;
    unsigned int state;
    unsigned int flags;
    unsigned int category;
    unsigned int metadata_flags;
    unsigned int icon_ready;
    unsigned int icon_format;
    unsigned int icon_width;
    unsigned int icon_height;
    unsigned int icon_bytes;
    char app_id[NOVA_APP_ID_BYTES];
    char display_name[NOVA_APP_DISPLAY_NAME_BYTES];
    char description[NOVA_APP_DESCRIPTION_BYTES];
    char icon_asset_id[NOVA_APP_ICON_ASSET_ID_BYTES];
    unsigned char icon_digest[NOVA_ASSET_DIGEST_BYTES];
    unsigned char icon_pixels[NOVA_DESKTOP_TASK_ICON_BYTES];
};


struct shell_asset_state {
    int connected;
    unsigned long reconnects;
    unsigned long requests;
    unsigned long replies;
    unsigned long rejected;
    unsigned long resolved;
    unsigned int last_request_id;
};

struct shell_catalog_state {
    unsigned long reconnects;
    unsigned long enumerations;
    unsigned long requests;
    unsigned long replies;
    unsigned long rejected;
    unsigned long metadata_requests;
    unsigned long metadata_replies;
    unsigned long metadata_rejected;
    unsigned int count;
    unsigned int last_request_id;
    unsigned long long generation;
    struct shell_catalog_entry entries[NOVA_APP_CATALOG_MAX_ENTRIES];
};

static struct shell_desktop_state g_desktop;
static struct shell_lifecycle_state g_lifecycle;
static struct shell_catalog_state g_catalog;
static struct shell_asset_state g_assets;

static void fill_addr_path(struct nova_sockaddr_un *addr, const char *path) {
    nova_zero(addr, sizeof(*addr));
    addr->sun_family = NOVA_AF_UNIX;
    for (unsigned long i = 0; path[i] && i + 1U < sizeof(addr->sun_path); i++) addr->sun_path[i] = path[i];
}

static void copy_bounded_string(char *dst, unsigned long dst_size, const char *src) {
    unsigned long i = 0;
    if (!dst || dst_size == 0U) return;
    while (src && src[i] && i + 1U < dst_size) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
    for (i++; i < dst_size; i++) dst[i] = '\0';
}

static void copy_app_id(char dst[NOVA_APP_ID_BYTES], const char *src) {
    copy_bounded_string(dst, NOVA_APP_ID_BYTES, src);
}

static int bounded_string_terminated(const char *value, unsigned long size, int require_nonempty) {
    if (!value || size == 0U) return 0;
    for (unsigned long i = 0; i < size; i++) {
        if (value[i] == '\0') return require_nonempty ? i != 0U : 1;
    }
    return 0;
}

static int catalog_app_id_valid(const char app_id[NOVA_APP_ID_BYTES]) {
    if (!bounded_string_terminated(app_id, NOVA_APP_ID_BYTES, 1)) return 0;
    for (unsigned long i = 0; i < NOVA_APP_ID_BYTES && app_id[i]; i++) {
        char c = app_id[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-')) return 0;
    }
    return 1;
}

static int catalog_display_name_valid(const char name[NOVA_APP_DISPLAY_NAME_BYTES]) {
    if (!bounded_string_terminated(name, NOVA_APP_DISPLAY_NAME_BYTES, 1)) return 0;
    for (unsigned long i = 0; i < NOVA_APP_DISPLAY_NAME_BYTES && name[i]; i++) {
        unsigned char c = (unsigned char)name[i];
        if (c < 0x20U || c > 0x7eU || c == '/' || c == '\\') return 0;
    }
    return 1;
}

static int catalog_description_valid(const char description[NOVA_APP_DESCRIPTION_BYTES]) {
    if (!bounded_string_terminated(description, NOVA_APP_DESCRIPTION_BYTES, 0)) return 0;
    for (unsigned long i = 0; i < NOVA_APP_DESCRIPTION_BYTES && description[i]; i++) {
        unsigned char c = (unsigned char)description[i];
        if (c < 0x20U || c > 0x7eU) return 0;
    }
    return 1;
}

static int catalog_icon_asset_id_valid(const char asset_id[NOVA_APP_ICON_ASSET_ID_BYTES]) {
    if (!bounded_string_terminated(asset_id, NOVA_APP_ICON_ASSET_ID_BYTES, 0)) return 0;
    for (unsigned long i = 0; i < NOVA_APP_ICON_ASSET_ID_BYTES && asset_id[i]; i++) {
        char c = asset_id[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-')) return 0;
    }
    return 1;
}

static int catalog_metadata_fields_valid(const struct nova_app_metadata_reply *reply) {
    const unsigned int allowed = NOVA_APP_METADATA_FLAG_HAS_DESCRIPTION | NOVA_APP_METADATA_FLAG_HAS_ICON;
    if (!reply || reply->category > NOVA_APP_CATEGORY_UTILITY || (reply->metadata_flags & ~allowed) != 0U) return 0;
    if (!catalog_description_valid(reply->description) || !catalog_icon_asset_id_valid(reply->icon_asset_id)) return 0;
    if ((reply->metadata_flags & NOVA_APP_METADATA_FLAG_HAS_DESCRIPTION) != 0U) {
        if (!reply->description[0]) return 0;
    } else if (reply->description[0]) return 0;
    if ((reply->metadata_flags & NOVA_APP_METADATA_FLAG_HAS_ICON) != 0U) {
        if (!reply->icon_asset_id[0]) return 0;
    } else if (reply->icon_asset_id[0]) return 0;
    return 1;
}

static struct shell_catalog_entry *catalog_entry_for_client(unsigned int client_id) {
    if (!client_id) return (struct shell_catalog_entry *)0;
    for (unsigned int i = 0U; i < g_catalog.count; i++) {
        if (g_catalog.entries[i].client_id == client_id) return &g_catalog.entries[i];
    }
    return (struct shell_catalog_entry *)0;
}

static int catalog_cache_conflicts(const char *app_id, unsigned int client_id) {
    for (unsigned int i = 0U; i < g_catalog.count; i++) {
        if (g_catalog.entries[i].client_id == client_id) return 1;
        if (nova_streq(g_catalog.entries[i].app_id, app_id)) return 1;
    }
    return 0;
}

static void write_state(const char *stage, unsigned long sequence, unsigned long reconnects,
                        int desktop_connected, int lifecycle_connected, int catalog_connected) {
    long fd = nova_open_write(SHELL_STATE, 0644);
    if (fd < 0) return;
    nova_fd_putln((int)fd, "schema=3");
    nova_fd_puts((int)fd, "stage="); nova_fd_putln((int)fd, stage);
    nova_fd_puts((int)fd, "protocol="); nova_fd_put_u64((int)fd, NOVA_SHELL_PROTOCOL_VERSION); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "sequence="); nova_fd_put_u64((int)fd, sequence); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "reconnects="); nova_fd_put_u64((int)fd, reconnects); nova_fd_putln((int)fd, "");
    nova_fd_putln((int)fd, "desktop_model=nova-shell-scene-v1");
    nova_fd_puts((int)fd, "desktop_control_connected="); nova_fd_putln((int)fd, desktop_connected ? "yes" : "no");
    nova_fd_puts((int)fd, "desktop_protocol="); nova_fd_put_u64((int)fd, NOVA_DESKTOP_PROTOCOL_VERSION); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "task_events="); nova_fd_put_u64((int)fd, g_desktop.task_events); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "task_commands="); nova_fd_put_u64((int)fd, g_desktop.commands); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "task_activates="); nova_fd_put_u64((int)fd, g_desktop.activates); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "task_closes="); nova_fd_put_u64((int)fd, g_desktop.closes); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "task_launches="); nova_fd_put_u64((int)fd, g_desktop.launches); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "task_model_syncs="); nova_fd_put_u64((int)fd, g_desktop.task_model_syncs); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "task_model_rejected="); nova_fd_put_u64((int)fd, g_desktop.task_model_rejected); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "task_model_entries="); nova_fd_put_u64((int)fd, g_desktop.task_model_entries); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "task_model_generation="); nova_fd_put_u64((int)fd, (unsigned long)g_desktop.task_model_generation); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "last_task_client="); nova_fd_put_u64((int)fd, g_desktop.last_client_id); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "last_task_action="); nova_fd_put_u64((int)fd, g_desktop.last_action); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "app_lifecycle_connected="); nova_fd_putln((int)fd, lifecycle_connected ? "yes" : "no");
    nova_fd_puts((int)fd, "app_lifecycle_protocol="); nova_fd_put_u64((int)fd, NOVA_APP_PROTOCOL_VERSION); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "app_lifecycle_reconnects="); nova_fd_put_u64((int)fd, g_lifecycle.reconnects); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "app_lifecycle_requests="); nova_fd_put_u64((int)fd, g_lifecycle.requests); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "app_lifecycle_statuses="); nova_fd_put_u64((int)fd, g_lifecycle.statuses); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "app_launch_spawned="); nova_fd_put_u64((int)fd, g_lifecycle.launch_spawned); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "app_launch_resident_fallbacks="); nova_fd_put_u64((int)fd, g_lifecycle.launch_resident_fallbacks); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "app_stop_intents="); nova_fd_put_u64((int)fd, g_lifecycle.stop_intents); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "app_stop_commands="); nova_fd_put_u64((int)fd, g_lifecycle.stop_commands); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "app_stop_not_running="); nova_fd_put_u64((int)fd, g_lifecycle.stop_not_running); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "app_lifecycle_rejected="); nova_fd_put_u64((int)fd, g_lifecycle.rejected); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "app_last_request_id="); nova_fd_put_u64((int)fd, g_lifecycle.last_request_id); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "app_last_result="); nova_fd_put_u64((int)fd, g_lifecycle.last_result); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "app_last_state="); nova_fd_put_u64((int)fd, g_lifecycle.last_state); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "app_last_generation="); nova_fd_put_u64((int)fd, g_lifecycle.last_generation); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "app_last_pid="); nova_fd_put_u64((int)fd, g_lifecycle.last_pid > 0 ? (unsigned long)g_lifecycle.last_pid : 0UL); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "app_last_id="); nova_fd_putln((int)fd, g_lifecycle.last_app_id[0] ? g_lifecycle.last_app_id : "none");
    nova_fd_puts((int)fd, "app_catalog_connected="); nova_fd_putln((int)fd, catalog_connected ? "yes" : "no");
    nova_fd_puts((int)fd, "app_catalog_protocol="); nova_fd_put_u64((int)fd, NOVA_APP_CATALOG_PROTOCOL_VERSION); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "app_catalog_reconnects="); nova_fd_put_u64((int)fd, g_catalog.reconnects); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "app_catalog_enumerations="); nova_fd_put_u64((int)fd, g_catalog.enumerations); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "app_catalog_requests="); nova_fd_put_u64((int)fd, g_catalog.requests); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "app_catalog_replies="); nova_fd_put_u64((int)fd, g_catalog.replies); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "app_catalog_rejected="); nova_fd_put_u64((int)fd, g_catalog.rejected); nova_fd_putln((int)fd, "");
    nova_fd_putln((int)fd, "app_metadata_extension=phase15-v1");
    nova_fd_puts((int)fd, "app_metadata_requests="); nova_fd_put_u64((int)fd, g_catalog.metadata_requests); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "app_metadata_replies="); nova_fd_put_u64((int)fd, g_catalog.metadata_replies); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "app_metadata_rejected="); nova_fd_put_u64((int)fd, g_catalog.metadata_rejected); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "app_catalog_count="); nova_fd_put_u64((int)fd, g_catalog.count); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "app_catalog_generation="); nova_fd_put_u64((int)fd, (unsigned long)g_catalog.generation); nova_fd_putln((int)fd, "");
    nova_fd_putln((int)fd, "app_asset_extension=phase15-rich-launcher-v1");
    nova_fd_puts((int)fd, "app_assets_connected="); nova_fd_putln((int)fd, g_assets.connected ? "yes" : "no");
    nova_fd_puts((int)fd, "app_asset_reconnects="); nova_fd_put_u64((int)fd, g_assets.reconnects); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "app_asset_requests="); nova_fd_put_u64((int)fd, g_assets.requests); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "app_asset_replies="); nova_fd_put_u64((int)fd, g_assets.replies); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "app_asset_rejected="); nova_fd_put_u64((int)fd, g_assets.rejected); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "app_asset_resolved="); nova_fd_put_u64((int)fd, g_assets.resolved); nova_fd_putln((int)fd, "");
    for (unsigned int i = 0U; i < g_catalog.count; i++) {
        unsigned int slot = i + 1U;
        struct shell_catalog_entry *entry = &g_catalog.entries[i];
        nova_fd_puts((int)fd, "catalog_entry"); nova_fd_put_u64((int)fd, slot); nova_fd_puts((int)fd, "_id="); nova_fd_putln((int)fd, entry->app_id);
        nova_fd_puts((int)fd, "catalog_entry"); nova_fd_put_u64((int)fd, slot); nova_fd_puts((int)fd, "_client_id="); nova_fd_put_u64((int)fd, entry->client_id); nova_fd_putln((int)fd, "");
        nova_fd_puts((int)fd, "catalog_entry"); nova_fd_put_u64((int)fd, slot); nova_fd_puts((int)fd, "_display_name="); nova_fd_putln((int)fd, entry->display_name);
        nova_fd_puts((int)fd, "catalog_entry"); nova_fd_put_u64((int)fd, slot); nova_fd_puts((int)fd, "_state="); nova_fd_put_u64((int)fd, entry->state); nova_fd_putln((int)fd, "");
        nova_fd_puts((int)fd, "catalog_entry"); nova_fd_put_u64((int)fd, slot); nova_fd_puts((int)fd, "_flags="); nova_fd_put_u64((int)fd, entry->flags); nova_fd_putln((int)fd, "");
        nova_fd_puts((int)fd, "catalog_entry"); nova_fd_put_u64((int)fd, slot); nova_fd_puts((int)fd, "_category="); nova_fd_put_u64((int)fd, entry->category); nova_fd_putln((int)fd, "");
        nova_fd_puts((int)fd, "catalog_entry"); nova_fd_put_u64((int)fd, slot); nova_fd_puts((int)fd, "_metadata_flags="); nova_fd_put_u64((int)fd, entry->metadata_flags); nova_fd_putln((int)fd, "");
        nova_fd_puts((int)fd, "catalog_entry"); nova_fd_put_u64((int)fd, slot); nova_fd_puts((int)fd, "_description="); nova_fd_putln((int)fd, entry->description[0] ? entry->description : "none");
        nova_fd_puts((int)fd, "catalog_entry"); nova_fd_put_u64((int)fd, slot); nova_fd_puts((int)fd, "_icon_asset_id="); nova_fd_putln((int)fd, entry->icon_asset_id[0] ? entry->icon_asset_id : "none");
        nova_fd_puts((int)fd, "catalog_entry"); nova_fd_put_u64((int)fd, slot); nova_fd_puts((int)fd, "_icon_ready="); nova_fd_putln((int)fd, entry->icon_ready ? "yes" : "no");
        nova_fd_puts((int)fd, "catalog_entry"); nova_fd_put_u64((int)fd, slot); nova_fd_puts((int)fd, "_icon_width="); nova_fd_put_u64((int)fd, entry->icon_width); nova_fd_putln((int)fd, "");
        nova_fd_puts((int)fd, "catalog_entry"); nova_fd_put_u64((int)fd, slot); nova_fd_puts((int)fd, "_icon_height="); nova_fd_put_u64((int)fd, entry->icon_height); nova_fd_putln((int)fd, "");
    }
    (void)nova_fsync((int)fd); (void)nova_close((int)fd);
}

static int send_scene_message(int fd, unsigned int opcode, unsigned int sequence) {
    struct nova_shell_message m;
    nova_zero(&m, sizeof(m));
    m.magic = NOVA_SHELL_PROTOCOL_MAGIC;
    m.version = NOVA_SHELL_PROTOCOL_VERSION;
    m.opcode = opcode;
    m.size = sizeof(m);
    m.sequence = sequence;
    if (opcode == NOVA_SHELL_OP_SCENE) {
        m.panel_height = NOVA_UI_PANEL_HEIGHT;
        m.window_x = 72; /* R10: independent Quick Controls must not cover Library */
        m.window_y = 108;
        m.window_width = 936U;
        m.window_height = 520U;
        m.accent_rgb = 0x00ff8800U;
        m.initial_focus = 0U;
    }
    return nova_fd_write_all(fd, &m, sizeof(m)) == 0;
}

static int send_desktop_message(int fd, unsigned int opcode, unsigned int client_id,
                                unsigned int action, unsigned int sequence) {
    struct nova_desktop_message m;
    nova_zero(&m, sizeof(m));
    m.magic = NOVA_DESKTOP_PROTOCOL_MAGIC;
    m.version = NOVA_DESKTOP_PROTOCOL_VERSION;
    m.opcode = opcode;
    m.size = sizeof(m);
    m.client_id = client_id;
    m.action = action;
    m.sequence = sequence;
    return nova_fd_write_all(fd, &m, sizeof(m)) == 0;
}


static int bytes_equal(const unsigned char *a, const unsigned char *b, unsigned long count) {
    if (!a || !b) return 0;
    for (unsigned long i = 0U; i < count; i++) if (a[i] != b[i]) return 0;
    return 1;
}

static int asset_reply_valid(const struct nova_asset_reply *reply, unsigned int request_id,
                             unsigned int opcode) {
    if (!reply) return 0;
    if (reply->magic != NOVA_ASSET_PROTOCOL_MAGIC || reply->version != NOVA_ASSET_PROTOCOL_VERSION ||
        reply->size != sizeof(*reply) || reply->request_id != request_id ||
        reply->opcode != opcode || reply->result != NOVA_ASSET_RESULT_OK ||
        reply->registry_count > NOVA_ASSET_MAX_ENTRIES) return 0;
    return 1;
}

static int asset_request(int fd, unsigned int opcode, const char *asset_id,
                         unsigned int offset, unsigned int length, unsigned int request_id,
                         struct nova_asset_reply *reply) {
    struct nova_asset_request request;
    struct nova_iovec iov;
    struct nova_msghdr msg;
    nova_zero(&request, sizeof(request));
    nova_zero(&iov, sizeof(iov));
    nova_zero(&msg, sizeof(msg));
    request.magic = NOVA_ASSET_PROTOCOL_MAGIC;
    request.version = NOVA_ASSET_PROTOCOL_VERSION;
    request.opcode = opcode;
    request.size = sizeof(request);
    request.request_id = request_id;
    request.offset = offset;
    request.length = length;
    if (asset_id) copy_bounded_string(request.asset_id, sizeof(request.asset_id), asset_id);
    if (nova_fd_write_all(fd, &request, sizeof(request)) != 0) return 0;
    g_assets.requests++;
    nova_zero(reply, sizeof(*reply));
    iov.iov_base = reply;
    iov.iov_len = sizeof(*reply);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    long n = nova_recvmsg(fd, &msg, 0);
    if (n != (long)sizeof(*reply) || (msg.msg_flags & (NOVA_MSG_TRUNC | NOVA_MSG_CTRUNC))) {
        g_assets.rejected++;
        return 0;
    }
    g_assets.replies++;
    g_assets.last_request_id = request_id;
    return 1;
}

static int asset_hello(int fd, unsigned int *request_id) {
    struct nova_asset_reply reply;
    unsigned int id = (*request_id)++;
    if (!asset_request(fd, NOVA_ASSET_OP_HELLO, (const char *)0, 0U, 0U, id, &reply) ||
        !asset_reply_valid(&reply, id, NOVA_ASSET_OP_HELLO_REPLY) ||
        reply.asset_id[0] != '\0' || reply.chunk_bytes != 0U) {
        g_assets.rejected++;
        return 0;
    }
    return 1;
}

static int resolve_entry_icon(int fd, struct shell_catalog_entry *entry, unsigned int *request_id) {
    if (!entry || !request_id) return 0;
    entry->icon_ready = 0U;
    entry->icon_format = 0U;
    entry->icon_width = 0U;
    entry->icon_height = 0U;
    entry->icon_bytes = 0U;
    nova_zero(entry->icon_digest, sizeof(entry->icon_digest));
    nova_zero(entry->icon_pixels, sizeof(entry->icon_pixels));

    if (!(entry->metadata_flags & NOVA_APP_METADATA_FLAG_HAS_ICON)) return 1;
    if (!catalog_icon_asset_id_valid(entry->icon_asset_id)) {
        g_assets.rejected++;
        return 0;
    }

    struct nova_asset_reply info;
    unsigned int info_id = (*request_id)++;
    if (!asset_request(fd, NOVA_ASSET_OP_GET_INFO, entry->icon_asset_id, 0U, 0U, info_id, &info) ||
        !asset_reply_valid(&info, info_id, NOVA_ASSET_OP_INFO) ||
        info.format != NOVA_ASSET_FORMAT_A8_MASK ||
        (info.asset_flags & NOVA_ASSET_FLAG_IMMUTABLE) == 0U ||
        info.width != NOVA_DESKTOP_TASK_ICON_WIDTH ||
        info.height != NOVA_DESKTOP_TASK_ICON_HEIGHT ||
        info.total_bytes != NOVA_DESKTOP_TASK_ICON_BYTES ||
        info.offset != 0U || info.chunk_bytes != 0U ||
        !nova_streq(info.asset_id, entry->icon_asset_id)) {
        g_assets.rejected++;
        return 0;
    }

    struct nova_asset_reply data;
    unsigned int data_id = (*request_id)++;
    if (!asset_request(fd, NOVA_ASSET_OP_READ, entry->icon_asset_id, 0U,
                       NOVA_DESKTOP_TASK_ICON_BYTES, data_id, &data) ||
        !asset_reply_valid(&data, data_id, NOVA_ASSET_OP_DATA) ||
        data.format != info.format || data.asset_flags != info.asset_flags ||
        data.width != info.width || data.height != info.height ||
        data.total_bytes != info.total_bytes || data.offset != 0U ||
        data.chunk_bytes != NOVA_DESKTOP_TASK_ICON_BYTES ||
        !nova_streq(data.asset_id, entry->icon_asset_id) ||
        !bytes_equal(data.digest, info.digest, NOVA_ASSET_DIGEST_BYTES)) {
        g_assets.rejected++;
        return 0;
    }

    struct nova_sha256_ctx ctx;
    unsigned char digest[NOVA_ASSET_DIGEST_BYTES];
    nova_sha256_init(&ctx);
    nova_sha256_update(&ctx, data.payload, NOVA_DESKTOP_TASK_ICON_BYTES);
    nova_sha256_final(&ctx, digest);
    if (!bytes_equal(digest, info.digest, NOVA_ASSET_DIGEST_BYTES)) {
        g_assets.rejected++;
        return 0;
    }

    entry->icon_format = data.format;
    entry->icon_width = data.width;
    entry->icon_height = data.height;
    entry->icon_bytes = data.chunk_bytes;
    nova_copy(entry->icon_digest, data.digest, NOVA_ASSET_DIGEST_BYTES);
    nova_copy(entry->icon_pixels, data.payload, NOVA_DESKTOP_TASK_ICON_BYTES);
    entry->icon_ready = 1U;
    g_assets.resolved++;
    nova_puts("[nova-shell] app_asset=RESOLVED app="); nova_puts(entry->app_id);
    nova_puts(" asset="); nova_puts(entry->icon_asset_id);
    nova_puts(" bytes="); nova_put_u64(entry->icon_bytes); nova_putln("");
    return 1;
}

static int resolve_catalog_assets(int fd, unsigned int *request_id) {
    for (unsigned int i = 0U; i < g_catalog.count; i++)
        if (!resolve_entry_icon(fd, &g_catalog.entries[i], request_id)) return 0;
    return 1;
}

static int send_desktop_task_icon_message(int fd, unsigned int sequence,
                                          unsigned int index, unsigned int total_count,
                                          const struct shell_catalog_entry *entry,
                                          unsigned long long catalog_generation) {
    if (!entry || !entry->icon_ready) return 0;
    struct nova_desktop_task_icon_message m;
    nova_zero(&m, sizeof(m));
    m.magic = NOVA_DESKTOP_PROTOCOL_MAGIC;
    m.version = NOVA_DESKTOP_PROTOCOL_VERSION;
    m.opcode = NOVA_DESKTOP_OP_TASK_MODEL_ICON;
    m.size = sizeof(m);
    m.sequence = sequence;
    m.index = index;
    m.total_count = total_count;
    m.client_id = entry->client_id;
    m.icon_format = NOVA_DESKTOP_TASK_ICON_FORMAT_A8;
    m.icon_width = entry->icon_width;
    m.icon_height = entry->icon_height;
    m.icon_bytes = entry->icon_bytes;
    m.catalog_generation = catalog_generation;
    nova_copy(m.digest, entry->icon_digest, sizeof(m.digest));
    copy_bounded_string(m.asset_id, sizeof(m.asset_id), entry->icon_asset_id);
    nova_copy(m.pixels, entry->icon_pixels, sizeof(m.pixels));
    return nova_fd_write_all(fd, &m, sizeof(m)) == 0;
}

static int send_desktop_task_message(int fd, unsigned int opcode, unsigned int sequence,
                                     unsigned int index, unsigned int total_count,
                                     unsigned int client_id, unsigned int task_flags,
                                     unsigned long long catalog_generation, const char *display_name) {
    struct nova_desktop_task_message m;
    nova_zero(&m, sizeof(m));
    m.magic = NOVA_DESKTOP_PROTOCOL_MAGIC;
    m.version = NOVA_DESKTOP_PROTOCOL_VERSION;
    m.opcode = opcode;
    m.size = sizeof(m);
    m.sequence = sequence;
    m.index = index;
    m.total_count = total_count;
    m.client_id = client_id;
    m.task_flags = task_flags;
    m.catalog_generation = catalog_generation;
    if (display_name) copy_bounded_string(m.display_name, sizeof(m.display_name), display_name);
    return nova_fd_write_all(fd, &m, sizeof(m)) == 0;
}

static int sync_desktop_task_model(int fd, unsigned int *sequence) {
    if (!sequence) return 0;
    unsigned int visible_count = 0U;
    for (unsigned int i = 0U; i < g_catalog.count; i++)
        if (g_catalog.entries[i].flags & NOVA_APP_CATALOG_FLAG_VISIBLE) visible_count++;
    if (visible_count > NOVA_DESKTOP_TASK_MAX_ENTRIES) {
        g_desktop.task_model_rejected++;
        return 0;
    }

    if (!send_desktop_task_message(fd, NOVA_DESKTOP_OP_TASK_MODEL_RESET, (*sequence)++,
                                   0U, visible_count, 0U, 0U, g_catalog.generation, (const char *)0)) {
        g_desktop.task_model_rejected++;
        return 0;
    }

    unsigned int task_index = 0U;
    for (unsigned int i = 0U; i < g_catalog.count; i++) {
        struct shell_catalog_entry *entry = &g_catalog.entries[i];
        if (!(entry->flags & NOVA_APP_CATALOG_FLAG_VISIBLE)) continue;
        unsigned int task_flags = (entry->flags & NOVA_APP_CATALOG_FLAG_LAUNCHABLE)
                                ? NOVA_DESKTOP_TASK_FLAG_LAUNCHABLE : 0U;
        if (entry->icon_ready) task_flags |= NOVA_DESKTOP_TASK_FLAG_HAS_ICON;
        if (!send_desktop_task_message(fd, NOVA_DESKTOP_OP_TASK_MODEL_DEFINE, (*sequence)++,
                                       task_index, visible_count, entry->client_id, task_flags,
                                       g_catalog.generation, entry->display_name)) {
            g_desktop.task_model_rejected++;
            return 0;
        }
        if (entry->icon_ready &&
            !send_desktop_task_icon_message(fd, (*sequence)++, task_index, visible_count,
                                            entry, g_catalog.generation)) {
            g_desktop.task_model_rejected++;
            return 0;
        }
        task_index++;
    }

    if (!send_desktop_task_message(fd, NOVA_DESKTOP_OP_TASK_MODEL_COMMIT, (*sequence)++,
                                   visible_count, visible_count, 0U, 0U, g_catalog.generation,
                                   (const char *)0)) {
        g_desktop.task_model_rejected++;
        return 0;
    }
    g_desktop.task_model_syncs++;
    g_desktop.task_model_entries = visible_count;
    g_desktop.task_model_generation = g_catalog.generation;
    nova_puts("[nova-shell] desktop_task_model=SYNCED entries="); nova_put_u64(visible_count);
    nova_puts(" generation="); nova_put_u64((unsigned long)g_catalog.generation); nova_putln("");
    return 1;
}

static long connect_path(const char *path) {
    struct nova_sockaddr_un addr;
    fill_addr_path(&addr, path);
    long fd = nova_socket(NOVA_AF_UNIX, NOVA_SOCK_SEQPACKET, 0);
    if (fd < 0) return fd;
    if (nova_connect((int)fd, &addr, sizeof(addr)) < 0) { (void)nova_close((int)fd); return -1; }
    return fd;
}

static int app_status_valid(const struct nova_app_message *reply, unsigned int request_id) {
    if (!reply) return 0;
    if (reply->magic != NOVA_APP_PROTOCOL_MAGIC || reply->version != NOVA_APP_PROTOCOL_VERSION) return 0;
    if (reply->opcode != NOVA_APP_OP_STATUS || reply->size != sizeof(*reply)) return 0;
    if (reply->request_id != request_id || reply->flags != 0U) return 0;
    return 1;
}

static int lifecycle_request(int fd, unsigned int opcode, const char *app_id,
                             unsigned int request_id, struct nova_app_message *reply) {
    struct nova_app_message request;
    nova_zero(&request, sizeof(request));
    request.magic = NOVA_APP_PROTOCOL_MAGIC;
    request.version = NOVA_APP_PROTOCOL_VERSION;
    request.opcode = opcode;
    request.size = sizeof(request);
    request.request_id = request_id;
    if (app_id) copy_app_id(request.app_id, app_id);
    if (nova_fd_write_all(fd, &request, sizeof(request)) != 0) return 0;
    g_lifecycle.requests++;
    long n = nova_read(fd, reply, sizeof(*reply));
    if (n != (long)sizeof(*reply) || !app_status_valid(reply, request_id)) return 0;
    g_lifecycle.statuses++;
    g_lifecycle.last_request_id = request_id;
    g_lifecycle.last_result = reply->result;
    g_lifecycle.last_state = reply->state;
    g_lifecycle.last_generation = reply->generation;
    g_lifecycle.last_pid = reply->pid;
    copy_app_id(g_lifecycle.last_app_id, reply->app_id);
    return 1;
}

static int lifecycle_hello(int fd, unsigned int *request_id) {
    struct nova_app_message reply;
    unsigned int id = (*request_id)++;
    nova_zero(&reply, sizeof(reply));
    if (!lifecycle_request(fd, NOVA_APP_OP_HELLO, (const char *)0, id, &reply)) return 0;
    return reply.result == NOVA_APP_RESULT_OK && reply.state == NOVA_APP_STATE_UNKNOWN;
}

static int catalog_reply_header_valid(const struct nova_app_catalog_reply *reply,
                                      unsigned int request_id) {
    if (!reply) return 0;
    if (reply->magic != NOVA_APP_CATALOG_PROTOCOL_MAGIC ||
        reply->version != NOVA_APP_CATALOG_PROTOCOL_VERSION ||
        reply->size != sizeof(*reply)) return 0;
    if (reply->request_id != request_id || reply->reserved != 0U) return 0;
    if (reply->total_count > NOVA_APP_CATALOG_MAX_ENTRIES) return 0;
    return 1;
}

static int catalog_request(int fd, unsigned int opcode, unsigned int index,
                           unsigned int request_id, struct nova_app_catalog_reply *reply) {
    struct nova_app_catalog_request request;
    nova_zero(&request, sizeof(request));
    request.magic = NOVA_APP_CATALOG_PROTOCOL_MAGIC;
    request.version = NOVA_APP_CATALOG_PROTOCOL_VERSION;
    request.opcode = opcode;
    request.size = sizeof(request);
    request.request_id = request_id;
    request.index = index;
    if (nova_fd_write_all(fd, &request, sizeof(request)) != 0) return 0;
    g_catalog.requests++;
    long n = nova_read(fd, reply, sizeof(*reply));
    if (n != (long)sizeof(*reply) || !catalog_reply_header_valid(reply, request_id)) return 0;
    g_catalog.replies++;
    g_catalog.last_request_id = request_id;
    return 1;
}

static int catalog_metadata_reply_header_valid(const struct nova_app_metadata_reply *reply,
                                               unsigned int request_id) {
    if (!reply) return 0;
    if (reply->magic != NOVA_APP_CATALOG_PROTOCOL_MAGIC ||
        reply->version != NOVA_APP_CATALOG_PROTOCOL_VERSION ||
        reply->opcode != NOVA_APP_CATALOG_OP_METADATA ||
        reply->size != sizeof(*reply)) return 0;
    if (reply->request_id != request_id || reply->reserved != 0U) return 0;
    if (reply->total_count > NOVA_APP_CATALOG_MAX_ENTRIES ||
        reply->result != NOVA_APP_CATALOG_RESULT_OK) return 0;
    return 1;
}

static int catalog_metadata_request(int fd, unsigned int index, unsigned int request_id,
                                    struct nova_app_metadata_reply *reply) {
    struct nova_app_catalog_request request;
    struct nova_iovec iov;
    struct nova_msghdr msg;
    nova_zero(&request, sizeof(request));
    nova_zero(&iov, sizeof(iov));
    nova_zero(&msg, sizeof(msg));
    request.magic = NOVA_APP_CATALOG_PROTOCOL_MAGIC;
    request.version = NOVA_APP_CATALOG_PROTOCOL_VERSION;
    request.opcode = NOVA_APP_CATALOG_OP_GET_METADATA;
    request.size = sizeof(request);
    request.request_id = request_id;
    request.index = index;
    if (nova_fd_write_all(fd, &request, sizeof(request)) != 0) return 0;
    g_catalog.requests++;
    g_catalog.metadata_requests++;
    nova_zero(reply, sizeof(*reply));
    iov.iov_base = reply;
    iov.iov_len = sizeof(*reply);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    long n = nova_recvmsg(fd, &msg, 0);
    if (n != (long)sizeof(*reply) || (msg.msg_flags & (NOVA_MSG_TRUNC | NOVA_MSG_CTRUNC)) ||
        !catalog_metadata_reply_header_valid(reply, request_id)) return 0;
    g_catalog.replies++;
    g_catalog.metadata_replies++;
    g_catalog.last_request_id = request_id;
    return 1;
}

static int catalog_enumerate(int fd, unsigned int *request_id) {
    struct nova_app_catalog_reply reply;
    unsigned int hello_id = (*request_id)++;
    nova_zero(&reply, sizeof(reply));
    if (!catalog_request(fd, NOVA_APP_CATALOG_OP_HELLO, 0U, hello_id, &reply)) return 0;
    if (reply.opcode != NOVA_APP_CATALOG_OP_HELLO_REPLY || reply.result != NOVA_APP_CATALOG_RESULT_OK ||
        reply.index != 0U || reply.client_id != 0U || reply.state != 0U || reply.catalog_flags != 0U ||
        reply.app_id[0] != '\0' || reply.display_name[0] != '\0') return 0;

    unsigned int expected_count = reply.total_count;
    unsigned long long expected_generation = reply.catalog_generation;
    nova_zero(g_catalog.entries, sizeof(g_catalog.entries));
    g_catalog.count = 0U;
    g_catalog.generation = expected_generation;

    for (unsigned int index = 0U; index < expected_count; index++) {
        unsigned int id = (*request_id)++;
        nova_zero(&reply, sizeof(reply));
        if (!catalog_request(fd, NOVA_APP_CATALOG_OP_GET_ENTRY, index, id, &reply)) return 0;
        if (reply.opcode != NOVA_APP_CATALOG_OP_ENTRY || reply.result != NOVA_APP_CATALOG_RESULT_OK ||
            reply.index != index || reply.total_count != expected_count ||
            reply.catalog_generation != expected_generation || reply.client_id == 0U ||
            reply.state > NOVA_APP_STATE_FAILED ||
            (reply.catalog_flags & ~(NOVA_APP_CATALOG_FLAG_VISIBLE | NOVA_APP_CATALOG_FLAG_LAUNCHABLE)) != 0U ||
            !catalog_app_id_valid(reply.app_id) || !catalog_display_name_valid(reply.display_name) ||
            catalog_cache_conflicts(reply.app_id, reply.client_id)) {
            g_catalog.rejected++;
            return 0;
        }
        struct shell_catalog_entry *entry = &g_catalog.entries[g_catalog.count];
        entry->client_id = reply.client_id;
        entry->state = reply.state;
        entry->flags = reply.catalog_flags;
        copy_bounded_string(entry->app_id, sizeof(entry->app_id), reply.app_id);
        copy_bounded_string(entry->display_name, sizeof(entry->display_name), reply.display_name);

        struct nova_app_metadata_reply metadata;
        unsigned int metadata_id = (*request_id)++;
        if (!catalog_metadata_request(fd, index, metadata_id, &metadata) ||
            metadata.index != index || metadata.total_count != expected_count ||
            metadata.catalog_generation != expected_generation ||
            metadata.client_id != entry->client_id || !catalog_app_id_valid(metadata.app_id) ||
            !nova_streq(metadata.app_id, entry->app_id) || !catalog_metadata_fields_valid(&metadata)) {
            g_catalog.rejected++;
            g_catalog.metadata_rejected++;
            return 0;
        }
        entry->category = metadata.category;
        entry->metadata_flags = metadata.metadata_flags;
        copy_bounded_string(entry->description, sizeof(entry->description), metadata.description);
        copy_bounded_string(entry->icon_asset_id, sizeof(entry->icon_asset_id), metadata.icon_asset_id);
        g_catalog.count++;
    }

    unsigned int end_id = (*request_id)++;
    nova_zero(&reply, sizeof(reply));
    if (!catalog_request(fd, NOVA_APP_CATALOG_OP_GET_ENTRY, expected_count, end_id, &reply)) return 0;
    if (reply.opcode != NOVA_APP_CATALOG_OP_END || reply.result != NOVA_APP_CATALOG_RESULT_OK ||
        reply.index != expected_count || reply.total_count != expected_count ||
        reply.catalog_generation != expected_generation || reply.client_id != 0U || reply.state != 0U ||
        reply.catalog_flags != 0U || reply.app_id[0] != '\0' || reply.display_name[0] != '\0') {
        g_catalog.rejected++;
        return 0;
    }

    g_catalog.enumerations++;
    nova_puts("[nova-shell] app_catalog=ENUMERATED count="); nova_put_u64(g_catalog.count);
    nova_puts(" generation="); nova_put_u64((unsigned long)g_catalog.generation); nova_putln("");
    for (unsigned int i = 0U; i < g_catalog.count; i++) {
        struct shell_catalog_entry *entry = &g_catalog.entries[i];
        nova_puts("[nova-shell] app_catalog_entry index="); nova_put_u64(i);
        nova_puts(" app="); nova_puts(entry->app_id);
        nova_puts(" client="); nova_put_u64(entry->client_id);
        nova_puts(" name="); nova_puts(entry->display_name); nova_putln("");
        nova_puts("[nova-shell] app_metadata_entry index="); nova_put_u64(i);
        nova_puts(" app="); nova_puts(entry->app_id);
        nova_puts(" client="); nova_put_u64(entry->client_id);
        nova_puts(" category="); nova_put_u64(entry->category);
        nova_puts(" icon="); nova_puts(entry->icon_asset_id[0] ? entry->icon_asset_id : "none"); nova_putln("");
    }
    return 1;
}

static int lifecycle_launch(int fd, struct shell_catalog_entry *entry,
                            unsigned int *request_id, struct nova_app_message *reply) {
    if (!entry || !reply || !(entry->flags & NOVA_APP_CATALOG_FLAG_LAUNCHABLE)) return 0;
    unsigned int id = (*request_id)++;
    nova_zero(reply, sizeof(*reply));
    if (!lifecycle_request(fd, NOVA_APP_OP_LAUNCH, entry->app_id, id, reply)) return 0;
    entry->state = reply->state;
    nova_puts("[nova-shell] app_lifecycle_launch app="); nova_puts(entry->app_id);
    nova_puts(" result="); nova_put_u64(reply->result);
    nova_puts(" state="); nova_put_u64(reply->state);
    nova_puts(" generation="); nova_put_u64(reply->generation);
    nova_putln("");
    return 1;
}

static int lifecycle_stop(int fd, struct shell_catalog_entry *entry,
                          unsigned int *request_id, struct nova_app_message *reply) {
    if (!entry || !reply) return 0;
    unsigned int id = (*request_id)++;
    nova_zero(reply, sizeof(*reply));
    if (!lifecycle_request(fd, NOVA_APP_OP_STOP, entry->app_id, id, reply)) return 0;
    entry->state = reply->state;
    nova_puts("[nova-shell] app_lifecycle_stop app="); nova_puts(entry->app_id);
    nova_puts(" result="); nova_put_u64(reply->result);
    nova_puts(" state="); nova_put_u64(reply->state);
    nova_puts(" generation="); nova_put_u64(reply->generation);
    nova_putln("");
    return 1;
}

static int respond_to_task_event(int desktop_fd, int lifecycle_fd,
                                 const struct nova_desktop_message *event,
                                 unsigned int *desktop_sequence,
                                 unsigned int *app_request_id) {
    unsigned int opcode = 0U;
    if (!event || event->opcode != NOVA_DESKTOP_OP_TASK_EVENT || !event->client_id) return 1;

    g_desktop.task_events++;
    g_desktop.last_client_id = event->client_id;
    g_desktop.last_action = event->action;

    struct shell_catalog_entry *entry = catalog_entry_for_client(event->client_id);
    if (!entry) {
        g_catalog.rejected++;
        nova_puts("[nova-shell] task_event=REJECTED reason=client-not-in-catalog client=");
        nova_put_u64(event->client_id); nova_putln("");
        return 1;
    }

    if (event->action == NOVA_DESKTOP_ACTION_ACTIVATE) {
        opcode = NOVA_DESKTOP_OP_ACTIVATE;
        g_desktop.activates++;
    } else if (event->action == NOVA_DESKTOP_ACTION_CLOSE) {
        struct nova_app_message status;
        g_desktop.closes++;
        if (!lifecycle_stop(lifecycle_fd, entry, app_request_id, &status)) return 0;
        if (status.result == NOVA_APP_RESULT_OK && status.state == NOVA_APP_STATE_STOPPING) {
            opcode = NOVA_DESKTOP_OP_CLOSE;
            g_lifecycle.stop_intents++;
        } else if (status.result == NOVA_APP_RESULT_NOT_RUNNING && status.state == NOVA_APP_STATE_STOPPED) {
            g_lifecycle.stop_not_running++;
            nova_puts("[nova-shell] app_stop=ALREADY_STOPPED client="); nova_put_u64(event->client_id); nova_putln("");
            return 1;
        } else {
            g_lifecycle.rejected++;
            nova_puts("[nova-shell] app_stop=REJECTED client="); nova_put_u64(event->client_id);
            nova_puts(" result="); nova_put_u64(status.result); nova_putln("");
            return 1;
        }
    } else if (event->action == NOVA_DESKTOP_ACTION_LAUNCH) {
        struct nova_app_message status;
        g_desktop.launches++;
        if (!(entry->flags & NOVA_APP_CATALOG_FLAG_LAUNCHABLE)) {
            g_catalog.rejected++;
            nova_puts("[nova-shell] launcher=REJECTED reason=not-launchable client="); nova_put_u64(event->client_id); nova_putln("");
            return 1;
        }
        if (!lifecycle_launch(lifecycle_fd, entry, app_request_id, &status)) return 0;

        if (status.result == NOVA_APP_RESULT_OK && status.state == NOVA_APP_STATE_RUNNING) {
            g_lifecycle.launch_spawned++;
            nova_puts("[nova-shell] launcher=NOVAD_PROCESS_SPAWN client="); nova_put_u64(event->client_id); nova_putln("");
            return 1;
        }
        if (status.result == NOVA_APP_RESULT_ALREADY_RUNNING && status.state == NOVA_APP_STATE_RUNNING) {
            opcode = NOVA_DESKTOP_OP_LAUNCH;
            g_lifecycle.launch_resident_fallbacks++;
            nova_puts("[nova-shell] launcher=RESIDENT_WINDOW_FALLBACK client="); nova_put_u64(event->client_id); nova_putln("");
        } else {
            g_lifecycle.rejected++;
            nova_puts("[nova-shell] launcher=REJECTED client="); nova_put_u64(event->client_id);
            nova_puts(" result="); nova_put_u64(status.result); nova_putln("");
            return 1;
        }
    } else {
        return 1;
    }

    g_desktop.commands++;
    if (!send_desktop_message(desktop_fd, opcode, event->client_id, event->action, (*desktop_sequence)++)) return 0;
    if (event->action == NOVA_DESKTOP_ACTION_CLOSE) g_lifecycle.stop_commands++;
    nova_puts("[nova-shell] task_command client="); nova_put_u64(event->client_id);
    nova_puts(" action="); nova_put_u64(event->action); nova_putln("");
    return 1;
}

static int connect_once(unsigned long reconnects) {
    long scene_fd = connect_path(NOVA_SHELL_SOCKET_PATH);
    if (scene_fd < 0) return 0;
    long desktop_fd = connect_path(NOVA_DESKTOP_SOCKET_PATH);
    if (desktop_fd < 0) { (void)nova_close((int)scene_fd); return 0; }
    long lifecycle_fd = connect_path(NOVA_APP_SOCKET_PATH);
    if (lifecycle_fd < 0) { (void)nova_close((int)desktop_fd); (void)nova_close((int)scene_fd); return 0; }
    long catalog_fd = connect_path(NOVA_APP_CATALOG_SOCKET_PATH);
    if (catalog_fd < 0) { (void)nova_close((int)lifecycle_fd); (void)nova_close((int)desktop_fd); (void)nova_close((int)scene_fd); return 0; }
    long asset_fd = connect_path(NOVA_ASSET_SOCKET_PATH);
    if (asset_fd < 0) { (void)nova_close((int)catalog_fd); (void)nova_close((int)lifecycle_fd); (void)nova_close((int)desktop_fd); (void)nova_close((int)scene_fd); return 0; }

    unsigned int scene_seq = 1U;
    unsigned int desktop_seq = 1U;
    unsigned int app_request_id = 1U;
    unsigned int catalog_request_id = 1U;
    unsigned int asset_request_id = 1U;
    if (!send_scene_message((int)scene_fd, NOVA_SHELL_OP_HELLO, scene_seq++) ||
        !send_scene_message((int)scene_fd, NOVA_SHELL_OP_SCENE, scene_seq++) ||
        !send_desktop_message((int)desktop_fd, NOVA_DESKTOP_OP_HELLO, 0U, NOVA_DESKTOP_ACTION_NONE, desktop_seq++) ||
        !lifecycle_hello((int)lifecycle_fd, &app_request_id) ||
        !catalog_enumerate((int)catalog_fd, &catalog_request_id) ||
        !asset_hello((int)asset_fd, &asset_request_id) ||
        !resolve_catalog_assets((int)asset_fd, &asset_request_id) ||
        !sync_desktop_task_model((int)desktop_fd, &desktop_seq)) {
        (void)nova_close((int)asset_fd); (void)nova_close((int)catalog_fd); (void)nova_close((int)lifecycle_fd);
        (void)nova_close((int)desktop_fd); (void)nova_close((int)scene_fd); return 0;
    }
    g_assets.connected = 1;

    nova_putln("[nova-shell] stage=CONNECTED protocol=1");
    nova_putln("[nova-shell] scene=nova-shell-scene-v1");
    nova_putln("[nova-shell] desktop_control=CONNECTED protocol=1 taskbar=dynamic-catalog-v1 launcher=dynamic-catalog-v1");
    nova_putln("[nova-shell] app_lifecycle=CONNECTED protocol=1 launcher=novad-process-v1 close=graceful-stop-v1 compatibility=resident-window-v1");
    nova_puts("[nova-shell] app_catalog=CONNECTED protocol=1 entries="); nova_put_u64(g_catalog.count);
    nova_puts(" generation="); nova_put_u64((unsigned long)g_catalog.generation); nova_putln(" source=novad-registry-v1");
    nova_puts("[nova-shell] app_metadata=CONNECTED extension=phase15-v1 entries="); nova_put_u64(g_catalog.count); nova_putln(" source=novad-trusted-manifest");
    nova_puts("[nova-shell] app_assets=CONNECTED protocol=1 resolved="); nova_put_u64(g_assets.resolved); nova_putln(" source=nova-assets");
    write_state("CONNECTED", scene_seq - 1U, reconnects, 1, 1, 1);

    unsigned int ticks = 0U;
    for (;;) {
        struct nova_pollfd pfds[4];
        nova_zero(pfds, sizeof(pfds));
        pfds[0].fd = (int)desktop_fd;
        pfds[0].events = NOVA_POLLIN | NOVA_POLLERR | NOVA_POLLHUP;
        pfds[1].fd = (int)lifecycle_fd;
        pfds[1].events = NOVA_POLLERR | NOVA_POLLHUP;
        pfds[2].fd = (int)catalog_fd;
        pfds[2].events = NOVA_POLLERR | NOVA_POLLHUP;
        pfds[3].fd = (int)asset_fd;
        pfds[3].events = NOVA_POLLERR | NOVA_POLLHUP;
        /* Linux's raw ppoll syscall may modify the timeout argument.
         * Recreate it for every wait so a consumed timeout cannot collapse
         * the shell into a heartbeat busy loop and starve lifecycle commands. */
        struct nova_timespec quarter = {0, 250000000L};
        long rc = nova_poll(pfds, 4U, &quarter);
        if (rc < 0 && rc != -NOVA_EINTR) break;
        if (pfds[0].revents & (NOVA_POLLERR | NOVA_POLLHUP)) break;
        if (pfds[1].revents & (NOVA_POLLERR | NOVA_POLLHUP)) break;
        if (pfds[2].revents & (NOVA_POLLERR | NOVA_POLLHUP)) break;
        if (pfds[3].revents & (NOVA_POLLERR | NOVA_POLLHUP)) break;
        if (pfds[0].revents & NOVA_POLLIN) {
            struct nova_desktop_message event;
            long n = nova_read((int)desktop_fd, &event, sizeof(event));
            if (n <= 0) break;
            if ((unsigned long)n != sizeof(event) || event.magic != NOVA_DESKTOP_PROTOCOL_MAGIC ||
                event.version != NOVA_DESKTOP_PROTOCOL_VERSION || event.size != sizeof(event)) break;
            if (!respond_to_task_event((int)desktop_fd, (int)lifecycle_fd, &event,
                                       &desktop_seq, &app_request_id)) break;
            write_state("CONNECTED", scene_seq - 1U, reconnects, 1, 1, 1);
        }
        ticks++;
        if ((ticks % 4U) == 0U) {
            if (!send_scene_message((int)scene_fd, NOVA_SHELL_OP_HEARTBEAT, scene_seq++) ||
                !send_desktop_message((int)desktop_fd, NOVA_DESKTOP_OP_HEARTBEAT, 0U, NOVA_DESKTOP_ACTION_NONE, desktop_seq++)) break;
            if ((ticks % 40U) == 0U) write_state("CONNECTED", scene_seq - 1U, reconnects, 1, 1, 1);
        }
    }

    g_assets.connected = 0;
    (void)nova_close((int)asset_fd);
    (void)nova_close((int)catalog_fd);
    (void)nova_close((int)lifecycle_fd);
    (void)nova_close((int)desktop_fd);
    (void)nova_close((int)scene_fd);
    g_desktop.reconnects++;
    g_lifecycle.reconnects++;
    g_catalog.reconnects++;
    g_assets.reconnects++;
    write_state("RECONNECTING", scene_seq, reconnects + 1UL, 0, 0, 0);
    return 1;
}

int nova_main(void) {
    struct nova_timespec retry = {0, 250000000L};
    unsigned long reconnects = 0;
    nova_zero(&g_desktop, sizeof(g_desktop));
    nova_zero(&g_lifecycle, sizeof(g_lifecycle));
    nova_zero(&g_catalog, sizeof(g_catalog));
    nova_zero(&g_assets, sizeof(g_assets));
    nova_putln("[nova-shell] NovaOS Stage-5A Phase-13 desktop shell/application-lifecycle starting");
    nova_putln("[nova-shell] NovaOS Stage-5A Phase-14 dynamic application catalog/taskbar starting");
    nova_putln("[nova-shell] NovaOS Stage-5A Phase-15 rich application metadata cache starting");
    nova_putln("[nova-shell] NovaOS Stage-5A Phase-15 trusted icon resolution/rich launcher starting");
    for (;;) {
        if (connect_once(reconnects)) reconnects++;
        (void)nova_nanosleep(&retry, (struct nova_timespec *)0);
    }
    return 0;
}
