/**
 * @file multi-output-manager.cpp
 * @brief Implementación del gestor de salidas múltiples RTMP con encoder compartido.
 *
 * Proyecto: La Cueva del Dolor - OBS MultiStream Plugin
 * @copyright 2026 La Cueva del Dolor
 */

#include "multi-output-manager.h"
#include <util/platform.h>
#include <util/dstr.h>
#include <algorithm>

namespace CuevaMultiStream {

MultiOutputManager &MultiOutputManager::instance() {
    static MultiOutputManager s_instance;
    return s_instance;
}

MultiOutputManager::MultiOutputManager() = default;

MultiOutputManager::~MultiOutputManager() {
    shutdown();
}

bool MultiOutputManager::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_) return true;

    // Detectar el encoder de video por hardware más eficiente disponible
    const char *candidates[] = {
        "jim_nvenc",         // NVIDIA NVENC (mínimo consumo CPU, ultra eficiente)
        "obs_amf_h264",      // AMD AMF hardware encoder
        "obs_qsv11",         // Intel QuickSync
        "obs_x264"           // CPU x264 como respaldo universal
    };

    preferredVideoEncoderId_ = "obs_x264";
    for (const char *encId : candidates) {
        obs_encoder_t *test = obs_video_encoder_create(encId, "test_probe_enc", nullptr, nullptr);
        if (test) {
            preferredVideoEncoderId_ = encId;
            obs_encoder_release(test);
            blog(LOG_INFO, "[CuevaMultiStream] Encoder de hardware detectado y seleccionado: %s", encId);
            break;
        }
    }

    initialized_ = true;
    return true;
}

void MultiOutputManager::shutdown() {
    stopAll();

    std::lock_guard<std::mutex> lock(mutex_);
    for (auto &pair : outputs_) {
        destroyOutputForContext(pair.second);
    }
    outputs_.clear();

    if (sharedVideoEncoder_) {
        obs_encoder_release(sharedVideoEncoder_);
        sharedVideoEncoder_ = nullptr;
    }

    if (sharedAudioEncoder_) {
        obs_encoder_release(sharedAudioEncoder_);
        sharedAudioEncoder_ = nullptr;
    }

    initialized_ = false;
    blog(LOG_INFO, "[CuevaMultiStream] MultiOutputManager cerrado correctamente.");
}

void MultiOutputManager::setPlatformConfig(const PlatformConfig &config) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = outputs_.find(config.id);
    if (it != outputs_.end()) {
        it->second.config = config;
    } else {
        StreamOutputContext ctx;
        ctx.config = config;
        outputs_[config.id] = ctx;
    }
}

void MultiOutputManager::removePlatform(const std::string &platformId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = outputs_.find(platformId);
    if (it != outputs_.end()) {
        if (it->second.output && obs_output_active(it->second.output)) {
            obs_output_stop(it->second.output);
        }
        destroyOutputForContext(it->second);
        outputs_.erase(it);
    }
}

bool MultiOutputManager::ensureSharedEncoders(int baseBitrateKbps, int audioBitrateKbps) {
    if (!sharedVideoEncoder_) {
        obs_data_t *vsettings = obs_data_create();
        obs_data_set_int(vsettings, "bitrate", baseBitrateKbps);
        obs_data_set_string(vsettings, "rate_control", "CBR");
        obs_data_set_int(vsettings, "keyint_sec", 2); // 2 segundos universal
        obs_data_set_string(vsettings, "preset", "veryfast");
        obs_data_set_string(vsettings, "profile", "high");
        obs_data_set_bool(vsettings, "psycho_aq", true);

        sharedVideoEncoder_ = obs_video_encoder_create(
            preferredVideoEncoderId_.c_str(),
            "cueva_shared_vencoder",
            vsettings,
            nullptr
        );
        obs_data_release(vsettings);

        if (!sharedVideoEncoder_) {
            blog(LOG_ERROR, "[CuevaMultiStream] ERROR CRÍTICO: No se pudo instanciar el encoder de video compartido %s",
                 preferredVideoEncoderId_.c_str());
            return false;
        }

        obs_encoder_set_video(sharedVideoEncoder_, obs_get_video());
        blog(LOG_INFO, "[CuevaMultiStream] Encoder compartido de video creado exitosamente (%s @ %d kbps)",
             preferredVideoEncoderId_.c_str(), baseBitrateKbps);
    }

    if (!sharedAudioEncoder_) {
        obs_data_t *asettings = obs_data_create();
        obs_data_set_int(asettings, "bitrate", audioBitrateKbps);

        sharedAudioEncoder_ = obs_audio_encoder_create(
            "ffmpeg_aac",
            "cueva_shared_aencoder",
            asettings,
            0,
            nullptr
        );
        obs_data_release(asettings);

        if (!sharedAudioEncoder_) {
            blog(LOG_ERROR, "[CuevaMultiStream] No se pudo instanciar el encoder de audio ffmpeg_aac");
            return false;
        }

        obs_encoder_set_audio(sharedAudioEncoder_, obs_get_audio());
        blog(LOG_INFO, "[CuevaMultiStream] Encoder compartido de audio creado exitosamente (AAC @ %d kbps)",
             audioBitrateKbps);
    }

    return true;
}

void MultiOutputManager::releaseSharedEncodersIfUnused() {
    if (!isAnyStreaming()) {
        if (sharedVideoEncoder_) {
            obs_encoder_release(sharedVideoEncoder_);
            sharedVideoEncoder_ = nullptr;
            blog(LOG_INFO, "[CuevaMultiStream] Encoder compartido de video liberado para ahorrar memoria.");
        }
        if (sharedAudioEncoder_) {
            obs_encoder_release(sharedAudioEncoder_);
            sharedAudioEncoder_ = nullptr;
            blog(LOG_INFO, "[CuevaMultiStream] Encoder compartido de audio liberado para ahorrar memoria.");
        }
    }
}

bool MultiOutputManager::createOutputForContext(StreamOutputContext &ctx) {
    if (ctx.output) return true;

    // 1. Servicio RTMP personalizado
    obs_data_t *serviceSettings = obs_data_create();
    obs_data_set_string(serviceSettings, "server", ctx.config.rtmpUrl.c_str());
    obs_data_set_string(serviceSettings, "key", ctx.config.streamKey.c_str());

    ctx.service = obs_service_create("rtmp_custom", (ctx.config.id + "_service").c_str(), serviceSettings, nullptr);
    obs_data_release(serviceSettings);

    if (!ctx.service) {
        blog(LOG_ERROR, "[CuevaMultiStream] Falló al crear obs_service_t para %s", ctx.config.displayName.c_str());
        return false;
    }

    // 2. Salida RTMP
    obs_data_t *outputSettings = obs_data_create();
    ctx.output = obs_output_create("rtmp_output", (ctx.config.id + "_output").c_str(), outputSettings, nullptr);
    obs_data_release(outputSettings);

    if (!ctx.output) {
        blog(LOG_ERROR, "[CuevaMultiStream] Falló al crear obs_output_t para %s", ctx.config.displayName.c_str());
        obs_service_release(ctx.service);
        ctx.service = nullptr;
        return false;
    }

    // 3. Vincular servicio y los encoders compartidos
    obs_output_set_service(ctx.output, ctx.service);
    obs_output_set_video_encoder(ctx.output, sharedVideoEncoder_);
    obs_output_set_audio_encoder(ctx.output, sharedAudioEncoder_, 0);

    // 4. Conectar señales de eventos
    ctx.signalHandler = obs_output_get_signal_handler(ctx.output);
    if (ctx.signalHandler) {
        signal_handler_connect(ctx.signalHandler, "starting", onOutputStarting, this);
        signal_handler_connect(ctx.signalHandler, "start", onOutputStart, this);
        signal_handler_connect(ctx.signalHandler, "stop", onOutputStop, this);
        signal_handler_connect(ctx.signalHandler, "reconnect", onOutputReconnect, this);
        signal_handler_connect(ctx.signalHandler, "reconnect_success", onOutputReconnectSuccess, this);
    }

    return true;
}

void MultiOutputManager::destroyOutputForContext(StreamOutputContext &ctx) {
    if (ctx.output) {
        if (ctx.signalHandler) {
            signal_handler_disconnect(ctx.signalHandler, "starting", onOutputStarting, this);
            signal_handler_disconnect(ctx.signalHandler, "start", onOutputStart, this);
            signal_handler_disconnect(ctx.signalHandler, "stop", onOutputStop, this);
            signal_handler_disconnect(ctx.signalHandler, "reconnect", onOutputReconnect, this);
            signal_handler_disconnect(ctx.signalHandler, "reconnect_success", onOutputReconnectSuccess, this);
            ctx.signalHandler = nullptr;
        }
        obs_output_release(ctx.output);
        ctx.output = nullptr;
    }
    if (ctx.service) {
        obs_service_release(ctx.service);
        ctx.service = nullptr;
    }
    ctx.telemetry.status = OutputStatus::Stopped;
}

bool MultiOutputManager::startPlatform(const std::string &platformId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = outputs_.find(platformId);
    if (it == outputs_.end()) {
        blog(LOG_WARNING, "[CuevaMultiStream] Plataforma %s no registrada.", platformId.c_str());
        return false;
    }

    StreamOutputContext &ctx = it->second;
    if (ctx.config.rtmpUrl.empty() || ctx.config.streamKey.empty()) {
        blog(LOG_ERROR, "[CuevaMultiStream] No se puede iniciar %s: URL o Stream Key vacía.", ctx.config.displayName.c_str());
        handleSignal(platformId, OutputStatus::Failed, "URL o clave vacía");
        return false;
    }

    // Asegurar encoder compartido
    if (!ensureSharedEncoders(ctx.config.videoBitrate, ctx.config.audioBitrate)) {
        return false;
    }

    // Crear salida si no existe
    if (!createOutputForContext(ctx)) {
        return false;
    }

    blog(LOG_INFO, "[CuevaMultiStream] Iniciando stream hacia %s (%s)...",
         ctx.config.displayName.c_str(), ctx.config.rtmpUrl.c_str());

    bool ok = obs_output_start(ctx.output);
    if (!ok) {
        const char *err = obs_output_get_last_error(ctx.output);
        std::string errStr = err ? err : "Error desconocido al iniciar output";
        blog(LOG_ERROR, "[CuevaMultiStream] Error al iniciar %s: %s", ctx.config.displayName.c_str(), errStr.c_str());
        handleSignal(platformId, OutputStatus::Failed, errStr);
        return false;
    }

    return true;
}

void MultiOutputManager::stopPlatform(const std::string &platformId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = outputs_.find(platformId);
    if (it != outputs_.end() && it->second.output) {
        if (obs_output_active(it->second.output)) {
            blog(LOG_INFO, "[CuevaMultiStream] Deteniendo stream hacia %s...", it->second.config.displayName.c_str());
            obs_output_stop(it->second.output);
        }
    }
}

bool MultiOutputManager::startAllEnabled() {
    bool anyStarted = false;
    std::vector<std::string> toStart;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto &pair : outputs_) {
            if (pair.second.config.enabled) {
                toStart.push_back(pair.first);
            }
        }
    }

    for (const auto &id : toStart) {
        if (startPlatform(id)) {
            anyStarted = true;
        }
    }

    return anyStarted;
}

void MultiOutputManager::stopAll() {
    std::vector<std::string> toStop;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto &pair : outputs_) {
            if (pair.second.output && obs_output_active(pair.second.output)) {
                toStop.push_back(pair.first);
            }
        }
    }

    for (const auto &id : toStop) {
        stopPlatform(id);
    }

    releaseSharedEncodersIfUnused();
}

bool MultiOutputManager::isAnyStreaming() const {
    for (const auto &pair : outputs_) {
        if (pair.second.output && obs_output_active(pair.second.output)) {
            return true;
        }
    }
    return false;
}

OutputStatus MultiOutputManager::getPlatformStatus(const std::string &platformId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = outputs_.find(platformId);
    if (it != outputs_.end()) {
        return it->second.telemetry.status;
    }
    return OutputStatus::Stopped;
}

OutputTelemetry MultiOutputManager::getTelemetry(const std::string &platformId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = outputs_.find(platformId);
    if (it != outputs_.end()) {
        return it->second.telemetry;
    }
    return OutputTelemetry{};
}

void MultiOutputManager::setStatusCallback(StatusCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    statusCallback_ = std::move(cb);
}

void MultiOutputManager::setTelemetryCallback(TelemetryCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    telemetryCallback_ = std::move(cb);
}

void MultiOutputManager::updateTelemetry() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto &pair : outputs_) {
        StreamOutputContext &ctx = pair.second;
        if (!ctx.output || !obs_output_active(ctx.output)) continue;

        ctx.telemetry.totalBytesSent = obs_output_get_total_bytes(ctx.output);
        ctx.telemetry.droppedFrames = obs_output_get_frames_dropped(ctx.output);
        ctx.telemetry.totalFrames = obs_output_get_total_frames(ctx.output);
        ctx.telemetry.congestionRatio = obs_output_get_congestion(ctx.output);

        if (telemetryCallback_) {
            telemetryCallback_(pair.first, ctx.telemetry);
        }
    }
}

void MultiOutputManager::handleSignal(const std::string &platformId, OutputStatus status, const std::string &msg) {
    StatusCallback cbCopy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = outputs_.find(platformId);
        if (it != outputs_.end()) {
            it->second.telemetry.status = status;
            it->second.telemetry.errorMessage = msg;
        }
        cbCopy = statusCallback_;
    }

    if (cbCopy) {
        cbCopy(platformId, status, msg);
    }
}

// Funciones puente de señales estáticas de OBS
void MultiOutputManager::onOutputStarting(void *param, calldata_t *data) {
    auto *manager = static_cast<MultiOutputManager *>(param);
    obs_output_t *output = static_cast<obs_output_t *>(calldata_ptr(data, "output"));
    const char *name = obs_output_get_name(output);
    std::string id = name ? name : "";
    size_t pos = id.find("_output");
    if (pos != std::string::npos) id = id.substr(0, pos);

    manager->handleSignal(id, OutputStatus::Starting, "Conectando al servidor RTMP...");
}

void MultiOutputManager::onOutputStart(void *param, calldata_t *data) {
    auto *manager = static_cast<MultiOutputManager *>(param);
    obs_output_t *output = static_cast<obs_output_t *>(calldata_ptr(data, "output"));
    const char *name = obs_output_get_name(output);
    std::string id = name ? name : "";
    size_t pos = id.find("_output");
    if (pos != std::string::npos) id = id.substr(0, pos);

    manager->handleSignal(id, OutputStatus::Active, "Transmisión en vivo iniciada");
}

void MultiOutputManager::onOutputStop(void *param, calldata_t *data) {
    auto *manager = static_cast<MultiOutputManager *>(param);
    obs_output_t *output = static_cast<obs_output_t *>(calldata_ptr(data, "output"));
    int code = (int)calldata_int(data, "code");
    const char *lastErr = obs_output_get_last_error(output);
    const char *name = obs_output_get_name(output);
    std::string id = name ? name : "";
    size_t pos = id.find("_output");
    if (pos != std::string::npos) id = id.substr(0, pos);

    OutputStatus st = (code == OBS_OUTPUT_SUCCESS) ? OutputStatus::Stopped : OutputStatus::Failed;
    std::string msg = (code == OBS_OUTPUT_SUCCESS) ? "Transmisión finalizada" : (lastErr ? lastErr : "Error en transmisión");

    manager->handleSignal(id, st, msg);
    manager->releaseSharedEncodersIfUnused();
}

void MultiOutputManager::onOutputReconnect(void *param, calldata_t *data) {
    auto *manager = static_cast<MultiOutputManager *>(param);
    obs_output_t *output = static_cast<obs_output_t *>(calldata_ptr(data, "output"));
    const char *name = obs_output_get_name(output);
    std::string id = name ? name : "";
    size_t pos = id.find("_output");
    if (pos != std::string::npos) id = id.substr(0, pos);

    manager->handleSignal(id, OutputStatus::Reconnecting, "Reconectando con el servidor...");
}

void MultiOutputManager::onOutputReconnectSuccess(void *param, calldata_t *data) {
    auto *manager = static_cast<MultiOutputManager *>(param);
    obs_output_t *output = static_cast<obs_output_t *>(calldata_ptr(data, "output"));
    const char *name = obs_output_get_name(output);
    std::string id = name ? name : "";
    size_t pos = id.find("_output");
    if (pos != std::string::npos) id = id.substr(0, pos);

    manager->handleSignal(id, OutputStatus::Active, "Reconexión exitosa");
}

} // namespace CuevaMultiStream
