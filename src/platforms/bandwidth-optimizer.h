/**
 * @file bandwidth-optimizer.h
 * @brief Optimizador de ancho de banda para distribución entre múltiples streams.
 *
 * Detecta la velocidad de subida disponible, calcula la distribución óptima
 * de bitrate entre las plataformas activas, y degrada automáticamente la
 * calidad de las plataformas de menor prioridad cuando el ancho de banda
 * es insuficiente.
 *
 * Proyecto: La Cueva del Dolor - OBS MultiStream Plugin
 * @copyright 2026 La Cueva del Dolor
 */

#pragma once

#include <obs-module.h>
#include "platform-validator.h"  // Para PlatformType
#include "platform-presets.h"    // Para QualityProfile, EncoderPreset
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <atomic>

// ============================================================================
// Resultado de optimización de ancho de banda
// ============================================================================

/**
 * @brief Configuración optimizada para una plataforma individual.
 */
struct PlatformBandwidthAllocation {
    PlatformType platform;              ///< Plataforma
    int allocatedBitrateKbps = 0;       ///< Bitrate de video asignado en kbps
    int audioBitrateKbps = 128;         ///< Bitrate de audio en kbps
    QualityProfile quality;             ///< Perfil de calidad resultante
    int width = 1920;                   ///< Resolución ancho
    int height = 1080;                  ///< Resolución alto
    int fps = 30;                       ///< FPS asignados
    bool degraded = false;              ///< ¿Se degradó respecto al óptimo?
    std::string degradationReason;      ///< Razón de la degradación en español
};

/**
 * @brief Resultado completo de la optimización de ancho de banda.
 */
struct BandwidthOptimizationResult {
    bool success = false;                   ///< ¿Se pudo distribuir el ancho de banda?
    double detectedUploadMbps = 0.0;        ///< Velocidad de subida detectada
    double usableUploadMbps = 0.0;          ///< 80% de la velocidad de subida
    int totalAllocatedKbps = 0;             ///< Total de bitrate asignado
    std::vector<PlatformBandwidthAllocation> allocations; ///< Asignación por plataforma
    std::vector<std::string> warnings;      ///< Advertencias en español
    std::string errorMessage;               ///< Mensaje de error si no se pudo optimizar

    /**
     * @brief Verifica si alguna plataforma fue degradada.
     */
    bool hasDowngrades() const {
        for (const auto &a : allocations) {
            if (a.degraded)
                return true;
        }
        return false;
    }
};

// ============================================================================
// Clase BandwidthOptimizer
// ============================================================================

/**
 * @brief Optimizador de distribución de ancho de banda para multi-streaming.
 *
 * Analiza la velocidad de subida disponible y distribuye el bitrate entre
 * las plataformas activas respetando el límite del 80% del ancho de banda.
 * Las plataformas de menor prioridad se degradan automáticamente cuando
 * el ancho de banda es insuficiente.
 *
 * Orden de prioridad: YouTube > Facebook > TikTok > X (Twitter) > Instagram
 */
class BandwidthOptimizer {
public:
    BandwidthOptimizer();
    ~BandwidthOptimizer();

    // --- Detección de velocidad ---

    /**
     * @brief Detecta la velocidad de subida disponible.
     *
     * Realiza una prueba de velocidad simple enviando una ráfaga de datos
     * y midiendo el throughput. Esta operación puede tardar varios segundos.
     *
     * @return Velocidad de subida estimada en Mbps, o -1.0 si falla
     */
    double detectUploadSpeed();

    /**
     * @brief Establece manualmente la velocidad de subida.
     *
     * Útil cuando el usuario conoce su velocidad o no se desea hacer
     * la prueba automática.
     *
     * @param speedMbps Velocidad de subida en Mbps
     */
    void setUploadSpeed(double speedMbps);

    /**
     * @brief Obtiene la última velocidad de subida detectada/establecida.
     *
     * @return Velocidad en Mbps, o 0.0 si no se ha medido
     */
    double getUploadSpeed() const;

    // --- Optimización de distribución ---

    /**
     * @brief Calcula la distribución óptima de bitrate entre las plataformas activas.
     *
     * Regla principal: el bitrate total de todos los streams no debe superar
     * el 80% de la velocidad de subida disponible.
     *
     * Si el ancho de banda es insuficiente, se degradan las plataformas
     * de menor prioridad (720p, menor bitrate) automáticamente.
     *
     * @param uploadSpeedMbps    Velocidad de subida en Mbps
     * @param enabledPlatforms   Vector de plataformas activas
     * @return                   Resultado con la distribución optimizada
     */
    BandwidthOptimizationResult calculateOptimalSettings(
        double uploadSpeedMbps,
        const std::vector<PlatformType> &enabledPlatforms);

    /**
     * @brief Verifica si el ancho de banda es suficiente para las plataformas dadas.
     *
     * @param uploadSpeedMbps    Velocidad de subida en Mbps
     * @param enabledPlatforms   Plataformas activas
     * @return                   true si el ancho de banda es suficiente sin degradación
     */
    bool isBandwidthSufficient(double uploadSpeedMbps,
                                const std::vector<PlatformType> &enabledPlatforms);

    /**
     * @brief Obtiene el bitrate mínimo requerido para una plataforma.
     *
     * @param platform Plataforma
     * @return         Bitrate mínimo en kbps
     */
    static int getMinimumBitrate(PlatformType platform);

    /**
     * @brief Obtiene el bitrate ideal (óptimo) para una plataforma a 1080p30.
     *
     * @param platform Plataforma
     * @return         Bitrate ideal en kbps
     */
    static int getIdealBitrate(PlatformType platform);

    /**
     * @brief Obtiene la prioridad de la plataforma (menor = mayor prioridad).
     *
     * @param platform Plataforma
     * @return         Valor de prioridad (0 = máxima)
     */
    static int getPlatformPriority(PlatformType platform);

    /// Callback para notificar cambios en el ancho de banda
    using BandwidthChangeCallback =
        std::function<void(const BandwidthOptimizationResult &)>;

    /**
     * @brief Registra un callback para cambios en la distribución de ancho de banda.
     *
     * @param callback Función a llamar cuando cambia la distribución
     */
    void setOnBandwidthChange(BandwidthChangeCallback callback);

private:
    /**
     * @brief Calcula el bitrate total necesario para todas las plataformas al máximo.
     */
    int calculateTotalIdealBitrate(
        const std::vector<PlatformType> &platforms) const;

    /**
     * @brief Degrada una plataforma al siguiente nivel de calidad inferior.
     *
     * @param allocation     Asignación actual (se modifica in-place)
     * @param availableKbps  Bitrate máximo disponible para esta plataforma
     */
    void degradePlatform(PlatformBandwidthAllocation &allocation,
                          int availableKbps);

    /**
     * @brief Ordena las plataformas por prioridad (mayor prioridad primero).
     */
    std::vector<PlatformType> sortByPriority(
        const std::vector<PlatformType> &platforms) const;

    double uploadSpeedMbps_ = 0.0;          ///< Última velocidad de subida medida
    mutable std::mutex mutex_;               ///< Mutex para acceso thread-safe
    BandwidthChangeCallback changeCallback_; ///< Callback de cambios
};
