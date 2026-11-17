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
OBS_MODULE_USE_DEFAULT_LOCALE("lacueva-multistream", "es-ES")

static CuevaMultiStream::MultiStreamDock *s_dock = nullptr;

MODULE_EXPORT const char *obs_module_name(void) {
    return "La Cueva MultiStream";
}

MODULE_EXPORT const char *obs_module_description(void) {
    return "Plugin de transmisión en vivo simultánea a TikTok, YouTube, Instagram, Facebook y X "
           "con optimización extrema de recursos de CPU y GPU.";
}

// Callback oficial de OBS Frontend: se ejecuta cuando la ventana de OBS ya existe
static void on_frontend_event(enum obs_frontend_event event, void *private_data) {
    UNUSED_PARAMETER(private_data);

    if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING) {
        blog(LOG_INFO, "[CuevaMultiStream] OBS termino de cargar. Creando panel dock...");

        QMainWindow *mainWindow = static_cast<QMainWindow *>(obs_frontend_get_main_window());
        if (mainWindow && !s_dock) {
            s_dock = new CuevaMultiStream::MultiStreamDock(mainWindow);
            obs_frontend_add_dock(s_dock);
            blog(LOG_INFO, "[CuevaMultiStream] Panel 'La Cueva MultiStream' anclado con éxito en OBS Studio.");
        }

        // Iniciar monitor de recursos una vez que el sistema está listo
        ResourceMonitor::instance().start();
    }
}

MODULE_EXPORT bool obs_module_load(void) {
    blog(LOG_INFO, "==========================================================");
    blog(LOG_INFO, "[CuevaMultiStream] Cargando plugin v1.0.0 (La Cueva del Dolor)...");
    blog(LOG_INFO, "==========================================================");

    // 1. Cargar configuraciones guardadas
    CuevaMultiStream::SettingsManager::instance().initialize();

    // 2. Esperar a que OBS termine de dibujar su interfaz para inyectar el panel
    obs_frontend_add_event_callback(on_frontend_event, nullptr);

    blog(LOG_INFO, "[CuevaMultiStream] Modulo registrado exitosamente.");
    return true;
}

MODULE_EXPORT void obs_module_unload(void) {
    blog(LOG_INFO, "[CuevaMultiStream] Descargando plugin y liberando recursos...");

    obs_frontend_remove_event_callback(on_frontend_event, nullptr);

    ResourceMonitor::instance().stop();
    CuevaMultiStream::MultiOutputManager::instance().shutdown();
    CuevaMultiStream::SettingsManager::instance().save();

    s_dock = nullptr;
    blog(LOG_INFO, "[CuevaMultiStream] Plugin descargado correctamente.");
}
