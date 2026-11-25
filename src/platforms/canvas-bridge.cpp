/**
 * @file canvas-bridge.cpp
 * @brief Implementación del puente de integración para Canvas Vertical (Aitum o Propio).
 *
 * Proyecto: La Cueva del Dolor - OBS MultiStream Plugin
 * @copyright 2026 La Cueva del Dolor
 */

#include "canvas-bridge.h"
#include <callback/calldata.h>
#include <callback/proc.h>
#include <windows.h>

namespace CuevaMultiStream {

CanvasBridge &CanvasBridge::instance() {
    static CanvasBridge s_instance;
    return s_instance;
}

CanvasBridge::CanvasBridge() {
    if (isAitumVerticalAvailable()) {
        currentSourceMode_ = VerticalCanvasSource::AitumVertical;
        blog(LOG_INFO, "[CuevaMultiStream] Aitum Vertical detectado en OBS Studio.");
    } else {
        currentSourceMode_ = VerticalCanvasSource::OwnSceneCanvas;
        blog(LOG_INFO, "[CuevaMultiStream] Modo Canvas Propio de La Cueva activado.");
    }
}

bool CanvasBridge::isAitumVerticalAvailable() {
    // 1. Verificar si el módulo vertical-canvas.dll está cargado en el proceso de OBS
    HMODULE hVc = GetModuleHandleW(L"vertical-canvas.dll");
    if (hVc) return true;

    // 2. Verificar vía proc_handler de OBS
    proc_handler_t *ph = obs_get_proc_handler();
    if (!ph) return false;

    calldata_t cd;
    calldata_init(&cd);
    calldata_set_int(&cd, "width", 1080);
    calldata_set_int(&cd, "height", 1920);
    bool ok = proc_handler_call(ph, "aitum_vertical_get_video", &cd);
    calldata_free(&cd);
    return ok;
}

video_t *CanvasBridge::getVerticalVideo(int width, int height) {
    if (currentSourceMode_ == VerticalCanvasSource::AitumVertical) {
        proc_handler_t *ph = obs_get_proc_handler();
        if (ph) {
            calldata_t cd;
            calldata_init(&cd);
            calldata_set_int(&cd, "width", width);
            calldata_set_int(&cd, "height", height);
            if (proc_handler_call(ph, "aitum_vertical_get_video", &cd)) {
                video_t *v = (video_t *)calldata_ptr(&cd, "video");
                calldata_free(&cd);
                if (v) {
                    blog(LOG_INFO, "[CuevaMultiStream] Video Vertical obtenido directamente de Aitum Vertical (%dx%d).", width, height);
                    return v;
                }
            } else {
                calldata_free(&cd);
            }
        }
    }

    // Fallback: Retorna el subsistema de video de OBS
    return obs_get_video();
}

bool CanvasBridge::switchAitumVerticalScene(const std::string &sceneName, int width, int height) {
    proc_handler_t *ph = obs_get_proc_handler();
    if (!ph) return false;

    calldata_t cd;
    calldata_init(&cd);
    calldata_set_int(&cd, "width", width);
    calldata_set_int(&cd, "height", height);
    calldata_set_string(&cd, "scene", sceneName.c_str());

    bool ok = proc_handler_call(ph, "aitum_vertical_switch_scene", &cd);
    calldata_free(&cd);
    return ok;
}

std::string CanvasBridge::getAitumVerticalScene(int width, int height) {
    proc_handler_t *ph = obs_get_proc_handler();
    if (!ph) return "";

    calldata_t cd;
    calldata_init(&cd);
    calldata_set_int(&cd, "width", width);
    calldata_set_int(&cd, "height", height);

    std::string res = "";
    if (proc_handler_call(ph, "aitum_vertical_get_scene", &cd)) {
        const char *name = calldata_string(&cd, "scene");
        if (name) res = name;
    }
    calldata_free(&cd);
    return res;
}

void CanvasBridge::setVerticalSourceMode(VerticalCanvasSource mode) {
    currentSourceMode_ = mode;
}

VerticalCanvasSource CanvasBridge::getVerticalSourceMode() const {
    return currentSourceMode_;
}

void CanvasBridge::setDedicatedVerticalScene(const std::string &sceneName) {
    dedicatedSceneName_ = sceneName;
}

std::string CanvasBridge::getDedicatedVerticalScene() const {
    return dedicatedSceneName_;
}

} // namespace CuevaMultiStream
