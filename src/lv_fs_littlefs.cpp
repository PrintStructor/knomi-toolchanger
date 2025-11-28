#include <Arduino.h>
#include <LittleFS.h>
#include <lvgl.h>

// ========================================================================
// LittleFS Driver for LVGL - SAFE VERSION
// ========================================================================

static void * fs_open_cb(lv_fs_drv_t * drv, const char * path, lv_fs_mode_t mode) {
    (void)drv; // Unused
    
    Serial.printf("[LVGL-FS] open: %s\n", path);
    
    if (mode != LV_FS_MODE_RD) {
        Serial.println("[LVGL-FS] ERROR: Only read mode supported!");
        return NULL;
    }
    
    // SICHER: Neuen File-Pointer auf dem Heap erstellen
    File file = LittleFS.open(path, "r");
    if (!file) {
        Serial.printf("[LVGL-FS] ERROR: Cannot open %s\n", path);
        return NULL;
    }
    
    // KRITISCH: File-Objekt per Kopie auf Heap speichern
    File * file_ptr = new File(file);
    
    Serial.printf("[LVGL-FS] SUCCESS: Opened %s (%d bytes)\n", path, file_ptr->size());
    return (void *)file_ptr;
}

static lv_fs_res_t fs_close_cb(lv_fs_drv_t * drv, void * file_p) {
    (void)drv;
    
    if (!file_p) {
        Serial.println("[LVGL-FS] WARNING: close called with NULL");
        return LV_FS_RES_OK;
    }
    
    File * file = (File *)file_p;
    file->close();
    delete file;
    
    Serial.println("[LVGL-FS] close OK");
    return LV_FS_RES_OK;
}

static lv_fs_res_t fs_read_cb(lv_fs_drv_t * drv, void * file_p, void * buf, uint32_t btr, uint32_t * br) {
    (void)drv;
    
    if (!file_p || !buf || !br) {
        Serial.println("[LVGL-FS] ERROR: Invalid parameters in read");
        return LV_FS_RES_INV_PARAM;
    }
    
    File * file = (File *)file_p;
    *br = file->read((uint8_t *)buf, btr);
    
    return (*br >= 0) ? LV_FS_RES_OK : LV_FS_RES_UNKNOWN;
}

static lv_fs_res_t fs_seek_cb(lv_fs_drv_t * drv, void * file_p, uint32_t pos, lv_fs_whence_t whence) {
    (void)drv;
    
    if (!file_p) {
        Serial.println("[LVGL-FS] ERROR: NULL file in seek");
        return LV_FS_RES_INV_PARAM;
    }
    
    File * file = (File *)file_p;
    
    SeekMode mode = SeekSet;
    if (whence == LV_FS_SEEK_CUR) mode = SeekCur;
    else if (whence == LV_FS_SEEK_END) mode = SeekEnd;
    
    bool success = file->seek(pos, mode);
    return success ? LV_FS_RES_OK : LV_FS_RES_UNKNOWN;
}

static lv_fs_res_t fs_tell_cb(lv_fs_drv_t * drv, void * file_p, uint32_t * pos_p) {
    (void)drv;
    
    if (!file_p || !pos_p) {
        Serial.println("[LVGL-FS] ERROR: Invalid parameters in tell");
        return LV_FS_RES_INV_PARAM;
    }
    
    File * file = (File *)file_p;
    *pos_p = file->position();
    
    return LV_FS_RES_OK;
}

void lv_fs_littlefs_init() {
    Serial.println("[LVGL-FS] === Registering LittleFS driver ===");
    
    // IMPORTANT: static so the driver struct remains persistent!
    static lv_fs_drv_t fs_drv;
    lv_fs_drv_init(&fs_drv);
    
    fs_drv.letter = 'L';
    fs_drv.cache_size = 0;
    
    fs_drv.open_cb = fs_open_cb;
    fs_drv.close_cb = fs_close_cb;
    fs_drv.read_cb = fs_read_cb;
    fs_drv.seek_cb = fs_seek_cb;
    fs_drv.tell_cb = fs_tell_cb;
    
    lv_fs_drv_register(&fs_drv);
    
    Serial.println("[LVGL-FS] === LittleFS driver registered successfully ===");
}
