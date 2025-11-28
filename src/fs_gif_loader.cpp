#include "fs_gif_loader.h"
#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>

static bool g_fs_mounted = false;

// Static buffer for the lv_img_dsc_t structure (one per Tool-GIF)
static lv_img_dsc_t gif_descriptors[6];
static uint8_t* gif_data_buffers[6] = {NULL};

void fs_mount_once() {
    if (g_fs_mounted) return;
    if (LittleFS.begin()) {
        g_fs_mounted = true;
        Serial.printf("[FS] LittleFS OK  total=%u used=%u\n",
                      LittleFS.totalBytes(), LittleFS.usedBytes());
    } else {
        Serial.println("[FS] LittleFS mount FAILED!");
    }
}

// ========================================================================
// KORRIGIERTE VERSION: GIF in PSRAM laden + lv_img_dsc_t korrekt bauen
// ========================================================================
void fs_set_tool_gif(lv_obj_t* gif_obj, int tool) {
    fs_mount_once();
    
    if (!gif_obj) {
        Serial.println("[FS] ERROR: gif_obj is NULL!");
        return;
    }
    
    if (tool < 0 || tool > 5) {
        Serial.printf("[FS] ERROR: Invalid tool number: %d\n", tool);
        return;
    }

    // If this tool's GIF is already loaded into PSRAM, reuse it silently
    if (gif_data_buffers[tool] && gif_descriptors[tool].data == gif_data_buffers[tool]) {
        lv_gif_set_src(gif_obj, &gif_descriptors[tool]);
        return;
    }

    String path = "/gifs/tool_" + String(tool) + ".gif";
    Serial.printf("[FS] Loading GIF for tool %d from %s\n", tool, path.c_str());
    
    // Check if file exists
    if (!LittleFS.exists(path)) {
        Serial.printf("[FS] ERROR: File not found: %s\n", path.c_str());
        lv_obj_t* label = lv_label_create(lv_obj_get_parent(gif_obj));
        lv_label_set_text_fmt(label, "GIF %d\nnot found", tool);
        lv_obj_center(label);
        lv_obj_add_flag(gif_obj, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    
    // Open file
    File file = LittleFS.open(path, "r");
    if (!file) {
        Serial.printf("[FS] ERROR: Cannot open: %s\n", path.c_str());
        return;
    }
    
    size_t fileSize = file.size();
    
    // Falls schon geladen, alten Buffer freigeben
    if (gif_data_buffers[tool]) {
        free(gif_data_buffers[tool]);
        gif_data_buffers[tool] = NULL;
    }
    
    // GIF komplett in PSRAM laden
    gif_data_buffers[tool] = (uint8_t*)ps_malloc(fileSize);
    if (!gif_data_buffers[tool]) {
        Serial.println("[FS] ERROR: Out of PSRAM memory!");
        file.close();
        return;
    }
    
    // Datei einlesen
    size_t bytesRead = file.read(gif_data_buffers[tool], fileSize);
    file.close();
    
    if (bytesRead != fileSize) {
        Serial.printf("[FS] ERROR: Read only %u of %u bytes\n", bytesRead, fileSize);
        free(gif_data_buffers[tool]);
        gif_data_buffers[tool] = NULL;
        return;
    }
    
    Serial.printf("[FS] Loaded tool %d GIF: %u bytes into PSRAM\n", tool, bytesRead);
    
    // Struktur initialisieren
    gif_descriptors[tool].header.always_zero = 0;
    gif_descriptors[tool].header.w = 0;
    gif_descriptors[tool].header.h = 0;
    gif_descriptors[tool].data_size = fileSize;
    gif_descriptors[tool].header.cf = LV_IMG_CF_RAW;
    gif_descriptors[tool].data = gif_data_buffers[tool];
    
    // GIF setzen
    lv_gif_set_src(gif_obj, &gif_descriptors[tool]);
}

lv_obj_t* fs_load_tool_gif(lv_obj_t* parent, int tool) {
    fs_mount_once();
    
    if (tool < 0 || tool > 5) {
        Serial.printf("[FS] ERROR: Invalid tool number: %d\n", tool);
        tool = 0;
    }
    
    String path = "/gifs/tool_" + String(tool) + ".gif";
    Serial.printf("[FS] Loading tool GIF: %s\n", path.c_str());
    
    if (!LittleFS.exists(path)) {
        lv_obj_t* label = lv_label_create(parent);
        lv_label_set_text_fmt(label, "Tool %d", tool);
        lv_obj_center(label);
        return label;
    }
    
    lv_obj_t* gif = lv_gif_create(parent);
    lv_obj_center(gif);
    
    fs_set_tool_gif(gif, tool);
    
    return gif;
}

int detect_my_tool_number() {
    static int cached_tool = -1;
    static bool first_run = true;
    
    // Cache the result to avoid spamming the log
    if (cached_tool >= 0) {
        return cached_tool;
    }
    
    String hn = WiFi.getHostname();
    
    int p = hn.lastIndexOf("-t");
    if (p >= 0 && p + 2 <= (int)hn.length()) {
        int n = hn.substring(p + 2).toInt();
        if (n >= 0 && n <= 5) {
            cached_tool = n;
            if (first_run) {
                Serial.printf("[FS] Detected tool number: %d (Hostname: %s)\n", n, hn.c_str());
                first_run = false;
            }
            return n;
        }
    }
    
    cached_tool = 0;
    if (first_run) {
        Serial.printf("[FS] No tool number in hostname (%s), using tool_0\n", hn.c_str());
        first_run = false;
    }
    return 0;
}
