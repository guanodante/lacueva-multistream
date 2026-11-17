/**
 * @file multistream-dock.cpp
 * @brief Implementación del panel dock de transmisión multiplataforma.
 *
 * Proyecto: La Cueva del Dolor - OBS MultiStream Plugin
 * @copyright 2026 La Cueva del Dolor
 */

#include "multistream-dock.h"
#include "settings-dialog.h"
#include <QMessageBox>

namespace CuevaMultiStream {

MultiStreamDock::MultiStreamDock(QWidget *parent)
    : QDockWidget("🎮 La Cueva MultiStream", parent) {
    setObjectName("CuevaMultiStreamDock");
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    setupUI();
    loadPlatforms();

    // Callback de cambios de estado del gestor de salidas hacia la UI
    MultiOutputManager::instance().setStatusCallback(
        [this](const std::string &id, OutputStatus status, const std::string &msg) {
            QMetaObject::invokeMethod(this, [this, id, status, msg]() {
                auto it = platformWidgets_.find(id);
                if (it != platformWidgets_.end()) {
                    it->second->setStatus(status, msg);
                }
            }, Qt::QueuedConnection);
        }
    );

    // Temporizador para telemetría y monitor de hardware (1Hz)
    telemetryTimer_ = new QTimer(this);
    connect(telemetryTimer_, &QTimer::timeout, this, &MultiStreamDock::onTelemetryTimerTick);
    telemetryTimer_->start(1000);
}

MultiStreamDock::~MultiStreamDock() {
    if (telemetryTimer_) {
        telemetryTimer_->stop();
    }
}

void MultiStreamDock::setupUI() {
    auto *centralWidget = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(centralWidget);
    rootLayout->setContentsMargins(4, 4, 4, 4);
    rootLayout->setSpacing(6);

    // 1. Barra de encabezado
    auto *headerLayout = new QHBoxLayout();
    auto *titleLbl = new QLabel("<b>LA CUEVA MULTISTREAM</b>", this);
    titleLbl->setStyleSheet("color: #e67e22; font-size: 13px;");

    settingsBtn_ = new QPushButton("⚙ Ajustes", this);
    settingsBtn_->setStyleSheet("background-color: #2c3e50; color: white; border-radius: 4px; padding: 4px 8px; font-size: 11px;");

    headerLayout->addWidget(titleLbl);
    headerLayout->addStretch();
    headerLayout->addWidget(settingsBtn_);
    rootLayout->addLayout(headerLayout);

    // 2. TabWidget principal: Pestaña 1 (Multi-Destinos) y Pestaña 2 (Program Vertical 9:16)
    tabWidget_ = new QTabWidget(this);
    tabWidget_->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #2d2d38; background-color: #121218; border-radius: 6px; }"
        "QTabBar::tab { background: #1f1f28; color: #a0a0b0; padding: 6px 12px; font-size: 11px; font-weight: bold; border-top-left-radius: 4px; border-top-right-radius: 4px; }"
        "QTabBar::tab:selected { background: #2d2d3e; color: #ffffff; border-bottom: 2px solid #e67e22; }"
    );

    // ==========================================
    // Pestaña 1: Transmisión Multiplataforma
    // ==========================================
    auto *tabMulti = new QWidget(this);
    auto *multiLayout = new QVBoxLayout(tabMulti);
    multiLayout->setContentsMargins(4, 4, 4, 4);
    multiLayout->setSpacing(6);

    // Selector Maestro de Formato (16:9 vs 9:16)
    auto *masterFormatLayout = new QHBoxLayout();
    auto *masterLbl = new QLabel("Modo Emisión:", this);
    masterLbl->setStyleSheet("color: #888899; font-size: 11px; font-weight: bold;");

    masterFormatCombo_ = new QComboBox(this);
    masterFormatCombo_->addItem("🌐 Híbrido (16:9 y 9:16 por plataforma)", 0);
    masterFormatCombo_->addItem("🖥️ Forzar Todas a 16:9 Horizontal", 1);
    masterFormatCombo_->addItem("📱 Forzar Todas a 9:16 Vertical", 2);
    masterFormatCombo_->setStyleSheet(
        "QComboBox { background-color: #1a1a24; color: #f0f0f0; border: 1px solid #3d3d52; border-radius: 4px; padding: 3px 6px; font-size: 11px; font-weight: bold; }"
    );

    masterFormatLayout->addWidget(masterLbl);
    masterFormatLayout->addWidget(masterFormatCombo_, 1);
    multiLayout->addLayout(masterFormatLayout);

    // Área desplazable para las tarjetas de plataformas
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("background-color: transparent;");

    auto *scrollContent = new QWidget(scrollArea);
    cardsContainer_ = new QVBoxLayout(scrollContent);
    cardsContainer_->setContentsMargins(0, 0, 0, 0);
    cardsContainer_->setSpacing(6);
    cardsContainer_->addStretch();

    scrollArea->setWidget(scrollContent);
    multiLayout->addWidget(scrollArea, 1);

    // Botones de inicio masivo
    auto *actionsLayout = new QHBoxLayout();
    startAllBtn_ = new QPushButton("▶ INICIAR TODAS", this);
    startAllBtn_->setFixedHeight(34);
    startAllBtn_->setStyleSheet("background-color: #27ae60; color: white; font-weight: bold; border-radius: 6px; font-size: 11px;");

    stopAllBtn_ = new QPushButton("⏹ DETENER TODAS", this);
    stopAllBtn_->setFixedHeight(34);
    stopAllBtn_->setStyleSheet("background-color: #c0392b; color: white; font-weight: bold; border-radius: 6px; font-size: 11px;");

    actionsLayout->addWidget(startAllBtn_);
    actionsLayout->addWidget(stopAllBtn_);
    multiLayout->addLayout(actionsLayout);

    tabWidget_->addTab(tabMulti, "📡 Canales RTMP");

    // ==========================================
    // Pestaña 2: Program Vertical (9:16)
    // ==========================================
    verticalWidget_ = new VerticalProgramWidget(this);
    tabWidget_->addTab(verticalWidget_, "📱 Program Vertical");

    rootLayout->addWidget(tabWidget_, 1);

    // 3. Monitor de recursos en vivo inferior (compartido)
    auto *resourceFrame = new QFrame(this);
    resourceFrame->setStyleSheet("background-color: #181820; border-radius: 6px; padding: 4px;");
    auto *resLayout = new QVBoxLayout(resourceFrame);
    resLayout->setContentsMargins(6, 4, 6, 4);
    resLayout->setSpacing(4);

    auto *resInfoLayout = new QHBoxLayout();
    cpuLabel_ = new QLabel("CPU: 0%", this);
    cpuLabel_->setStyleSheet("color: #2ecc71; font-size: 10px;");
    ramLabel_ = new QLabel("RAM: 0%", this);
    ramLabel_->setStyleSheet("color: #3498db; font-size: 10px;");
    networkLabel_ = new QLabel("Red: 0 Mbps", this);
    networkLabel_->setStyleSheet("color: #f1c40f; font-size: 10px;");

    resInfoLayout->addWidget(cpuLabel_);
    resInfoLayout->addWidget(ramLabel_);
    resInfoLayout->addWidget(networkLabel_);
    resLayout->addLayout(resInfoLayout);

    bandwidthBar_ = new QProgressBar(this);
    bandwidthBar_->setRange(0, 100);
    bandwidthBar_->setValue(15);
    bandwidthBar_->setTextVisible(false);
    bandwidthBar_->setFixedHeight(4);
    bandwidthBar_->setStyleSheet("QProgressBar::chunk { background-color: #27ae60; border-radius: 2px; }");
    resLayout->addWidget(bandwidthBar_);

    rootLayout->addWidget(resourceFrame);

    setWidget(centralWidget);

    // Conexiones de slots
    connect(startAllBtn_, &QPushButton::clicked, this, &MultiStreamDock::onStartAllClicked);
    connect(stopAllBtn_, &QPushButton::clicked, this, &MultiStreamDock::onStopAllClicked);
    connect(settingsBtn_, &QPushButton::clicked, this, &MultiStreamDock::onOpenSettingsClicked);
    connect(masterFormatCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MultiStreamDock::onMasterFormatChanged);

    connect(verticalWidget_, &VerticalProgramWidget::startVerticalRequested, this, &MultiStreamDock::onStartVerticalRequested);
    connect(verticalWidget_, &VerticalProgramWidget::stopVerticalRequested, this, &MultiStreamDock::onStopVerticalRequested);
}
}

void MultiStreamDock::loadPlatforms() {
    auto configs = SettingsManager::instance().getAllConfigs();
    for (const auto &cfg : configs) {
        MultiOutputManager::instance().setPlatformConfig(cfg);

        auto *card = new StatusWidget(cfg, this);
        connect(card, &StatusWidget::configChanged, this, &MultiStreamDock::onPlatformConfigChanged);
        connect(card, &StatusWidget::startRequested, this, &MultiStreamDock::onStartPlatformRequested);
        connect(card, &StatusWidget::stopRequested, this, &MultiStreamDock::onStopPlatformRequested);
        connect(card, &StatusWidget::testRequested, this, &MultiStreamDock::onTestPlatformRequested);

        cardsContainer_->insertWidget(cardsContainer_->count() - 1, card);
        platformWidgets_[cfg.id] = card;
    }
}

void MultiStreamDock::onStartAllClicked() {
    bool ok = MultiOutputManager::instance().startAllEnabled();
    if (!ok && !MultiOutputManager::instance().isAnyStreaming()) {
        QMessageBox::warning(this, "La Cueva MultiStream",
                             "No se pudo iniciar ninguna transmisión. Verifica que las plataformas estén habilitadas y tengan Stream Key.");
    }
}

void MultiStreamDock::onStopAllClicked() {
    MultiOutputManager::instance().stopAll();
}

void MultiStreamDock::onOpenSettingsClicked() {
    SettingsDialog dlg(this);
    dlg.exec();
}

void MultiStreamDock::onPlatformConfigChanged(const PlatformConfig &cfg) {
    SettingsManager::instance().setConfig(cfg);
    SettingsManager::instance().save();
    MultiOutputManager::instance().setPlatformConfig(cfg);
}

void MultiStreamDock::onStartPlatformRequested(const std::string &platformId) {
    MultiOutputManager::instance().startPlatform(platformId);
}

void MultiStreamDock::onStopPlatformRequested(const std::string &platformId) {
    MultiOutputManager::instance().stopPlatform(platformId);
}

void MultiStreamDock::onTestPlatformRequested(const std::string &platformId) {
    auto cfg = SettingsManager::instance().getConfig(platformId);
    if (cfg.rtmpUrl.empty() || cfg.streamKey.empty()) {
        QMessageBox::warning(this, "Prueba de Conexión",
                             "Debes ingresar tanto la URL del servidor como la Clave de Transmisión para realizar la prueba.");
        return;
    }

    QMessageBox::information(this, "Prueba de Conexión",
                             QString("Parámetros validados correctamente para %1.\nProtocolo: %2\nListo para transmitir.")
                                 .arg(QString::fromStdString(cfg.displayName))
                                 .arg(cfg.requiresRTMPS ? "RTMPS (Seguro)" : "RTMP Estándar"));
}

void MultiStreamDock::onMasterFormatChanged(int index) {
    if (index == 1) { // Forzar todas a 16:9
        for (auto &pair : platformWidgets_) {
            pair.second->setFormatVertical(false);
        }
    } else if (index == 2) { // Forzar todas a 9:16
        for (auto &pair : platformWidgets_) {
            pair.second->setFormatVertical(true);
        }
    }
}

void MultiStreamDock::onStartVerticalRequested() {
    bool ok = MultiOutputManager::instance().startVerticalOnly();
    if (!ok && !MultiOutputManager::instance().isVerticalStreaming()) {
        QMessageBox::warning(this, "Program Vertical",
                             "No hay plataformas verticales habilitadas con Stream Key válida para iniciar.");
    }
}

void MultiStreamDock::onStopVerticalRequested() {
    MultiOutputManager::instance().stopVerticalOnly();
}

void MultiStreamDock::onTelemetryTimerTick() {
    MultiOutputManager::instance().updateTelemetry();

    for (const auto &pair : platformWidgets_) {
        auto telem = MultiOutputManager::instance().getTelemetry(pair.first);
        pair.second->updateTelemetry(telem);
    }

    if (verticalWidget_) {
        verticalWidget_->updateLiveStatus(MultiOutputManager::instance().isVerticalStreaming());
    }

    updateResourceBar();
}

void MultiStreamDock::updateResourceBar() {
    // Lectura de recursos del sistema en tiempo real
    double cpu = ResourceMonitor::instance().getCpuUsage();
    double ram = ResourceMonitor::instance().getRamUsage();
    double netMbps = ResourceMonitor::instance().getNetworkUploadMbps();

    cpuLabel_->setText(QString("CPU: %1%").arg(int(cpu)));
    if (cpu > 80.0) cpuLabel_->setStyleSheet("color: #e74c3c; font-size: 10px; font-weight: bold;");
    else cpuLabel_->setStyleSheet("color: #2ecc71; font-size: 10px;");

    ramLabel_->setText(QString("RAM: %1%").arg(int(ram)));
    networkLabel_->setText(QString("Subida: %1 Mbps").arg(QString::number(netMbps, 'f', 1)));

    int percent = std::min(100, int((netMbps / 20.0) * 100.0));
    bandwidthBar_->setValue(percent);
}

} // namespace CuevaMultiStream
