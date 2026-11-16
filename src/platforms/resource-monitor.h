/**
 * @file resource-monitor.h
 * @brief Monitor de recursos del sistema (CPU, GPU, RAM, red).
 *
 * Monitorea periódicamente el uso de recursos del sistema en un hilo de fondo,
 * recomienda el tipo de codificador según la carga actual, y emite advertencias
 * cuando los recursos están críticamente altos.
 *
 * Proyecto: La Cueva del Dolor - OBS MultiStream Plugin
 * @copyright 2026 La Cueva del Dolor
 */

#pragma once

#include <obs-module.h>
#include <string>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>

// ============================================================================
// Tipo de codificador recomendado
// ============================================================================

/**
 * @brief Tipos de codificador de hardware/software.
 */
enum class EncoderType {
    Software_x264 = 0, ///< Codificación por software (CPU)
    Hardware_NVENC,     ///< NVIDIA NVENC (GPU NVIDIA)
    Hardware_AMF,       ///< AMD AMF (GPU AMD)
    Hardware_QSV,       ///< Intel Quick Sync Video (iGPU Intel)
    Unknown
};

/**
 * @brief Convierte EncoderType a nombre legible.
 */
const char *encoderTypeToString(EncoderType type);

/**
 * @brief Convierte EncoderType al ID del codificador de OBS.
 */
const char *encoderTypeToObsId(EncoderType type);

// ============================================================================
// Snapshot de recursos del sistema
// ============================================================================

/**
 * @brief Captura instantánea del estado de recursos del sistema.
 */
struct ResourceSnapshot {
    // --- CPU ---
    double cpuUsagePercent = 0.0;          ///< Uso de CPU en porcentaje (0-100)
    int cpuCoreCount = 0;                   ///< Número de núcleos lógicos

    // --- GPU ---
    double gpuUsagePercent = -1.0;         ///< Uso de GPU (0-100), -1 si no disponible
    double gpuMemoryUsagePercent = -1.0;   ///< Uso de VRAM (0-100), -1 si no disponible
    std::string gpuName;                    ///< Nombre de la GPU detectada
    bool gpuAvailable = false;              ///< ¿Se pudo obtener información de la GPU?

    // --- Memoria RAM ---
    double memoryUsagePercent = 0.0;       ///< Uso de RAM en porcentaje (0-100)
    uint64_t totalMemoryMB = 0;            ///< RAM total en MB
    uint64_t usedMemoryMB = 0;             ///< RAM usada en MB
    uint64_t availableMemoryMB = 0;        ///< RAM disponible en MB

    // --- Red ---
    double networkUploadMbps = 0.0;        ///< Velocidad de subida actual en Mbps
    double networkDownloadMbps = 0.0;      ///< Velocidad de descarga actual en Mbps
    uint64_t bytesSentTotal = 0;           ///< Total de bytes enviados desde inicio
    uint64_t bytesReceivedTotal = 0;       ///< Total de bytes recibidos desde inicio

    // --- Timestamp ---
    std::chrono::steady_clock::time_point timestamp; ///< Momento de la captura

    // --- Evaluación de estado ---

    /**
     * @brief Verifica si el CPU está en nivel crítico (>85%).
     */
    bool isCpuCritical() const { return cpuUsagePercent > 85.0; }

    /**
     * @brief Verifica si la RAM está en nivel crítico (>90%).
     */
    bool isMemoryCritical() const { return memoryUsagePercent > 90.0; }

    /**
     * @brief Verifica si la GPU está en nivel crítico (>90%).
     */
    bool isGpuCritical() const {
        return gpuAvailable && gpuUsagePercent > 90.0;
    }

    /**
     * @brief Verifica si algún recurso está en nivel crítico.
     */
    bool isAnyCritical() const {
        return isCpuCritical() || isMemoryCritical() || isGpuCritical();
    }
};

// ============================================================================
// Niveles de advertencia
// ============================================================================

/**
 * @brief Niveles de severidad para advertencias de recursos.
 */
enum class ResourceWarningLevel {
    Normal = 0,     ///< Todo bien
    Elevated,       ///< Uso elevado (>70%) - información
    High,           ///< Uso alto (>80%) - precaución
    Critical        ///< Uso crítico (>85% CPU, >90% RAM) - alerta
};

/**
 * @brief Advertencia de recurso emitida por el monitor.
 */
struct ResourceWarning {
    ResourceWarningLevel level;        ///< Nivel de severidad
    std::string resourceName;          ///< Nombre del recurso (CPU, GPU, RAM, Red)
    double currentUsage;               ///< Uso actual en porcentaje
    std::string message;               ///< Mensaje en español
    std::chrono::steady_clock::time_point timestamp; ///< Momento de la advertencia
};

// ============================================================================
// Clase ResourceMonitor
// ============================================================================

/**
 * @brief Monitor de recursos del sistema con hilo de fondo periódico.
 *
 * Realiza muestreo cada 2 segundos del uso de CPU, GPU, RAM y red.
 * Recomienda el tipo de codificador según la carga y emite advertencias
 * cuando los recursos superan umbrales críticos.
 */
class ResourceMonitor {
public:
    ResourceMonitor();
    ~ResourceMonitor();

    // ---- No copiable ni movible ----
    ResourceMonitor(const ResourceMonitor &) = delete;
    ResourceMonitor &operator=(const ResourceMonitor &) = delete;
    ResourceMonitor(ResourceMonitor &&) = delete;
    ResourceMonitor &operator=(ResourceMonitor &&) = delete;

    // --- Control del monitor ---

    /**
     * @brief Inicia el monitoreo periódico en un hilo de fondo.
     *
     * El hilo muestrea recursos cada 2 segundos.
     */
    void start();

    /**
     * @brief Detiene el monitoreo y espera a que termine el hilo de fondo.
     */
    void stop();

    /**
     * @brief Verifica si el monitor está activo.
     */
    bool isRunning() const;

    /**
     * @brief Establece el intervalo de muestreo.
     *
     * @param intervalMs Intervalo en milisegundos (mínimo 500)
     */
    void setSampleInterval(int intervalMs);

    // --- Lectura de recursos (thread-safe) ---

    /**
     * @brief Obtiene el uso actual de CPU en porcentaje.
     */
    double getCpuUsage() const;

    /**
     * @brief Obtiene el uso actual de GPU en porcentaje.
     * @return Uso de GPU (0-100), o -1.0 si no disponible
     */
    double getGpuUsage() const;

    /**
     * @brief Obtiene el uso actual de RAM en porcentaje.
     */
    double getMemoryUsage() const;

    /**
     * @brief Obtiene el uso actual de red (upload) en Mbps.
     */
    double getNetworkUsage() const;

    /**
     * @brief Obtiene la captura completa más reciente de recursos.
     */
    ResourceSnapshot getLatestSnapshot() const;

    // --- Recomendaciones ---

    /**
     * @brief Recomienda el tipo de codificador basado en la carga actual.
     *
     * Si el CPU está muy cargado, recomienda codificación por hardware (NVENC/AMF/QSV).
     * Si la GPU está muy cargada, recomienda codificación por software (x264).
     * Si ambos están disponibles y con carga moderada, prefiere hardware.
     *
     * @return Tipo de codificador recomendado
     */
    EncoderType recommendEncoderType() const;

    /**
     * @brief Obtiene una descripción de la recomendación en español.
     */
    std::string getRecommendationReason() const;

    // --- Advertencias ---

    /**
     * @brief Obtiene las advertencias activas.
     */
    std::vector<ResourceWarning> getActiveWarnings() const;

    /**
     * @brief Obtiene el nivel de advertencia más alto actual.
     */
    ResourceWarningLevel getHighestWarningLevel() const;

    /// Callback para advertencias de recursos
    using WarningCallback = std::function<void(const ResourceWarning &)>;

    /**
     * @brief Registra un callback que se invoca al detectar recursos críticos.
     *
     * @param callback Función a llamar con cada advertencia
     */
    void setWarningCallback(WarningCallback callback);

    /// Callback para snapshots periódicos
    using SnapshotCallback = std::function<void(const ResourceSnapshot &)>;

    /**
     * @brief Registra un callback que se invoca en cada muestreo.
     *
     * @param callback Función a llamar con cada snapshot
     */
    void setSnapshotCallback(SnapshotCallback callback);

    // --- Detección de hardware ---

    /**
     * @brief Detecta los codificadores de hardware disponibles en el sistema.
     *
     * @return Vector con los tipos de codificador disponibles
     */
    static std::vector<EncoderType> detectAvailableEncoders();

    /**
     * @brief Verifica si un codificador de hardware específico está disponible.
     */
    static bool isEncoderAvailable(EncoderType type);

private:
    /**
     * @brief Función principal del hilo de monitoreo.
     */
    void monitorThreadFunc();

    /**
     * @brief Muestrea el uso de CPU del sistema.
     */
    double sampleCpuUsage();

    /**
     * @brief Muestrea el uso de GPU (si está disponible).
     */
    double sampleGpuUsage();

    /**
     * @brief Muestrea el uso de memoria RAM.
     */
    void sampleMemoryUsage(ResourceSnapshot &snapshot);

    /**
     * @brief Muestrea el uso de red.
     */
    void sampleNetworkUsage(ResourceSnapshot &snapshot);

    /**
     * @brief Evalúa los niveles de recursos y genera advertencias.
     */
    void evaluateWarnings(const ResourceSnapshot &snapshot);

    /**
     * @brief Genera una advertencia de recurso.
     */
    void emitWarning(ResourceWarningLevel level,
                      const std::string &resource,
                      double usage,
                      const std::string &message);

    // --- Estado del hilo ---
    std::thread monitorThread_;           ///< Hilo de monitoreo
    std::atomic<bool> running_{false};    ///< ¿Está activo el monitoreo?
    int sampleIntervalMs_ = 2000;        ///< Intervalo de muestreo (2 segundos)

    // --- Datos de recursos (protegidos por mutex) ---
    mutable std::mutex dataMutex_;
    ResourceSnapshot latestSnapshot_;
    std::vector<ResourceWarning> activeWarnings_;

    // --- Callbacks ---
    WarningCallback warningCallback_;
    SnapshotCallback snapshotCallback_;

    // --- Estado interno para cálculos incrementales ---
    uint64_t prevIdleTime_ = 0;           ///< Tiempo idle anterior (para delta CPU)
    uint64_t prevKernelTime_ = 0;         ///< Tiempo kernel anterior
    uint64_t prevUserTime_ = 0;           ///< Tiempo usuario anterior
    uint64_t prevBytesSent_ = 0;          ///< Bytes enviados en la muestra anterior
    uint64_t prevBytesReceived_ = 0;      ///< Bytes recibidos en la muestra anterior
    std::chrono::steady_clock::time_point prevNetworkSample_; ///< Timestamp de muestra anterior

    // --- Control de advertencias ---
    std::chrono::steady_clock::time_point lastCpuWarning_;    ///< Última advertencia de CPU
    std::chrono::steady_clock::time_point lastMemoryWarning_; ///< Última advertencia de RAM
    std::chrono::steady_clock::time_point lastGpuWarning_;    ///< Última advertencia de GPU
    static constexpr int WARNING_COOLDOWN_SEC = 30; ///< Segundos entre advertencias iguales
};
