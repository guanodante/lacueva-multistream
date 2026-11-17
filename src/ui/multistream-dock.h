/**
 * @file multistream-dock.h
 * @brief Panel dock principal integrado en la ventana de OBS Studio.
 *
 * Proyecto: La Cueva del Dolor - OBS MultiStream Plugin
 * @copyright 2026 La Cueva del Dolor
 */

#pragma once

#include <QDockWidget>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QScrollArea>
#include <QTabWidget>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <vector>
#include <unordered_map>
#include "status-widget.h"
#include "vertical-program-widget.h"
#include "../core/multi-output-manager.h"
#include "../core/settings-manager.h"
#include "../platforms/resource-monitor.h"
#include "../platforms/bandwidth-optimizer.h"

namespace CuevaMultiStream {

class MultiStreamDock : public QDockWidget {
    Q_OBJECT

public:
    explicit MultiStreamDock(QWidget *parent = nullptr);
    ~MultiStreamDock() override;

private slots:
    void onStartAllClicked();
    void onStopAllClicked();
    void onOpenSettingsClicked();
    void onPlatformConfigChanged(const PlatformConfig &cfg);
    void onStartPlatformRequested(const std::string &platformId);
    void onStopPlatformRequested(const std::string &platformId);
    void onTestPlatformRequested(const std::string &platformId);
    void onTelemetryTimerTick();
    void onMasterFormatChanged(int index);
    void onStartVerticalRequested();
    void onStopVerticalRequested();

private:
    void setupUI();
    void loadPlatforms();
    void updateResourceBar();

    QTabWidget *tabWidget_ = nullptr;
    VerticalProgramWidget *verticalWidget_ = nullptr;
    QComboBox *masterFormatCombo_ = nullptr;

    QVBoxLayout *cardsContainer_ = nullptr;
    QPushButton *startAllBtn_ = nullptr;
    QPushButton *stopAllBtn_ = nullptr;
    QPushButton *settingsBtn_ = nullptr;

    QLabel *cpuLabel_ = nullptr;
    QLabel *ramLabel_ = nullptr;
    QLabel *networkLabel_ = nullptr;
    QProgressBar *bandwidthBar_ = nullptr;

    QTimer *telemetryTimer_ = nullptr;
    std::unordered_map<std::string, StatusWidget *> platformWidgets_;
};

} // namespace CuevaMultiStream
