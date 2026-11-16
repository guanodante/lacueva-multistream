/**
 * @file bandwidth-optimizer.cpp
 * @brief Implementación del optimizador de ancho de banda para multi-streaming.
 *
 * Detecta la velocidad de subida, distribuye el bitrate entre plataformas
 * activas respetando el límite del 80%, y degrada automáticamente las
 * plataformas de menor prioridad cuando el ancho de banda es insuficiente.
 *
 * Proyecto: La Cueva del Dolor - OBS MultiStream Plugin
 * @copyright 2026 La Cueva del Dolor
 */

#include "bandwidth-optimizer.h"
#include <obs-module.h>
#include <util/base.h>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <chrono>

// ============================================================================
// Macro de log con prefijo del módulo
// ============================================================================
#define LOG_PREFIX "[CuevaMultiStream::Bandwidth] "

// ============================================================================
// Constantes de configuración
// ============================================================================

/// Factor de seguridad: usar máximo 80% del ancho de banda disponible
static constexpr double BANDWIDTH_SAFETY_FACTOR = 0.80;

/// Bitrate mínimo absoluto para cualquier stream (kbps)
static constexpr int MIN_ABSOLUTE_BITRATE_KBPS = 800;

/// Audio consume bitrate adicional por cada stream
static constexpr int AUDIO_OVERHEAD_KBPS = 128;

/// Overhead de protocolo RTMP estimado (~5%)
static constexpr double RTMP_OVERHEAD_FACTOR = 0.05;

// ============================================================================
// Tabla de bitrates ideales y mínimos por plataforma
// ============================================================================

/**
 * @brief Estructura interna con los parámetros de ancho de banda por plataforma.
 */
struct PlatformBandwidthParams {
    PlatformType platform;
    int idealBitrateKbps;     ///< Bitrate ideal a 1080p30
    int minimumBitrateKbps;   ///< Bitrate mínimo aceptable
    int priority;             ///< Prioridad (0 = máxima)
};

/// Tabla de parámetros: YouTube > Facebook > TikTok > X > Instagram
static const PlatformBandwidthParams BANDWIDTH_PARAMS[] = {
    { PlatformType::YouTube,   4500, 1500, 0 },  // Mayor prioridad
    { PlatformType::Facebook,  4500, 1500, 1 },
    { PlatformType::TikTok,    4000, 1200, 2 },
    { PlatformType::Instagram, 3500, 1000, 3 },
    { PlatformType::Twitter,   4000, 1500, 4 },  // Menor prioridad
    { PlatformType::Custom,    3000, 1000, 5 },
};

/**
 * @brief Busca los parámetros de ancho de banda para una plataforma.
 */
static const PlatformBandwidthParams *findBandwidthParams(PlatformType platform)
{
    for (const auto &params : BANDWIDTH_PARAMS) {
        if (params.platform == platform)
            return &params;
    }
    return nullptr;
}

// ============================================================================
// Constructor / Destructor
// ============================================================================

BandwidthOptimizer::BandwidthOptimizer()
{
    blog(LOG_INFO, LOG_PREFIX "Optimizador de ancho de banda inicializado");
}

BandwidthOptimizer::~BandwidthOptimizer()
{
    blog(LOG_INFO, LOG_PREFIX "Optimizador de ancho de banda destruido");
}

// ============================================================================
// Detección de velocidad de subida
// ============================================================================

double BandwidthOptimizer::detectUploadSpeed()
{
    blog(LOG_INFO, LOG_PREFIX "Iniciando prueba de velocidad de subida...");

    // -------------------------------------------------------------------------
    // Estrategia: Medir la velocidad de subida enviando datos a un endpoint
    // conocido y midiendo el throughput. Como alternativa, usamos la
    // información de la interfaz de red del sistema operativo.
    //
    // En un entorno de producción, se podría:
    // 1. Usar un servidor de prueba dedicado
    // 2. Medir el throughput durante los primeros segundos de streaming
    // 3. Usar APIs del SO para obtener la velocidad del adaptador
    //
    // Por ahora, medimos la velocidad del adaptador de red como estimación.
    // -------------------------------------------------------------------------

#ifdef _WIN32
    // En Windows, podemos obtener la velocidad del adaptador via WinAPI
    // Por ahora, usamos una estimación conservadora basada en la velocidad
    // de la interfaz de red reportada por el sistema

    // Intentar obtener estadísticas de red del sistema
    DWORD bufferSize = 0;
    DWORD result = GetAdaptersInfo(nullptr, &bufferSize);

    if (result == ERROR_BUFFER_OVERFLOW && bufferSize > 0) {
        std::vector<BYTE> buffer(bufferSize);
        PIP_ADAPTER_INFO adapterInfo =
            reinterpret_cast<PIP_ADAPTER_INFO>(buffer.data());

        result = GetAdaptersInfo(adapterInfo, &bufferSize);
        if (result == NO_ERROR) {
            // Usar el primer adaptador activo
            // La velocidad del adaptador no siempre refleja la velocidad real
            // de subida a internet, pero es una cota superior útil
            blog(LOG_INFO, LOG_PREFIX
                 "Adaptador de red detectado: %s", adapterInfo->Description);
        }
    }
#endif

    // -------------------------------------------------------------------------
    // Método de estimación: prueba de ráfaga de datos
    // Enviamos una pequeña cantidad de datos a un servidor y medimos el tiempo
    // -------------------------------------------------------------------------

    // Crear un buffer de prueba (256 KB)
    const size_t testDataSize = 256 * 1024;
    std::vector<uint8_t> testData(testDataSize, 0xAA);

    auto startTime = std::chrono::high_resolution_clock::now();

    // Simular el envío (en producción, se enviaría a un servidor real)
    // Usamos obs_output_get_total_bytes como indicador si hay un output activo
    obs_output_t *activeOutput = obs_get_output_by_name("cueva_speed_test");
    if (activeOutput) {
        // Si existe un output de prueba, medir su throughput real
        uint64_t totalBytes = obs_output_get_total_bytes(activeOutput);
        int totalFrames = obs_output_get_total_frames(activeOutput);

        if (totalBytes > 0 && totalFrames > 0) {
            // Calcular bitrate efectivo basado en datos reales
            double durationSec = static_cast<double>(totalFrames) / 30.0;
            double bitsPerSecond = (static_cast<double>(totalBytes) * 8.0) / durationSec;
            double mbps = bitsPerSecond / 1000000.0;

            blog(LOG_INFO, LOG_PREFIX
                 "Velocidad medida desde output activo: %.2f Mbps", mbps);

            obs_output_release(activeOutput);

            std::lock_guard<std::mutex> lock(mutex_);
            uploadSpeedMbps_ = mbps;
            return mbps;
        }
        obs_output_release(activeOutput);
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    double elapsedMs =
        std::chrono::duration<double, std::milli>(endTime - startTime).count();

    // Si no podemos medir directamente, usar un valor conservador
    // El usuario debería configurar esto manualmente para mejor precisión
    double estimatedMbps = 10.0; // Valor conservador por defecto

    blog(LOG_WARNING, LOG_PREFIX
         "No se pudo medir la velocidad real de subida. "
         "Usando estimación conservadora: %.1f Mbps. "
         "Se recomienda configurar la velocidad manualmente.", estimatedMbps);

    std::lock_guard<std::mutex> lock(mutex_);
    uploadSpeedMbps_ = estimatedMbps;
    return estimatedMbps;
}

void BandwidthOptimizer::setUploadSpeed(double speedMbps)
{
    if (speedMbps <= 0.0) {
        blog(LOG_WARNING, LOG_PREFIX
             "Velocidad de subida inválida: %.2f Mbps. Ignorando.", speedMbps);
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    uploadSpeedMbps_ = speedMbps;
    blog(LOG_INFO, LOG_PREFIX
         "Velocidad de subida establecida manualmente: %.2f Mbps", speedMbps);
}

double BandwidthOptimizer::getUploadSpeed() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return uploadSpeedMbps_;
}

// ============================================================================
// Cálculo de distribución óptima de ancho de banda
// ============================================================================

BandwidthOptimizationResult BandwidthOptimizer::calculateOptimalSettings(
    double uploadSpeedMbps,
    const std::vector<PlatformType> &enabledPlatforms)
{
    BandwidthOptimizationResult result;
    result.detectedUploadMbps = uploadSpeedMbps;

    blog(LOG_INFO, LOG_PREFIX
         "========== Optimización de ancho de banda ==========");
    blog(LOG_INFO, LOG_PREFIX
         "Velocidad de subida: %.2f Mbps", uploadSpeedMbps);
    blog(LOG_INFO, LOG_PREFIX
         "Plataformas activas: %zu", enabledPlatforms.size());

    // Validar entrada
    if (uploadSpeedMbps <= 0.0) {
        result.errorMessage =
            "La velocidad de subida debe ser mayor a 0. "
            "Configure su velocidad de internet.";
        blog(LOG_ERROR, LOG_PREFIX "%s", result.errorMessage.c_str());
        return result;
    }

    if (enabledPlatforms.empty()) {
        result.errorMessage =
            "No hay plataformas activas. "
            "Active al menos una plataforma para optimizar.";
        blog(LOG_WARNING, LOG_PREFIX "%s", result.errorMessage.c_str());
        return result;
    }

    // -------------------------------------------------------------------------
    // Paso 1: Calcular ancho de banda utilizable (80% del total)
    // -------------------------------------------------------------------------
    double usableMbps = uploadSpeedMbps * BANDWIDTH_SAFETY_FACTOR;
    int usableKbps = static_cast<int>(usableMbps * 1000.0);

    // Descontar overhead de protocolo RTMP
    int effectiveKbps = static_cast<int>(
        usableKbps * (1.0 - RTMP_OVERHEAD_FACTOR));

    result.usableUploadMbps = usableMbps;

    blog(LOG_INFO, LOG_PREFIX
         "Ancho de banda utilizable: %.2f Mbps (%d kbps efectivos)",
         usableMbps, effectiveKbps);

    // -------------------------------------------------------------------------
    // Paso 2: Descontar audio de cada stream del presupuesto total
    // -------------------------------------------------------------------------
    int totalAudioKbps = static_cast<int>(enabledPlatforms.size()) * AUDIO_OVERHEAD_KBPS;
    int availableForVideoKbps = effectiveKbps - totalAudioKbps;

    if (availableForVideoKbps <= 0) {
        result.errorMessage =
            "El ancho de banda es insuficiente incluso para el audio. "
            "Se requiere al menos " +
            std::to_string(totalAudioKbps) + " kbps para " +
            std::to_string(enabledPlatforms.size()) + " streams de audio.";
        blog(LOG_ERROR, LOG_PREFIX "%s", result.errorMessage.c_str());
        return result;
    }

    blog(LOG_INFO, LOG_PREFIX
         "Audio total: %d kbps (%zu streams x %d kbps)",
         totalAudioKbps, enabledPlatforms.size(), AUDIO_OVERHEAD_KBPS);
    blog(LOG_INFO, LOG_PREFIX
         "Disponible para video: %d kbps", availableForVideoKbps);

    // -------------------------------------------------------------------------
    // Paso 3: Ordenar plataformas por prioridad
    // -------------------------------------------------------------------------
    auto sortedPlatforms = sortByPriority(enabledPlatforms);

    // -------------------------------------------------------------------------
    // Paso 4: Calcular bitrate total ideal
    // -------------------------------------------------------------------------
    int totalIdealKbps = calculateTotalIdealBitrate(sortedPlatforms);

    blog(LOG_INFO, LOG_PREFIX
         "Bitrate ideal total: %d kbps, disponible: %d kbps",
         totalIdealKbps, availableForVideoKbps);

    // -------------------------------------------------------------------------
    // Paso 5: Distribuir ancho de banda
    // -------------------------------------------------------------------------

    if (availableForVideoKbps >= totalIdealKbps) {
        // ¡Hay suficiente ancho de banda para todas las plataformas al máximo!
        blog(LOG_INFO, LOG_PREFIX
             "Ancho de banda SUFICIENTE para todas las plataformas");

        for (const auto &platform : sortedPlatforms) {
            const auto *params = findBandwidthParams(platform);
            if (!params) continue;

            PlatformBandwidthAllocation alloc;
            alloc.platform = platform;
            alloc.allocatedBitrateKbps = params->idealBitrateKbps;
            alloc.audioBitrateKbps = AUDIO_OVERHEAD_KBPS;
            alloc.quality = QualityProfile::Full_1080p30;
            alloc.width = 1920;
            alloc.height = 1080;
            alloc.fps = 30;
            alloc.degraded = false;

            // Ajustar resolución para plataformas verticales
            if (platform == PlatformType::TikTok) {
                alloc.width = 1080;
                alloc.height = 1920;
            } else if (platform == PlatformType::Instagram) {
                alloc.width = 720;
                alloc.height = 1280;
                alloc.fps = 30; // Instagram máximo 30fps
            }

            result.allocations.push_back(alloc);
            result.totalAllocatedKbps += alloc.allocatedBitrateKbps +
                                         alloc.audioBitrateKbps;

            blog(LOG_INFO, LOG_PREFIX
                 "  %s: %d kbps video, %dx%d@%dfps (sin degradación)",
                 platformTypeToString(platform), alloc.allocatedBitrateKbps,
                 alloc.width, alloc.height, alloc.fps);
        }
    } else {
        // ¡Ancho de banda INSUFICIENTE! Necesitamos degradar plataformas
        blog(LOG_WARNING, LOG_PREFIX
             "Ancho de banda INSUFICIENTE. Iniciando degradación por prioridad...");

        result.warnings.push_back(
            "El ancho de banda disponible (" +
            std::to_string(availableForVideoKbps) +
            " kbps) es insuficiente para todas las plataformas al máximo (" +
            std::to_string(totalIdealKbps) +
            " kbps). Se degradarán las plataformas de menor prioridad.");

        int remainingKbps = availableForVideoKbps;

        for (const auto &platform : sortedPlatforms) {
            const auto *params = findBandwidthParams(platform);
            if (!params) continue;

            PlatformBandwidthAllocation alloc;
            alloc.platform = platform;
            alloc.audioBitrateKbps = AUDIO_OVERHEAD_KBPS;

            if (remainingKbps >= params->idealBitrateKbps) {
                // Esta plataforma recibe su bitrate ideal completo
                alloc.allocatedBitrateKbps = params->idealBitrateKbps;
                alloc.quality = QualityProfile::Full_1080p30;
                alloc.width = 1920;
                alloc.height = 1080;
                alloc.fps = 30;
                alloc.degraded = false;

                blog(LOG_INFO, LOG_PREFIX
                     "  %s: %d kbps (ideal, sin degradación)",
                     platformTypeToString(platform),
                     alloc.allocatedBitrateKbps);

            } else if (remainingKbps >= params->minimumBitrateKbps) {
                // Hay suficiente para el mínimo: degradar a 720p
                alloc.allocatedBitrateKbps = remainingKbps;
                alloc.degraded = true;
                degradePlatform(alloc, remainingKbps);

                std::string reason =
                    "Ancho de banda limitado. Se redujo de " +
                    std::to_string(params->idealBitrateKbps) + " kbps a " +
                    std::to_string(alloc.allocatedBitrateKbps) + " kbps.";
                alloc.degradationReason = reason;

                result.warnings.push_back(
                    platformTypeToString(platform) + std::string(": ") + reason);

                blog(LOG_WARNING, LOG_PREFIX
                     "  %s: DEGRADADO a %d kbps (%dx%d@%dfps)",
                     platformTypeToString(platform),
                     alloc.allocatedBitrateKbps,
                     alloc.width, alloc.height, alloc.fps);

            } else if (remainingKbps >= MIN_ABSOLUTE_BITRATE_KBPS) {
                // Bitrate muy bajo pero todavía viable
                alloc.allocatedBitrateKbps = remainingKbps;
                alloc.degraded = true;
                degradePlatform(alloc, remainingKbps);

                std::string reason =
                    "Ancho de banda críticamente bajo. Calidad muy reducida. "
                    "Se recomienda desactivar esta plataforma.";
                alloc.degradationReason = reason;

                result.warnings.push_back(
                    "⚠ " + std::string(platformTypeToString(platform)) +
                    ": " + reason);

                blog(LOG_WARNING, LOG_PREFIX
                     "  %s: CALIDAD CRÍTICA %d kbps (%dx%d@%dfps)",
                     platformTypeToString(platform),
                     alloc.allocatedBitrateKbps,
                     alloc.width, alloc.height, alloc.fps);
            } else {
                // No hay ancho de banda suficiente ni para el mínimo
                alloc.allocatedBitrateKbps = 0;
                alloc.degraded = true;
                alloc.quality = QualityProfile::SD_480p30;
                alloc.width = 0;
                alloc.height = 0;
                alloc.fps = 0;

                std::string reason =
                    "No hay ancho de banda disponible. "
                    "Esta plataforma no puede transmitir.";
                alloc.degradationReason = reason;

                result.warnings.push_back(
                    "❌ " + std::string(platformTypeToString(platform)) +
                    ": " + reason);

                blog(LOG_ERROR, LOG_PREFIX
                     "  %s: SIN ANCHO DE BANDA - no puede transmitir",
                     platformTypeToString(platform));
            }

            // Ajustar resolución para plataformas verticales
            if (!alloc.degraded || alloc.allocatedBitrateKbps > 0) {
                if (platform == PlatformType::TikTok && !alloc.degraded) {
                    alloc.width = 1080;
                    alloc.height = 1920;
                } else if (platform == PlatformType::Instagram) {
                    // Instagram siempre vertical, máximo 30fps
                    if (!alloc.degraded) {
                        alloc.width = 720;
                        alloc.height = 1280;
                    }
                    alloc.fps = std::min(alloc.fps, 30);
                }
            }

            remainingKbps -= alloc.allocatedBitrateKbps;
            result.allocations.push_back(alloc);
            result.totalAllocatedKbps += alloc.allocatedBitrateKbps +
                                         alloc.audioBitrateKbps;
        }
    }

    // -------------------------------------------------------------------------
    // Paso 6: Verificaciones finales y advertencias
    // -------------------------------------------------------------------------

    // Advertencia si el ancho de banda es muy justo
    double usagePercent = (static_cast<double>(result.totalAllocatedKbps) /
                            static_cast<double>(usableKbps)) * 100.0;
    if (usagePercent > 95.0) {
        result.warnings.push_back(
            "El uso de ancho de banda está al " +
            std::to_string(static_cast<int>(usagePercent)) +
            "% de la capacidad disponible. Puede haber inestabilidad. "
            "Considere reducir el número de plataformas activas.");

        blog(LOG_WARNING, LOG_PREFIX
             "Uso de ancho de banda al %.1f%% - riesgo de inestabilidad",
             usagePercent);
    }

    // Advertencia general si hay degradación
    if (result.hasDowngrades()) {
        blog(LOG_WARNING, LOG_PREFIX
             "Se degradaron una o más plataformas por ancho de banda insuficiente");
    }

    result.success = true;

    blog(LOG_INFO, LOG_PREFIX
         "Optimización completada: %d kbps asignados de %d kbps disponibles (%.1f%%)",
         result.totalAllocatedKbps, usableKbps, usagePercent);
    blog(LOG_INFO, LOG_PREFIX
         "=================================================");

    // Notificar cambios si hay callback registrado
    if (changeCallback_) {
        changeCallback_(result);
    }

    return result;
}

// ============================================================================
// Métodos de apoyo
// ============================================================================

bool BandwidthOptimizer::isBandwidthSufficient(
    double uploadSpeedMbps,
    const std::vector<PlatformType> &enabledPlatforms)
{
    int usableKbps = static_cast<int>(
        uploadSpeedMbps * 1000.0 * BANDWIDTH_SAFETY_FACTOR *
        (1.0 - RTMP_OVERHEAD_FACTOR));

    int totalAudioKbps = static_cast<int>(enabledPlatforms.size()) * AUDIO_OVERHEAD_KBPS;
    int availableForVideoKbps = usableKbps - totalAudioKbps;
    int totalIdealKbps = calculateTotalIdealBitrate(enabledPlatforms);

    return availableForVideoKbps >= totalIdealKbps;
}

int BandwidthOptimizer::getMinimumBitrate(PlatformType platform)
{
    const auto *params = findBandwidthParams(platform);
    return params ? params->minimumBitrateKbps : MIN_ABSOLUTE_BITRATE_KBPS;
}

int BandwidthOptimizer::getIdealBitrate(PlatformType platform)
{
    const auto *params = findBandwidthParams(platform);
    return params ? params->idealBitrateKbps : 3000;
}

int BandwidthOptimizer::getPlatformPriority(PlatformType platform)
{
    const auto *params = findBandwidthParams(platform);
    return params ? params->priority : 99;
}

void BandwidthOptimizer::setOnBandwidthChange(BandwidthChangeCallback callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    changeCallback_ = callback;
}

// ============================================================================
// Métodos privados
// ============================================================================

int BandwidthOptimizer::calculateTotalIdealBitrate(
    const std::vector<PlatformType> &platforms) const
{
    int total = 0;
    for (const auto &p : platforms) {
        const auto *params = findBandwidthParams(p);
        if (params) {
            total += params->idealBitrateKbps;
        }
    }
    return total;
}

void BandwidthOptimizer::degradePlatform(PlatformBandwidthAllocation &allocation,
                                          int availableKbps)
{
    // Determinar el nivel de degradación según el bitrate disponible
    if (availableKbps >= 3500) {
        // 720p60 o 1080p30 reducido
        allocation.quality = QualityProfile::HD_720p60;
        allocation.width = 1280;
        allocation.height = 720;
        allocation.fps = 60;
        allocation.allocatedBitrateKbps = std::min(availableKbps, 3500);
    } else if (availableKbps >= 2500) {
        // 720p30
        allocation.quality = QualityProfile::HD_720p30;
        allocation.width = 1280;
        allocation.height = 720;
        allocation.fps = 30;
        allocation.allocatedBitrateKbps = std::min(availableKbps, 2500);
    } else if (availableKbps >= 1500) {
        // 480p30
        allocation.quality = QualityProfile::SD_480p30;
        allocation.width = 854;
        allocation.height = 480;
        allocation.fps = 30;
        allocation.allocatedBitrateKbps = std::min(availableKbps, 1500);
    } else {
        // Calidad mínima absoluta
        allocation.quality = QualityProfile::SD_480p30;
        allocation.width = 640;
        allocation.height = 360;
        allocation.fps = 24;
        allocation.allocatedBitrateKbps = availableKbps;
    }

    // Ajustar para plataformas verticales
    if (allocation.platform == PlatformType::TikTok ||
        allocation.platform == PlatformType::Instagram) {
        // Invertir resolución para formato vertical
        std::swap(allocation.width, allocation.height);

        // Instagram siempre máximo 30fps
        if (allocation.platform == PlatformType::Instagram) {
            allocation.fps = std::min(allocation.fps, 30);
        }
    }

    blog(LOG_DEBUG, LOG_PREFIX
         "Plataforma %s degradada: %dx%d@%dfps, %d kbps",
         platformTypeToString(allocation.platform),
         allocation.width, allocation.height,
         allocation.fps, allocation.allocatedBitrateKbps);
}

std::vector<PlatformType> BandwidthOptimizer::sortByPriority(
    const std::vector<PlatformType> &platforms) const
{
    std::vector<PlatformType> sorted = platforms;
    std::sort(sorted.begin(), sorted.end(),
              [](PlatformType a, PlatformType b) {
                  return getPlatformPriority(a) < getPlatformPriority(b);
              });
    return sorted;
}
