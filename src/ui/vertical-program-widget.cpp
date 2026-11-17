/**
 * @file vertical-program-widget.cpp
 * @brief Implementación del monitor y controlador de Program Vertical (9:16).
 *
 * Proyecto: La Cueva del Dolor - OBS MultiStream Plugin
 * @copyright 2026 La Cueva del Dolor
 */

#include "vertical-program-widget.h"
#include <QPainter>
#include <QPainterPath>
#include <obs-frontend-api.h>
#include <util/base.h>

namespace CuevaMultiStream {

// ============================================================================
// VerticalScreenPreview: Dibujado de monitor simulador 9:16 tipo smartphone
// ============================================================================

VerticalScreenPreview::VerticalScreenPreview(QWidget *parent)
    : QFrame(parent) {
    setMinimumSize(180, 320);
    setMaximumSize(240, 426); // Relación de aspecto ~9:16
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setStyleSheet("background-color: transparent; border: none;");
}

void VerticalScreenPreview::setActiveScene(const QString &sceneName) {
    currentSceneName_ = sceneName;
    update();
}

void VerticalScreenPreview::setLiveState(bool isLive) {
    isLive_ = isLive;
    update();
}

void VerticalScreenPreview::setShowSafeZones(bool show) {
    showSafeZones_ = show;
    update();
}

void VerticalScreenPreview::setCaptureMode(const QString &mode) {
    currentCaptureMode_ = mode;
    update();
}

void VerticalScreenPreview::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRect rect = this->rect().adjusted(4, 4, -4, -4);

    // 1. Chasis del smartphone con borde Tally
    QPainterPath bodyPath;
    bodyPath.addRoundedRect(rect, 14, 14);

    // Color del chasis según estado de transmisión
    QColor borderColor = isLive_ ? QColor("#e74c3c") : QColor("#27ae60");
    p.setPen(QPen(borderColor, isLive_ ? 3 : 2));
    p.setBrush(QColor("#0d0d12"));
    p.drawPath(bodyPath);

    // 2. Isla Dinámica / Cámara superior
    int notchWidth = 50;
    int notchHeight = 12;
    QRect notchRect(rect.center().x() - (notchWidth / 2), rect.top() + 6, notchWidth, notchHeight);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#1f1f28"));
    p.drawRoundedRect(notchRect, 6, 6);

    // Cámara lente frontal pequeña
    p.setBrush(QColor("#0a0a0f"));
    p.drawEllipse(notchRect.right() - 14, notchRect.center().y() - 3, 6, 6);

    // 3. Área interna de video 9:16
    QRect videoRect = rect.adjusted(4, 22, -4, -14);
    p.setBrush(QColor("#15151e"));
    p.setPen(QPen(QColor("#252535"), 1));
    p.drawRect(videoRect);

    // Dibujar patrón suave de cuadrícula para representar el lienzo
    p.setPen(QPen(QColor(255, 255, 255, 12), 1, Qt::DotLine));
    int midX = videoRect.center().x();
    int midY = videoRect.center().y();
    p.drawLine(midX, videoRect.top(), midX, videoRect.bottom());
    p.drawLine(videoRect.left(), midY, videoRect.right(), midY);

    // 4. Guías de zona segura de TikTok / Instagram (Safe Zones)
    if (showSafeZones_) {
        // Zona de botones interactivos derecha (Likes, comentarios, compartir)
        int btnW = 34;
        int btnH = 110;
        QRect rightUiRect(videoRect.right() - btnW - 4, videoRect.bottom() - btnH - 45, btnW, btnH);
        p.setPen(QPen(QColor(241, 196, 15, 100), 1, Qt::DashLine));
        p.setBrush(QColor(241, 196, 15, 25));
        p.drawRoundedRect(rightUiRect, 4, 4);

        p.setFont(QFont("Segoe UI", 6));
        p.setPen(QColor(241, 196, 15, 180));
        p.drawText(rightUiRect, Qt::AlignCenter, "BOTONES\nTIKTOK");

        // Zona inferior de comentarios y descripción
        int bottomH = 40;
        QRect bottomUiRect(videoRect.left() + 4, videoRect.bottom() - bottomH - 4, videoRect.width() - 8, bottomH);
        p.setPen(QPen(QColor(231, 76, 60, 100), 1, Qt::DashLine));
        p.setBrush(QColor(231, 76, 60, 25));
        p.drawRoundedRect(bottomUiRect, 4, 4);
        p.drawText(bottomUiRect, Qt::AlignCenter, "CHAT & DESCRIPCIÓN");
    }

    // 5. Etiqueta central de la escena activa
    QRect textRect(videoRect.left() + 6, midY - 30, videoRect.width() - 12, 60);
    p.setPen(QColor("#ffffff"));
    p.setFont(QFont("Segoe UI", 9, QFont::Bold));
    p.drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, currentSceneName_);

    // Modo de captura
    QRect modeRect(videoRect.left() + 6, midY + 30, videoRect.width() - 12, 20);
    p.setPen(QColor("#a0a0b8"));
    p.setFont(QFont("Segoe UI", 7));
    p.drawText(modeRect, Qt::AlignCenter, "[" + currentCaptureMode_ + "]");

    // 6. Insignia Tally de estado (Arriba a la izquierda de la pantalla)
    QRect badgeRect(videoRect.left() + 6, videoRect.top() + 6, 75, 16);
    p.setPen(Qt::NoPen);
    p.setBrush(isLive_ ? QColor("#e74c3c") : QColor("#27ae60"));
    p.drawRoundedRect(badgeRect, 3, 3);

    p.setPen(QColor("#ffffff"));
    p.setFont(QFont("Segoe UI", 7, QFont::Bold));
    p.drawText(badgeRect, Qt::AlignCenter, isLive_ ? "● EN VIVO" : "LISTO");

    // Indicador 9:16 arriba a la derecha
    QRect formatRect(videoRect.right() - 40, videoRect.top() + 6, 35, 16);
    p.setBrush(QColor("#8e44ad"));
    p.drawRoundedRect(formatRect, 3, 3);
    p.drawText(formatRect, Qt::AlignCenter, "9:16");
}

// ============================================================================
// VerticalProgramWidget: Panel de producción y controlador de escenas
// ============================================================================

VerticalProgramWidget::VerticalProgramWidget(QWidget *parent)
    : QWidget(parent) {
    setupUI();
    refreshSceneList();
}

void VerticalProgramWidget::setupUI() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(6, 6, 6, 6);
    mainLayout->setSpacing(8);

    // Contenedor principal estilizado
    auto *box = new QFrame(this);
    box->setStyleSheet("background-color: #1a1a24; border-radius: 8px; padding: 6px;");
    auto *boxLayout = new QVBoxLayout(box);
    boxLayout->setContentsMargins(8, 8, 8, 8);
    boxLayout->setSpacing(8);

    // 1. Título y estado
    auto *headerLayout = new QHBoxLayout();
    auto *title = new QLabel("📱 <b>PROGRAM VERTICAL (9:16)</b>", this);
    title->setStyleSheet("color: #9b59b6; font-size: 12px;");

    liveBadge_ = new QLabel("⚪ EN ESPERA", this);
    liveBadge_->setStyleSheet("background-color: #2b2b36; color: #a0a0b0; border-radius: 4px; padding: 2px 6px; font-size: 10px; font-weight: bold;");

    headerLayout->addWidget(title);
    headerLayout->addStretch();
    headerLayout->addWidget(liveBadge_);
    boxLayout->addLayout(headerLayout);

    // 2. Monitor Program Vertical Centrado
    auto *previewCenterLayout = new QHBoxLayout();
    previewCenterLayout->addStretch();
    screenPreview_ = new VerticalScreenPreview(this);
    previewCenterLayout->addWidget(screenPreview_);
    previewCenterLayout->addStretch();
    boxLayout->addLayout(previewCenterLayout);

    // 3. Selector de Escena para Vertical con botón de refrescar
    auto *sceneLayout = new QHBoxLayout();
    auto *sceneLbl = new QLabel("Escena:", this);
    sceneLbl->setStyleSheet("color: #a0a0b0; font-size: 11px; font-weight: bold;");
    sceneLbl->setFixedWidth(50);

    sceneCombo_ = new QComboBox(this);
    sceneCombo_->setStyleSheet(
        "QComboBox { background-color: #121218; color: #ffffff; border: 1px solid #333348; border-radius: 4px; padding: 4px; font-size: 11px; }"
        "QComboBox::drop-down { border: none; }"
    );

    refreshBtn_ = new QPushButton("🔄", this);
    refreshBtn_->setToolTip("Recargar lista de escenas de OBS Studio");
    refreshBtn_->setFixedWidth(28);
    refreshBtn_->setStyleSheet("background-color: #2c3e50; border-radius: 4px; color: white;");

    sceneLayout->addWidget(sceneLbl);
    sceneLayout->addWidget(sceneCombo_, 1);
    sceneLayout->addWidget(refreshBtn_);
    boxLayout->addLayout(sceneLayout);

    // 4. Modos de Captura / Encuadre
    auto *captureLayout = new QHBoxLayout();
    auto *capLbl = new QLabel("Encuadre:", this);
    capLbl->setStyleSheet("color: #a0a0b0; font-size: 11px; font-weight: bold;");
    capLbl->setFixedWidth(50);

    captureModeCombo_ = new QComboBox(this);
    captureModeCombo_->addItem("📱 Escena Vertical Nativa (1080x1920)");
    captureModeCombo_->addItem("🔍 Recorte 9:16 Centro (Smart Crop)");
    captureModeCombo_->addItem("👥 Split (Cámara arriba + Pantalla)");
    captureModeCombo_->addItem("🎨 Fondo Desenfoque + Juego Centrado");
    captureModeCombo_->setStyleSheet(
        "QComboBox { background-color: #121218; color: #ffffff; border: 1px solid #333348; border-radius: 4px; padding: 4px; font-size: 11px; }"
    );

    captureLayout->addWidget(capLbl);
    captureLayout->addWidget(captureModeCombo_, 1);
    boxLayout->addLayout(captureLayout);

    // 5. Botones de Escenas Rápidas
    auto *quickLabel = new QLabel("Escenas Rápidas:", this);
    quickLabel->setStyleSheet("color: #888899; font-size: 10px;");
    boxLayout->addWidget(quickLabel);

    auto *quickLayout = new QHBoxLayout();
    quickGameBtn_ = new QPushButton("🎮 Juego", this);
    quickChatBtn_ = new QPushButton("🎙️ Charla", this);
    quickBrbBtn_ = new QPushButton("⏸️ Pausa", this);

    QString quickStyle = "QPushButton { background-color: #282836; color: #e0e0e0; border-radius: 4px; padding: 4px; font-size: 10px; }"
                         "QPushButton:hover { background-color: #3b3b4f; }";
    quickGameBtn_->setStyleSheet(quickStyle);
    quickChatBtn_->setStyleSheet(quickStyle);
    quickBrbBtn_->setStyleSheet(quickStyle);

    quickLayout->addWidget(quickGameBtn_);
    quickLayout->addWidget(quickChatBtn_);
    quickLayout->addWidget(quickBrbBtn_);
    boxLayout->addLayout(quickLayout);

    // 6. Checkbox de Guías de Zonas Seguras (Safe Zones)
    safeZonesCheck_ = new QCheckBox("Mostrar Zonas Seguras de TikTok / Reels", this);
    safeZonesCheck_->setChecked(true);
    safeZonesCheck_->setStyleSheet("color: #a0a0b0; font-size: 11px;");
    boxLayout->addWidget(safeZonesCheck_);

    // 7. Botones de Transmisión Vertical Rápida
    auto *streamActionLayout = new QHBoxLayout();
    startVerticalBtn_ = new QPushButton("▶ TRANSMITIR EN VERTICAL", this);
    startVerticalBtn_->setFixedHeight(32);
    startVerticalBtn_->setStyleSheet("background-color: #8e44ad; color: white; font-weight: bold; border-radius: 5px; font-size: 11px;");

    stopVerticalBtn_ = new QPushButton("⏹ DETENER", this);
    stopVerticalBtn_->setFixedHeight(32);
    stopVerticalBtn_->setFixedWidth(75);
    stopVerticalBtn_->setStyleSheet("background-color: #c0392b; color: white; font-weight: bold; border-radius: 5px; font-size: 11px;");

    streamActionLayout->addWidget(startVerticalBtn_, 1);
    streamActionLayout->addWidget(stopVerticalBtn_);
    boxLayout->addLayout(streamActionLayout);

    mainLayout->addWidget(box);

    // Conexiones de señales
    connect(sceneCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &VerticalProgramWidget::onSceneSelected);
    connect(refreshBtn_, &QPushButton::clicked, this, &VerticalProgramWidget::onRefreshClicked);
    connect(captureModeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &VerticalProgramWidget::onCaptureModeChanged);
    connect(safeZonesCheck_, &QCheckBox::toggled, this, &VerticalProgramWidget::onSafeZonesToggled);

    connect(quickGameBtn_, &QPushButton::clicked, this, [this]() { onQuickSceneClicked("game"); });
    connect(quickChatBtn_, &QPushButton::clicked, this, [this]() { onQuickSceneClicked("charla"); });
    connect(quickBrbBtn_, &QPushButton::clicked, this, [this]() { onQuickSceneClicked("pausa"); });

    connect(startVerticalBtn_, &QPushButton::clicked, this, &VerticalProgramWidget::startVerticalRequested);
    connect(stopVerticalBtn_, &QPushButton::clicked, this, &VerticalProgramWidget::stopVerticalRequested);
}

void VerticalProgramWidget::refreshSceneList() {
    sceneCombo_->blockSignals(true);
    sceneCombo_->clear();

    // Obtener escenas activas de OBS Studio mediante la API oficial
    char **scenes = obs_frontend_get_scene_names();
    if (scenes) {
        char **current = scenes;
        while (*current) {
            sceneCombo_->addItem(QString::fromUtf8(*current));
            current++;
        }
        bfree(scenes);
    }

    if (sceneCombo_->count() == 0) {
        sceneCombo_->addItem("Escena Principal (16:9)");
        sceneCombo_->addItem("Escena Vertical 1 (9:16)");
        sceneCombo_->addItem("Escena Cámara Completa");
    }

    sceneCombo_->blockSignals(false);

    if (screenPreview_ && sceneCombo_->count() > 0) {
        screenPreview_->setActiveScene(sceneCombo_->currentText());
    }
}

void VerticalProgramWidget::updateLiveStatus(bool isLive) {
    if (screenPreview_) {
        screenPreview_->setLiveState(isLive);
    }
    if (liveBadge_) {
        if (isLive) {
            liveBadge_->setText("🔴 EN VIVO VERTICAL");
            liveBadge_->setStyleSheet("background-color: #c0392b; color: white; border-radius: 4px; padding: 2px 6px; font-size: 10px; font-weight: bold;");
        } else {
            liveBadge_->setText("⚪ EN ESPERA");
            liveBadge_->setStyleSheet("background-color: #2b2b36; color: #a0a0b0; border-radius: 4px; padding: 2px 6px; font-size: 10px; font-weight: bold;");
        }
    }
}

void VerticalProgramWidget::onSceneSelected(int index) {
    if (index >= 0 && screenPreview_) {
        QString name = sceneCombo_->itemText(index);
        screenPreview_->setActiveScene(name);

        // Cambiar la escena activa en OBS si el streamer la seleccionó
        obs_source_t *scene = obs_get_source_by_name(name.toUtf8().constData());
        if (scene) {
            obs_frontend_set_current_scene(scene);
            obs_source_release(scene);
        }
    }
}

void VerticalProgramWidget::onRefreshClicked() {
    refreshSceneList();
}

void VerticalProgramWidget::onSafeZonesToggled(bool checked) {
    if (screenPreview_) {
        screenPreview_->setShowSafeZones(checked);
    }
}

void VerticalProgramWidget::onCaptureModeChanged(int index) {
    if (screenPreview_ && index >= 0) {
        screenPreview_->setCaptureMode(captureModeCombo_->itemText(index));
    }
}

void VerticalProgramWidget::onQuickSceneClicked(const QString &keyword) {
    // Buscar la escena en el combobox que contenga la palabra clave
    for (int i = 0; i < sceneCombo_->count(); ++i) {
        if (sceneCombo_->itemText(i).contains(keyword, Qt::CaseInsensitive)) {
            sceneCombo_->setCurrentIndex(i);
            return;
        }
    }
    // Si no encuentra coincidencia exacta, cambia al siguiente índice
    if (sceneCombo_->count() > 0) {
        int next = (sceneCombo_->currentIndex() + 1) % sceneCombo_->count();
        sceneCombo_->setCurrentIndex(next);
    }
}

} // namespace CuevaMultiStream
