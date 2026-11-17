/**
 * @file plugin-main.cpp
 * @brief Punto de entrada principal y registro del módulo en OBS Studio.
 *
 * Proyecto: La Cueva del Dolor - OBS MultiStream Plugin
 * @copyright 2026 La Cueva del Dolor
 */

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QMainWindow>
#include "core/multi-output-manager.h"
#include "core/settings-manager.h"
#include "platforms/resource-monitor.h"
#include "ui/multistream-dock.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("cueva-multistream", "es-ES")

static CuevaMultiStream::MultiStreamDock *s_dock = nullptr;

MODULE_EXPORT const char *obs_module_name(void) {
    return "La Cueva MultiStream";
}

MODULE_EXPORT const char *obs_module_description(void) {
    return "Plugin de transmisión en vivo simultánea a TikTok, YouTube, Instagram, Facebook y X "
           "con optimización extrema de recursos de CPU y GPU.";
}

MODULE_EXPORT bool obs_module_load(void) {
    blog(LOG_INFO, "==========================================================");
    blog(LOG_INFO, "[CuevaMultiStream] Cargando plugin v1.0.0 (La Cueva del Dolor)...");
    blog(LOG_INFO, "==========================================================");

    // 1. Cargar persistencia de configuraciones
    CuevaMultiStream::SettingsManager::instance().initialize();

    // 2. Inicializar gestor de salidas múltiples
    if (!CuevaMultiStream::MultiOutputManager::instance().initialize()) {
        blog(LOG_ERROR, "[CuevaMultiStream] Error al inicializar MultiOutputManager");
        return false;
    }

    // 3. Iniciar monitor de hardware
    ResourceMonitor::instance().start();

    // 4. Integrar el dock en la interfaz de OBS Studio
    QMainWindow *mainWindow = static_cast<QMainWindow *>(obs_frontend_get_main_window());
    if (mainWindow) {
        s_dock = new CuevaMultiStream::MultiStreamDock(mainWindow);
        obs_frontend_add_dock(s_dock);
        blog(LOG_INFO, "[CuevaMultiStream] Dock UI anclado exitosamente en OBS.");
    } else {
        blog(LOG_WARNING, "[CuevaMultiStream] No se pudo obtener la ventana principal de OBS para anclar el dock.");
    }

    blog(LOG_INFO, "[CuevaMultiStream] Plugin cargado y listo para transmitir.");
    return true;
}

MODULE_EXPORT void obs_module_unload(void) {
    blog(LOG_INFO, "[CuevaMultiStream] Descargando plugin y liberando recursos...");

    // Detener monitor
    ResourceMonitor::instance().stop();

    // Detener salidas y liberar encoders
    CuevaMultiStream::MultiOutputManager::instance().shutdown();

    // Guardar configuraciones finales
    CuevaMultiStream::SettingsManager::instance().save();

    s_dock = nullptr;
    blog(LOG_INFO, "[CuevaMultiStream] Plugin descargado correctamente.");
}
