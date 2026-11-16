/**
 * @file platform-config.h
 * @brief Estructuras de configuración y valores por defecto para cada plataforma.
 *
 * Proyecto: La Cueva del Dolor - OBS MultiStream Plugin
 * @copyright 2026 La Cueva del Dolor
 */

#pragma once

#include <string>
#include "../platforms/platform-validator.h"

namespace CuevaMultiStream {

/**
 * @brief Estructura de configuración por destino/plataforma.
 */
struct PlatformConfig {
    PlatformType type = PlatformType::Custom;
    std::string id;                         // ID único (ej: "youtube", "tiktok")
    std::string displayName;                // Nombre en UI ("YouTube Live", "TikTok", etc.)
    std::string rtmpUrl;                    // URL del servidor RTMP/RTMPS
    std::string streamKey;                  // Clave de transmisión (cifrada u ofuscada en disco)
    bool enabled = false;                   // Si está habilitada para transmitir

    // Parámetros recomendados / configurados
    int videoBitrate = 4500;                // kbps
    int audioBitrate = 128;                 // kbps
    int width = 1920;                       // Ancho de lienzo / escala
    int height = 1080;                      // Alto de lienzo / escala
    int fps = 30;                           // Cuadros por segundo
    int keyframeInterval = 2;               // Intervalo de fotogramas clave en segundos
    bool requiresRTMPS = false;             // Obligatoriedad de protocolo RTMPS
    bool isVertical = false;                // Formato vertical (9:16)
};

/**
 * @brief Retorna la configuración por defecto y óptima recomendada para una plataforma específica.
 * @param type Tipo de plataforma.
 * @return PlatformConfig preconfigurado con mejores prácticas de ingesta RTMP.
 */
inline PlatformConfig getDefaultConfig(PlatformType type) {
    PlatformConfig cfg;
    cfg.type = type;

    switch (type) {
    case PlatformType::YouTube:
        cfg.id = "youtube";
        cfg.displayName = "YouTube Live";
        cfg.rtmpUrl = "rtmp://a.rtmp.youtube.com/live2";
        cfg.streamKey = "";
        cfg.enabled = true;
        cfg.videoBitrate = 4500;
        cfg.audioBitrate = 128;
        cfg.width = 1920;
        cfg.height = 1080;
        cfg.fps = 30;
        cfg.keyframeInterval = 2;
        cfg.requiresRTMPS = false;
        cfg.isVertical = false;
        break;

    case PlatformType::Facebook:
        cfg.id = "facebook";
        cfg.displayName = "Facebook Live";
        cfg.rtmpUrl = "rtmps://live-api-s.facebook.com:443/rtmp/";
        cfg.streamKey = "";
        cfg.enabled = true;
        cfg.videoBitrate = 4500;
        cfg.audioBitrate = 128;
        cfg.width = 1920;
        cfg.height = 1080;
        cfg.fps = 30;
        cfg.keyframeInterval = 2;
        cfg.requiresRTMPS = true;
        cfg.isVertical = false;
        break;

    case PlatformType::TikTok:
        cfg.id = "tiktok";
        cfg.displayName = "TikTok Live";
        cfg.rtmpUrl = ""; // Se genera dinámicamente en TikTok Live Studio / Creator Center
        cfg.streamKey = "";
        cfg.enabled = false;
        cfg.videoBitrate = 4000;
        cfg.audioBitrate = 128;
        cfg.width = 1080;
        cfg.height = 1920;
        cfg.fps = 30;
        cfg.keyframeInterval = 2;
        cfg.requiresRTMPS = false;
        cfg.isVertical = true;
        break;

    case PlatformType::Instagram:
        cfg.id = "instagram";
        cfg.displayName = "Instagram Live";
        cfg.rtmpUrl = ""; // Generada via Instagram Live Producer
        cfg.streamKey = "";
        cfg.enabled = false;
        cfg.videoBitrate = 3500;
        cfg.audioBitrate = 128;
        cfg.width = 720;
        cfg.height = 1280;
        cfg.fps = 30;
        cfg.keyframeInterval = 2;
        cfg.requiresRTMPS = true; // Instagram Live Producer requiere SSL/RTMPS
        cfg.isVertical = true;
        break;

    case PlatformType::Twitter:
        cfg.id = "twitter";
        cfg.displayName = "X (Twitter)";
        cfg.rtmpUrl = ""; // Generada en X Media Studio / Producer
        cfg.streamKey = "";
        cfg.enabled = false;
        cfg.videoBitrate = 4000;
        cfg.audioBitrate = 128;
        cfg.width = 1920;
        cfg.height = 1080;
        cfg.fps = 30;
        cfg.keyframeInterval = 2;
        cfg.requiresRTMPS = false;
        cfg.isVertical = false;
        break;

    case PlatformType::Custom:
    default:
        cfg.id = "custom";
        cfg.displayName = "Servidor RTMP Personalizado";
        cfg.rtmpUrl = "rtmp://localhost/live";
        cfg.streamKey = "";
        cfg.enabled = false;
        cfg.videoBitrate = 4000;
        cfg.audioBitrate = 128;
        cfg.width = 1920;
        cfg.height = 1080;
        cfg.fps = 30;
        cfg.keyframeInterval = 2;
        cfg.requiresRTMPS = false;
        cfg.isVertical = false;
        break;
    }

    return cfg;
}

} // namespace CuevaMultiStream
