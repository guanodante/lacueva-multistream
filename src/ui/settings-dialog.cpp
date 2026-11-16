/**
 * @file settings-dialog.cpp
 * @brief Implementación del diálogo de configuración avanzada.
 *
 * Proyecto: La Cueva del Dolor - OBS MultiStream Plugin
 * @copyright 2026 La Cueva del Dolor
 */

#include "settings-dialog.h"
#include <QFormLayout>
#include <QGroupBox>

namespace CuevaMultiStream {

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("Ajustes - La Cueva MultiStream");
    resize(480, 360);
    setupUI();
}

void SettingsDialog::setupUI() {
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(12, 12, 12, 12);
    rootLayout->setSpacing(10);

    tabWidget_ = new QTabWidget(this);

    // ==========================================
    // Pestaña 1: Codificación y Rendimiento
    // ==========================================
    auto *tabGeneral = new QWidget(this);
    auto *genLayout = new QFormLayout(tabGeneral);
    genLayout->setContentsMargins(10, 10, 10, 10);
    genLayout->setSpacing(10);

    encoderCombo_ = new QComboBox(this);
    encoderCombo_->addItem("Auto-detectar Hardware (Recomendado - NVENC/AMF/QSV)");
    encoderCombo_->addItem("NVIDIA NVENC H.264 (Cero consumo CPU)");
    encoderCombo_->addItem("AMD AMF H.264");
    encoderCombo_->addItem("Intel QuickSync (QSV)");
    encoderCombo_->addItem("Software x264 (CPU - Mayor consumo)");
    genLayout->addRow("Encoder de Video:", encoderCombo_);

    qualityPresetCombo_ = new QComboBox(this);
    qualityPresetCombo_->addItem("Ultra Baja Latencia (Zero Latency)");
    qualityPresetCombo_->addItem("Baja Latencia (Equilibrado)");
    qualityPresetCombo_->addItem("Máxima Calidad (Para gaming exigente)");
    genLayout->addRow("Preset de Rendimiento:", qualityPresetCombo_);

    enableResourceGuardCheck_ = new QCheckBox("Activar guardián de recursos (alerta si CPU > 85%)", this);
    enableResourceGuardCheck_->setChecked(true);
    genLayout->addRow("", enableResourceGuardCheck_);

    tabWidget_->addTab(tabGeneral, "Rendimiento");

    // ==========================================
    // Pestaña 2: Optimización de Red
    // ==========================================
    auto *tabNetwork = new QWidget(this);
    auto *netLayout = new QFormLayout(tabNetwork);
    netLayout->setContentsMargins(10, 10, 10, 10);
    netLayout->setSpacing(10);

    bandwidthLimitSpin_ = new QSpinBox(this);
    bandwidthLimitSpin_->setRange(5, 500);
    bandwidthLimitSpin_->setValue(20);
    bandwidthLimitSpin_->setSuffix(" Mbps");
    netLayout->addRow("Velocidad de Subida de tu Conexión:", bandwidthLimitSpin_);

    auto *ruleLabel = new QLabel("El plugin limitará el bitrate total al 80% de este valor para evitar lag.", this);
    ruleLabel->setStyleSheet("color: #7f8c8d; font-size: 11px;");
    netLayout->addRow("", ruleLabel);

    autoDowngradeCheck_ = new QCheckBox("Degradar resolución automáticamente si hay pérdida de paquetes", this);
    autoDowngradeCheck_->setChecked(true);
    netLayout->addRow("", autoDowngradeCheck_);

    tabWidget_->addTab(tabNetwork, "Red y Ancho de Banda");

    // ==========================================
    // Pestaña 3: Acerca de
    // ==========================================
    auto *tabAbout = new QWidget(this);
    auto *aboutLayout = new QVBoxLayout(tabAbout);
    aboutLayout->setContentsMargins(15, 15, 15, 15);

    auto *aboutTitle = new QLabel("<h2>🎮 La Cueva MultiStream v1.0.0</h2>", this);
    aboutTitle->setStyleSheet("color: #e67e22;");
    auto *aboutDesc = new QLabel(
        "<p>Plugin optimizado de transmisión simultánea para OBS Studio.</p>"
        "<p><b>Plataformas soportadas:</b> TikTok, YouTube, Instagram, Facebook y X.</p>"
        "<p><b>Tecnología clave:</b> Pipeline de codificación única (Shared Encoder Architecture) "
        "diseñado para minimizar la carga de CPU y GPU al máximo.</p>"
        "<p style='color: #888;'>Desarrollado para: <b>La Cueva del Dolor</b></p>",
        this
    );
    aboutDesc->setWordWrap(true);

    aboutLayout->addWidget(aboutTitle);
    aboutLayout->addWidget(aboutDesc);
    aboutLayout->addStretch();

    tabWidget_->addTab(tabAbout, "Acerca de");

    rootLayout->addWidget(tabWidget_);

    // Botones Guardar / Cancelar
    auto *btnsLayout = new QHBoxLayout();
    saveBtn_ = new QPushButton("Guardar", this);
    saveBtn_->setStyleSheet("background-color: #27ae60; color: white; padding: 6px 16px; font-weight: bold; border-radius: 4px;");

    cancelBtn_ = new QPushButton("Cancelar", this);
    cancelBtn_->setStyleSheet("background-color: #7f8c8d; color: white; padding: 6px 16px; border-radius: 4px;");

    btnsLayout->addStretch();
    btnsLayout->addWidget(cancelBtn_);
    btnsLayout->addWidget(saveBtn_);
    rootLayout->addLayout(btnsLayout);

    connect(saveBtn_, &QPushButton::clicked, this, &SettingsDialog::onSaveClicked);
    connect(cancelBtn_, &QPushButton::clicked, this, &QDialog::reject);
}

void SettingsDialog::onSaveClicked() {
    accept();
}

} // namespace CuevaMultiStream
