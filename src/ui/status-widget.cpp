/**
 * @file status-widget.cpp
 * @brief Implementación del widget visual de estado por plataforma.
 *
 * Proyecto: La Cueva del Dolor - OBS MultiStream Plugin
 * @copyright 2026 La Cueva del Dolor
 */

#include "status-widget.h"
#include <QIcon>
#include <QFrame>

namespace CuevaMultiStream {

StatusWidget::StatusWidget(const PlatformConfig &cfg, QWidget *parent)
    : QWidget(parent), config_(cfg) {
    setupUI();
    setStatus(OutputStatus::Stopped, "Desconectado");
}

QString StatusWidget::getPlatformEmoji() const {
    switch (config_.type) {
    case PlatformType::YouTube:   return "🔴";
    case PlatformType::Facebook:  return "🔵";
    case PlatformType::TikTok:    return "🎵";
    case PlatformType::Instagram: return "📸";
    case PlatformType::Twitter:   return "🐦";
    default:                      return "🌐";
    }
}

void StatusWidget::setupUI() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // Marco contenedor estilizado
    auto *frame = new QFrame(this);
    frame->setObjectName("platformCard");
    frame->setStyleSheet(
        "#platformCard {"
        "  background-color: #1e1e24;"
        "  border: 1px solid #2d2d38;"
        "  border-radius: 8px;"
        "  padding: 6px;"
        "}"
        "#platformCard:hover {"
        "  border: 1px solid #4a4a5a;"
        "}"
    );

    auto *cardLayout = new QVBoxLayout(frame);
    cardLayout->setContentsMargins(10, 8, 10, 8);
    cardLayout->setSpacing(8);

    // Fila superior: Checkbox + Emoji + Nombre + Badge de estado + Botón On/Off individual
    auto *headerLayout = new QHBoxLayout();
    enableCheck_ = new QCheckBox(this);
    enableCheck_->setChecked(config_.enabled);
    enableCheck_->setToolTip("Habilitar esta plataforma para el inicio masivo");

    platformIcon_ = new QLabel(getPlatformEmoji(), this);
    QFont iconFont = platformIcon_->font();
    iconFont.setPointSize(14);
    platformIcon_->setFont(iconFont);

    nameLabel_ = new QLabel(QString::fromStdString(config_.displayName), this);
    QFont nameFont = nameLabel_->font();
    nameFont.setBold(true);
    nameFont.setPointSize(10);
    nameLabel_->setFont(nameFont);

    statusBadge_ = new QLabel("⚪ DETENIDO", this);
    statusBadge_->setStyleSheet("background-color: #2b2b36; color: #a0a0b0; border-radius: 4px; padding: 2px 6px; font-size: 11px;");

    toggleStreamBtn_ = new QPushButton("▶ Iniciar", this);
    toggleStreamBtn_->setFixedWidth(80);
    toggleStreamBtn_->setStyleSheet("background-color: #27ae60; color: white; border-radius: 4px; padding: 4px; font-weight: bold;");

    headerLayout->addWidget(enableCheck_);
    headerLayout->addWidget(platformIcon_);
    headerLayout->addWidget(nameLabel_);
    headerLayout->addStretch();
    headerLayout->addWidget(statusBadge_);
    headerLayout->addWidget(toggleStreamBtn_);

    cardLayout->addLayout(headerLayout);

    // Fila de URL de servidor
    auto *urlLayout = new QHBoxLayout();
    auto *urlLbl = new QLabel("URL:", this);
    urlLbl->setFixedWidth(35);
    urlLbl->setStyleSheet("color: #888899; font-size: 11px;");
    serverInput_ = new QLineEdit(QString::fromStdString(config_.rtmpUrl), this);
    serverInput_->setPlaceholderText("rtmp://servidor/live");
    serverInput_->setStyleSheet("background-color: #121216; color: #e0e0e0; border: 1px solid #333344; border-radius: 4px; padding: 4px; font-size: 11px;");

    urlLayout->addWidget(urlLbl);
    urlLayout->addWidget(serverInput_);
    cardLayout->addLayout(urlLayout);

    // Fila de Clave de Transmisión (con botón de Ver/Ocultar y botón Probar)
    auto *keyLayout = new QHBoxLayout();
    auto *keyLbl = new QLabel("Clave:", this);
    keyLbl->setFixedWidth(35);
    keyLbl->setStyleSheet("color: #888899; font-size: 11px;");

    keyInput_ = new QLineEdit(QString::fromStdString(config_.streamKey), this);
    keyInput_->setEchoMode(QLineEdit::Password);
    keyInput_->setPlaceholderText("Clave de transmisión (secreta)");
    keyInput_->setStyleSheet("background-color: #121216; color: #e0e0e0; border: 1px solid #333344; border-radius: 4px; padding: 4px; font-size: 11px;");

    showKeyBtn_ = new QPushButton("👁", this);
    showKeyBtn_->setFixedWidth(28);
    showKeyBtn_->setToolTip("Mostrar/Ocultar clave");
    showKeyBtn_->setStyleSheet("background-color: #2b2b36; border-radius: 4px;");

    testBtn_ = new QPushButton("Probar", this);
    testBtn_->setFixedWidth(60);
    testBtn_->setToolTip("Probar conexión con el servidor RTMP");
    testBtn_->setStyleSheet("background-color: #3d3d52; color: #ffffff; border-radius: 4px; font-size: 11px;");

    keyLayout->addWidget(keyLbl);
    keyLayout->addWidget(keyInput_);
    keyLayout->addWidget(showKeyBtn_);
    keyLayout->addWidget(testBtn_);
    cardLayout->addLayout(keyLayout);

    // Fila inferior: Métricas de telemetría (Bitrate actual, Dropped Frames)
    metricsLabel_ = new QLabel("Bitrate: 0 kbps | Pérdida: 0%", this);
    metricsLabel_->setStyleSheet("color: #777788; font-size: 10px;");
    cardLayout->addWidget(metricsLabel_);

    mainLayout->addWidget(frame);

    // Conexiones de eventos
    connect(enableCheck_, &QCheckBox::toggled, this, &StatusWidget::onFieldsEdited);
    connect(serverInput_, &QLineEdit::textChanged, this, &StatusWidget::onFieldsEdited);
    connect(keyInput_, &QLineEdit::textChanged, this, &StatusWidget::onFieldsEdited);
    connect(showKeyBtn_, &QPushButton::clicked, this, &StatusWidget::onKeyVisibilityToggled);
    connect(toggleStreamBtn_, &QPushButton::clicked, this, &StatusWidget::onToggleStreamClicked);
    connect(testBtn_, &QPushButton::clicked, this, [this]() {
        emit testRequested(config_.id);
    });
}

void StatusWidget::onKeyVisibilityToggled() {
    if (keyInput_->echoMode() == QLineEdit::Password) {
        keyInput_->setEchoMode(QLineEdit::Normal);
        showKeyBtn_->setText("🙈");
    } else {
        keyInput_->setEchoMode(QLineEdit::Password);
        showKeyBtn_->setText("👁");
    }
}

void StatusWidget::onFieldsEdited() {
    config_.enabled = enableCheck_->isChecked();
    config_.rtmpUrl = serverInput_->text().trimmed().toStdString();
    config_.streamKey = keyInput_->text().trimmed().toStdString();
    emit configChanged(config_);
}

void StatusWidget::onToggleStreamClicked() {
    if (currentStatus_ == OutputStatus::Active || currentStatus_ == OutputStatus::Starting) {
        emit stopRequested(config_.id);
    } else {
        emit startRequested(config_.id);
    }
}

void StatusWidget::setStatus(OutputStatus status, const std::string &msg) {
    currentStatus_ = status;
    updateStatusIndicator(status);

    if (status == OutputStatus::Active) {
        toggleStreamBtn_->setText("⏹ Detener");
        toggleStreamBtn_->setStyleSheet("background-color: #c0392b; color: white; border-radius: 4px; padding: 4px; font-weight: bold;");
    } else if (status == OutputStatus::Starting) {
        toggleStreamBtn_->setText("⏳ Conectando...");
        toggleStreamBtn_->setStyleSheet("background-color: #f39c12; color: white; border-radius: 4px; padding: 4px; font-weight: bold;");
    } else {
        toggleStreamBtn_->setText("▶ Iniciar");
        toggleStreamBtn_->setStyleSheet("background-color: #27ae60; color: white; border-radius: 4px; padding: 4px; font-weight: bold;");
    }

    if (!msg.empty() && status == OutputStatus::Failed) {
        metricsLabel_->setText(QString("Error: %1").arg(QString::fromStdString(msg)));
        metricsLabel_->setStyleSheet("color: #e74c3c; font-size: 10px; font-weight: bold;");
    }
}

void StatusWidget::updateStatusIndicator(OutputStatus status) {
    switch (status) {
    case OutputStatus::Active:
        statusBadge_->setText("🟢 EN VIVO");
        statusBadge_->setStyleSheet("background-color: #1b4332; color: #2ecc71; border-radius: 4px; padding: 2px 6px; font-size: 11px; font-weight: bold;");
        break;
    case OutputStatus::Starting:
    case OutputStatus::Reconnecting:
        statusBadge_->setText("🟡 CONECTANDO");
        statusBadge_->setStyleSheet("background-color: #4a3e1b; color: #f1c40f; border-radius: 4px; padding: 2px 6px; font-size: 11px; font-weight: bold;");
        break;
    case OutputStatus::Failed:
        statusBadge_->setText("🔴 ERROR");
        statusBadge_->setStyleSheet("background-color: #4a1b1b; color: #e74c3c; border-radius: 4px; padding: 2px 6px; font-size: 11px; font-weight: bold;");
        break;
    case OutputStatus::Stopped:
    default:
        statusBadge_->setText("⚪ DETENIDO");
        statusBadge_->setStyleSheet("background-color: #2b2b36; color: #a0a0b0; border-radius: 4px; padding: 2px 6px; font-size: 11px;");
        break;
    }
}

void StatusWidget::updateTelemetry(const OutputTelemetry &telemetry) {
    double dropPercent = (telemetry.totalFrames > 0)
        ? (double(telemetry.droppedFrames) / double(telemetry.totalFrames)) * 100.0
        : 0.0;

    metricsLabel_->setText(
        QString("Bitrate: %1 kbps | Pérdida: %2% (%3 frames)")
            .arg(config_.videoBitrate)
            .arg(QString::number(dropPercent, 'f', 1))
            .arg(telemetry.droppedFrames)
    );
    metricsLabel_->setStyleSheet("color: #777788; font-size: 10px;");
}

PlatformConfig StatusWidget::getConfig() const {
    return config_;
}

} // namespace CuevaMultiStream
