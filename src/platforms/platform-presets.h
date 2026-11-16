/**
 * @file platform-presets.h
 * @brief Base de datos estática de presets de codificación por plataforma.
 *
 * Define la configuración óptima de video y audio para cada plataforma de
 * streaming soportada, incluyendo resolución, bitrate, perfil de codificador,
 * y parámetros específicos. Proporciona métodos para obtener los ajustes
 * como obs_data_t* listos para aplicar al codificador de OBS.
 *
 * Proyecto: La Cueva del Dolor - OBS MultiStream Plugin
 * @copyright 2026 La Cueva del Dolor
 */

#pragma once

#include <obs-module.h>
#include "platform-validator.h"  // Para PlatformType
#include <string>
#include <vector>
#include <map>

// ============================================================================
// Perfiles de calidad disponibles
// ============================================================================

/**
 * @brief Niveles de calidad predefinidos para adaptar bitrate/resolución.
 */
enum class QualityProfile {
    Full_1080p60 = 0, ///< 1920x1080 a 60fps (máxima calidad)
    Full_1080p30,     ///< 1920x1080 a 30fps (calidad alta)
    HD_720p60,        ///< 1280x720 a 60fps  (calidad media-alta)
    HD_720p30,        ///< 1280x720 a 30fps  (calidad media)
    SD_480p30,        ///< 854x480 a 30fps   (calidad baja, ancho de banda limitado)
    Count
};

/**
 * @brief Convierte QualityProfile a nombre legible.
 */
const char *qualityProfileToString(QualityProfile profile);

// ============================================================================
// Estructura de configuración de codificación
// ============================================================================

/**
 * @brief Configuración completa de codificación para una plataforma.
 */
struct EncoderPreset {
    // --- Identificación ---
    PlatformType platform;           ///< Plataforma de destino
    QualityProfile quality;          ///< Perfil de calidad
    std::string presetName;          ///< Nombre descriptivo del preset

    // --- Video ---
    std::string videoEncoder;        ///< ID del codificador (obs_x264, ffmpeg_nvenc, etc.)
    std::string videoProfile;        ///< Perfil H.264 (baseline, main, high)
    std::string videoPreset;         ///< Preset del codificador (veryfast, medium, etc.)
    std::string videoTune;           ///< Tune (zerolatency, etc.)
    int videoBitrateKbps = 4500;     ///< Bitrate de video en kbps
    int videoWidth = 1920;           ///< Ancho de video en píxeles
    int videoHeight = 1080;          ///< Alto de video en píxeles
    int videoFps = 30;               ///< Frames por segundo
    int keyframeIntervalSec = 2;     ///< Intervalo de keyframes en segundos
    int bFrames = 2;                 ///< Número de B-frames
    bool isVertical = false;         ///< ¿Formato vertical (9:16)?

    // --- Audio ---
    std::string audioEncoder;        ///< Codificador de audio (aac)
    int audioBitrateKbps = 128;      ///< Bitrate de audio en kbps
    int audioSampleRate = 44100;     ///< Tasa de muestreo en Hz
    int audioChannels = 2;           ///< Número de canales (2 = estéreo)

    // --- Protocolo ---
    bool requiresRtmps = false;      ///< ¿Requiere conexión segura rtmps?
    int maxFps = 60;                 ///< FPS máximo permitido por la plataforma

    // --- Notas ---
    std::string notes;               ///< Notas adicionales en español
};

// ============================================================================
// Clase PlatformPresets
// ============================================================================

/**
 * @brief Base de datos estática de presets de codificación optimizados por plataforma.
 *
 * Proporciona la configuración recomendada de codificador de video y audio
 * para cada plataforma de streaming. Los presets se pueden obtener como
 * obs_data_t* listos para aplicar directamente a los codificadores de OBS.
 */
class PlatformPresets {
public:
    PlatformPresets();
    ~PlatformPresets();

    /**
     * @brief Obtiene el preset de codificación para una plataforma y calidad.
     *
     * @param platform  Plataforma de destino
     * @param quality   Perfil de calidad deseado
     * @return          Preset con la configuración completa
     */
    EncoderPreset getPreset(PlatformType platform,
                            QualityProfile quality = QualityProfile::Full_1080p30) const;

    /**
     * @brief Obtiene la configuración del codificador como obs_data_t*.
     *
     * Retorna un obs_data_t* con todos los parámetros del codificador listos
     * para aplicar. El caller es responsable de liberar el obs_data_t con
     * obs_data_release().
     *
     * @param platform  Plataforma de destino
     * @param quality   Perfil de calidad deseado
     * @return          obs_data_t* con configuración (caller debe liberar), o nullptr si error
     */
    obs_data_t *getEncoderSettings(PlatformType platform,
                                    QualityProfile quality = QualityProfile::Full_1080p30) const;

    /**
     * @brief Obtiene solo la configuración de audio como obs_data_t*.
     *
     * @param platform  Plataforma de destino
     * @return          obs_data_t* con configuración de audio (caller debe liberar)
     */
    obs_data_t *getAudioSettings(PlatformType platform) const;

    /**
     * @brief Obtiene la lista de codificadores de video disponibles en el sistema.
     *
     * Detecta si NVENC, AMF, QSV u otros están disponibles.
     *
     * @return Vector con los IDs de codificadores disponibles
     */
    static std::vector<std::string> getAvailableEncoders();

    /**
     * @brief Selecciona el mejor codificador de video disponible.
     *
     * Prioridad: NVENC > AMF > QSV > x264
     *
     * @return ID del codificador seleccionado
     */
    static std::string selectBestEncoder();

    /**
     * @brief Obtiene el perfil de calidad más cercano para un bitrate dado.
     *
     * @param bitrateKbps  Bitrate disponible en kbps
     * @param platform     Plataforma de destino
     * @return             Perfil de calidad recomendado
     */
    static QualityProfile getQualityForBitrate(int bitrateKbps,
                                                PlatformType platform);

    /**
     * @brief Obtiene todos los presets disponibles para una plataforma.
     *
     * @param platform  Plataforma de destino
     * @return          Vector con todos los presets de calidad disponibles
     */
    std::vector<EncoderPreset> getAllPresetsForPlatform(PlatformType platform) const;

    /**
     * @brief Obtiene la resolución recomendada para una plataforma.
     *
     * @param platform  Plataforma de destino
     * @param quality   Perfil de calidad
     * @param[out] width   Ancho recomendado
     * @param[out] height  Alto recomendado
     */
    void getRecommendedResolution(PlatformType platform,
                                   QualityProfile quality,
                                   int &width, int &height) const;

private:
    /**
     * @brief Inicializa la base de datos de presets.
     * Se llama desde el constructor.
     */
    void initializePresets();

    /**
     * @brief Crea el preset para YouTube.
     */
    void createYouTubePresets();

    /**
     * @brief Crea el preset para Facebook.
     */
    void createFacebookPresets();

    /**
     * @brief Crea el preset para TikTok.
     */
    void createTikTokPresets();

    /**
     * @brief Crea el preset para Instagram.
     */
    void createInstagramPresets();

    /**
     * @brief Crea el preset para X (Twitter).
     */
    void createTwitterPresets();

    /**
     * @brief Aplica configuración de video al obs_data_t.
     */
    void applyVideoSettings(obs_data_t *settings,
                             const EncoderPreset &preset) const;

    /// Mapa de presets: clave = {plataforma, calidad}
    using PresetKey = std::pair<PlatformType, QualityProfile>;
    std::map<PresetKey, EncoderPreset> presets_;
};
