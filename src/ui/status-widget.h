/**
 * @file status-widget.h
 * @brief Widget gráfico compacto que representa el estado individual de una plataforma.
 *
 * Proyecto: La Cueva del Dolor - OBS MultiStream Plugin
 * @copyright 2026 La Cueva del Dolor
 */

#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QComboBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTimer>
#include "../core/platform-config.h"
#include "../core/multi-output-manager.h"

namespace CuevaMultiStream {

class StatusWidget : public QWidget {
    Q_OBJECT

public:
    explicit StatusWidget(const PlatformConfig &cfg, QWidget *parent = nullptr);
    ~StatusWidget() override = default;

    PlatformConfig getConfig() const;
    void updateTelemetry(const OutputTelemetry &telemetry);
    void setStatus(OutputStatus status, const std::string &msg);
    void setFormatVertical(bool isVertical);

signals:
    void configChanged(const PlatformConfig &cfg);
    void startRequested(const std::string &platformId);
    void stopRequested(const std::string &platformId);
    void testRequested(const std::string &platformId);

private slots:
    void onToggleStreamClicked();
    void onKeyVisibilityToggled();
    void onFieldsEdited();
    void onFormatChanged(int index);

private:
    void setupUI();
    void updateStatusIndicator(OutputStatus status);
    QString getPlatformEmoji() const;

    PlatformConfig config_;
    OutputStatus currentStatus_ = OutputStatus::Stopped;

    QCheckBox *enableCheck_ = nullptr;
    QLabel *platformIcon_ = nullptr;
    QLabel *nameLabel_ = nullptr;
    QLabel *statusBadge_ = nullptr;
    QComboBox *formatCombo_ = nullptr;
    QLineEdit *serverInput_ = nullptr;
    QLineEdit *keyInput_ = nullptr;
    QPushButton *showKeyBtn_ = nullptr;
    QPushButton *testBtn_ = nullptr;
    QPushButton *toggleStreamBtn_ = nullptr;
    QLabel *metricsLabel_ = nullptr;
};

} // namespace CuevaMultiStream
