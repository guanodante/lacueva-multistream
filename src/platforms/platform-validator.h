/**
 * @file platform-validator.h
 * @brief Validador de URLs RTMP y claves de transmisión para cada plataforma.
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
#include <vector>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <future>

enum class PlatformType {
    YouTube = 0,
    Facebook,
    TikTok,
    Instagram,
    Twitter,
    Custom,
    Count
};

const char *platformTypeToString(PlatformType type);
const char *platformTypeToId(PlatformType type);

struct ValidationResult {
    bool valid = false;
    int errorCode = 0;
    std::string errorMessage;
    std::string warningMessage;

    static constexpr int OK = 0;
    static constexpr int ERR_EMPTY_URL = 1001;
    static constexpr int ERR_INVALID_PROTOCOL = 1002;
    static constexpr int ERR_INVALID_FORMAT = 1003;
    static constexpr int ERR_RTMPS_REQUIRED = 1004;
    static constexpr int ERR_EMPTY_KEY = 1005;
    static constexpr int ERR_KEY_FORMAT = 1006;
    static constexpr int ERR_CONNECTION_FAILED = 1007;
    static constexpr int ERR_TIMEOUT = 1008;

    static ValidationResult success() {
        ValidationResult r;
        r.valid = true;
        r.errorCode = OK;
        return r;
    }

    static ValidationResult successWithWarning(const std::string &warn) {
        ValidationResult r;
        r.valid = true;
        r.errorCode = OK;
        r.warningMessage = warn;
        return r;
    }

    static ValidationResult warning(const std::string &warn) {
        return successWithWarning(warn);
    }

    static ValidationResult error(int code, const std::string &msg) {
        ValidationResult r;
        r.valid = false;
        r.errorCode = code;
        r.errorMessage = msg;
        return r;
    }

    static ValidationResult failure(const std::string &msg) {
        return error(ERR_INVALID_FORMAT, msg);
    }
};

struct ConnectionTestResult {
    bool connected = false;
    double latencyMs = 0.0;
    std::string errorMessage;
    int errorCode = 0;

    static ConnectionTestResult success(double latency) {
        ConnectionTestResult r;
        r.connected = true;
        r.latencyMs = latency;
        return r;
    }

    static ConnectionTestResult successWithWarning(double latency, const std::string &warn) {
        ConnectionTestResult r;
        r.connected = true;
        r.latencyMs = latency;
        r.errorMessage = warn;
        return r;
    }

    static ConnectionTestResult failure(int code, const std::string &msg) {
        ConnectionTestResult r;
        r.connected = false;
        r.errorCode = code;
        r.errorMessage = msg;
        return r;
    }
};

class PlatformValidator {
public:
    PlatformValidator();
    ~PlatformValidator();

    PlatformValidator(const PlatformValidator &) = delete;
    PlatformValidator &operator=(const PlatformValidator &) = delete;

    static bool urlStartsWith(const std::string &url, const std::string &prefix);
    static bool urlContains(const std::string &url, const std::string &substring);
    static bool isAlphanumericDash(const std::string &str);

    static ValidationResult validateRtmpUrl(PlatformType type, const std::string &url);
    static ValidationResult validateUrl(PlatformType type, const std::string &url) {
        return validateRtmpUrl(type, url);
    }

    static ValidationResult validateYouTubeUrl(const std::string &url);
    static ValidationResult validateFacebookUrl(const std::string &url);
    static ValidationResult validateTikTokUrl(const std::string &url);
    static ValidationResult validateInstagramUrl(const std::string &url);
    static ValidationResult validateTwitterUrl(const std::string &url);
    static ValidationResult validateCustomUrl(const std::string &url);

    static ValidationResult validateStreamKey(PlatformType type, const std::string &key);
    static ValidationResult validateYouTubeKey(const std::string &key);
    static ValidationResult validateFacebookKey(const std::string &key);
    static ValidationResult validateTikTokKey(const std::string &key);
    static ValidationResult validateInstagramKey(const std::string &key);
    static ValidationResult validateTwitterKey(const std::string &key);
    static ValidationResult validateCustomKey(const std::string &key);

    static ValidationResult validateConfiguration(PlatformType type, const std::string &url, const std::string &key);

    ConnectionTestResult testConnection(const std::string &url, const std::string &key, int timeoutMs = 5000);
    std::future<ConnectionTestResult> testConnectionAsync(const std::string &url, const std::string &key, int timeoutMs = 5000);

    void cancelPendingTests();
    bool hasActiveTests() const;
    void cleanupFinishedTests();

private:
    std::atomic<bool> cancelRequested_{false};
    mutable std::mutex testsMutex_;
    std::vector<std::future<void>> pendingTests_;
};
