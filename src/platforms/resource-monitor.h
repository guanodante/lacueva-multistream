/**
 * @file resource-monitor.h
 * @brief Monitor de recursos del sistema (CPU, GPU, RAM, red).
 *
 * Proyecto: La Cueva del Dolor - OBS MultiStream Plugin
 * @copyright 2026 La Cueva del Dolor
 */

#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <obs-module.h>
#include <string>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>

enum class EncoderType {
    Software_x264 = 0,
    Hardware_NVENC,
    Hardware_AMF,
    Hardware_QSV,
    Unknown
};

const char *encoderTypeToString(EncoderType type);
const char *encoderTypeToObsId(EncoderType type);

enum class ResourceWarningLevel {
    Normal = 0,
    Elevated,
    High,
    Critical
};

static constexpr int WARNING_COOLDOWN_SEC = 30;

struct ResourceWarning {
    ResourceWarningLevel level = ResourceWarningLevel::Normal;
    std::string resourceName;
    std::string message;
    double currentUsage = 0.0;
    double thresholdValue = 0.0;
    std::chrono::steady_clock::time_point timestamp;
};

struct ResourceSnapshot {
    double cpuUsagePercent = 0.0;
    int cpuCoreCount = 0;
    double gpuUsagePercent = -1.0;
    std::string gpuName;
    bool gpuAvailable = false;
    double memoryUsagePercent = 0.0;
    uint64_t totalMemoryMB = 0;
    uint64_t availableMemoryMB = 0;
    uint64_t usedMemoryMB = 0;
    double networkUploadMbps = 0.0;
    double networkDownloadMbps = 0.0;
    uint64_t bytesSentTotal = 0;
    uint64_t bytesReceivedTotal = 0;
    EncoderType recommendedEncoder = EncoderType::Software_x264;
    std::string recommendationReason;
    std::chrono::steady_clock::time_point timestamp;
};

class ResourceMonitor {
public:
    using WarningCallback = std::function<void(const ResourceWarning &)>;
    using SnapshotCallback = std::function<void(const ResourceSnapshot &)>;

    static ResourceMonitor &instance() {
        static ResourceMonitor s_inst;
        return s_inst;
    }

    ResourceMonitor();
    ~ResourceMonitor();

    ResourceMonitor(const ResourceMonitor &) = delete;
    ResourceMonitor &operator=(const ResourceMonitor &) = delete;

    void start();
    void stop();
    bool isRunning() const;
    void setSampleInterval(int intervalMs);

    double getCpuUsage() const;
    double getGpuUsage() const;
    double getMemoryUsage() const;
    double getRamUsage() const { return getMemoryUsage(); }
    double getNetworkUsage() const;
    double getNetworkUploadMbps() const { return getNetworkUsage(); }

    ResourceSnapshot getLatestSnapshot() const;
    ResourceSnapshot getSnapshot() const { return getLatestSnapshot(); }

    EncoderType recommendEncoderType() const;
    EncoderType recommendEncoder() const { return recommendEncoderType(); }
    std::string getRecommendationReason() const;

    std::vector<ResourceWarning> getActiveWarnings() const;
    ResourceWarningLevel getHighestWarningLevel() const;

    void setWarningCallback(WarningCallback callback);
    void setSnapshotCallback(SnapshotCallback callback);

    static std::vector<EncoderType> detectAvailableEncoders();
    static bool isEncoderAvailable(EncoderType type);

private:
    void monitorThreadFunc();
    double sampleCpuUsage();
    double sampleGpuUsage();
    void sampleMemoryUsage(ResourceSnapshot &snapshot);
    void sampleNetworkUsage(ResourceSnapshot &snapshot);
    void evaluateWarnings(const ResourceSnapshot &snapshot);
    void emitWarning(ResourceWarningLevel level, const std::string &resource, double usage, const std::string &message);

    int sampleIntervalMs_ = 2000;
    std::atomic<bool> running_{false};
    std::thread monitorThread_;
    mutable std::mutex dataMutex_;

    ResourceSnapshot latestSnapshot_;
    std::vector<ResourceWarning> activeWarnings_;
    WarningCallback warningCallback_;
    SnapshotCallback snapshotCallback_;

    uint64_t prevIdleTime_ = 0;
    uint64_t prevKernelTime_ = 0;
    uint64_t prevUserTime_ = 0;

    std::chrono::steady_clock::time_point prevNetworkSample_;
    uint64_t prevBytesSent_ = 0;
    uint64_t prevBytesReceived_ = 0;

    std::chrono::steady_clock::time_point lastCpuWarning_;
    std::chrono::steady_clock::time_point lastMemoryWarning_;
    std::chrono::steady_clock::time_point lastGpuWarning_;
};

namespace CuevaMultiStream {
    using ResourceMonitor = ::ResourceMonitor;
    using EncoderType = ::EncoderType;
    using ResourceSnapshot = ::ResourceSnapshot;
    using ResourceWarning = ::ResourceWarning;
    using ResourceWarningLevel = ::ResourceWarningLevel;
}
