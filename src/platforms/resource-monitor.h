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

struct ResourceSnapshot {
    double cpuUsagePercent = 0.0;
    int cpuCoreCount = 0;
    double gpuUsagePercent = 0.0;
    std::string gpuName;
    double ramUsagePercent = 0.0;
    uint64_t ramUsedBytes = 0;
    uint64_t ramTotalBytes = 0;
    double networkUploadBps = 0.0;
    double networkDownloadBps = 0.0;
    EncoderType recommendedEncoder = EncoderType::Software_x264;
    std::chrono::system_clock::time_point timestamp;
};

class ResourceMonitor {
public:
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

    ResourceSnapshot getSnapshot();
    double getCpuUsage();
    double getGpuUsage();
    double getRamUsage();
    double getNetworkUploadMbps();

    EncoderType recommendEncoder();

    void setWarningCallback(std::function<void(const std::string &)> cb);

private:
    void monitorLoop();

    std::atomic<bool> running_{false};
    std::thread workerThread_;
    std::mutex snapshotMutex_;
    ResourceSnapshot latestSnapshot_;
    std::function<void(const std::string &)> warningCallback_;
};

namespace CuevaMultiStream {
    using ResourceMonitor = ::ResourceMonitor;
    using EncoderType = ::EncoderType;
    using ResourceSnapshot = ::ResourceSnapshot;
}
