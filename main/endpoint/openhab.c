#include "audio_hal.h"
#include "audio_thread.h"
#include "cJSON.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"

#include "../http.h"
#include "shared.h"
#include "slvgl.h"
#include "timer.h"

#define OH_URI_INTERPRETERS "/rest/voice/interpreters"

void openhab_send(const char *data)
{
    bool ok = false;
    char *body = NULL, *url = NULL;
    esp_err_t ret;
    int http_status = 0, len_url = 0;

    len_url = strlen(CONFIG_WILLOW_ENDPOINT_OPENHAB_URL) + strlen(OH_URI_INTERPRETERS) + 1;
    url = calloc(sizeof(char), len_url);
    snprintf(url, len_url, "%s%s", CONFIG_WILLOW_ENDPOINT_OPENHAB_URL, OH_URI_INTERPRETERS);

    cJSON *cjson = cJSON_Parse(data);
    if (!cJSON_IsObject(cjson)) {
        goto end;
    }
    cJSON *text = cJSON_GetObjectItemCaseSensitive(cjson, "text");
    if (!cJSON_IsString(text) && text->valuestring != NULL) {
        goto end;
    }

    esp_http_client_handle_t hdl_hc = init_http_client();

    ESP_LOGI(TAG, "sending '%s' to openHAB REST API on '%s'", text->valuestring, url);

    ret = http_post(hdl_hc, url, "text/plain", text->valuestring, &body, &http_status);
    if (ret == ESP_OK) {
        if (http_status >= 200 && http_status <= 299) {
            ok = true;
        }
    } else {
        ESP_LOGE(TAG, "failed to read HTTP POST response from openHAB");
    }

end:
    if (ok) {
        war.fn_ok("ok");
    } else {
        war.fn_err("error");
    }

    lvgl_port_lock(0);
    lv_obj_clear_flag(lbl_ln3, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(lbl_ln4, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align(lbl_ln3, LV_ALIGN_TOP_LEFT, 0, 120);
    lv_label_set_text_static(lbl_ln3, "Command status:");
    lv_obj_remove_event_cb(lbl_ln3, cb_btn_cancel);
    if (strlen(body) > 1) {
        ESP_LOGI(TAG, "REST response: %s", body);
        lv_label_set_text(lbl_ln4, body);
    } else {
        lv_label_set_text(lbl_ln4, ok ? "#008000 Success!" : "#ff0000 Error");
    }
    lvgl_port_unlock();

    reset_timer(hdl_display_timer, DISPLAY_TIMEOUT_US, false);

    free(body);
}
