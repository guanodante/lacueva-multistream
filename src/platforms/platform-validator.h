/**
 * @file platform-validator.h
 * @brief Validador de URLs RTMP y claves de transmisión para cada plataforma.
 *
 * Verifica el formato de las URLs RTMP, el protocolo (rtmp:// vs rtmps://),
 * patrones conocidos de servidores, y realiza pruebas de conexión no bloqueantes.
 *
 * Proyecto: La Cueva del Dolor - OBS MultiStream Plugin
 * @copyright 2026 La Cueva del Dolor
 */

#pragma once

#include <obs-module.h>
#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <future>

// ============================================================================
// Tipos de plataforma soportados
// ============================================================================

/**
 * @brief Enumeración de las plataformas de streaming soportadas.
 */
enum class PlatformType {
    YouTube = 0,
    Facebook,
    TikTok,
    Instagram,
    Twitter, // X (Twitter)
    Custom,  // Servidor RTMP personalizado
    Count    // Centinela para iterar
};

/**
 * @brief Convierte PlatformType a su nombre legible en español.
 */
const char *platformTypeToString(PlatformType type);

/**
 * @brief Convierte PlatformType a un identificador corto (ej: "youtube", "facebook").
 */
const char *platformTypeToId(PlatformType type);

// ============================================================================
// Resultado de validación
// ============================================================================

/**
 * @brief Estructura que contiene el resultado de una validación.
 */
struct ValidationResult {
    bool valid = false;               ///< ¿La validación fue exitosa?
    std::string errorMessage;         ///< Mensaje de error en español (vacío si válido)
    std::string warningMessage;       ///< Advertencias opcionales en español
    int errorCode = 0;                ///< Código de error numérico (0 = sin error)

    /// Códigos de error comunes
    static constexpr int ERR_NONE = 0;
    static constexpr int ERR_EMPTY_URL = 1;
    static constexpr int ERR_INVALID_PROTOCOL = 2;
    static constexpr int ERR_INVALID_FORMAT = 3;
    static constexpr int ERR_UNKNOWN_SERVER = 4;
    static constexpr int ERR_EMPTY_KEY = 5;
    static constexpr int ERR_KEY_FORMAT = 6;
    static constexpr int ERR_CONNECTION_FAILED = 7;
    static constexpr int ERR_TIMEOUT = 8;
    static constexpr int ERR_RTMPS_REQUIRED = 9;

    /**
     * @brief Crea un resultado exitoso.
     */
    static ValidationResult success() {
        ValidationResult r;
        r.valid = true;
        return r;
    }

    /**
     * @brief Crea un resultado de error con mensaje y código.
     */
    static ValidationResult error(int code, const std::string &msg) {
        ValidationResult r;
        r.valid = false;
        r.errorCode = code;
        r.errorMessage = msg;
        return r;
    }

    /**
     * @brief Crea un resultado exitoso con advertencia.
     */
    static ValidationResult successWithWarning(const std::string &warning) {
        ValidationResult r;
        r.valid = true;
        r.warningMessage = warning;
        return r;
    }
};

// ============================================================================
// Resultado de prueba de conexión
// ============================================================================

/**
 * @brief Resultado de una prueba de conexión RTMP.
 */
struct ConnectionTestResult {
    bool connected = false;           ///< ¿Se logró conectar?
    double latencyMs = 0.0;          ///< Latencia medida en milisegundos
    std::string errorMessage;         ///< Mensaje de error si falló
    int errorCode = 0;                ///< Código de error

    static ConnectionTestResult success(double latency) {
        ConnectionTestResult r;
        r.connected = true;
        r.latencyMs = latency;
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

// ============================================================================
// Clase PlatformValidator
// ============================================================================

/**
 * @brief Validador de URLs RTMP y claves de streaming para las plataformas soportadas.
 *
 * Provee validación sincrónica de formato y una prueba de conexión asincrónica
 * que ejecuta un handshake RTMP básico en un hilo de trabajo.
 */
class PlatformValidator {
public:
    PlatformValidator();
    ~PlatformValidator();

    // --- Validación de formato (sincrónica) ---

    /**
     * @brief Valida la URL RTMP para una plataforma específica.
     *
     * Verifica:
     *  - Que no esté vacía
     *  - Que use el protocolo correcto (rtmp:// o rtmps://)
     *  - Que coincida con patrones conocidos del servidor
     *
     * @param type     Plataforma de destino
     * @param url      URL RTMP a validar
     * @return         Resultado de la validación con mensaje en español
     */
    ValidationResult validateRtmpUrl(PlatformType type, const std::string &url);

    /**
     * @brief Valida la clave de transmisión para una plataforma específica.
     *
     * Realiza validación básica del formato según la plataforma:
     *  - Longitud mínima/máxima
     *  - Caracteres válidos
     *  - Prefijos conocidos
     *
     * @param type     Plataforma de destino
     * @param key      Clave de transmisión a validar
     * @return         Resultado de la validación con mensaje en español
     */
    ValidationResult validateStreamKey(PlatformType type, const std::string &key);

    // --- Prueba de conexión (asincrónica) ---

    /// Tipo de callback para resultado de conexión
    using ConnectionCallback = std::function<void(const ConnectionTestResult &)>;

    /**
     * @brief Intenta un handshake RTMP básico para verificar conectividad.
     *
     * Ejecuta la prueba en un hilo de trabajo para no bloquear el hilo principal.
     * El resultado se entrega a través del callback proporcionado.
     *
     * @param url       URL RTMP del servidor
     * @param key       Clave de transmisión
     * @param callback  Función llamada con el resultado de la prueba
     */
    void testConnection(const std::string &url, const std::string &key,
                        ConnectionCallback callback);

    /**
     * @brief Prueba de conexión sincrónica (bloquea hasta completarse o timeout).
     *
     * @param url       URL RTMP del servidor
     * @param key       Clave de transmisión
     * @param timeoutMs Tiempo máximo de espera en milisegundos
     * @return          Resultado de la prueba de conexión
     */
    ConnectionTestResult testConnectionSync(const std::string &url,
                                             const std::string &key,
                                             int timeoutMs = 5000);

    /**
     * @brief Cancela todas las pruebas de conexión en curso.
     */
    void cancelPendingTests();

    /**
     * @brief Verifica si hay pruebas de conexión en ejecución.
     */
    bool hasActiveTests() const;

private:
    // --- Validación interna por plataforma ---
    ValidationResult validateYouTubeUrl(const std::string &url);
    ValidationResult validateFacebookUrl(const std::string &url);
    ValidationResult validateTikTokUrl(const std::string &url);
    ValidationResult validateInstagramUrl(const std::string &url);
    ValidationResult validateTwitterUrl(const std::string &url);
    ValidationResult validateCustomUrl(const std::string &url);

    ValidationResult validateYouTubeKey(const std::string &key);
    ValidationResult validateFacebookKey(const std::string &key);
    ValidationResult validateTikTokKey(const std::string &key);
    ValidationResult validateInstagramKey(const std::string &key);
    ValidationResult validateTwitterKey(const std::string &key);
    ValidationResult validateCustomKey(const std::string &key);

    // --- Utilidades ---
    bool urlStartsWith(const std::string &url, const std::string &prefix);
    bool urlContains(const std::string &url, const std::string &substring);
    bool isAlphanumericDash(const std::string &str);

    // --- Control de hilos ---
    std::atomic<bool> cancelRequested_{false};
    std::vector<std::future<void>> pendingTests_;
    mutable std::mutex testsMutex_;

    /**
     * @brief Limpia las pruebas completadas del vector de pendientes.
     */
    void cleanupFinishedTests();
};
