#include "pwmdisplaywidget.h"
#include <QMessageBox>
#include <QFormLayout>
#include <QScrollArea>
#include <QTimer>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

PwmDisplayWidget::PwmDisplayWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(6);
    mainLayout->setContentsMargins(4, 4, 4, 4);

    QWidget *pwmWidget = new QWidget(this);
    QGridLayout *layout = new QGridLayout(pwmWidget);
    layout->setSpacing(4);
    layout->setContentsMargins(8, 8, 8, 8);

    QString chStyle = "color:#000;font-weight:bold;font-size:12px;";

    struct PwmPos { int index; int row; int col; };
    PwmPos positions[6] = {
        {0, 2, 1},
        {1, 1, 4},
        {2, 0, 3},
        {3, 1, 0},
        {4, 0, 1},
        {5, 2, 3}
    };

    for (int i = 0; i < 6; ++i) {
        int pwmIdx = positions[i].index;
        int row = positions[i].row;
        int col = positions[i].col;

        QLabel *chLabel = new QLabel(QString("PWM%1").arg(pwmIdx + 1), pwmWidget);
        chLabel->setAlignment(Qt::AlignCenter);
        chLabel->setStyleSheet(chStyle);
        layout->addWidget(chLabel, row * 2, col, Qt::AlignCenter);

        QLabel *valueLabel = new QLabel("1500", pwmWidget);
        valueLabel->setAlignment(Qt::AlignCenter);
        valueLabel->setStyleSheet("color:#000;font-weight:bold;font-size:16px;"
                                  "background:#FFF;border:2px solid #999;border-radius:4px;"
                                  "padding:4px 8px;min-width:60px;");
        layout->addWidget(valueLabel, row * 2 + 1, col, Qt::AlignCenter);

        m_valueLabels[pwmIdx] = valueLabel;

        QCheckBox *enCheckBox = new QCheckBox(tr("启用"), pwmWidget);
        enCheckBox->setStyleSheet("color:#000;font-weight:bold;font-size:10px;");
        enCheckBox->setChecked(true);
        layout->addWidget(enCheckBox, row * 2, col + 1, Qt::AlignLeft | Qt::AlignVCenter);

        m_enabledCheckBoxes[pwmIdx] = enCheckBox;

        connect(enCheckBox, &QCheckBox::toggled, this, [this, pwmIdx](bool checked) {
            onEnabledCheckboxToggled(pwmIdx, checked);
        });

        QCheckBox *revCheckBox = new QCheckBox(tr("反转"), pwmWidget);
        revCheckBox->setStyleSheet("color:#000;font-weight:bold;font-size:10px;");
        layout->addWidget(revCheckBox, row * 2 + 1, col + 1, Qt::AlignLeft | Qt::AlignVCenter);

        m_reverseCheckBoxes[pwmIdx] = revCheckBox;

        connect(revCheckBox, &QCheckBox::toggled, this, [this, pwmIdx](bool checked) {
            onReverseCheckboxToggled(pwmIdx, checked);
        });
    }

    mainLayout->addWidget(pwmWidget);

    QGroupBox *pidGroup = new QGroupBox(tr("PID 参数设置"), this);
    pidGroup->setStyleSheet(
        "QGroupBox{color:#000;font-weight:bold;font-size:12px;"
        "border:2px solid #666;border-radius:4px;margin-top:8px;padding-top:12px;}"
        "QGroupBox::title{subcontrol-origin:margin;left:10px;padding:0 4px;}");
    QHBoxLayout *pidColumns = new QHBoxLayout(pidGroup);
    pidColumns->setSpacing(12);
    pidColumns->setContentsMargins(8, 8, 8, 8);

    QString pidNames[10] = {
        "ATC_RAT_RLL_P", "ATC_RAT_RLL_I", "ATC_RAT_RLL_D",
        "ATC_RAT_PIT_P", "ATC_RAT_PIT_I", "ATC_RAT_PIT_D",
        "ATC_RAT_YAW_P", "ATC_RAT_YAW_I", "ATC_RAT_YAW_D",
        "ATC_SLEW_YAW"
    };

    QString spinStyle = "QDoubleSpinBox{color:#000;font-weight:bold;font-size:12px;"
                        "background:#FFF;border:1px solid #999;border-radius:2px;"
                        "padding:2px 4px;min-width:70px;}";
    QString groupStyle =
        "QGroupBox{color:#000;font-weight:bold;font-size:11px;"
        "border:1px solid #999;border-radius:3px;margin-top:6px;padding-top:10px;}"
        "QGroupBox::title{subcontrol-origin:margin;left:6px;padding:0 2px;}";

    auto addPidGroup = [&](const QString &title, const QStringList &labels, int startIdx) {
        QGroupBox *g = new QGroupBox(title, pidGroup);
        g->setStyleSheet(groupStyle);
        QFormLayout *fl = new QFormLayout(g);
        fl->setSpacing(2);
        fl->setContentsMargins(6, 6, 6, 6);
        for (int j = 0; j < labels.size(); ++j) {
            int idx = startIdx + j;
            m_pidParams[idx].paramId = pidNames[idx];
            m_pidParams[idx].spinBox = new QDoubleSpinBox(g);
            m_pidParams[idx].spinBox->setRange(0.0, 1.0);
            m_pidParams[idx].spinBox->setDecimals(4);
            m_pidParams[idx].spinBox->setSingleStep(0.001);
            m_pidParams[idx].spinBox->setStyleSheet(spinStyle);
            fl->addRow(labels[j] + ":", m_pidParams[idx].spinBox);
        }
        return g;
    };

    QVBoxLayout *leftCol = new QVBoxLayout();
    leftCol->addWidget(addPidGroup(tr("R (横滚)"), {tr("P"), tr("I"), tr("D")}, 0));
    leftCol->addWidget(addPidGroup(tr("P (俯仰)"), {tr("P"), tr("I"), tr("D")}, 3));
    leftCol->addStretch();

    QVBoxLayout *rightCol = new QVBoxLayout();
    rightCol->addWidget(addPidGroup(tr("Y (偏航)"), {tr("P"), tr("I"), tr("D")}, 6));
    rightCol->addWidget(addPidGroup(tr("其它"), {tr("SLEW YAW")}, 9));
    rightCol->addStretch();

    pidColumns->addLayout(leftCol);
    pidColumns->addLayout(rightCol);

    m_pidApplyBtn = new QPushButton(tr("写入PID参数"), this);
    m_pidApplyBtn->setStyleSheet(
        "QPushButton{color:#FFF;background:#2979FF;border:none;border-radius:4px;"
        "padding:6px 16px;font-weight:bold;font-size:12px;}"
        "QPushButton:hover{background:#448AFF;}"
        "QPushButton:pressed{background:#1565C0;}");

    m_pidExportBtn = new QPushButton(tr("导出"), this);
    m_pidExportBtn->setStyleSheet(
        "QPushButton{color:#FFF;background:#607D8B;border:none;border-radius:4px;"
        "padding:6px 12px;font-weight:bold;font-size:12px;}"
        "QPushButton:hover{background:#78909C;}"
        "QPushButton:pressed{background:#455A64;}");

    m_pidImportBtn = new QPushButton(tr("导入"), this);
    m_pidImportBtn->setStyleSheet(
        "QPushButton{color:#FFF;background:#607D8B;border:none;border-radius:4px;"
        "padding:6px 12px;font-weight:bold;font-size:12px;}"
        "QPushButton:hover{background:#78909C;}"
        "QPushButton:pressed{background:#455A64;}");

    QHBoxLayout *pidBtnLayout = new QHBoxLayout();
    pidBtnLayout->addStretch();
    pidBtnLayout->addWidget(m_pidImportBtn);
    pidBtnLayout->addWidget(m_pidExportBtn);
    pidBtnLayout->addWidget(m_pidApplyBtn);
    pidBtnLayout->addStretch();

    mainLayout->addWidget(pidGroup);
    mainLayout->addLayout(pidBtnLayout);
    connect(m_pidApplyBtn, &QPushButton::clicked, this, &PwmDisplayWidget::onPidApplyClicked);
    connect(m_pidExportBtn, &QPushButton::clicked, this, &PwmDisplayWidget::onPidExportClicked);
    connect(m_pidImportBtn, &QPushButton::clicked, this, &PwmDisplayWidget::onPidImportClicked);

    mainLayout->addStretch();
}

void PwmDisplayWidget::updatePwmValues(const uint16_t values[16], uint8_t count)
{
    for (int i = 0; i < 6 && i < count; ++i) {
        uint16_t val = values[i];
        m_valueLabels[i]->setText(QString::number(val));

        QString baseStyle = "color:#000;font-weight:bold;font-size:16px;"
                            "border:2px solid #999;border-radius:4px;"
                            "padding:4px 8px;min-width:60px;";
        if (val == 1500) {
            m_valueLabels[i]->setStyleSheet(baseStyle + "background:#FFF;");
        } else if (val > 1500) {
            m_valueLabels[i]->setStyleSheet(baseStyle + "background:#0F0;");
        } else {
            m_valueLabels[i]->setStyleSheet(baseStyle + "background:#F00;");
        }
    }
}

void PwmDisplayWidget::setArmed(bool armed)
{
    m_armed = armed;
    for (int i = 0; i < 6; ++i) {
        m_reverseCheckBoxes[i]->setEnabled(!armed);
        m_enabledCheckBoxes[i]->setEnabled(!armed);
    }
}

void PwmDisplayWidget::onReverseCheckboxToggled(int channel, bool checked)
{
    if (m_armed) {
        m_reverseCheckBoxes[channel]->blockSignals(true);
        m_reverseCheckBoxes[channel]->setChecked(!checked);
        m_reverseCheckBoxes[channel]->blockSignals(false);
        QMessageBox::warning(this, tr("操作失败"), tr("请在未解锁状态下设置电机正反转！"));
        return;
    }
    m_motorReversed[channel] = checked;
    emit reverseChanged(channel + 1, checked);
}

void PwmDisplayWidget::onEnabledCheckboxToggled(int channel, bool checked)
{
    if (m_armed) {
        m_enabledCheckBoxes[channel]->blockSignals(true);
        m_enabledCheckBoxes[channel]->setChecked(!checked);
        m_enabledCheckBoxes[channel]->blockSignals(false);
        QMessageBox::warning(this, tr("操作失败"), tr("请在未解锁状态下设置电机启用/禁用！"));
        return;
    }
    emit enabledChanged(channel + 1, checked);
}

void PwmDisplayWidget::onParamValueReceived(const QString &paramId, float value)
{
    for (int i = 0; i < 6; ++i) {
        QString dirParam = QString("MOT_%1_DIRECTION").arg(i + 1);
        if (paramId == dirParam) {
            m_motorReversed[i] = (qRound(value) == -1);
            m_reverseCheckBoxes[i]->blockSignals(true);
            m_reverseCheckBoxes[i]->setChecked(m_motorReversed[i]);
            m_reverseCheckBoxes[i]->blockSignals(false);
            break;
        }

        QString funcParam = QString("SERVO%1_FUNCTION").arg(i + 1);
        if (paramId == funcParam) {
            int funcVal = qRound(value);
            m_servoFunction[i] = funcVal;
            bool enabled = (funcVal != 0);
            m_enabledCheckBoxes[i]->blockSignals(true);
            m_enabledCheckBoxes[i]->setChecked(enabled);
            m_enabledCheckBoxes[i]->blockSignals(false);
            break;
        }
    }

    for (int i = 0; i < 10; ++i) {
        if (paramId == m_pidParams[i].paramId) {
            m_pidParams[i].spinBox->blockSignals(true);
            m_pidParams[i].spinBox->setValue(static_cast<double>(value));
            m_pidParams[i].spinBox->blockSignals(false);
            break;
        }
    }
}

void PwmDisplayWidget::onPidApplyClicked()
{
    if (m_armed) {
        QMessageBox::warning(this, tr("操作失败"), tr("请在未解锁状态下设置PID参数！"));
        return;
    }
    for (int i = 0; i < 10; ++i) {
        float val = static_cast<float>(m_pidParams[i].spinBox->value());
        emit pidParamSetRequested(m_pidParams[i].paramId, val);
    }
    QTimer::singleShot(2000, this, [this]() {
        emit pidSaveToEepromRequested();
    });
    QMessageBox::information(this, tr("PID参数"), tr("PID参数已发送，2秒后自动写入EEPROM持久化。"));
}

void PwmDisplayWidget::onPidExportClicked()
{
    QString defaultName = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + "_pid.param";
    QString filePath = QFileDialog::getSaveFileName(this, tr("导出PID参数"),
                                                     defaultName,
                                                     tr("参数文件 (*.param *.csv);;所有文件 (*)"));
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("导出失败"), tr("无法写入文件：%1").arg(file.errorString()));
        return;
    }

    QTextStream out(&file);
    out << "# PID参数导出 - " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n";
    for (int i = 0; i < 10; ++i) {
        out << m_pidParams[i].paramId << "," << m_pidParams[i].spinBox->value() << "\n";
    }
    file.close();
    QMessageBox::information(this, tr("导出成功"), tr("PID参数已导出到：\n%1").arg(filePath));
}

void PwmDisplayWidget::onPidImportClicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("导入PID参数"),
                                                     QString(),
                                                     tr("参数文件 (*.param *.csv);;所有文件 (*)"));
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("导入失败"), tr("无法读取文件：%1").arg(file.errorString()));
        return;
    }

    QMap<QString, double> paramMap;
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) continue;
        QStringList parts = line.split(',');
        if (parts.size() >= 2) {
            paramMap[parts[0].trimmed()] = parts[1].trimmed().toDouble();
        }
    }
    file.close();

    int applied = 0;
    for (int i = 0; i < 10; ++i) {
        if (paramMap.contains(m_pidParams[i].paramId)) {
            m_pidParams[i].spinBox->blockSignals(true);
            m_pidParams[i].spinBox->setValue(paramMap[m_pidParams[i].paramId]);
            m_pidParams[i].spinBox->blockSignals(false);
            applied++;
        }
    }

    if (applied > 0) {
        QMessageBox::information(this, tr("导入成功"), tr("已导入 %1 个PID参数。\n请点击\"写入PID参数\"使其生效。").arg(applied));
    } else {
        QMessageBox::warning(this, tr("导入失败"), tr("文件中未找到匹配的PID参数。"));
    }
}
