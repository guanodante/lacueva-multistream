/**
 * @file vertical-program-widget.h
 * @brief Monitor y panel de control de producción para Program Vertical (9:16).
 *
 * Proyecto: La Cueva del Dolor - OBS MultiStream Plugin
 * @copyright 2026 La Cueva del Dolor
 */

#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QTimer>

namespace CuevaMultiStream {

/**
 * @brief Widget que simula el monitor de Program en formato vertical (9:16) tipo smartphone.
 */
class VerticalScreenPreview : public QFrame {
    Q_OBJECT

public:
    explicit VerticalScreenPreview(QWidget *parent = nullptr);

    void setActiveScene(const QString &sceneName);
    void setLiveState(bool isLive);
    void setShowSafeZones(bool show);
    void setCaptureMode(const QString &mode);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString currentSceneName_ = "Escena Principal";
    QString currentCaptureMode_ = "Escena Vertical Nativa";
    bool isLive_ = false;
    bool showSafeZones_ = true;
};

/**
 * @brief Panel completo de control de escenas, capturas y monitor Program Vertical.
 */
class VerticalProgramWidget : public QWidget {
    Q_OBJECT

public:
    explicit VerticalProgramWidget(QWidget *parent = nullptr);
    ~VerticalProgramWidget() override = default;

    void refreshSceneList();
    void updateLiveStatus(bool isLive);

signals:
    void startVerticalRequested();
    void stopVerticalRequested();

private slots:
    void onSceneSelected(int index);
    void onRefreshClicked();
    void onSafeZonesToggled(bool checked);
    void onCaptureModeChanged(int index);
    void onQuickSceneClicked(const QString &keyword);

private:
    void setupUI();

    VerticalScreenPreview *screenPreview_ = nullptr;
    QComboBox *sceneCombo_ = nullptr;
    QComboBox *captureModeCombo_ = nullptr;
    QCheckBox *safeZonesCheck_ = nullptr;
    QLabel *liveBadge_ = nullptr;

    QPushButton *startVerticalBtn_ = nullptr;
    QPushButton *stopVerticalBtn_ = nullptr;
    QPushButton *refreshBtn_ = nullptr;

    QPushButton *quickGameBtn_ = nullptr;
    QPushButton *quickChatBtn_ = nullptr;
    QPushButton *quickBrbBtn_ = nullptr;
};

} // namespace CuevaMultiStream
