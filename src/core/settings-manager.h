/**
 * @file settings-manager.h
 * @brief Gestor de persistencia de configuración de plataformas y claves RTMP.
 *
 * Proyecto: La Cueva del Dolor - OBS MultiStream Plugin
 * @copyright 2026 La Cueva del Dolor
 */

#pragma once

#include <string>
#include <vector>
#include <obs-module.h>
#include "platform-config.h"

namespace CuevaMultiStream {

class SettingsManager {
public:
    static SettingsManager &instance();

    SettingsManager(const SettingsManager &) = delete;
    SettingsManager &operator=(const SettingsManager &) = delete;

    /**
     * @brief Inicializa el gestor y carga configuraciones guardadas.
     */
    void initialize();

    /**
     * @brief Guarda todas las configuraciones actuales a disco.
     */
    bool save();

    /**
     * @brief Carga las configuraciones desde disco.
     */
    bool load();

    /**
     * @brief Obtiene la lista de todas las plataformas configuradas.
     */
    std::vector<PlatformConfig> getAllConfigs() const;

    /**
     * @brief Obtiene la configuración de una plataforma por su ID.
     */
    PlatformConfig getConfig(const std::string &id) const;

    /**
     * @brief Actualiza la configuración de una plataforma.
     */
    void setConfig(const PlatformConfig &cfg);

    /**
     * @brief Funciones de ofuscación de stream key para almacenamiento seguro en disco.
     */
    static std::string obfuscateKey(const std::string &plain);
    static std::string deobfuscateKey(const std::string &cipher);

private:
    SettingsManager();
    ~SettingsManager();

    std::string getConfigFilePath() const;

    std::vector<PlatformConfig> configs_;
};

} // namespace CuevaMultiStream
