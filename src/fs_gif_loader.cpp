#include "fs_gif_loader.h"
#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>

static bool g_fs_mounted = false;

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

static String build_lvgl_path_for_tool(int tool) {
    String rel = "/gifs/tool_" + String(tool) + ".gif";   // LittleFS.exists: ohne Laufwerksbuchstaben
    if (!LittleFS.exists(rel)) return String();
    return "L:" + rel;                                    // LVGL: mit Laufwerksbuchstaben
}

void fs_set_tool_gif(lv_obj_t* gif_obj, int tool) {
    fs_mount_once();
    String p = build_lvgl_path_for_tool(tool);
    if (p.isEmpty()) {
        lv_obj_t* label = lv_label_create(lv_obj_get_parent(gif_obj));
        lv_label_set_text_fmt(label, "GIF %d fehlt", tool);
        lv_obj_center(label);
        Serial.printf("[FS] Missing: /gifs/tool_%d.gif\n", tool);
        return;
    }
    lv_gif_set_src(gif_obj, p.c_str());
    Serial.printf("[FS] Idle GIF -> %s\n", p.c_str());
}

lv_obj_t* fs_load_tool_gif(lv_obj_t* parent, int tool) {
    fs_mount_once();
    String p = build_lvgl_path_for_tool(tool);
    if (p.isEmpty()) {
        lv_obj_t* label = lv_label_create(parent);
        lv_label_set_text_fmt(label, "GIF %d fehlt", tool);
        lv_obj_center(label);
        Serial.printf("[FS] Missing: /gifs/tool_%d.gif\n", tool);
        return label;
    }
    lv_obj_t* gif = lv_gif_create(parent);
    lv_obj_center(gif);
    lv_gif_set_src(gif, p.c_str());
    Serial.printf("[FS] load %s\n", p.c_str());
    return gif;
}

int detect_my_tool_number() {
    String hn = WiFi.getHostname();
    int p = hn.lastIndexOf("-t");
    if (p >= 0 && p + 2 <= (int)hn.length()) {
        int n = hn.substring(p + 2).toInt();
        if (n >= 0 && n <= 5) return n;
    }
    return 0;
}