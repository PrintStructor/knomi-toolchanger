#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "moonraker.h"
#include "knomi.h"
#include "power_management/display_sleep.h"

//#define MOONRAKER_DEBUG  // Enable for layer debug

void lv_popup_warning(const char * warning, bool clickable);

String MOONRAKER::send_request(const char * type, String path) {
    String ip = knomi_config.moonraker_ip;
    String port = knomi_config.moonraker_port;
    String url = "http://" + ip + ":" + port + path;
    String response = "";
    HTTPClient client;
    // replace all " " space to "%20" for http
    url.replace(" ", "%20");
    client.begin(url);
    // set timeout to 60 seconds since some gcode like G28 need long time to feedback
    client.setTimeout(60000);
    int code = client.sendRequest(type, "");
    // http request success
    if (code > 0) {
        unconnected = false;
        response = client.getString();
        if (code == 400) {
            if (!response.isEmpty()) {
                // Serial.println(response.c_str());
                DynamicJsonDocument json_parse(response.length() * 2);
                deserializeJson(json_parse, response);
                String msg = json_parse["error"]["message"].as<String>();
#ifdef MOONRAKER_DEBUG
                Serial.println(msg.c_str());
#endif
                msg.remove(0, 41); //  remove header {'error': 'WebRequestError', 'message':
                msg.remove(msg.length() - 2, 2); // remove tail }
                msg.replace("\\n", "\n");
                lv_popup_warning(msg.c_str(), true);
            }
        }
    } else {
        /*
         * since some gcode need long time cause code=-11 error
         * so don't set status when POST gcode
         * only set when GET
         */
        if (strcmp(type, "GET") == 0)
            unconnected = true;
        Serial.printf("moonraker http %s error.\r\n", type);
    }
    client.end(); //Free the resources

#ifdef MOONRAKER_DEBUG
    Serial.printf("\r\n\r\n %s code:%d************ %s *******************\r\n\r\n", type, code, url.c_str());
    Serial.println(response.c_str());
    Serial.println("\r\n*******************************\r\n\r\n");
#endif

    return response;
}

void MOONRAKER::http_post_loop(void) {
    if (post_queue.count == 0) return;
    send_request("POST", post_queue.queue[post_queue.index_r]);
    post_queue.count--;
    post_queue.index_r = (post_queue.index_r + 1) % QUEUE_LEN;
}

bool MOONRAKER::post_to_queue(String path) {
    if (post_queue.count >= QUEUE_LEN) {
        Serial.println("moonraker post queue overflow!");
        return false;
    }
    post_queue.queue[post_queue.index_w] = path;
    post_queue.index_w = (post_queue.index_w + 1) % QUEUE_LEN;
    post_queue.count++;
#ifdef MOONRAKER_DEBUG
    Serial.printf("\r\n\r\n ************ post queue *******************\r\n\r\n");
    Serial.print("count: ");   Serial.println(post_queue.count);
    Serial.print("index_w: "); Serial.println(post_queue.index_w);
    Serial.print("queue: ");   Serial.println(path);
    Serial.println("\r\n*******************************\r\n\r\n");
#endif
    return true;
}

bool MOONRAKER::post_gcode_to_queue(String gcode) {
    String path = "/printer/gcode/script?script=" + gcode;
    return post_to_queue(path);
}

void MOONRAKER::get_printer_ready(void) {
    bool was_unready = unready;
    
    String webhooks = send_request("GET", "/printer/objects/query?webhooks");
    if (!webhooks.isEmpty()) {
        DynamicJsonDocument json_parse(webhooks.length() * 2);
        deserializeJson(json_parse, webhooks);
        String state = json_parse["result"]["status"]["webhooks"]["state"].as<String>();
        unready = (state == "ready") ? false : true;
#ifdef MOONRAKER_DEBUG
        Serial.print("unready: ");
        Serial.println(unready);
#endif
        
        // CRITICAL: When Klipper changes from unready → ready (e.g. after FIRMWARE_RESTART)
        // → Reset Klipper Idle Timer, otherwise display sleeps immediately!
        if (was_unready && !unready) {
            display_reset_klipper_idle_timer();
        }
    } else {
        unready = true;
        Serial.println("Empty: moonraker: get_printer_ready");
    }
}

void MOONRAKER::get_printer_info(void) {
    String printer_info = send_request("GET", "/api/printer");
    if (!printer_info.isEmpty()) {
        DynamicJsonDocument json_parse(printer_info.length() * 2);
        deserializeJson(json_parse, printer_info);
        data.pause = json_parse["state"]["flags"]["pausing"].as<bool>(); // pausing
        data.pause |= json_parse["state"]["flags"]["paused"].as<bool>(); // paused
        data.printing = json_parse["state"]["flags"]["printing"].as<bool>(); // printing
        data.printing |= json_parse["state"]["flags"]["cancelling"].as<bool>(); // cancelling
        data.printing |= data.pause; // pause
        data.bed_actual = int16_t(json_parse["temperature"]["bed"]["actual"].as<double>() + 0.5f);
        data.bed_target = int16_t(json_parse["temperature"]["bed"]["target"].as<double>() + 0.5f);
        data.nozzle_actual = int16_t(json_parse["temperature"][knomi_config.moonraker_tool]["actual"].as<double>() + 0.5f);
        data.nozzle_target = int16_t(json_parse["temperature"][knomi_config.moonraker_tool]["target"].as<double>() + 0.5f);
#ifdef MOONRAKER_DEBUG
        Serial.print("unready: ");
        Serial.println(unready);
        Serial.print("printing: ");
        Serial.println(data.printing);
        Serial.print("bed_actual: ");
        Serial.println(data.bed_actual);
        Serial.print("bed_target: ");
        Serial.println(data.bed_target);
        Serial.print("nozzle_actual: ");
        Serial.println(data.nozzle_actual);
        Serial.print("nozzle_target: ");
        Serial.println(data.nozzle_target);
#endif
    } else {
        Serial.println("Empty: moonraker: get_printer_info");
    }
}

// only return gcode file name except path
// for example:"SD:/test/123.gcode"
// only return "123.gcode"
const char * path_only_gcode(const char * path)
{
  char * name = strrchr(path, '/');

  if (name != NULL)
    return (name + 1);
  else
    return path;
}

void MOONRAKER::get_progress(void) {
    // Skip progress query if we are not actively printing (saves traffic/CPU)
    if (!data.printing) {
        return;
    }
    String display_status = send_request("GET", "/printer/objects/query?virtual_sdcard&print_stats&display_status&toolhead&extruder&extruder1&extruder2&extruder3&extruder4&extruder5");
    if (!display_status.isEmpty()) {
        DynamicJsonDocument json_parse(display_status.length() * 2);
        DeserializationError error = deserializeJson(json_parse, display_status);
        if (error) {
            Serial.println("JSON parse error in get_progress");
            return;
        }
        
        // Progress & File Path
        data.progress = (uint8_t)(json_parse["result"]["status"]["virtual_sdcard"]["progress"].as<double>() * 100 + 0.5f);
        String path = json_parse["result"]["status"]["virtual_sdcard"]["file_path"].as<String>();
        strlcpy(data.file_path, path_only_gcode(path.c_str()), sizeof(data.file_path));  // strlcpy handles null-termination correctly
        
        // Layer data (from print_stats.info, not display_status!)
        JsonObject info = json_parse["result"]["status"]["print_stats"]["info"];
        if (!info.isNull()) {
            data.current_layer = info["current_layer"].as<int>();
            data.total_layers = info["total_layer"].as<int>();  // Note: "total_layer" without 's'!
        } else {
            data.current_layer = 0;
            data.total_layers = 0;
        }
        
        // Time data (from print_stats)
        data.print_duration = json_parse["result"]["status"]["print_stats"]["print_duration"].as<int>();
        data.estimated_time = json_parse["result"]["status"]["print_stats"]["total_duration"].as<int>();

        // Active tool (from toolhead)
        String active_name = json_parse["result"]["status"]["toolhead"]["extruder"].as<String>();
        if (active_name.length() == 0) active_name = "extruder"; // default
        strlcpy(data.active_tool_name, active_name.c_str(), sizeof(data.active_tool_name));

        // Derive index: "extruder" -> 0, "extruderN" -> N
        int idx = 0;
        if (active_name.length() > 8) {
            const char *p = active_name.c_str() + 8; // skip "extruder"
            if (*p) idx = atoi(p);
        }
        data.active_tool_index = idx;

        // Read temperature from the matching extruder object
        JsonVariant ex = json_parse["result"]["status"][active_name];
        double t = 0.0;
        if (!ex.isNull()) {
            t = ex["temperature"].as<double>();
        } else {
            // Fallback to previous single-nozzle field if object not present
            t = data.nozzle_actual;
        }
        data.active_tool_temp = (int16_t)(t + 0.5f);

        if (data.active_tool_temp < 0 || data.active_tool_temp > 500) {
            data.active_tool_temp = 0;
        }
        
        // Read ALL extruder temperatures (for multi-tool displays)
        // Each display will show its own tool's temperature
        for (int i = 0; i < 6; i++) {
            String extruder_name = (i == 0) ? "extruder" : "extruder" + String(i);
            JsonVariant ext_obj = json_parse["result"]["status"][extruder_name];
            
            if (!ext_obj.isNull()) {
                double temp = ext_obj["temperature"].as<double>();
                data.extruder_temps[i] = (int16_t)(temp + 0.5f);
                
                // Sanity check
                if (data.extruder_temps[i] < 0 || data.extruder_temps[i] > 500) {
                    data.extruder_temps[i] = 0;
                }
            } else {
                // Extruder not present
                data.extruder_temps[i] = 0;
            }
        }
        
#ifdef MOONRAKER_DEBUG
        Serial.print("progress: ");
        Serial.println(data.progress);
        Serial.print("path: ");
        Serial.println(data.file_path);
        Serial.print("current_layer: ");
        Serial.println(data.current_layer);
        Serial.print("total_layers: ");
        Serial.println(data.total_layers);
        Serial.print("print_duration: ");
        Serial.println(data.print_duration);
        Serial.print("estimated_time: ");
        Serial.println(data.estimated_time);
        Serial.print("active_tool_name: ");
        Serial.println(data.active_tool_name);
        Serial.print("active_tool_index: ");
        Serial.println(data.active_tool_index);
        Serial.print("active_tool_temp: ");
        Serial.println(data.active_tool_temp);
#endif
    } else {
        Serial.println("Empty: moonraker: get_progress");
    }
    data_unlock = true;
}

void MOONRAKER::get_knomi_status(void) {
    String knomi_status = send_request("GET", "/printer/objects/query?gcode_macro%20_KNOMI_STATUS");
    if (!knomi_status.isEmpty()) {
        DynamicJsonDocument json_parse(knomi_status.length() * 2);
        deserializeJson(json_parse, knomi_status);
        data.homing = json_parse["result"]["status"]["gcode_macro _KNOMI_STATUS"]["homing"].as<bool>();
        data.probing = json_parse["result"]["status"]["gcode_macro _KNOMI_STATUS"]["probing"].as<bool>();
        data.qgling = json_parse["result"]["status"]["gcode_macro _KNOMI_STATUS"]["qgling"].as<bool>();
        data.heating_nozzle = json_parse["result"]["status"]["gcode_macro _KNOMI_STATUS"]["heating_nozzle"].as<bool>();
        data.heating_bed = json_parse["result"]["status"]["gcode_macro _KNOMI_STATUS"]["heating_bed"].as<bool>();
#ifdef MOONRAKER_DEBUG
        Serial.print("homing: ");
        Serial.println(data.homing);
        Serial.print("probing: ");
        Serial.println(data.probing);
        Serial.print("qgling: ");
        Serial.println(data.qgling);
        Serial.print("heating_nozzle: ");
        Serial.println(data.heating_nozzle);
        Serial.print("heating_bed: ");
        Serial.println(data.heating_bed);
#endif
    } else {
        Serial.println("Empty: moonraker: get_knomi_status");
    }
}

void MOONRAKER::get_idle_timeout(void) {
    String idle_status = send_request("GET", "/printer/objects/query?idle_timeout");
    if (!idle_status.isEmpty()) {
        DynamicJsonDocument json_parse(idle_status.length() * 2);
        deserializeJson(json_parse, idle_status);
        String state = json_parse["result"]["status"]["idle_timeout"]["state"].as<String>();
        if (!state.isEmpty()) {
            display_update_klipper_idle_state(state.c_str());
#ifdef MOONRAKER_DEBUG
            Serial.print("idle_timeout state: ");
            Serial.println(state);
#endif
        }
    } else {
        Serial.println("Empty: moonraker: get_idle_timeout");
    }
}

void MOONRAKER::http_get_loop(void) {
    data_unlock = false;
    get_printer_ready();
    if (!unready) {
        // get_knomi_status() must before get_printer_info()
        // avoid homing, qgling, etc action flag = 1
        // but printing flag has not refresh
        get_knomi_status();
        get_printer_info();
        get_idle_timeout();  // Update idle_timeout state for display sleep management
        if (data.printing) {
            get_progress();
        }
    }
    data_unlock = true;
}


MOONRAKER moonraker;

extern "C" void knomi_request_pause(void) {
    // Moonraker HTTP API: POST /printer/print/pause
    // Queue the request so it is sent from the moonraker post task
    moonraker.post_to_queue("/printer/print/pause");
}

extern "C" void knomi_request_cancel(void) {
    // Moonraker HTTP API: POST /printer/print/cancel
    // Queue the request so it is sent from the moonraker post task
    moonraker.post_to_queue("/printer/print/cancel");
}

void moonraker_post_task(void * parameter) {
    for(;;) {
        moonraker.http_post_loop();
        delay(500);
    }
}

void moonraker_task(void * parameter) {

    xTaskCreate(moonraker_post_task, "moonraker post",
        4096,  // Stack size (bytes)
        NULL,  // Parameter to pass
        8,     // Task priority
        NULL   // Task handle
        );

    for(;;) {
        if (wifi_get_connect_status() == WIFI_STATUS_CONNECTED) {
            moonraker.http_get_loop();
        }
        delay(200);
    }
}

// Klipper Control: Restart Firmware Restart. /printer/restart, /printer/firmware_restart
// Service Control: stop start restart. POST /machine/services/stop|restart|start?service={name}
// Host Control: Reboot, Shutdown. POST /machine/shutdown, POST /machine/reboot
