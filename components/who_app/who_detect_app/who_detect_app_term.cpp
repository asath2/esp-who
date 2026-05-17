// save to sd card 

#include "who_detect_app_term.hpp"
#include "who_detect_result_handle.hpp"
#include "who_yield2idle.hpp"

#include "esp_log.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "sys/stat.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "WHO_APP";

#define SAVE_DIR  "/sdcard/detections"

static bool s_dir_checked = false;

static void ensure_save_dir(void)
{
    if (s_dir_checked) return;
    struct stat st;
    if (stat(SAVE_DIR, &st) != 0) {
        if (mkdir(SAVE_DIR, 0755) == 0) {
            ESP_LOGI(TAG, "Created dir: %s", SAVE_DIR);
        } else {
            ESP_LOGE(TAG, "Failed to create dir: %s", SAVE_DIR);
        }
    } else {
        ESP_LOGI(TAG, "Save dir exists: %s", SAVE_DIR);
    }
    s_dir_checked = true;
}

static void save_frame_to_sd(camera_fb_t *fb)
{
    if (!fb) {
        ESP_LOGE(TAG, "Frame is NULL");
        return;
    }

    ensure_save_dir();

    // Use timestamp in ms as filename
    uint64_t ts = esp_timer_get_time() / 1000;
    char path[80];
    snprintf(path, sizeof(path), "%s/person_%llu.jpg", SAVE_DIR, ts);

    FILE *f = fopen(path, "wb");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open file: %s (errno=%d)", path, errno);
        return;
    }

    size_t written = fwrite(fb->buf, 1, fb->len, f);
    fclose(f);

    if (written == fb->len) {
        ESP_LOGI(TAG, "Saved: %s (%d bytes)", path, (int)written);
    } else {
        ESP_LOGE(TAG, "Write incomplete: %d/%d bytes", (int)written, (int)fb->len);
    }
}

namespace who {
namespace app {

WhoDetectAppTerm::WhoDetectAppTerm(frame_cap::WhoFrameCap *frame_cap)
    : WhoDetectAppBase(frame_cap)
{
    m_detect->set_detect_result_cb(
        std::bind(&WhoDetectAppTerm::detect_result_cb, this, std::placeholders::_1));
}

bool WhoDetectAppTerm::run()
{
    bool ret = WhoYield2Idle::get_instance()->run();

    for (const auto &frame_cap_node : m_frame_cap->get_all_nodes()) {
        ret &= frame_cap_node->run(4096, 2, 0);
    }

    ret &= m_detect->run(2560, 2, 1);

    return ret;
}

void WhoDetectAppTerm::detect_result_cb(const detect::WhoDetect::result_t &result)
{
    detect::print_detect_results(result.det_res);

    if (!result.det_res.empty()) {
        static uint64_t last_save_ms = 0;
        uint64_t now_ms = esp_timer_get_time() / 1000;

        // Save at most once every 5 seconds
        if (now_ms - last_save_ms > 5000) {
            last_save_ms = now_ms;

            camera_fb_t *fb = esp_camera_fb_get();
            if (fb) {
                save_frame_to_sd(fb);
                esp_camera_fb_return(fb);
            } else {
                ESP_LOGE(TAG, "Camera capture failed");
            }
        }
    }
}

} // namespace app
} // namespace who
