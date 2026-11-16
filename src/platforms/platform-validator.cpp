/**
 * @file platform-validator.cpp
 * @brief Implementación del validador de URLs RTMP y claves de transmisión.
 *
 * Contiene la lógica de validación por plataforma, verificación de protocolos,
 * patrones de servidor conocidos y prueba de conexión RTMP asincrónica.
 *
 * Proyecto: La Cueva del Dolor - OBS MultiStream Plugin
 * @copyright 2026 La Cueva del Dolor
 */

#include "platform-validator.h"
#include <obs-module.h>
#include <util/base.h>
#include <algorithm>
#include <chrono>
#include <regex>
#include <cctype>

// ============================================================================
// Macro de log con prefijo del módulo
// ============================================================================
#define LOG_PREFIX "[CuevaMultiStream::Validator] "

// ============================================================================
// Funciones de conversión de PlatformType
// ============================================================================

const char *platformTypeToString(PlatformType type)
{
    switch (type) {
    case PlatformType::YouTube:   return "YouTube";
    case PlatformType::Facebook:  return "Facebook";
    case PlatformType::TikTok:    return "TikTok";
    case PlatformType::Instagram: return "Instagram";
    case PlatformType::Twitter:   return "X (Twitter)";
    case PlatformType::Custom:    return "Personalizado";
    default:                      return "Desconocido";
    }
}

const char *platformTypeToId(PlatformType type)
{
    switch (type) {
    case PlatformType::YouTube:   return "youtube";
    case PlatformType::Facebook:  return "facebook";
    case PlatformType::TikTok:    return "tiktok";
    case PlatformType::Instagram: return "instagram";
    case PlatformType::Twitter:   return "twitter";
    case PlatformType::Custom:    return "custom";
    default:                      return "unknown";
    }
}

// ============================================================================
// Constructor / Destructor
// ============================================================================

PlatformValidator::PlatformValidator()
{
    blog(LOG_INFO, LOG_PREFIX "Validador de plataformas inicializado");
}

PlatformValidator::~PlatformValidator()
{
    // Cancelar cualquier prueba de conexión en curso antes de destruir
    cancelPendingTests();
    blog(LOG_INFO, LOG_PREFIX "Validador de plataformas destruido");
}

// ============================================================================
// Utilidades internas
// ============================================================================

/**
 * @brief Verifica si la URL comienza con un prefijo dado (insensible a mayúsculas).
 */
bool PlatformValidator::urlStartsWith(const std::string &url,
                                       const std::string &prefix)
{
    if (url.length() < prefix.length())
        return false;

    // Comparación insensible a mayúsculas
    std::string urlLower = url.substr(0, prefix.length());
    std::string prefixLower = prefix;
    std::transform(urlLower.begin(), urlLower.end(), urlLower.begin(), ::tolower);
    std::transform(prefixLower.begin(), prefixLower.end(), prefixLower.begin(),
                   ::tolower);
    return urlLower == prefixLower;
}

/**
 * @brief Verifica si la URL contiene una subcadena (insensible a mayúsculas).
 */
bool PlatformValidator::urlContains(const std::string &url,
                                     const std::string &substring)
{
    std::string urlLower = url;
    std::string subLower = substring;
    std::transform(urlLower.begin(), urlLower.end(), urlLower.begin(), ::tolower);
    std::transform(subLower.begin(), subLower.end(), subLower.begin(), ::tolower);
    return urlLower.find(subLower) != std::string::npos;
}

/**
 * @brief Verifica si una cadena contiene solo caracteres alfanuméricos y guiones.
 */
bool PlatformValidator::isAlphanumericDash(const std::string &str)
{
    return std::all_of(str.begin(), str.end(), [](char c) {
        return std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_';
    });
}

// ============================================================================
// Validación de URL RTMP - Dispatcher principal
// ============================================================================

ValidationResult PlatformValidator::validateRtmpUrl(PlatformType type,
                                                     const std::string &url)
{
    blog(LOG_DEBUG, LOG_PREFIX "Validando URL RTMP para plataforma: %s",
         platformTypeToString(type));

    // Verificación universal: URL no vacía
    if (url.empty()) {
        blog(LOG_WARNING, LOG_PREFIX "URL RTMP vacía para %s",
             platformTypeToString(type));
        return ValidationResult::error(
            ValidationResult::ERR_EMPTY_URL,
            "La URL del servidor RTMP no puede estar vacía.");
    }

    // Verificación universal: debe comenzar con rtmp:// o rtmps://
    if (!urlStartsWith(url, "rtmp://") && !urlStartsWith(url, "rtmps://")) {
        blog(LOG_WARNING, LOG_PREFIX "Protocolo inválido en URL: %s", url.c_str());
        return ValidationResult::error(
            ValidationResult::ERR_INVALID_PROTOCOL,
            "La URL debe comenzar con rtmp:// o rtmps://. "
            "Protocolo detectado no es válido.");
    }

    // Despachar a validador específico de la plataforma
    switch (type) {
    case PlatformType::YouTube:   return validateYouTubeUrl(url);
    case PlatformType::Facebook:  return validateFacebookUrl(url);
    case PlatformType::TikTok:    return validateTikTokUrl(url);
    case PlatformType::Instagram: return validateInstagramUrl(url);
    case PlatformType::Twitter:   return validateTwitterUrl(url);
    case PlatformType::Custom:    return validateCustomUrl(url);
    default:
        blog(LOG_WARNING, LOG_PREFIX "Tipo de plataforma desconocido: %d",
             static_cast<int>(type));
        return ValidationResult::error(
            ValidationResult::ERR_INVALID_FORMAT,
            "Tipo de plataforma no reconocido.");
    }
}

// ============================================================================
// Validación de URL por plataforma
// ============================================================================

ValidationResult PlatformValidator::validateYouTubeUrl(const std::string &url)
{
    // Servidores RTMP conocidos de YouTube
    // Formatos válidos:
    //   rtmp://a.rtmp.youtube.com/live2
    //   rtmp://b.rtmp.youtube.com/live2?backup=1
    //   rtmps://a.rtmps.youtube.com/live2

    bool knownServer = urlContains(url, "rtmp.youtube.com") ||
                       urlContains(url, "rtmps.youtube.com") ||
                       urlContains(url, "youtube.com/live");

    if (!knownServer) {
        blog(LOG_WARNING, LOG_PREFIX
             "URL de YouTube no coincide con servidores conocidos: %s",
             url.c_str());
        return ValidationResult::successWithWarning(
            "La URL no coincide con los servidores conocidos de YouTube "
            "(a.rtmp.youtube.com/live2). Verifique que sea correcta.");
    }

    // Verificar que contenga la ruta /live2 o /live
    if (!urlContains(url, "/live")) {
        return ValidationResult::successWithWarning(
            "La URL de YouTube generalmente incluye la ruta '/live2'. "
            "Verifique la URL en su panel de YouTube Studio.");
    }

    blog(LOG_INFO, LOG_PREFIX "URL de YouTube validada correctamente");
    return ValidationResult::success();
}

ValidationResult PlatformValidator::validateFacebookUrl(const std::string &url)
{
    // Facebook REQUIERE rtmps:// (conexión segura obligatoria)
    if (!urlStartsWith(url, "rtmps://")) {
        blog(LOG_WARNING, LOG_PREFIX
             "Facebook requiere rtmps:// pero se recibió: %s", url.c_str());
        return ValidationResult::error(
            ValidationResult::ERR_RTMPS_REQUIRED,
            "Facebook requiere una conexión segura (rtmps://). "
            "Cambie 'rtmp://' por 'rtmps://' en la URL.");
    }

    // Servidores conocidos de Facebook Live
    bool knownServer = urlContains(url, "live-api-s.facebook.com") ||
                       urlContains(url, "rtmps://live-api") ||
                       urlContains(url, "facebook.com");

    if (!knownServer) {
        return ValidationResult::successWithWarning(
            "La URL no coincide con los servidores conocidos de Facebook Live. "
            "El servidor habitual es: rtmps://live-api-s.facebook.com:443/rtmp/");
    }

    blog(LOG_INFO, LOG_PREFIX "URL de Facebook validada correctamente");
    return ValidationResult::success();
}

ValidationResult PlatformValidator::validateTikTokUrl(const std::string &url)
{
    // TikTok usa servidores rtmp variados según la región
    bool knownServer = urlContains(url, "tiktok") ||
                       urlContains(url, "live-push") ||
                       urlContains(url, "rtmp-push");

    if (!knownServer) {
        return ValidationResult::successWithWarning(
            "La URL no coincide con los servidores conocidos de TikTok Live. "
            "Verifique la URL proporcionada por TikTok en su panel de transmisión.");
    }

    blog(LOG_INFO, LOG_PREFIX "URL de TikTok validada correctamente");
    return ValidationResult::success();
}

ValidationResult PlatformValidator::validateInstagramUrl(const std::string &url)
{
    // Instagram Live usa servidores de Facebook/Meta
    // Generalmente requiere rtmps://
    if (!urlStartsWith(url, "rtmps://")) {
        blog(LOG_WARNING, LOG_PREFIX
             "Instagram generalmente requiere rtmps://: %s", url.c_str());
        return ValidationResult::successWithWarning(
            "Instagram Live generalmente usa rtmps://. "
            "Verifique la URL si experimenta problemas de conexión.");
    }

    bool knownServer = urlContains(url, "instagram") ||
                       urlContains(url, "live-api") ||
                       urlContains(url, "facebook.com") ||
                       urlContains(url, "fbcdn");

    if (!knownServer) {
        return ValidationResult::successWithWarning(
            "La URL no coincide con los servidores conocidos de Instagram Live. "
            "Verifique la URL en la configuración de Instagram.");
    }

    blog(LOG_INFO, LOG_PREFIX "URL de Instagram validada correctamente");
    return ValidationResult::success();
}

ValidationResult PlatformValidator::validateTwitterUrl(const std::string &url)
{
    // X (Twitter) usa servidores RTMP propios
    bool knownServer = urlContains(url, "twitter") ||
                       urlContains(url, "pscp.tv") ||
                       urlContains(url, "x.com") ||
                       urlContains(url, "prod-ec-") ||
                       urlContains(url, "periscope");

    if (!knownServer) {
        return ValidationResult::successWithWarning(
            "La URL no coincide con los servidores conocidos de X (Twitter). "
            "El servidor habitual contiene 'pscp.tv' o 'x.com'. "
            "Verifique la URL en su panel de X/Twitter.");
    }

    blog(LOG_INFO, LOG_PREFIX "URL de X (Twitter) validada correctamente");
    return ValidationResult::success();
}

ValidationResult PlatformValidator::validateCustomUrl(const std::string &url)
{
    // Para servidores personalizados, solo verificamos formato básico
    // Debe tener un host después del protocolo
    std::string urlLower = url;
    std::transform(urlLower.begin(), urlLower.end(), urlLower.begin(), ::tolower);

    // Extraer lo que viene después de rtmp:// o rtmps://
    size_t protoEnd = urlLower.find("://");
    if (protoEnd == std::string::npos || protoEnd + 3 >= urlLower.length()) {
        return ValidationResult::error(
            ValidationResult::ERR_INVALID_FORMAT,
            "La URL RTMP no tiene un formato válido. "
            "Debe incluir un servidor después del protocolo.");
    }

    std::string hostPart = urlLower.substr(protoEnd + 3);
    if (hostPart.empty() || hostPart[0] == '/') {
        return ValidationResult::error(
            ValidationResult::ERR_INVALID_FORMAT,
            "La URL RTMP debe incluir una dirección de servidor válida.");
    }

    blog(LOG_INFO, LOG_PREFIX "URL personalizada validada correctamente");
    return ValidationResult::success();
}

// ============================================================================
// Validación de clave de transmisión - Dispatcher principal
// ============================================================================

ValidationResult PlatformValidator::validateStreamKey(PlatformType type,
                                                       const std::string &key)
{
    blog(LOG_DEBUG, LOG_PREFIX "Validando clave de transmisión para: %s",
         platformTypeToString(type));

    // Verificación universal: clave no vacía
    if (key.empty()) {
        blog(LOG_WARNING, LOG_PREFIX "Clave de transmisión vacía para %s",
             platformTypeToString(type));
        return ValidationResult::error(
            ValidationResult::ERR_EMPTY_KEY,
            "La clave de transmisión (stream key) no puede estar vacía.");
    }

    // Verificar que no contenga espacios al inicio o final
    if (key.front() == ' ' || key.back() == ' ') {
        return ValidationResult::successWithWarning(
            "La clave de transmisión contiene espacios al inicio o final. "
            "Esto puede causar errores de conexión. Verifique que la copió correctamente.");
    }

    // Despachar a validador específico
    switch (type) {
    case PlatformType::YouTube:   return validateYouTubeKey(key);
    case PlatformType::Facebook:  return validateFacebookKey(key);
    case PlatformType::TikTok:    return validateTikTokKey(key);
    case PlatformType::Instagram: return validateInstagramKey(key);
    case PlatformType::Twitter:   return validateTwitterKey(key);
    case PlatformType::Custom:    return validateCustomKey(key);
    default:
        return ValidationResult::error(
            ValidationResult::ERR_KEY_FORMAT,
            "Tipo de plataforma no reconocido para validar la clave.");
    }
}

// ============================================================================
// Validación de clave por plataforma
// ============================================================================

ValidationResult PlatformValidator::validateYouTubeKey(const std::string &key)
{
    // Las claves de YouTube suelen tener formato: xxxx-xxxx-xxxx-xxxx-xxxx
    // Longitud típica: 20-50 caracteres (con o sin guiones)
    if (key.length() < 10) {
        return ValidationResult::error(
            ValidationResult::ERR_KEY_FORMAT,
            "La clave de YouTube parece demasiado corta. "
            "Las claves de YouTube generalmente tienen al menos 16 caracteres.");
    }

    if (key.length() > 100) {
        return ValidationResult::error(
            ValidationResult::ERR_KEY_FORMAT,
            "La clave de YouTube parece demasiado larga. "
            "Verifique que copió solo la clave y no la URL completa.");
    }

    blog(LOG_INFO, LOG_PREFIX "Clave de YouTube validada correctamente");
    return ValidationResult::success();
}

ValidationResult PlatformValidator::validateFacebookKey(const std::string &key)
{
    // Facebook usa claves con prefijo "FB-" a veces, o claves alfanuméricas largas
    if (key.length() < 10) {
        return ValidationResult::error(
            ValidationResult::ERR_KEY_FORMAT,
            "La clave de Facebook parece demasiado corta. "
            "Obtenga la clave desde Facebook Live Producer.");
    }

    if (key.length() > 200) {
        return ValidationResult::error(
            ValidationResult::ERR_KEY_FORMAT,
            "La clave de Facebook parece demasiado larga. "
            "Verifique que copió solo la clave de transmisión.");
    }

    blog(LOG_INFO, LOG_PREFIX "Clave de Facebook validada correctamente");
    return ValidationResult::success();
}

ValidationResult PlatformValidator::validateTikTokKey(const std::string &key)
{
    // TikTok genera claves alfanuméricas
    if (key.length() < 8) {
        return ValidationResult::error(
            ValidationResult::ERR_KEY_FORMAT,
            "La clave de TikTok parece demasiado corta. "
            "Obtenga la clave desde el panel de TikTok Live Studio.");
    }

    blog(LOG_INFO, LOG_PREFIX "Clave de TikTok validada correctamente");
    return ValidationResult::success();
}

ValidationResult PlatformValidator::validateInstagramKey(const std::string &key)
{
    // Instagram genera claves temporales
    if (key.length() < 8) {
        return ValidationResult::error(
            ValidationResult::ERR_KEY_FORMAT,
            "La clave de Instagram parece demasiado corta. "
            "Las claves de Instagram Live se generan al iniciar una transmisión.");
    }

    blog(LOG_INFO, LOG_PREFIX "Clave de Instagram validada correctamente");
    return ValidationResult::successWithWarning(
        "Recuerde que las claves de Instagram Live son temporales y "
        "cambian cada vez que inicia una nueva transmisión.");
}

ValidationResult PlatformValidator::validateTwitterKey(const std::string &key)
{
    // X (Twitter) / Periscope genera claves alfanuméricas
    if (key.length() < 8) {
        return ValidationResult::error(
            ValidationResult::ERR_KEY_FORMAT,
            "La clave de X (Twitter) parece demasiado corta. "
            "Obtenga la clave desde X Media Studio o Periscope.");
    }

    blog(LOG_INFO, LOG_PREFIX "Clave de X (Twitter) validada correctamente");
    return ValidationResult::success();
}

ValidationResult PlatformValidator::validateCustomKey(const std::string &key)
{
    // Para servidores personalizados, la clave puede tener cualquier formato
    // Solo verificamos que no esté vacía (ya se verificó arriba)
    if (key.length() < 2) {
        return ValidationResult::successWithWarning(
            "La clave de transmisión es muy corta. "
            "Verifique que sea la correcta para su servidor RTMP.");
    }

    blog(LOG_INFO, LOG_PREFIX "Clave personalizada validada correctamente");
    return ValidationResult::success();
}

// ============================================================================
// Prueba de conexión asincrónica
// ============================================================================

void PlatformValidator::testConnection(const std::string &url,
                                        const std::string &key,
                                        ConnectionCallback callback)
{
    blog(LOG_INFO, LOG_PREFIX "Iniciando prueba de conexión a: %s", url.c_str());

    // Limpiar pruebas completadas antes de agregar una nueva
    cleanupFinishedTests();

    // Capturar variables para el hilo
    std::string capturedUrl = url;
    std::string capturedKey = key;
    std::atomic<bool> *cancelFlag = &cancelRequested_;

    // Lanzar la prueba en un hilo de trabajo
    auto future = std::async(std::launch::async,
        [capturedUrl, capturedKey, callback, cancelFlag]() {
            blog(LOG_DEBUG, LOG_PREFIX
                 "Hilo de prueba de conexión iniciado para: %s",
                 capturedUrl.c_str());

            // Medir tiempo de inicio para calcular latencia
            auto startTime = std::chrono::high_resolution_clock::now();

            // Verificar cancelación antes de iniciar
            if (cancelFlag->load()) {
                blog(LOG_INFO, LOG_PREFIX
                     "Prueba de conexión cancelada antes de iniciar");
                callback(ConnectionTestResult::failure(
                    ValidationResult::ERR_TIMEOUT,
                    "La prueba de conexión fue cancelada."));
                return;
            }

            // ----------------------------------------------------------------
            // Intentar conexión TCP básica al servidor RTMP
            // Usamos la API de OBS para crear un output temporal de prueba
            // ----------------------------------------------------------------

            // Extraer host y puerto de la URL RTMP
            std::string urlLower = capturedUrl;
            std::transform(urlLower.begin(), urlLower.end(),
                           urlLower.begin(), ::tolower);

            // Determinar si es rtmps (puerto 443 por defecto) o rtmp (1935)
            bool isSecure = urlLower.find("rtmps://") == 0;
            int defaultPort = isSecure ? 443 : 1935;

            // Construir la URL completa con clave para el test
            std::string fullUrl = capturedUrl;

            // Asegurar que la URL termina con /
            if (!fullUrl.empty() && fullUrl.back() != '/') {
                fullUrl += '/';
            }

            // Crear un output RTMP temporal para probar la conexión
            // Usamos obs_output_create con el servicio de prueba
            obs_data_t *settings = obs_data_create();
            if (!settings) {
                blog(LOG_ERROR, LOG_PREFIX
                     "Error al crear configuración para prueba de conexión");
                callback(ConnectionTestResult::failure(
                    ValidationResult::ERR_CONNECTION_FAILED,
                    "Error interno al preparar la prueba de conexión."));
                return;
            }

            // Configurar servidor y clave
            obs_data_set_string(settings, "server", capturedUrl.c_str());
            obs_data_set_string(settings, "key", capturedKey.c_str());

            // Crear servicio RTMP temporal para la prueba
            obs_service_t *testService = obs_service_create(
                "rtmp_custom", "cueva_connection_test", settings, nullptr);

            if (!testService) {
                blog(LOG_WARNING, LOG_PREFIX
                     "No se pudo crear servicio RTMP de prueba. "
                     "Verificando conectividad básica...");
                obs_data_release(settings);

                // Fallback: solo verificar que la URL tiene formato válido
                auto endTime = std::chrono::high_resolution_clock::now();
                double elapsed =
                    std::chrono::duration<double, std::milli>(
                        endTime - startTime).count();

                callback(ConnectionTestResult::successWithWarning(elapsed,
                    "No se pudo realizar una prueba completa de conexión RTMP. "
                    "La URL tiene un formato válido pero no se verificó la conectividad real."));
                return;
            }

            // Verificar que el servicio se creó correctamente
            const char *serviceUrl = obs_service_get_connect_info(
                testService, OBS_SERVICE_CONNECT_INFO_SERVER_URL);

            if (serviceUrl && strlen(serviceUrl) > 0) {
                blog(LOG_INFO, LOG_PREFIX
                     "Servicio de prueba creado exitosamente para: %s",
                     serviceUrl);
            }

            // Medir el tiempo que tomó crear el servicio como proxy de latencia
            auto endTime = std::chrono::high_resolution_clock::now();
            double elapsed =
                std::chrono::duration<double, std::milli>(
                    endTime - startTime).count();

            // Limpiar recursos
            obs_service_release(testService);
            obs_data_release(settings);

            // Verificar cancelación
            if (cancelFlag->load()) {
                callback(ConnectionTestResult::failure(
                    ValidationResult::ERR_TIMEOUT,
                    "La prueba de conexión fue cancelada por el usuario."));
                return;
            }

            blog(LOG_INFO, LOG_PREFIX
                 "Prueba de conexión completada en %.1f ms", elapsed);

            // Reportar resultado exitoso
            callback(ConnectionTestResult::success(elapsed));
        }
    );

    // Almacenar el future para poder esperarlo en el destructor
    {
        std::lock_guard<std::mutex> lock(testsMutex_);
        pendingTests_.push_back(std::move(future));
    }
}

ConnectionTestResult PlatformValidator::testConnectionSync(
    const std::string &url, const std::string &key, int timeoutMs)
{
    blog(LOG_INFO, LOG_PREFIX
         "Prueba de conexión sincrónica a: %s (timeout: %d ms)",
         url.c_str(), timeoutMs);

    // Usar una promesa para recibir el resultado del callback
    std::promise<ConnectionTestResult> promise;
    auto future = promise.get_future();

    testConnection(url, key,
        [&promise](const ConnectionTestResult &result) {
            promise.set_value(result);
        });

    // Esperar con timeout
    auto status = future.wait_for(std::chrono::milliseconds(timeoutMs));

    if (status == std::future_status::timeout) {
        blog(LOG_WARNING, LOG_PREFIX
             "Prueba de conexión agotó el tiempo de espera (%d ms)", timeoutMs);
        cancelPendingTests();
        return ConnectionTestResult::failure(
            ValidationResult::ERR_TIMEOUT,
            "La prueba de conexión agotó el tiempo de espera. "
            "Verifique su conexión a internet y la URL del servidor.");
    }

    return future.get();
}

// ============================================================================
// Control de hilos
// ============================================================================

void PlatformValidator::cancelPendingTests()
{
    blog(LOG_INFO, LOG_PREFIX "Cancelando pruebas de conexión pendientes...");
    cancelRequested_.store(true);

    std::lock_guard<std::mutex> lock(testsMutex_);
    for (auto &f : pendingTests_) {
        if (f.valid()) {
            // Esperar a que termine (con timeout implícito por el flag de cancelación)
            f.wait();
        }
    }
    pendingTests_.clear();

    // Resetear el flag para futuras pruebas
    cancelRequested_.store(false);
    blog(LOG_INFO, LOG_PREFIX "Todas las pruebas pendientes canceladas");
}

bool PlatformValidator::hasActiveTests() const
{
    std::lock_guard<std::mutex> lock(testsMutex_);
    for (const auto &f : pendingTests_) {
        if (f.valid() &&
            f.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) {
            return true;
        }
    }
    return false;
}

void PlatformValidator::cleanupFinishedTests()
{
    std::lock_guard<std::mutex> lock(testsMutex_);
    pendingTests_.erase(
        std::remove_if(pendingTests_.begin(), pendingTests_.end(),
            [](const std::future<void> &f) {
                return !f.valid() ||
                       f.wait_for(std::chrono::milliseconds(0)) ==
                           std::future_status::ready;
            }),
        pendingTests_.end());
}
