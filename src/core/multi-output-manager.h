/**
 * @file multi-output-manager.h
 * @brief Gestor central de transmisiones RTMP simultáneas con encoder compartido.
 *
 * Arquitectura Clave:
 * En lugar de codificar N veces el video (lo cual saturaría el CPU o los límites de sesiones NVENC del GPU),
 * este gestor instancia o reutiliza un ÚNICO encoder de video y audio, y distribuye los paquetes
 * ya codificados a múltiples instancias de obs_output_t conectadas a cada servidor RTMP.
 *
 * Proyecto: La Cueva del Dolor - OBS MultiStream Plugin
 * @copyright 2026 La Cueva del Dolor
 */

#pragma once

#include <obs-module.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <memory>
#include "platform-config.h"

namespace CuevaMultiStream {

enum class OutputStatus {
    Stopped = 0,
    Starting,
    Active,
    Stopping,
    Reconnecting,
    Failed
};

/**
 * @brief Información de telemetría de una salida activa.
 */
struct OutputTelemetry {
    OutputStatus status = OutputStatus::Stopped;
    uint64_t totalBytesSent = 0;
    int currentBitrateKbps = 0;
    int droppedFrames = 0;
    int totalFrames = 0;
    double congestionRatio = 0.0;
    std::string errorMessage;
};

/**
 * @brief Contexto por cada salida/plataforma administrada.
 */
struct StreamOutputContext {
    PlatformConfig config;
    obs_output_t *output = nullptr;
    obs_service_t *service = nullptr;
    OutputTelemetry telemetry;
    signal_handler_t *signalHandler = nullptr;
};

/**
 * @brief Gestor de múltiples salidas RTMP compartiendo encoders.
 */
class MultiOutputManager {
public:
    using StatusCallback = std::function<void(const std::string &platformId, OutputStatus newStatus, const std::string &message)>;
    using TelemetryCallback = std::function<void(const std::string &platformId, const OutputTelemetry &telemetry)>;

    static MultiOutputManager &instance();

    MultiOutputManager(const MultiOutputManager &) = delete;
    MultiOutputManager &operator=(const MultiOutputManager &) = delete;

    /**
     * @brief Inicializa el gestor y busca el encoder idóneo disponible (NVENC, AMF, QSV, x264).
     */
    bool initialize();

    /**
     * @brief Libera todas las salidas y recursos.
     */
    void shutdown();

    /**
     * @brief Registra o actualiza la configuración de una plataforma.
     */
    void setPlatformConfig(const PlatformConfig &config);

    /**
     * @brief Remueve una plataforma del gestor.
     */
    void removePlatform(const std::string &platformId);

    /**
     * @brief Inicia la transmisión en una plataforma individual.
     */
    bool startPlatform(const std::string &platformId);

    /**
     * @brief Detiene la transmisión en una plataforma individual.
     */
    void stopPlatform(const std::string &platformId);

    /**
     * @brief Inicia la transmisión en todas las plataformas habilitadas simultáneamente.
     */
    bool startAllEnabled();

    /**
     * @brief Detiene todas las transmisiones activas.
     */
    void stopAll();

    /**
     * @brief Consulta si al menos una salida está transmitiendo.
     */
    bool isAnyStreaming() const;

    /**
     * @brief Consulta el estado actual de una plataforma específica.
     */
    OutputStatus getPlatformStatus(const std::string &platformId) const;

    /**
     * @brief Obtiene la telemetría actual de una plataforma.
     */
    OutputTelemetry getTelemetry(const std::string &platformId) const;

    /**
     * @brief Registra callback para cambios de estado de las salidas.
     */
    void setStatusCallback(StatusCallback cb);

    /**
     * @brief Registra callback para telemetría periódica.
     */
    void setTelemetryCallback(TelemetryCallback cb);

    /**
     * @brief Actualiza periódicamente métricas de tasa de bits y paquetes perdidos.
     */
    void updateTelemetry();

private:
    MultiOutputManager();
    ~MultiOutputManager();

    // Gestión de encoders compartidos
    bool ensureSharedEncoders(int baseBitrateKbps, int audioBitrateKbps);
    void releaseSharedEncodersIfUnused();

    // Creación de salida OBS
    bool createOutputForContext(StreamOutputContext &ctx);
    void destroyOutputForContext(StreamOutputContext &ctx);

    // Callbacks de señal de OBS
    static void onOutputStarting(void *param, calldata_t *data);
    static void onOutputStart(void *param, calldata_t *data);
    static void onOutputStop(void *param, calldata_t *data);
    static void onOutputReconnect(void *param, calldata_t *data);
    static void onOutputReconnectSuccess(void *param, calldata_t *data);

    void handleSignal(const std::string &platformId, OutputStatus status, const std::string &msg);

    mutable std::mutex mutex_;
    std::unordered_map<std::string, StreamOutputContext> outputs_;

    obs_encoder_t *sharedVideoEncoder_ = nullptr;
    obs_encoder_t *sharedAudioEncoder_ = nullptr;
    std::string preferredVideoEncoderId_;

    StatusCallback statusCallback_;
    TelemetryCallback telemetryCallback_;
    bool initialized_ = false;
};

} // namespace CuevaMultiStream
