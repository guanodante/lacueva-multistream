/**
 * @file platform-presets.cpp
 * @brief Implementación de la base de datos estática de presets de codificación.
 *
 * Contiene las configuraciones óptimas de video y audio para YouTube, Facebook,
 * TikTok, Instagram y X (Twitter). Incluye detección de codificadores de
 * hardware y selección automática del mejor disponible.
 *
 * Proyecto: La Cueva del Dolor - OBS MultiStream Plugin
 * @copyright 2026 La Cueva del Dolor
 */

#include "platform-presets.h"
#include <obs-module.h>
#include <util/base.h>
#include <algorithm>

// ============================================================================
// Macro de log con prefijo del módulo
// ============================================================================
#define LOG_PREFIX "[CuevaMultiStream::Presets] "

// ============================================================================
// IDs de codificadores de OBS
// ============================================================================
static const char *ENCODER_X264       = "obs_x264";
static const char *ENCODER_NVENC      = "jim_nvenc";       // NVENC H.264 (Jim)
static const char *ENCODER_NVENC_NEW  = "ffmpeg_nvenc";    // NVENC H.264 (FFmpeg)
static const char *ENCODER_AMF        = "h264_texture_amf"; // AMD AMF H.264
static const char *ENCODER_QSV        = "obs_qsv11";       // Intel QSV H.264

// ============================================================================
// Funciones de utilidad
// ============================================================================

const char *qualityProfileToString(QualityProfile profile)
{
    switch (profile) {
    case QualityProfile::Full_1080p60: return "1080p60 (Máxima calidad)";
    case QualityProfile::Full_1080p30: return "1080p30 (Calidad alta)";
    case QualityProfile::HD_720p60:    return "720p60 (Calidad media-alta)";
    case QualityProfile::HD_720p30:    return "720p30 (Calidad media)";
    case QualityProfile::SD_480p30:    return "480p30 (Calidad baja)";
    default:                           return "Desconocido";
    }
}

// ============================================================================
// Constructor / Destructor
// ============================================================================

PlatformPresets::PlatformPresets()
{
    blog(LOG_INFO, LOG_PREFIX "Inicializando base de datos de presets de codificación");
    initializePresets();
    blog(LOG_INFO, LOG_PREFIX "Presets inicializados: %zu configuraciones cargadas",
         presets_.size());
}

PlatformPresets::~PlatformPresets()
{
    blog(LOG_INFO, LOG_PREFIX "Base de datos de presets destruida");
}

// ============================================================================
// Inicialización de presets
// ============================================================================

void PlatformPresets::initializePresets()
{
    createYouTubePresets();
    createFacebookPresets();
    createTikTokPresets();
    createInstagramPresets();
    createTwitterPresets();
}

// ----------------------------------------------------------------------------
// YouTube: x264/NVENC, profile=high, preset=veryfast, tune=zerolatency
//   1080p30 → 4500 kbps, 1080p60 → 6000 kbps
//   Audio: AAC 128kbps stereo 44100Hz, Keyframe: 2s, B-frames: 2
// ----------------------------------------------------------------------------

void PlatformPresets::createYouTubePresets()
{
    blog(LOG_DEBUG, LOG_PREFIX "Creando presets de YouTube");

    // YouTube 1080p60
    {
        EncoderPreset p;
        p.platform        = PlatformType::YouTube;
        p.quality          = QualityProfile::Full_1080p60;
        p.presetName       = "YouTube 1080p60";
        p.videoEncoder     = selectBestEncoder();
        p.videoProfile     = "high";
        p.videoPreset      = "veryfast";
        p.videoTune        = "zerolatency";
        p.videoBitrateKbps = 6000;
        p.videoWidth       = 1920;
        p.videoHeight      = 1080;
        p.videoFps         = 60;
        p.keyframeIntervalSec = 2;
        p.bFrames          = 2;
        p.isVertical       = false;
        p.audioEncoder     = "aac";
        p.audioBitrateKbps = 128;
        p.audioSampleRate  = 44100;
        p.audioChannels    = 2;
        p.requiresRtmps    = false;
        p.maxFps           = 60;
        p.notes            = "YouTube recomienda 6000 kbps para 1080p60. "
                             "Soporte para rtmp:// y rtmps://.";
        presets_[{PlatformType::YouTube, QualityProfile::Full_1080p60}] = p;
    }

    // YouTube 1080p30
    {
        EncoderPreset p;
        p.platform        = PlatformType::YouTube;
        p.quality          = QualityProfile::Full_1080p30;
        p.presetName       = "YouTube 1080p30";
        p.videoEncoder     = selectBestEncoder();
        p.videoProfile     = "high";
        p.videoPreset      = "veryfast";
        p.videoTune        = "zerolatency";
        p.videoBitrateKbps = 4500;
        p.videoWidth       = 1920;
        p.videoHeight      = 1080;
        p.videoFps         = 30;
        p.keyframeIntervalSec = 2;
        p.bFrames          = 2;
        p.isVertical       = false;
        p.audioEncoder     = "aac";
        p.audioBitrateKbps = 128;
        p.audioSampleRate  = 44100;
        p.audioChannels    = 2;
        p.requiresRtmps    = false;
        p.maxFps           = 60;
        p.notes            = "YouTube recomienda 4500 kbps para 1080p30.";
        presets_[{PlatformType::YouTube, QualityProfile::Full_1080p30}] = p;
    }

    // YouTube 720p60
    {
        EncoderPreset p;
        p.platform        = PlatformType::YouTube;
        p.quality          = QualityProfile::HD_720p60;
        p.presetName       = "YouTube 720p60";
        p.videoEncoder     = selectBestEncoder();
        p.videoProfile     = "high";
        p.videoPreset      = "veryfast";
        p.videoTune        = "zerolatency";
        p.videoBitrateKbps = 4000;
        p.videoWidth       = 1280;
        p.videoHeight      = 720;
        p.videoFps         = 60;
        p.keyframeIntervalSec = 2;
        p.bFrames          = 2;
        p.isVertical       = false;
        p.audioEncoder     = "aac";
        p.audioBitrateKbps = 128;
        p.audioSampleRate  = 44100;
        p.audioChannels    = 2;
        p.requiresRtmps    = false;
        p.maxFps           = 60;
        p.notes            = "YouTube 720p60 para ancho de banda medio.";
        presets_[{PlatformType::YouTube, QualityProfile::HD_720p60}] = p;
    }

    // YouTube 720p30
    {
        EncoderPreset p;
        p.platform        = PlatformType::YouTube;
        p.quality          = QualityProfile::HD_720p30;
        p.presetName       = "YouTube 720p30";
        p.videoEncoder     = selectBestEncoder();
        p.videoProfile     = "high";
        p.videoPreset      = "veryfast";
        p.videoTune        = "zerolatency";
        p.videoBitrateKbps = 2500;
        p.videoWidth       = 1280;
        p.videoHeight      = 720;
        p.videoFps         = 30;
        p.keyframeIntervalSec = 2;
        p.bFrames          = 2;
        p.isVertical       = false;
        p.audioEncoder     = "aac";
        p.audioBitrateKbps = 128;
        p.audioSampleRate  = 44100;
        p.audioChannels    = 2;
        p.requiresRtmps    = false;
        p.maxFps           = 60;
        p.notes            = "YouTube 720p30 para ancho de banda limitado.";
        presets_[{PlatformType::YouTube, QualityProfile::HD_720p30}] = p;
    }

    // YouTube 480p30
    {
        EncoderPreset p;
        p.platform        = PlatformType::YouTube;
        p.quality          = QualityProfile::SD_480p30;
        p.presetName       = "YouTube 480p30";
        p.videoEncoder     = selectBestEncoder();
        p.videoProfile     = "main";
        p.videoPreset      = "veryfast";
        p.videoTune        = "zerolatency";
        p.videoBitrateKbps = 1500;
        p.videoWidth       = 854;
        p.videoHeight      = 480;
        p.videoFps         = 30;
        p.keyframeIntervalSec = 2;
        p.bFrames          = 0;
        p.isVertical       = false;
        p.audioEncoder     = "aac";
        p.audioBitrateKbps = 96;
        p.audioSampleRate  = 44100;
        p.audioChannels    = 2;
        p.requiresRtmps    = false;
        p.maxFps           = 60;
        p.notes            = "YouTube 480p30 para ancho de banda muy limitado.";
        presets_[{PlatformType::YouTube, QualityProfile::SD_480p30}] = p;
    }
}

// ----------------------------------------------------------------------------
// Facebook: x264/NVENC, profile=high, preset=veryfast
//   1080p30 → 4500 kbps, 1080p60 → 6000 kbps
//   Audio: AAC 128kbps stereo 44100Hz, Keyframe: 2s, RTMPS requerido
// ----------------------------------------------------------------------------

void PlatformPresets::createFacebookPresets()
{
    blog(LOG_DEBUG, LOG_PREFIX "Creando presets de Facebook");

    // Facebook 1080p60
    {
        EncoderPreset p;
        p.platform        = PlatformType::Facebook;
        p.quality          = QualityProfile::Full_1080p60;
        p.presetName       = "Facebook 1080p60";
        p.videoEncoder     = selectBestEncoder();
        p.videoProfile     = "high";
        p.videoPreset      = "veryfast";
        p.videoTune        = "";
        p.videoBitrateKbps = 6000;
        p.videoWidth       = 1920;
        p.videoHeight      = 1080;
        p.videoFps         = 60;
        p.keyframeIntervalSec = 2;
        p.bFrames          = 2;
        p.isVertical       = false;
        p.audioEncoder     = "aac";
        p.audioBitrateKbps = 128;
        p.audioSampleRate  = 44100;
        p.audioChannels    = 2;
        p.requiresRtmps    = true;  // ¡Facebook requiere RTMPS!
        p.maxFps           = 60;
        p.notes            = "Facebook requiere RTMPS obligatorio. "
                             "6000 kbps para 1080p60.";
        presets_[{PlatformType::Facebook, QualityProfile::Full_1080p60}] = p;
    }

    // Facebook 1080p30
    {
        EncoderPreset p;
        p.platform        = PlatformType::Facebook;
        p.quality          = QualityProfile::Full_1080p30;
        p.presetName       = "Facebook 1080p30";
        p.videoEncoder     = selectBestEncoder();
        p.videoProfile     = "high";
        p.videoPreset      = "veryfast";
        p.videoTune        = "";
        p.videoBitrateKbps = 4500;
        p.videoWidth       = 1920;
        p.videoHeight      = 1080;
        p.videoFps         = 30;
        p.keyframeIntervalSec = 2;
        p.bFrames          = 2;
        p.isVertical       = false;
        p.audioEncoder     = "aac";
        p.audioBitrateKbps = 128;
        p.audioSampleRate  = 44100;
        p.audioChannels    = 2;
        p.requiresRtmps    = true;
        p.maxFps           = 60;
        p.notes            = "Facebook 1080p30 con RTMPS obligatorio.";
        presets_[{PlatformType::Facebook, QualityProfile::Full_1080p30}] = p;
    }

    // Facebook 720p30
    {
        EncoderPreset p;
        p.platform        = PlatformType::Facebook;
        p.quality          = QualityProfile::HD_720p30;
        p.presetName       = "Facebook 720p30";
        p.videoEncoder     = selectBestEncoder();
        p.videoProfile     = "high";
        p.videoPreset      = "veryfast";
        p.videoTune        = "";
        p.videoBitrateKbps = 2500;
        p.videoWidth       = 1280;
        p.videoHeight      = 720;
        p.videoFps         = 30;
        p.keyframeIntervalSec = 2;
        p.bFrames          = 2;
        p.isVertical       = false;
        p.audioEncoder     = "aac";
        p.audioBitrateKbps = 128;
        p.audioSampleRate  = 44100;
        p.audioChannels    = 2;
        p.requiresRtmps    = true;
        p.maxFps           = 60;
        p.notes            = "Facebook 720p30 para ancho de banda limitado.";
        presets_[{PlatformType::Facebook, QualityProfile::HD_720p30}] = p;
    }

    // Facebook 720p60
    {
        EncoderPreset p;
        p.platform        = PlatformType::Facebook;
        p.quality          = QualityProfile::HD_720p60;
        p.presetName       = "Facebook 720p60";
        p.videoEncoder     = selectBestEncoder();
        p.videoProfile     = "high";
        p.videoPreset      = "veryfast";
        p.videoTune        = "";
        p.videoBitrateKbps = 4000;
        p.videoWidth       = 1280;
        p.videoHeight      = 720;
        p.videoFps         = 60;
        p.keyframeIntervalSec = 2;
        p.bFrames          = 2;
        p.isVertical       = false;
        p.audioEncoder     = "aac";
        p.audioBitrateKbps = 128;
        p.audioSampleRate  = 44100;
        p.audioChannels    = 2;
        p.requiresRtmps    = true;
        p.maxFps           = 60;
        p.notes            = "Facebook 720p60 con RTMPS obligatorio.";
        presets_[{PlatformType::Facebook, QualityProfile::HD_720p60}] = p;
    }

    // Facebook 480p30
    {
        EncoderPreset p;
        p.platform        = PlatformType::Facebook;
        p.quality          = QualityProfile::SD_480p30;
        p.presetName       = "Facebook 480p30";
        p.videoEncoder     = selectBestEncoder();
        p.videoProfile     = "main";
        p.videoPreset      = "veryfast";
        p.videoTune        = "";
        p.videoBitrateKbps = 1500;
        p.videoWidth       = 854;
        p.videoHeight      = 480;
        p.videoFps         = 30;
        p.keyframeIntervalSec = 2;
        p.bFrames          = 0;
        p.isVertical       = false;
        p.audioEncoder     = "aac";
        p.audioBitrateKbps = 96;
        p.audioSampleRate  = 44100;
        p.audioChannels    = 2;
        p.requiresRtmps    = true;
        p.maxFps           = 60;
        p.notes            = "Facebook 480p30 para ancho de banda muy limitado.";
        presets_[{PlatformType::Facebook, QualityProfile::SD_480p30}] = p;
    }
}

// ----------------------------------------------------------------------------
// TikTok: x264/NVENC, profile=main, preset=veryfast
//   Resolución VERTICAL 1080x1920 (9:16), 4000 kbps
//   Audio: AAC 128kbps stereo 44100Hz, Keyframe: 2s
// ----------------------------------------------------------------------------

void PlatformPresets::createTikTokPresets()
{
    blog(LOG_DEBUG, LOG_PREFIX "Creando presets de TikTok (formato VERTICAL 9:16)");

    // TikTok 1080x1920 (Full vertical)
    {
        EncoderPreset p;
        p.platform        = PlatformType::TikTok;
        p.quality          = QualityProfile::Full_1080p30;
        p.presetName       = "TikTok Vertical 1080x1920";
        p.videoEncoder     = selectBestEncoder();
        p.videoProfile     = "main";
        p.videoPreset      = "veryfast";
        p.videoTune        = "";
        p.videoBitrateKbps = 4000;
        p.videoWidth       = 1080;   // ¡VERTICAL! Ancho menor que alto
        p.videoHeight      = 1920;   // ¡VERTICAL! 9:16
        p.videoFps         = 30;
        p.keyframeIntervalSec = 2;
        p.bFrames          = 0;
        p.isVertical       = true;   // Marca como formato vertical
        p.audioEncoder     = "aac";
        p.audioBitrateKbps = 128;
        p.audioSampleRate  = 44100;
        p.audioChannels    = 2;
        p.requiresRtmps    = false;
        p.maxFps           = 30;
        p.notes            = "TikTok requiere formato VERTICAL 9:16 (1080x1920). "
                             "4000 kbps recomendado.";
        presets_[{PlatformType::TikTok, QualityProfile::Full_1080p30}] = p;
    }

    // TikTok 1080x1920 60fps (mismo que 30fps pero con más FPS)
    {
        EncoderPreset p;
        p.platform        = PlatformType::TikTok;
        p.quality          = QualityProfile::Full_1080p60;
        p.presetName       = "TikTok Vertical 1080x1920 60fps";
        p.videoEncoder     = selectBestEncoder();
        p.videoProfile     = "main";
        p.videoPreset      = "veryfast";
        p.videoTune        = "";
        p.videoBitrateKbps = 5000;
        p.videoWidth       = 1080;
        p.videoHeight      = 1920;
        p.videoFps         = 60;
        p.keyframeIntervalSec = 2;
        p.bFrames          = 0;
        p.isVertical       = true;
        p.audioEncoder     = "aac";
        p.audioBitrateKbps = 128;
        p.audioSampleRate  = 44100;
        p.audioChannels    = 2;
        p.requiresRtmps    = false;
        p.maxFps           = 60;
        p.notes            = "TikTok vertical 60fps para mayor fluidez.";
        presets_[{PlatformType::TikTok, QualityProfile::Full_1080p60}] = p;
    }

    // TikTok 720x1280 (vertical reducido)
    {
        EncoderPreset p;
        p.platform        = PlatformType::TikTok;
        p.quality          = QualityProfile::HD_720p30;
        p.presetName       = "TikTok Vertical 720x1280";
        p.videoEncoder     = selectBestEncoder();
        p.videoProfile     = "main";
        p.videoPreset      = "veryfast";
        p.videoTune        = "";
        p.videoBitrateKbps = 2500;
        p.videoWidth       = 720;
        p.videoHeight      = 1280;
        p.videoFps         = 30;
        p.keyframeIntervalSec = 2;
        p.bFrames          = 0;
        p.isVertical       = true;
        p.audioEncoder     = "aac";
        p.audioBitrateKbps = 128;
        p.audioSampleRate  = 44100;
        p.audioChannels    = 2;
        p.requiresRtmps    = false;
        p.maxFps           = 30;
        p.notes            = "TikTok vertical 720x1280 para ancho de banda limitado.";
        presets_[{PlatformType::TikTok, QualityProfile::HD_720p30}] = p;
    }

    // TikTok 720x1280 60fps
    {
        EncoderPreset p;
        p.platform        = PlatformType::TikTok;
        p.quality          = QualityProfile::HD_720p60;
        p.presetName       = "TikTok Vertical 720x1280 60fps";
        p.videoEncoder     = selectBestEncoder();
        p.videoProfile     = "main";
        p.videoPreset      = "veryfast";
        p.videoTune        = "";
        p.videoBitrateKbps = 3000;
        p.videoWidth       = 720;
        p.videoHeight      = 1280;
        p.videoFps         = 60;
        p.keyframeIntervalSec = 2;
        p.bFrames          = 0;
        p.isVertical       = true;
        p.audioEncoder     = "aac";
        p.audioBitrateKbps = 128;
        p.audioSampleRate  = 44100;
        p.audioChannels    = 2;
        p.requiresRtmps    = false;
        p.maxFps           = 60;
        p.notes            = "TikTok vertical 720p60.";
        presets_[{PlatformType::TikTok, QualityProfile::HD_720p60}] = p;
    }

    // TikTok 480x854 (vertical mínimo)
    {
        EncoderPreset p;
        p.platform        = PlatformType::TikTok;
        p.quality          = QualityProfile::SD_480p30;
        p.presetName       = "TikTok Vertical 480x854";
        p.videoEncoder     = selectBestEncoder();
        p.videoProfile     = "main";
        p.videoPreset      = "veryfast";
        p.videoTune        = "";
        p.videoBitrateKbps = 1500;
        p.videoWidth       = 480;
        p.videoHeight      = 854;
        p.videoFps         = 30;
        p.keyframeIntervalSec = 2;
        p.bFrames          = 0;
        p.isVertical       = true;
        p.audioEncoder     = "aac";
        p.audioBitrateKbps = 96;
        p.audioSampleRate  = 44100;
        p.audioChannels    = 2;
        p.requiresRtmps    = false;
        p.maxFps           = 30;
        p.notes            = "TikTok vertical mínimo para ancho de banda muy limitado.";
        presets_[{PlatformType::TikTok, QualityProfile::SD_480p30}] = p;
    }
}

// ----------------------------------------------------------------------------
// Instagram: x264/NVENC, profile=main, preset=veryfast
//   Resolución VERTICAL 720x1280 (9:16), 3500 kbps, Max 30fps
//   Audio: AAC 128kbps stereo 44100Hz, Keyframe: 2s
// ----------------------------------------------------------------------------

void PlatformPresets::createInstagramPresets()
{
    blog(LOG_DEBUG, LOG_PREFIX
         "Creando presets de Instagram (formato VERTICAL 9:16, máximo 30fps)");

    // Instagram 720x1280 (vertical, calidad máxima = 30fps)
    {
        EncoderPreset p;
        p.platform        = PlatformType::Instagram;
        p.quality          = QualityProfile::Full_1080p30; // Mejor calidad disponible
        p.presetName       = "Instagram Vertical 720x1280";
        p.videoEncoder     = selectBestEncoder();
        p.videoProfile     = "main";
        p.videoPreset      = "veryfast";
        p.videoTune        = "";
        p.videoBitrateKbps = 3500;
        p.videoWidth       = 720;    // ¡VERTICAL! Instagram máximo 720p
        p.videoHeight      = 1280;   // ¡VERTICAL! 9:16
        p.videoFps         = 30;     // ¡Instagram máximo 30fps!
        p.keyframeIntervalSec = 2;
        p.bFrames          = 0;
        p.isVertical       = true;
        p.audioEncoder     = "aac";
        p.audioBitrateKbps = 128;
        p.audioSampleRate  = 44100;
        p.audioChannels    = 2;
        p.requiresRtmps    = false;
        p.maxFps           = 30;     // ¡Límite estricto de 30fps!
        p.notes            = "Instagram requiere formato VERTICAL 9:16 (720x1280). "
                             "Máximo 30fps. 3500 kbps recomendado.";
        presets_[{PlatformType::Instagram, QualityProfile::Full_1080p30}] = p;
    }

    // Instagram 1080p60 → limitado a 720x1280@30fps (Instagram no soporta 60fps)
    {
        EncoderPreset p;
        p.platform        = PlatformType::Instagram;
        p.quality          = QualityProfile::Full_1080p60;
        p.presetName       = "Instagram Vertical 720x1280 (máx. 30fps)";
        p.videoEncoder     = selectBestEncoder();
        p.videoProfile     = "main";
        p.videoPreset      = "veryfast";
        p.videoTune        = "";
        p.videoBitrateKbps = 3500;
        p.videoWidth       = 720;
        p.videoHeight      = 1280;
        p.videoFps         = 30;     // Forzado a 30fps aunque se pida 60
        p.keyframeIntervalSec = 2;
        p.bFrames          = 0;
        p.isVertical       = true;
        p.audioEncoder     = "aac";
        p.audioBitrateKbps = 128;
        p.audioSampleRate  = 44100;
        p.audioChannels    = 2;
        p.requiresRtmps    = false;
        p.maxFps           = 30;
        p.notes            = "Instagram NO soporta 60fps. Se limita automáticamente a 30fps.";
        presets_[{PlatformType::Instagram, QualityProfile::Full_1080p60}] = p;
    }

    // Instagram 720p30 reducido
    {
        EncoderPreset p;
        p.platform        = PlatformType::Instagram;
        p.quality          = QualityProfile::HD_720p30;
        p.presetName       = "Instagram Vertical 540x960";
        p.videoEncoder     = selectBestEncoder();
        p.videoProfile     = "main";
        p.videoPreset      = "veryfast";
        p.videoTune        = "";
        p.videoBitrateKbps = 2500;
        p.videoWidth       = 540;
        p.videoHeight      = 960;
        p.videoFps         = 30;
        p.keyframeIntervalSec = 2;
        p.bFrames          = 0;
        p.isVertical       = true;
        p.audioEncoder     = "aac";
        p.audioBitrateKbps = 128;
        p.audioSampleRate  = 44100;
        p.audioChannels    = 2;
        p.requiresRtmps    = false;
        p.maxFps           = 30;
        p.notes            = "Instagram vertical reducido 540x960 para ancho de banda limitado.";
        presets_[{PlatformType::Instagram, QualityProfile::HD_720p30}] = p;
    }

    // Instagram 720p60 → igual que 720p30 (Instagram no soporta 60fps)
    {
        EncoderPreset p;
        p.platform        = PlatformType::Instagram;
        p.quality          = QualityProfile::HD_720p60;
        p.presetName       = "Instagram Vertical 720x1280 (máx. 30fps)";
        p.videoEncoder     = selectBestEncoder();
        p.videoProfile     = "main";
        p.videoPreset      = "veryfast";
        p.videoTune        = "";
        p.videoBitrateKbps = 3500;
        p.videoWidth       = 720;
        p.videoHeight      = 1280;
        p.videoFps         = 30;
        p.keyframeIntervalSec = 2;
        p.bFrames          = 0;
        p.isVertical       = true;
        p.audioEncoder     = "aac";
        p.audioBitrateKbps = 128;
        p.audioSampleRate  = 44100;
        p.audioChannels    = 2;
        p.requiresRtmps    = false;
        p.maxFps           = 30;
        p.notes            = "Instagram forzado a 30fps.";
        presets_[{PlatformType::Instagram, QualityProfile::HD_720p60}] = p;
    }

    // Instagram 480p30 (mínimo)
    {
        EncoderPreset p;
        p.platform        = PlatformType::Instagram;
        p.quality          = QualityProfile::SD_480p30;
        p.presetName       = "Instagram Vertical 360x640";
        p.videoEncoder     = selectBestEncoder();
        p.videoProfile     = "baseline";
        p.videoPreset      = "veryfast";
        p.videoTune        = "";
        p.videoBitrateKbps = 1200;
        p.videoWidth       = 360;
        p.videoHeight      = 640;
        p.videoFps         = 30;
        p.keyframeIntervalSec = 2;
        p.bFrames          = 0;
        p.isVertical       = true;
        p.audioEncoder     = "aac";
        p.audioBitrateKbps = 96;
        p.audioSampleRate  = 44100;
        p.audioChannels    = 2;
        p.requiresRtmps    = false;
        p.maxFps           = 30;
        p.notes            = "Instagram mínimo 360x640 para ancho de banda crítico.";
        presets_[{PlatformType::Instagram, QualityProfile::SD_480p30}] = p;
    }
}

// ----------------------------------------------------------------------------
// X (Twitter): x264/NVENC, profile=high, preset=veryfast
//   1080p30 → 4000 kbps
//   Audio: AAC 128kbps stereo 44100Hz, Keyframe: 2s
// ----------------------------------------------------------------------------

void PlatformPresets::createTwitterPresets()
{
    blog(LOG_DEBUG, LOG_PREFIX "Creando presets de X (Twitter)");

    // Twitter 1080p30
    {
        EncoderPreset p;
        p.platform        = PlatformType::Twitter;
        p.quality          = QualityProfile::Full_1080p30;
        p.presetName       = "X (Twitter) 1080p30";
        p.videoEncoder     = selectBestEncoder();
        p.videoProfile     = "high";
        p.videoPreset      = "veryfast";
        p.videoTune        = "";
        p.videoBitrateKbps = 4000;
        p.videoWidth       = 1920;
        p.videoHeight      = 1080;
        p.videoFps         = 30;
        p.keyframeIntervalSec = 2;
        p.bFrames          = 2;
        p.isVertical       = false;
        p.audioEncoder     = "aac";
        p.audioBitrateKbps = 128;
        p.audioSampleRate  = 44100;
        p.audioChannels    = 2;
        p.requiresRtmps    = false;
        p.maxFps           = 60;
        p.notes            = "X (Twitter) recomienda 4000 kbps para 1080p30.";
        presets_[{PlatformType::Twitter, QualityProfile::Full_1080p30}] = p;
    }

    // Twitter 1080p60
    {
        EncoderPreset p;
        p.platform        = PlatformType::Twitter;
        p.quality          = QualityProfile::Full_1080p60;
        p.presetName       = "X (Twitter) 1080p60";
        p.videoEncoder     = selectBestEncoder();
        p.videoProfile     = "high";
        p.videoPreset      = "veryfast";
        p.videoTune        = "";
        p.videoBitrateKbps = 5000;
        p.videoWidth       = 1920;
        p.videoHeight      = 1080;
        p.videoFps         = 60;
        p.keyframeIntervalSec = 2;
        p.bFrames          = 2;
        p.isVertical       = false;
        p.audioEncoder     = "aac";
        p.audioBitrateKbps = 128;
        p.audioSampleRate  = 44100;
        p.audioChannels    = 2;
        p.requiresRtmps    = false;
        p.maxFps           = 60;
        p.notes            = "X (Twitter) 1080p60.";
        presets_[{PlatformType::Twitter, QualityProfile::Full_1080p60}] = p;
    }

    // Twitter 720p60
    {
        EncoderPreset p;
        p.platform        = PlatformType::Twitter;
        p.quality          = QualityProfile::HD_720p60;
        p.presetName       = "X (Twitter) 720p60";
        p.videoEncoder     = selectBestEncoder();
        p.videoProfile     = "high";
        p.videoPreset      = "veryfast";
        p.videoTune        = "";
        p.videoBitrateKbps = 3500;
        p.videoWidth       = 1280;
        p.videoHeight      = 720;
        p.videoFps         = 60;
        p.keyframeIntervalSec = 2;
        p.bFrames          = 2;
        p.isVertical       = false;
        p.audioEncoder     = "aac";
        p.audioBitrateKbps = 128;
        p.audioSampleRate  = 44100;
        p.audioChannels    = 2;
        p.requiresRtmps    = false;
        p.maxFps           = 60;
        p.notes            = "X (Twitter) 720p60 para ancho de banda medio.";
        presets_[{PlatformType::Twitter, QualityProfile::HD_720p60}] = p;
    }

    // Twitter 720p30
    {
        EncoderPreset p;
        p.platform        = PlatformType::Twitter;
        p.quality          = QualityProfile::HD_720p30;
        p.presetName       = "X (Twitter) 720p30";
        p.videoEncoder     = selectBestEncoder();
        p.videoProfile     = "high";
        p.videoPreset      = "veryfast";
        p.videoTune        = "";
        p.videoBitrateKbps = 2500;
        p.videoWidth       = 1280;
        p.videoHeight      = 720;
        p.videoFps         = 30;
        p.keyframeIntervalSec = 2;
        p.bFrames          = 2;
        p.isVertical       = false;
        p.audioEncoder     = "aac";
        p.audioBitrateKbps = 128;
        p.audioSampleRate  = 44100;
        p.audioChannels    = 2;
        p.requiresRtmps    = false;
        p.maxFps           = 60;
        p.notes            = "X (Twitter) 720p30 para ancho de banda limitado.";
        presets_[{PlatformType::Twitter, QualityProfile::HD_720p30}] = p;
    }

    // Twitter 480p30
    {
        EncoderPreset p;
        p.platform        = PlatformType::Twitter;
        p.quality          = QualityProfile::SD_480p30;
        p.presetName       = "X (Twitter) 480p30";
        p.videoEncoder     = selectBestEncoder();
        p.videoProfile     = "main";
        p.videoPreset      = "veryfast";
        p.videoTune        = "";
        p.videoBitrateKbps = 1500;
        p.videoWidth       = 854;
        p.videoHeight      = 480;
        p.videoFps         = 30;
        p.keyframeIntervalSec = 2;
        p.bFrames          = 0;
        p.isVertical       = false;
        p.audioEncoder     = "aac";
        p.audioBitrateKbps = 96;
        p.audioSampleRate  = 44100;
        p.audioChannels    = 2;
        p.requiresRtmps    = false;
        p.maxFps           = 60;
        p.notes            = "X (Twitter) 480p30 para ancho de banda muy limitado.";
        presets_[{PlatformType::Twitter, QualityProfile::SD_480p30}] = p;
    }
}

// ============================================================================
// Obtención de presets
// ============================================================================

EncoderPreset PlatformPresets::getPreset(PlatformType platform,
                                          QualityProfile quality) const
{
    auto it = presets_.find({platform, quality});
    if (it != presets_.end()) {
        blog(LOG_DEBUG, LOG_PREFIX "Preset encontrado: %s - %s",
             platformTypeToString(platform), qualityProfileToString(quality));
        return it->second;
    }

    // Si no hay un preset exacto, buscar el más cercano de menor calidad
    blog(LOG_WARNING, LOG_PREFIX
         "Preset no encontrado para %s/%s, buscando alternativa...",
         platformTypeToString(platform), qualityProfileToString(quality));

    // Recorrer calidades de mayor a menor
    for (int q = static_cast<int>(quality); q < static_cast<int>(QualityProfile::Count); ++q) {
        auto fallback = presets_.find({platform, static_cast<QualityProfile>(q)});
        if (fallback != presets_.end()) {
            blog(LOG_INFO, LOG_PREFIX "Usando preset alternativo: %s",
                 fallback->second.presetName.c_str());
            return fallback->second;
        }
    }

    // Último recurso: devolver preset genérico
    blog(LOG_WARNING, LOG_PREFIX
         "No se encontró ningún preset para %s, usando valores por defecto",
         platformTypeToString(platform));

    EncoderPreset defaultPreset;
    defaultPreset.platform        = platform;
    defaultPreset.quality          = QualityProfile::HD_720p30;
    defaultPreset.presetName       = "Preset por defecto";
    defaultPreset.videoEncoder     = selectBestEncoder();
    defaultPreset.videoProfile     = "main";
    defaultPreset.videoPreset      = "veryfast";
    defaultPreset.videoBitrateKbps = 2500;
    defaultPreset.videoWidth       = 1280;
    defaultPreset.videoHeight      = 720;
    defaultPreset.videoFps         = 30;
    defaultPreset.keyframeIntervalSec = 2;
    defaultPreset.audioEncoder     = "aac";
    defaultPreset.audioBitrateKbps = 128;
    defaultPreset.audioSampleRate  = 44100;
    defaultPreset.audioChannels    = 2;
    return defaultPreset;
}

// ============================================================================
// Generación de obs_data_t* con configuración del codificador
// ============================================================================

obs_data_t *PlatformPresets::getEncoderSettings(PlatformType platform,
                                                  QualityProfile quality) const
{
    EncoderPreset preset = getPreset(platform, quality);

    obs_data_t *settings = obs_data_create();
    if (!settings) {
        blog(LOG_ERROR, LOG_PREFIX
             "Error al crear obs_data_t para configuración del codificador");
        return nullptr;
    }

    blog(LOG_INFO, LOG_PREFIX
         "Generando configuración del codificador para %s (%s)",
         preset.presetName.c_str(), preset.videoEncoder.c_str());

    // --- Configuración de video ---
    applyVideoSettings(settings, preset);

    // --- Configuración de audio ---
    obs_data_set_int(settings, "audio_bitrate", preset.audioBitrateKbps);
    obs_data_set_int(settings, "audio_sample_rate", preset.audioSampleRate);
    obs_data_set_int(settings, "audio_channels", preset.audioChannels);

    // --- Metadatos adicionales ---
    obs_data_set_string(settings, "cueva_platform",
                         platformTypeToId(platform));
    obs_data_set_string(settings, "cueva_preset_name",
                         preset.presetName.c_str());
    obs_data_set_bool(settings, "cueva_is_vertical", preset.isVertical);
    obs_data_set_bool(settings, "cueva_requires_rtmps", preset.requiresRtmps);

    blog(LOG_INFO, LOG_PREFIX
         "Configuración generada: %dx%d@%dfps, %dkbps video, %dkbps audio",
         preset.videoWidth, preset.videoHeight, preset.videoFps,
         preset.videoBitrateKbps, preset.audioBitrateKbps);

    return settings;
}

void PlatformPresets::applyVideoSettings(obs_data_t *settings,
                                           const EncoderPreset &preset) const
{
    // Parámetros comunes a todos los codificadores
    obs_data_set_int(settings, "bitrate", preset.videoBitrateKbps);
    obs_data_set_string(settings, "profile", preset.videoProfile.c_str());
    obs_data_set_int(settings, "keyint_sec", preset.keyframeIntervalSec);
    obs_data_set_int(settings, "width", preset.videoWidth);
    obs_data_set_int(settings, "height", preset.videoHeight);
    obs_data_set_int(settings, "fps", preset.videoFps);
    obs_data_set_string(settings, "encoder_id", preset.videoEncoder.c_str());

    // Parámetros específicos según el tipo de codificador
    if (preset.videoEncoder == ENCODER_X264) {
        // Configuración para x264 (software)
        obs_data_set_string(settings, "preset", preset.videoPreset.c_str());
        if (!preset.videoTune.empty()) {
            obs_data_set_string(settings, "tune", preset.videoTune.c_str());
        }
        obs_data_set_int(settings, "bf", preset.bFrames);
        obs_data_set_string(settings, "rate_control", "CBR");
        obs_data_set_bool(settings, "vfr", false);

        blog(LOG_DEBUG, LOG_PREFIX
             "x264: preset=%s, tune=%s, profile=%s, bf=%d",
             preset.videoPreset.c_str(),
             preset.videoTune.empty() ? "(ninguno)" : preset.videoTune.c_str(),
             preset.videoProfile.c_str(), preset.bFrames);

    } else if (preset.videoEncoder == ENCODER_NVENC ||
               preset.videoEncoder == ENCODER_NVENC_NEW) {
        // Configuración para NVENC (hardware NVIDIA)
        obs_data_set_string(settings, "preset", "p4");  // Calidad/rendimiento equilibrado
        obs_data_set_string(settings, "rate_control", "CBR");
        obs_data_set_int(settings, "bf", preset.bFrames);
        obs_data_set_bool(settings, "lookahead", false);
        obs_data_set_bool(settings, "psycho_aq", true);

        // Mapear perfil H.264
        if (preset.videoProfile == "high") {
            obs_data_set_int(settings, "profile", 2); // High
        } else if (preset.videoProfile == "main") {
            obs_data_set_int(settings, "profile", 1); // Main
        } else {
            obs_data_set_int(settings, "profile", 0); // Baseline
        }

        blog(LOG_DEBUG, LOG_PREFIX
             "NVENC: preset=p4, profile=%s, bf=%d",
             preset.videoProfile.c_str(), preset.bFrames);

    } else if (preset.videoEncoder == ENCODER_AMF) {
        // Configuración para AMF (hardware AMD)
        obs_data_set_string(settings, "rate_control", "CBR");
        obs_data_set_string(settings, "preset", "speed"); // Equivalente a veryfast
        obs_data_set_int(settings, "bf", preset.bFrames);

        blog(LOG_DEBUG, LOG_PREFIX "AMF: preset=speed, profile=%s",
             preset.videoProfile.c_str());

    } else if (preset.videoEncoder == ENCODER_QSV) {
        // Configuración para QSV (hardware Intel)
        obs_data_set_string(settings, "target_usage", "speed");
        obs_data_set_string(settings, "rate_control", "CBR");
        obs_data_set_int(settings, "bf", preset.bFrames);

        blog(LOG_DEBUG, LOG_PREFIX "QSV: target_usage=speed, profile=%s",
             preset.videoProfile.c_str());
    }
}

obs_data_t *PlatformPresets::getAudioSettings(PlatformType platform) const
{
    // Obtener el preset base (1080p30 como referencia para audio)
    EncoderPreset preset = getPreset(platform, QualityProfile::Full_1080p30);

    obs_data_t *settings = obs_data_create();
    if (!settings) {
        blog(LOG_ERROR, LOG_PREFIX "Error al crear configuración de audio");
        return nullptr;
    }

    obs_data_set_int(settings, "bitrate", preset.audioBitrateKbps);
    obs_data_set_int(settings, "samplerate", preset.audioSampleRate);
    obs_data_set_int(settings, "channels", preset.audioChannels);

    blog(LOG_DEBUG, LOG_PREFIX
         "Audio para %s: %dkbps, %dHz, %d canales",
         platformTypeToString(platform), preset.audioBitrateKbps,
         preset.audioSampleRate, preset.audioChannels);

    return settings;
}

// ============================================================================
// Detección de codificadores de hardware
// ============================================================================

std::vector<std::string> PlatformPresets::getAvailableEncoders()
{
    std::vector<std::string> available;

    // x264 siempre está disponible (software)
    available.push_back(ENCODER_X264);

    // Verificar NVENC (NVIDIA)
    if (obs_get_encoder_type(ENCODER_NVENC) != OBS_ENCODER_VIDEO) {
        // Intentar la versión FFmpeg de NVENC
        if (obs_get_encoder_type(ENCODER_NVENC_NEW) == OBS_ENCODER_VIDEO) {
            available.push_back(ENCODER_NVENC_NEW);
            blog(LOG_INFO, LOG_PREFIX "Codificador NVENC (FFmpeg) detectado");
        }
    } else {
        available.push_back(ENCODER_NVENC);
        blog(LOG_INFO, LOG_PREFIX "Codificador NVENC (Jim) detectado");
    }

    // Verificar AMF (AMD)
    if (obs_get_encoder_type(ENCODER_AMF) == OBS_ENCODER_VIDEO) {
        available.push_back(ENCODER_AMF);
        blog(LOG_INFO, LOG_PREFIX "Codificador AMF (AMD) detectado");
    }

    // Verificar QSV (Intel)
    if (obs_get_encoder_type(ENCODER_QSV) == OBS_ENCODER_VIDEO) {
        available.push_back(ENCODER_QSV);
        blog(LOG_INFO, LOG_PREFIX "Codificador QSV (Intel) detectado");
    }

    blog(LOG_INFO, LOG_PREFIX "Total de codificadores disponibles: %zu",
         available.size());
    return available;
}

std::string PlatformPresets::selectBestEncoder()
{
    // Prioridad: NVENC > AMF > QSV > x264
    auto encoders = getAvailableEncoders();

    for (const auto &enc : encoders) {
        if (enc == ENCODER_NVENC || enc == ENCODER_NVENC_NEW) {
            blog(LOG_INFO, LOG_PREFIX "Codificador seleccionado: NVENC");
            return enc;
        }
    }

    for (const auto &enc : encoders) {
        if (enc == ENCODER_AMF) {
            blog(LOG_INFO, LOG_PREFIX "Codificador seleccionado: AMF");
            return enc;
        }
    }

    for (const auto &enc : encoders) {
        if (enc == ENCODER_QSV) {
            blog(LOG_INFO, LOG_PREFIX "Codificador seleccionado: QSV");
            return enc;
        }
    }

    blog(LOG_INFO, LOG_PREFIX
         "Codificador seleccionado: x264 (software, sin hardware disponible)");
    return ENCODER_X264;
}

QualityProfile PlatformPresets::getQualityForBitrate(int bitrateKbps,
                                                       PlatformType platform)
{
    // Determinar el mejor perfil de calidad para el bitrate disponible
    // Los umbrales se ajustan según la plataforma

    // Para plataformas verticales (TikTok, Instagram) los requisitos son menores
    bool isVerticalPlatform = (platform == PlatformType::TikTok ||
                               platform == PlatformType::Instagram);

    if (isVerticalPlatform) {
        // Plataformas verticales necesitan menos bitrate
        if (bitrateKbps >= 4000) return QualityProfile::Full_1080p60;
        if (bitrateKbps >= 3000) return QualityProfile::Full_1080p30;
        if (bitrateKbps >= 2000) return QualityProfile::HD_720p30;
        return QualityProfile::SD_480p30;
    } else {
        // Plataformas horizontales estándar
        if (bitrateKbps >= 5500) return QualityProfile::Full_1080p60;
        if (bitrateKbps >= 4000) return QualityProfile::Full_1080p30;
        if (bitrateKbps >= 3000) return QualityProfile::HD_720p60;
        if (bitrateKbps >= 2000) return QualityProfile::HD_720p30;
        return QualityProfile::SD_480p30;
    }
}

std::vector<EncoderPreset> PlatformPresets::getAllPresetsForPlatform(
    PlatformType platform) const
{
    std::vector<EncoderPreset> result;
    for (const auto &pair : presets_) {
        if (pair.first.first == platform) {
            result.push_back(pair.second);
        }
    }

    // Ordenar por calidad (mayor primero)
    std::sort(result.begin(), result.end(),
              [](const EncoderPreset &a, const EncoderPreset &b) {
                  return static_cast<int>(a.quality) < static_cast<int>(b.quality);
              });

    return result;
}

void PlatformPresets::getRecommendedResolution(PlatformType platform,
                                                 QualityProfile quality,
                                                 int &width, int &height) const
{
    EncoderPreset preset = getPreset(platform, quality);
    width  = preset.videoWidth;
    height = preset.videoHeight;
}
