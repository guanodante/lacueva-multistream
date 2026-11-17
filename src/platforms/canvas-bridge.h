/**
 * @file canvas-bridge.h
 * @brief Puente de integración para Canvas Vertical (Aitum Vertical o Canvas Propio).
 *
 * Proyecto: La Cueva del Dolor - OBS MultiStream Plugin
 * @copyright 2026 La Cueva del Dolor
 */

#pragma once

#include <obs.h>
#include <string>
#include <vector>

namespace CuevaMultiStream {

enum class VerticalCanvasSource {
    AitumVertical = 0,   // Detecta y se vincula al canvas de Aitum Vertical
    OwnSceneCanvas,      // Canvas propio basado en una escena de OBS dedicada para vertical
    SmartCropMain        // Recorte inteligente 9:16 de la escena principal horizontal
};

class CanvasBridge {
public:
    static CanvasBridge &instance();

    CanvasBridge(const CanvasBridge &) = delete;
    CanvasBridge &operator=(const CanvasBridge &) = delete;

    /**
     * @brief Detecta si el plugin Aitum Vertical está cargado en OBS Studio.
     */
    bool isAitumVerticalAvailable();

    /**
     * @brief Retorna el manejador de video adecuado para alimentar el encoder vertical.
     */
    video_t *getVerticalVideo(int width = 1080, int height = 1920);

    /**
     * @brief Cambia la escena activa en el canvas de Aitum Vertical.
     */
    bool switchAitumVerticalScene(const std::string &sceneName, int width = 1080, int height = 1920);

    /**
     * @brief Consulta el nombre de la escena activa en Aitum Vertical.
     */
    std::string getAitumVerticalScene(int width = 1080, int height = 1920);

    /**
     * @brief Define el modo de origen para el Canvas Vertical.
     */
    void setVerticalSourceMode(VerticalCanvasSource mode);
    VerticalCanvasSource getVerticalSourceMode() const;

    /**
     * @brief Define la escena de OBS dedicada al Canvas Vertical propio.
     */
    void setDedicatedVerticalScene(const std::string &sceneName);
    std::string getDedicatedVerticalScene() const;

private:
    CanvasBridge();
    ~CanvasBridge() = default;

    VerticalCanvasSource currentSourceMode_ = VerticalCanvasSource::AitumVertical;
    std::string dedicatedSceneName_ = "";
};

} // namespace CuevaMultiStream
