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
    std::string errorMessage;
    std::string warningMessage;

    static ValidationResult success() {
        ValidationResult r;
        r.valid = true;
        return r;
    }

    static ValidationResult failure(const std::string &msg) {
        ValidationResult r;
        r.valid = false;
        r.errorMessage = msg;
        return r;
    }

    static ValidationResult warning(const std::string &warn) {
        ValidationResult r;
        r.valid = true;
        r.warningMessage = warn;
        return r;
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
    static ValidationResult validateUrl(PlatformType type, const std::string &url);
    static ValidationResult validateStreamKey(PlatformType type, const std::string &key);
    static ValidationResult validateConfiguration(PlatformType type, const std::string &url, const std::string &key);
    static ConnectionTestResult testConnection(const std::string &url, const std::string &key, int timeoutMs = 5000);
    static std::future<ConnectionTestResult> testConnectionAsync(const std::string &url, const std::string &key, int timeoutMs = 5000);
};
