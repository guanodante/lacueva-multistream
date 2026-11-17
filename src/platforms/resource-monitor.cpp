/**
 * @file resource-monitor.cpp
 * @brief Implementación del monitor de recursos del sistema.
 *
 * Monitorea CPU, GPU, RAM y red en un hilo de fondo cada 2 segundos.
 * Usa APIs de Windows (PDH, NVAPI/DXGI, GlobalMemoryStatusEx, GetIfTable)
 * para obtener métricas del sistema. Emite advertencias cuando los recursos
 * superan umbrales críticos y recomienda el tipo de codificador.
 *
 * Proyecto: La Cueva del Dolor - OBS MultiStream Plugin
 * @copyright 2026 La Cueva del Dolor
 */

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "resource-monitor.h"
#include <obs-module.h>
#include <util/base.h>
#include <algorithm>
#include <cmath>

#ifdef _WIN32
#include <winsock2.h>
#include <Windows.h>
#include <Psapi.h>
#include <iphlpapi.h>
#include <dxgi.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "psapi.lib")
#endif

// ============================================================================
// Macro de log con prefijo del módulo
// ============================================================================
#define LOG_PREFIX "[CuevaMultiStream::ResourceMonitor] "

// ============================================================================
// Umbrales de advertencia
// ============================================================================
static constexpr double CPU_WARNING_ELEVATED  = 70.0;  ///< CPU uso elevado
static constexpr double CPU_WARNING_HIGH      = 80.0;  ///< CPU uso alto
static constexpr double CPU_WARNING_CRITICAL  = 85.0;  ///< CPU uso crítico

static constexpr double MEM_WARNING_ELEVATED  = 75.0;  ///< RAM uso elevado
static constexpr double MEM_WARNING_HIGH      = 85.0;  ///< RAM uso alto
static constexpr double MEM_WARNING_CRITICAL  = 90.0;  ///< RAM uso crítico

static constexpr double GPU_WARNING_ELEVATED  = 75.0;  ///< GPU uso elevado
static constexpr double GPU_WARNING_HIGH      = 85.0;  ///< GPU uso alto
static constexpr double GPU_WARNING_CRITICAL  = 90.0;  ///< GPU uso crítico

// ============================================================================
// Funciones de conversión de EncoderType
// ============================================================================

const char *encoderTypeToString(EncoderType type)
{
    switch (type) {
    case EncoderType::Software_x264:   return "x264 (Software/CPU)";
    case EncoderType::Hardware_NVENC:  return "NVENC (NVIDIA GPU)";
    case EncoderType::Hardware_AMF:    return "AMF (AMD GPU)";
    case EncoderType::Hardware_QSV:    return "QSV (Intel iGPU)";
    default:                           return "Desconocido";
    }
}

const char *encoderTypeToObsId(EncoderType type)
{
    switch (type) {
    case EncoderType::Software_x264:   return "obs_x264";
    case EncoderType::Hardware_NVENC:  return "jim_nvenc";
    case EncoderType::Hardware_AMF:    return "h264_texture_amf";
    case EncoderType::Hardware_QSV:    return "obs_qsv11";
    default:                           return "obs_x264";
    }
}

// ============================================================================
// Constructor / Destructor
// ============================================================================

ResourceMonitor::ResourceMonitor()
{
    blog(LOG_INFO, LOG_PREFIX "Monitor de recursos inicializado");

    // Inicializar timestamps para cálculos incrementales
    prevNetworkSample_ = std::chrono::steady_clock::now();
    lastCpuWarning_    = std::chrono::steady_clock::time_point{};
    lastMemoryWarning_ = std::chrono::steady_clock::time_point{};
    lastGpuWarning_    = std::chrono::steady_clock::time_point{};

    // Tomar una primera muestra de CPU para tener un baseline
    sampleCpuUsage();
}

ResourceMonitor::~ResourceMonitor()
{
    // Asegurar que el hilo de monitoreo se detenga antes de destruir
    if (running_.load()) {
        blog(LOG_INFO, LOG_PREFIX
             "Deteniendo monitor de recursos desde destructor...");
        stop();
    }

    blog(LOG_INFO, LOG_PREFIX "Monitor de recursos destruido");
}

// ============================================================================
// Control del monitor
// ============================================================================

void ResourceMonitor::start()
{
    if (running_.load()) {
        blog(LOG_WARNING, LOG_PREFIX
             "El monitor ya está en ejecución, ignorando start()");
        return;
    }

    blog(LOG_INFO, LOG_PREFIX
         "Iniciando monitoreo periódico (intervalo: %d ms)", sampleIntervalMs_);

    running_.store(true);
    monitorThread_ = std::thread(&ResourceMonitor::monitorThreadFunc, this);

    blog(LOG_INFO, LOG_PREFIX "Hilo de monitoreo iniciado correctamente");
}

void ResourceMonitor::stop()
{
    if (!running_.load()) {
        blog(LOG_DEBUG, LOG_PREFIX "El monitor ya está detenido");
        return;
    }

    blog(LOG_INFO, LOG_PREFIX "Deteniendo hilo de monitoreo...");
    running_.store(false);

    if (monitorThread_.joinable()) {
        monitorThread_.join();
        blog(LOG_INFO, LOG_PREFIX "Hilo de monitoreo detenido correctamente");
    }
}

bool ResourceMonitor::isRunning() const
{
    return running_.load();
}

void ResourceMonitor::setSampleInterval(int intervalMs)
{
    // Mínimo 500ms para no saturar el sistema
    sampleIntervalMs_ = std::max(500, intervalMs);
    blog(LOG_INFO, LOG_PREFIX
         "Intervalo de muestreo actualizado: %d ms", sampleIntervalMs_);
}

// ============================================================================
// Lectura de recursos (thread-safe)
// ============================================================================

double ResourceMonitor::getCpuUsage() const
{
    std::lock_guard<std::mutex> lock(dataMutex_);
    return latestSnapshot_.cpuUsagePercent;
}

double ResourceMonitor::getGpuUsage() const
{
    std::lock_guard<std::mutex> lock(dataMutex_);
    return latestSnapshot_.gpuUsagePercent;
}

double ResourceMonitor::getMemoryUsage() const
{
    std::lock_guard<std::mutex> lock(dataMutex_);
    return latestSnapshot_.memoryUsagePercent;
}

double ResourceMonitor::getNetworkUsage() const
{
    std::lock_guard<std::mutex> lock(dataMutex_);
    return latestSnapshot_.networkUploadMbps;
}

ResourceSnapshot ResourceMonitor::getLatestSnapshot() const
{
    std::lock_guard<std::mutex> lock(dataMutex_);
    return latestSnapshot_;
}

// ============================================================================
// Recomendación de codificador
// ============================================================================

EncoderType ResourceMonitor::recommendEncoderType() const
{
    std::lock_guard<std::mutex> lock(dataMutex_);

    const auto &snap = latestSnapshot_;

    // Obtener codificadores disponibles
    auto available = detectAvailableEncoders();

    // Si solo hay x264 disponible, no hay opción
    if (available.size() <= 1) {
        blog(LOG_DEBUG, LOG_PREFIX
             "Solo x264 disponible, recomendando software");
        return EncoderType::Software_x264;
    }

    // -------------------------------------------------------------------------
    // Lógica de decisión basada en carga actual:
    //
    // - CPU alta (>75%) + GPU baja (<70%) → usar GPU (NVENC/AMF/QSV)
    // - GPU alta (>80%) + CPU baja (<60%) → usar CPU (x264)
    // - Ambos moderados → preferir GPU por eficiencia
    // - Ambos altos → preferir GPU (generalmente menos impacto en streaming)
    // -------------------------------------------------------------------------

    bool cpuHigh = snap.cpuUsagePercent > 75.0;
    bool gpuHigh = snap.gpuAvailable && snap.gpuUsagePercent > 80.0;

    // Buscar el mejor codificador de hardware disponible
    EncoderType bestHardware = EncoderType::Unknown;
    for (const auto &enc : available) {
        if (enc != EncoderType::Software_x264) {
            bestHardware = enc;
            break; // Tomar el primero (ya están en orden de prioridad)
        }
    }

    if (bestHardware == EncoderType::Unknown) {
        // No hay hardware disponible
        return EncoderType::Software_x264;
    }

    if (cpuHigh && !gpuHigh) {
        // CPU sobrecargado, GPU libre → usar GPU
        blog(LOG_INFO, LOG_PREFIX
             "CPU alto (%.1f%%), GPU libre (%.1f%%) → recomendando %s",
             snap.cpuUsagePercent, snap.gpuUsagePercent,
             encoderTypeToString(bestHardware));
        return bestHardware;
    }

    if (gpuHigh && !cpuHigh) {
        // GPU sobrecargada, CPU libre → usar CPU
        blog(LOG_INFO, LOG_PREFIX
             "GPU alta (%.1f%%), CPU libre (%.1f%%) → recomendando x264",
             snap.gpuUsagePercent, snap.cpuUsagePercent);
        return EncoderType::Software_x264;
    }

    // En caso de duda, preferir codificación por hardware
    blog(LOG_DEBUG, LOG_PREFIX
         "Carga moderada/equilibrada → recomendando %s (hardware preferido)",
         encoderTypeToString(bestHardware));
    return bestHardware;
}

std::string ResourceMonitor::getRecommendationReason() const
{
    std::lock_guard<std::mutex> lock(dataMutex_);

    const auto &snap = latestSnapshot_;

    bool cpuHigh = snap.cpuUsagePercent > 75.0;
    bool gpuHigh = snap.gpuAvailable && snap.gpuUsagePercent > 80.0;

    if (cpuHigh && !gpuHigh) {
        return "El CPU está al " +
               std::to_string(static_cast<int>(snap.cpuUsagePercent)) +
               "%. Se recomienda codificación por hardware (GPU) "
               "para reducir la carga del procesador.";
    }

    if (gpuHigh && !cpuHigh) {
        return "La GPU está al " +
               std::to_string(static_cast<int>(snap.gpuUsagePercent)) +
               "%. Se recomienda codificación por software (x264/CPU) "
               "para no sobrecargar la tarjeta gráfica.";
    }

    if (cpuHigh && gpuHigh) {
        return "Tanto el CPU (" +
               std::to_string(static_cast<int>(snap.cpuUsagePercent)) +
               "%) como la GPU (" +
               std::to_string(static_cast<int>(snap.gpuUsagePercent)) +
               "%) están bajo carga alta. Se recomienda reducir la calidad "
               "o el número de streams activos.";
    }

    return "Los recursos del sistema están en niveles normales. "
           "Se recomienda codificación por hardware para mayor eficiencia.";
}

// ============================================================================
// Advertencias
// ============================================================================

std::vector<ResourceWarning> ResourceMonitor::getActiveWarnings() const
{
    std::lock_guard<std::mutex> lock(dataMutex_);
    return activeWarnings_;
}

ResourceWarningLevel ResourceMonitor::getHighestWarningLevel() const
{
    std::lock_guard<std::mutex> lock(dataMutex_);

    ResourceWarningLevel highest = ResourceWarningLevel::Normal;
    for (const auto &w : activeWarnings_) {
        if (static_cast<int>(w.level) > static_cast<int>(highest)) {
            highest = w.level;
        }
    }
    return highest;
}

void ResourceMonitor::setWarningCallback(WarningCallback callback)
{
    std::lock_guard<std::mutex> lock(dataMutex_);
    warningCallback_ = callback;
}

void ResourceMonitor::setSnapshotCallback(SnapshotCallback callback)
{
    std::lock_guard<std::mutex> lock(dataMutex_);
    snapshotCallback_ = callback;
}

// ============================================================================
// Detección de codificadores de hardware
// ============================================================================

std::vector<EncoderType> ResourceMonitor::detectAvailableEncoders()
{
    std::vector<EncoderType> available;

    // x264 siempre disponible
    available.push_back(EncoderType::Software_x264);

    // Verificar NVENC
    if (obs_get_encoder_type("jim_nvenc") == OBS_ENCODER_VIDEO ||
        obs_get_encoder_type("ffmpeg_nvenc") == OBS_ENCODER_VIDEO) {
        available.push_back(EncoderType::Hardware_NVENC);
        blog(LOG_DEBUG, LOG_PREFIX "NVENC detectado como disponible");
    }

    // Verificar AMF
    if (obs_get_encoder_type("h264_texture_amf") == OBS_ENCODER_VIDEO) {
        available.push_back(EncoderType::Hardware_AMF);
        blog(LOG_DEBUG, LOG_PREFIX "AMF detectado como disponible");
    }

    // Verificar QSV
    if (obs_get_encoder_type("obs_qsv11") == OBS_ENCODER_VIDEO) {
        available.push_back(EncoderType::Hardware_QSV);
        blog(LOG_DEBUG, LOG_PREFIX "QSV detectado como disponible");
    }

    return available;
}

bool ResourceMonitor::isEncoderAvailable(EncoderType type)
{
    auto available = detectAvailableEncoders();
    return std::find(available.begin(), available.end(), type) != available.end();
}

// ============================================================================
// Hilo de monitoreo principal
// ============================================================================

void ResourceMonitor::monitorThreadFunc()
{
    blog(LOG_INFO, LOG_PREFIX
         "Hilo de monitoreo de recursos iniciado (intervalo: %d ms)",
         sampleIntervalMs_);

    while (running_.load()) {
        // ---- Crear nuevo snapshot ----
        ResourceSnapshot snapshot;
        snapshot.timestamp = std::chrono::steady_clock::now();

        // Muestrear cada recurso
        snapshot.cpuUsagePercent = sampleCpuUsage();
        snapshot.gpuUsagePercent = sampleGpuUsage();
        snapshot.gpuAvailable   = (snapshot.gpuUsagePercent >= 0.0);
        sampleMemoryUsage(snapshot);
        sampleNetworkUsage(snapshot);

        // Obtener número de núcleos de CPU
#ifdef _WIN32
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        snapshot.cpuCoreCount = static_cast<int>(sysInfo.dwNumberOfProcessors);
#else
        snapshot.cpuCoreCount = std::thread::hardware_concurrency();
#endif

        // ---- Actualizar snapshot y evaluar advertencias ----
        {
            std::lock_guard<std::mutex> lock(dataMutex_);
            latestSnapshot_ = snapshot;
        }

        // Evaluar y emitir advertencias
        evaluateWarnings(snapshot);

        // Notificar snapshot si hay callback
        SnapshotCallback cb;
        {
            std::lock_guard<std::mutex> lock(dataMutex_);
            cb = snapshotCallback_;
        }
        if (cb) {
            cb(snapshot);
        }

        // Log periódico (cada 10 muestreos = ~20 segundos)
        static int logCounter = 0;
        if (++logCounter >= 10) {
            blog(LOG_DEBUG, LOG_PREFIX
                 "Recursos: CPU=%.1f%%, GPU=%.1f%%, RAM=%.1f%% (%llu/%llu MB), "
                 "Red↑=%.2f Mbps",
                 snapshot.cpuUsagePercent,
                 snapshot.gpuAvailable ? snapshot.gpuUsagePercent : -1.0,
                 snapshot.memoryUsagePercent,
                 (unsigned long long)snapshot.usedMemoryMB,
                 (unsigned long long)snapshot.totalMemoryMB,
                 snapshot.networkUploadMbps);
            logCounter = 0;
        }

        // ---- Esperar hasta el próximo muestreo ----
        // Dormir en intervalos cortos para poder detectar stop() rápidamente
        int remainingMs = sampleIntervalMs_;
        const int sleepChunkMs = 100;

        while (remainingMs > 0 && running_.load()) {
            int sleepMs = std::min(remainingMs, sleepChunkMs);
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
            remainingMs -= sleepMs;
        }
    }

    blog(LOG_INFO, LOG_PREFIX "Hilo de monitoreo de recursos finalizado");
}

// ============================================================================
// Muestreo de CPU
// ============================================================================

double ResourceMonitor::sampleCpuUsage()
{
#ifdef _WIN32
    // Obtener tiempos del sistema usando GetSystemTimes
    FILETIME idleTime, kernelTime, userTime;

    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        blog(LOG_WARNING, LOG_PREFIX
             "Error al obtener tiempos del sistema (GetSystemTimes falló)");
        return 0.0;
    }

    // Convertir FILETIME a uint64_t
    uint64_t idle   = (static_cast<uint64_t>(idleTime.dwHighDateTime) << 32) |
                       idleTime.dwLowDateTime;
    uint64_t kernel = (static_cast<uint64_t>(kernelTime.dwHighDateTime) << 32) |
                       kernelTime.dwLowDateTime;
    uint64_t user   = (static_cast<uint64_t>(userTime.dwHighDateTime) << 32) |
                       userTime.dwLowDateTime;

    // Calcular deltas desde la última muestra
    uint64_t idleDelta   = idle - prevIdleTime_;
    uint64_t kernelDelta = kernel - prevKernelTime_;
    uint64_t userDelta   = user - prevUserTime_;

    // Guardar para la próxima muestra
    prevIdleTime_   = idle;
    prevKernelTime_ = kernel;
    prevUserTime_   = user;

    // El tiempo total incluye kernel + user
    // El tiempo idle ya está incluido en kernel
    uint64_t totalDelta = kernelDelta + userDelta;

    if (totalDelta == 0)
        return 0.0;

    // CPU usage = (total - idle) / total * 100
    double cpuPercent = (1.0 - static_cast<double>(idleDelta) /
                                static_cast<double>(totalDelta)) * 100.0;

    // Clamp al rango válido
    cpuPercent = std::max(0.0, std::min(100.0, cpuPercent));

    return cpuPercent;

#else
    // Plataforma no soportada - retornar 0
    blog(LOG_WARNING, LOG_PREFIX
         "Muestreo de CPU no implementado para esta plataforma");
    return 0.0;
#endif
}

// ============================================================================
// Muestreo de GPU
// ============================================================================

double ResourceMonitor::sampleGpuUsage()
{
#ifdef _WIN32
    // -------------------------------------------------------------------------
    // Intentar obtener uso de GPU via DXGI (compatible con múltiples GPUs)
    // DXGI QueryVideoMemoryInfo proporciona info de VRAM pero no uso de GPU
    // Para uso de GPU real necesitaríamos NVAPI/NVML o ADL
    //
    // Estrategia alternativa: usar los contadores de rendimiento de OBS
    // si están disponibles, o estimar basándose en frames dropeados
    // -------------------------------------------------------------------------

    // Intentar crear DXGI Factory para enumerar adaptadores
    IDXGIFactory *factory = nullptr;
    HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory),
                                    reinterpret_cast<void **>(&factory));

    if (FAILED(hr) || !factory) {
        // DXGI no disponible
        return -1.0;
    }

    // Enumerar el primer adaptador (GPU primaria)
    IDXGIAdapter *adapter = nullptr;
    hr = factory->EnumAdapters(0, &adapter);

    if (FAILED(hr) || !adapter) {
        factory->Release();
        return -1.0;
    }

    // Obtener descripción del adaptador
    DXGI_ADAPTER_DESC desc;
    hr = adapter->GetDesc(&desc);

    if (SUCCEEDED(hr)) {
        // Guardar nombre de GPU en el snapshot (solo una vez)
        std::lock_guard<std::mutex> lock(dataMutex_);
        if (latestSnapshot_.gpuName.empty()) {
            // Convertir nombre de wchar a char
            char gpuNameBuf[256];
            size_t converted = 0;
            wcstombs_s(&converted, gpuNameBuf, sizeof(gpuNameBuf),
                       desc.Description, _TRUNCATE);
            latestSnapshot_.gpuName = gpuNameBuf;

            blog(LOG_INFO, LOG_PREFIX "GPU detectada: %s", gpuNameBuf);
            blog(LOG_INFO, LOG_PREFIX
                 "VRAM dedicada: %llu MB",
                 (unsigned long long)(desc.DedicatedVideoMemory / (1024 * 1024)));
        }
    }

    adapter->Release();
    factory->Release();

    // -------------------------------------------------------------------------
    // Estimación de uso de GPU basada en OBS
    // Si hay un output activo, los frames dropeados indican presión de GPU
    // -------------------------------------------------------------------------

    // Verificar si hay encoding activo y estimar carga
    // Esta es una aproximación; para métricas reales se necesitaría NVML/ADL
    obs_video_info ovi;
    if (obs_get_video_info(&ovi)) {
        // Si OBS reporta frames lagged, la GPU probablemente está bajo presión
        uint32_t laggedFrames = obs_get_lagged_frames();
        uint32_t totalFrames = obs_get_total_frames();

        if (totalFrames > 0) {
            double lagRatio = static_cast<double>(laggedFrames) /
                              static_cast<double>(totalFrames);

            // Estimar uso de GPU: frames lagged = presión
            // Esto es una estimación burda pero útil sin NVML/ADL
            double estimatedGpuUsage = std::min(100.0, lagRatio * 500.0 + 30.0);
            return estimatedGpuUsage;
        }
    }

    // No hay información suficiente para estimar
    return -1.0;

#else
    return -1.0;
#endif
}

// ============================================================================
// Muestreo de memoria RAM
// ============================================================================

void ResourceMonitor::sampleMemoryUsage(ResourceSnapshot &snapshot)
{
#ifdef _WIN32
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);

    if (!GlobalMemoryStatusEx(&memInfo)) {
        blog(LOG_WARNING, LOG_PREFIX
             "Error al obtener información de memoria (GlobalMemoryStatusEx falló)");
        return;
    }

    // Windows proporciona el porcentaje de uso directamente
    snapshot.memoryUsagePercent = static_cast<double>(memInfo.dwMemoryLoad);

    // Convertir bytes a megabytes
    snapshot.totalMemoryMB     = memInfo.ullTotalPhys / (1024 * 1024);
    snapshot.availableMemoryMB = memInfo.ullAvailPhys / (1024 * 1024);
    snapshot.usedMemoryMB      = snapshot.totalMemoryMB - snapshot.availableMemoryMB;

#else
    blog(LOG_WARNING, LOG_PREFIX
         "Muestreo de memoria no implementado para esta plataforma");
#endif
}

// ============================================================================
// Muestreo de uso de red
// ============================================================================

void ResourceMonitor::sampleNetworkUsage(ResourceSnapshot &snapshot)
{
#ifdef _WIN32
    // Usar GetIfTable para obtener estadísticas de interfaz de red
    DWORD bufferSize = 0;
    GetIfTable(nullptr, &bufferSize, FALSE);

    if (bufferSize == 0) {
        blog(LOG_WARNING, LOG_PREFIX
             "No se pudo obtener información de interfaces de red");
        return;
    }

    std::vector<BYTE> buffer(bufferSize);
    PMIB_IFTABLE ifTable = reinterpret_cast<PMIB_IFTABLE>(buffer.data());

    if (GetIfTable(ifTable, &bufferSize, FALSE) != NO_ERROR) {
        blog(LOG_WARNING, LOG_PREFIX
             "Error al obtener tabla de interfaces de red");
        return;
    }

    // Sumar bytes enviados y recibidos de todas las interfaces activas
    uint64_t totalSent = 0;
    uint64_t totalReceived = 0;

    for (DWORD i = 0; i < ifTable->dwNumEntries; ++i) {
        const MIB_IFROW &row = ifTable->table[i];

        // Solo considerar interfaces activas y relevantes
        // (excluir loopback y interfaces inactivas)
        if (row.dwOperStatus == IF_OPER_STATUS_OPERATIONAL &&
            row.dwType != IF_TYPE_SOFTWARE_LOOPBACK) {
            totalSent     += row.dwOutOctets;
            totalReceived += row.dwInOctets;
        }
    }

    snapshot.bytesSentTotal     = totalSent;
    snapshot.bytesReceivedTotal = totalReceived;

    // Calcular velocidad actual (delta desde la última muestra)
    auto now = std::chrono::steady_clock::now();
    double elapsedSec =
        std::chrono::duration<double>(now - prevNetworkSample_).count();

    if (elapsedSec > 0.0 && prevBytesSent_ > 0) {
        uint64_t sentDelta     = totalSent - prevBytesSent_;
        uint64_t receivedDelta = totalReceived - prevBytesReceived_;

        // Convertir bytes/segundo a Mbps
        // Manejar desbordamiento de contadores de 32 bits
        if (totalSent >= prevBytesSent_) {
            snapshot.networkUploadMbps =
                (static_cast<double>(sentDelta) * 8.0) /
                (elapsedSec * 1000000.0);
        }

        if (totalReceived >= prevBytesReceived_) {
            snapshot.networkDownloadMbps =
                (static_cast<double>(receivedDelta) * 8.0) /
                (elapsedSec * 1000000.0);
        }
    }

    // Guardar para la próxima muestra
    prevBytesSent_      = totalSent;
    prevBytesReceived_  = totalReceived;
    prevNetworkSample_  = now;

#else
    blog(LOG_WARNING, LOG_PREFIX
         "Muestreo de red no implementado para esta plataforma");
#endif
}

// ============================================================================
// Evaluación de advertencias
// ============================================================================

void ResourceMonitor::evaluateWarnings(const ResourceSnapshot &snapshot)
{
    auto now = std::chrono::steady_clock::now();

    // Limpiar advertencias anteriores
    {
        std::lock_guard<std::mutex> lock(dataMutex_);
        activeWarnings_.clear();
    }

    // ---- Evaluación de CPU ----
    if (snapshot.cpuUsagePercent > CPU_WARNING_CRITICAL) {
        // Verificar cooldown para no spamear advertencias
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - lastCpuWarning_).count();

        if (elapsed >= WARNING_COOLDOWN_SEC) {
            emitWarning(ResourceWarningLevel::Critical, "CPU",
                        snapshot.cpuUsagePercent,
                        "¡ALERTA CRÍTICA! El uso de CPU está al " +
                        std::to_string(static_cast<int>(snapshot.cpuUsagePercent)) +
                        "%. El rendimiento del streaming puede verse afectado gravemente. "
                        "Considere usar codificación por hardware (NVENC/AMF/QSV) "
                        "o reducir la calidad de transmisión.");
            lastCpuWarning_ = now;
        }
    } else if (snapshot.cpuUsagePercent > CPU_WARNING_HIGH) {
        emitWarning(ResourceWarningLevel::High, "CPU",
                    snapshot.cpuUsagePercent,
                    "El uso de CPU está alto (" +
                    std::to_string(static_cast<int>(snapshot.cpuUsagePercent)) +
                    "%). Se recomienda precaución.");
    } else if (snapshot.cpuUsagePercent > CPU_WARNING_ELEVATED) {
        emitWarning(ResourceWarningLevel::Elevated, "CPU",
                    snapshot.cpuUsagePercent,
                    "El uso de CPU está elevado (" +
                    std::to_string(static_cast<int>(snapshot.cpuUsagePercent)) +
                    "%).");
    }

    // ---- Evaluación de RAM ----
    if (snapshot.memoryUsagePercent > MEM_WARNING_CRITICAL) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - lastMemoryWarning_).count();

        if (elapsed >= WARNING_COOLDOWN_SEC) {
            emitWarning(ResourceWarningLevel::Critical, "RAM",
                        snapshot.memoryUsagePercent,
                        "¡ALERTA CRÍTICA! El uso de memoria RAM está al " +
                        std::to_string(static_cast<int>(snapshot.memoryUsagePercent)) +
                        "% (" + std::to_string(snapshot.usedMemoryMB) + " de " +
                        std::to_string(snapshot.totalMemoryMB) + " MB). "
                        "El sistema puede volverse inestable. "
                        "Cierre aplicaciones innecesarias inmediatamente.");
            lastMemoryWarning_ = now;
        }
    } else if (snapshot.memoryUsagePercent > MEM_WARNING_HIGH) {
        emitWarning(ResourceWarningLevel::High, "RAM",
                    snapshot.memoryUsagePercent,
                    "El uso de memoria está alto (" +
                    std::to_string(static_cast<int>(snapshot.memoryUsagePercent)) +
                    "%). Disponible: " +
                    std::to_string(snapshot.availableMemoryMB) + " MB.");
    } else if (snapshot.memoryUsagePercent > MEM_WARNING_ELEVATED) {
        emitWarning(ResourceWarningLevel::Elevated, "RAM",
                    snapshot.memoryUsagePercent,
                    "El uso de memoria está elevado (" +
                    std::to_string(static_cast<int>(snapshot.memoryUsagePercent)) +
                    "%).");
    }

    // ---- Evaluación de GPU ----
    if (snapshot.gpuAvailable) {
        if (snapshot.gpuUsagePercent > GPU_WARNING_CRITICAL) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                now - lastGpuWarning_).count();

            if (elapsed >= WARNING_COOLDOWN_SEC) {
                emitWarning(ResourceWarningLevel::Critical, "GPU",
                            snapshot.gpuUsagePercent,
                            "¡ALERTA! El uso de GPU está al " +
                            std::to_string(static_cast<int>(snapshot.gpuUsagePercent)) +
                            "%. Si usa codificación por hardware (NVENC/AMF), "
                            "considere cambiar a codificación por software (x264) "
                            "o reducir la calidad de video.");
                lastGpuWarning_ = now;
            }
        } else if (snapshot.gpuUsagePercent > GPU_WARNING_HIGH) {
            emitWarning(ResourceWarningLevel::High, "GPU",
                        snapshot.gpuUsagePercent,
                        "El uso de GPU está alto (" +
                        std::to_string(static_cast<int>(snapshot.gpuUsagePercent)) +
                        "%).");
        } else if (snapshot.gpuUsagePercent > GPU_WARNING_ELEVATED) {
            emitWarning(ResourceWarningLevel::Elevated, "GPU",
                        snapshot.gpuUsagePercent,
                        "El uso de GPU está elevado (" +
                        std::to_string(static_cast<int>(snapshot.gpuUsagePercent)) +
                        "%).");
        }
    }
}

void ResourceMonitor::emitWarning(ResourceWarningLevel level,
                                    const std::string &resource,
                                    double usage,
                                    const std::string &message)
{
    ResourceWarning warning;
    warning.level        = level;
    warning.resourceName = resource;
    warning.currentUsage = usage;
    warning.message      = message;
    warning.timestamp    = std::chrono::steady_clock::now();

    // Agregar a la lista de advertencias activas
    {
        std::lock_guard<std::mutex> lock(dataMutex_);
        activeWarnings_.push_back(warning);
    }

    // Log según nivel de severidad
    switch (level) {
    case ResourceWarningLevel::Critical:
        blog(LOG_ERROR, LOG_PREFIX "⚠ %s: %s", resource.c_str(), message.c_str());
        break;
    case ResourceWarningLevel::High:
        blog(LOG_WARNING, LOG_PREFIX "%s: %s", resource.c_str(), message.c_str());
        break;
    case ResourceWarningLevel::Elevated:
        blog(LOG_INFO, LOG_PREFIX "%s: %s", resource.c_str(), message.c_str());
        break;
    default:
        break;
    }

    // Invocar callback si está registrado
    WarningCallback cb;
    {
        std::lock_guard<std::mutex> lock(dataMutex_);
        cb = warningCallback_;
    }
    if (cb) {
        cb(warning);
    }
}
