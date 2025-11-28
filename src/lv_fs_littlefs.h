#ifndef LV_FS_LITTLEFS_H
#define LV_FS_LITTLEFS_H

/**
 * @brief Registriert LittleFS als LVGL Filesystem-Driver mit Buchstabe 'L'
 * 
 * Nach dem Aufruf dieser Funktion kann LVGL Dateien via "L:/pfad/datei.gif" laden.
 * MUSS nach lv_init() und LittleFS.begin() aufgerufen werden!
 */
void lv_fs_littlefs_init();

#endif // LV_FS_LITTLEFS_H
