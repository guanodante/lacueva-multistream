/**
 * @file settings-dialog.h
 * @brief Diálogo modal de ajustes avanzados del plugin.
 *
 * Proyecto: La Cueva del Dolor - OBS MultiStream Plugin
 * @copyright 2026 La Cueva del Dolor
 */

#pragma once

#include <QDialog>
#include <QTabWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

namespace CuevaMultiStream {

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog() override = default;

private slots:
    void onSaveClicked();

private:
    void setupUI();

    QTabWidget *tabWidget_ = nullptr;
    QComboBox *encoderCombo_ = nullptr;
    QComboBox *qualityPresetCombo_ = nullptr;
    QSpinBox *bandwidthLimitSpin_ = nullptr;
    QCheckBox *autoDowngradeCheck_ = nullptr;
    QCheckBox *enableResourceGuardCheck_ = nullptr;

    QPushButton *saveBtn_ = nullptr;
    QPushButton *cancelBtn_ = nullptr;
};

} // namespace CuevaMultiStream
