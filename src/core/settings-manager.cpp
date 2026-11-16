/**
 * @file settings-manager.cpp
 * @brief Implementación del gestor de persistencia de configuración.
 *
 * Proyecto: La Cueva del Dolor - OBS MultiStream Plugin
 * @copyright 2026 La Cueva del Dolor
 */

#include "settings-manager.h"
#include <util/platform.h>
#include <sstream>
#include <iomanip>

namespace CuevaMultiStream {

static const char OBFUSCATION_SALT[] = "LaCuevaDelDolor_OBS_Secret_Salt_2026";

SettingsManager &SettingsManager::instance() {
    static SettingsManager s_instance;
    return s_instance;
}

SettingsManager::SettingsManager() {
    // Inicializar con las plataformas estándar por defecto
    configs_.push_back(getDefaultConfig(PlatformType::YouTube));
    configs_.push_back(getDefaultConfig(PlatformType::Facebook));
    configs_.push_back(getDefaultConfig(PlatformType::TikTok));
    configs_.push_back(getDefaultConfig(PlatformType::Instagram));
    configs_.push_back(getDefaultConfig(PlatformType::Twitter));
}

SettingsManager::~SettingsManager() = default;

void SettingsManager::initialize() {
    load();
}

std::string SettingsManager::getConfigFilePath() const {
    char *configDir = obs_module_config_path("");
    if (!configDir) {
        return "cueva_multistream_settings.json";
    }
    os_mkdirs(configDir);
    std::string path = std::string(configDir) + "/cueva_multistream.json";
    bfree(configDir);
    return path;
}

std::string SettingsManager::obfuscateKey(const std::string &plain) {
    if (plain.empty()) return "";
    std::string res;
    size_t saltLen = sizeof(OBFUSCATION_SALT) - 1;
    for (size_t i = 0; i < plain.length(); ++i) {
        char c = plain[i] ^ OBFUSCATION_SALT[i % saltLen];
        char hex[3];
        snprintf(hex, sizeof(hex), "%02x", (unsigned char)c);
        res += hex;
    }
    return res;
}

std::string SettingsManager::deobfuscateKey(const std::string &cipher) {
    if (cipher.empty() || cipher.length() % 2 != 0) return "";
    std::string res;
    size_t saltLen = sizeof(OBFUSCATION_SALT) - 1;
    for (size_t i = 0; i < cipher.length(); i += 2) {
        std::string byteString = cipher.substr(i, 2);
        char byte = (char)strtol(byteString.c_str(), nullptr, 16);
        res += (byte ^ OBFUSCATION_SALT[(i / 2) % saltLen]);
    }
    return res;
}

bool SettingsManager::save() {
    obs_data_t *root = obs_data_create();
    obs_data_array_t *array = obs_data_array_create();

    for (const auto &cfg : configs_) {
        obs_data_t *item = obs_data_create();
        obs_data_set_int(item, "type", static_cast<int>(cfg.type));
        obs_data_set_string(item, "id", cfg.id.c_str());
        obs_data_set_string(item, "displayName", cfg.displayName.c_str());
        obs_data_set_string(item, "rtmpUrl", cfg.rtmpUrl.c_str());
        obs_data_set_string(item, "streamKeyEnc", obfuscateKey(cfg.streamKey).c_str());
        obs_data_set_bool(item, "enabled", cfg.enabled);
        obs_data_set_int(item, "videoBitrate", cfg.videoBitrate);
        obs_data_set_int(item, "audioBitrate", cfg.audioBitrate);
        obs_data_set_int(item, "width", cfg.width);
        obs_data_set_int(item, "height", cfg.height);
        obs_data_set_int(item, "fps", cfg.fps);
        obs_data_set_int(item, "keyframeInterval", cfg.keyframeInterval);
        obs_data_set_bool(item, "requiresRTMPS", cfg.requiresRTMPS);
        obs_data_set_bool(item, "isVertical", cfg.isVertical);

        obs_data_array_push_back(array, item);
        obs_data_release(item);
    }

    obs_data_set_array(root, "platforms", array);
    obs_data_array_release(array);

    std::string path = getConfigFilePath();
    bool ok = obs_data_save_json_safe(root, path.c_str(), "tmp", "bak");
    obs_data_release(root);

    if (ok) {
        blog(LOG_INFO, "[CuevaMultiStream] Configuración guardada exitosamente en %s", path.c_str());
    } else {
        blog(LOG_ERROR, "[CuevaMultiStream] Error al guardar configuración en %s", path.c_str());
    }
    return ok;
}

bool SettingsManager::load() {
    std::string path = getConfigFilePath();
    obs_data_t *root = obs_data_create_from_json_file(path.c_str());
    if (!root) {
        blog(LOG_INFO, "[CuevaMultiStream] No existe archivo previo de configuración. Usando valores predeterminados.");
        return false;
    }

    obs_data_array_t *array = obs_data_get_array(root, "platforms");
    if (array) {
        size_t count = obs_data_array_count(array);
        std::vector<PlatformConfig> loaded;

        for (size_t i = 0; i < count; ++i) {
            obs_data_t *item = obs_data_array_item(array, i);
            PlatformConfig cfg;
            cfg.type = static_cast<PlatformType>(obs_data_get_int(item, "type"));
            cfg.id = obs_data_get_string(item, "id");
            cfg.displayName = obs_data_get_string(item, "displayName");
            cfg.rtmpUrl = obs_data_get_string(item, "rtmpUrl");
            const char *enc = obs_data_get_string(item, "streamKeyEnc");
            cfg.streamKey = deobfuscateKey(enc ? enc : "");
            cfg.enabled = obs_data_get_bool(item, "enabled");
            cfg.videoBitrate = (int)obs_data_get_int(item, "videoBitrate");
            cfg.audioBitrate = (int)obs_data_get_int(item, "audioBitrate");
            cfg.width = (int)obs_data_get_int(item, "width");
            cfg.height = (int)obs_data_get_int(item, "height");
            cfg.fps = (int)obs_data_get_int(item, "fps");
            cfg.keyframeInterval = (int)obs_data_get_int(item, "keyframeInterval");
            cfg.requiresRTMPS = obs_data_get_bool(item, "requiresRTMPS");
            cfg.isVertical = obs_data_get_bool(item, "isVertical");

            loaded.push_back(cfg);
            obs_data_release(item);
        }
        obs_data_array_release(array);

        if (!loaded.empty()) {
            configs_ = loaded;
        }
    }

    obs_data_release(root);
    blog(LOG_INFO, "[CuevaMultiStream] Configuración cargada (%zu plataformas)", configs_.size());
    return true;
}

std::vector<PlatformConfig> SettingsManager::getAllConfigs() const {
    return configs_;
}

PlatformConfig SettingsManager::getConfig(const std::string &id) const {
    for (const auto &cfg : configs_) {
        if (cfg.id == id) return cfg;
    }
    return PlatformConfig{};
}

void SettingsManager::setConfig(const PlatformConfig &cfg) {
    for (auto &existing : configs_) {
        if (existing.id == cfg.id) {
            existing = cfg;
            return;
        }
    }
    configs_.push_back(cfg);
}

} // namespace CuevaMultiStream
