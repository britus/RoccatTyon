// ********************************************************************
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// Copyright by libgaminggear Project (some copied parts)
// Copyright by roccat-tools Project (some copied parts)
// SPDX-License-Identifier: GPL-3.0
// ********************************************************************
#include "rtshortcutdialog.h"
#include "ui_rtshortcutdialog.h"
#include <QDebug>
#include <QDialogButtonBox>
#include <QPushButton>

RTShortcutDialog::RTShortcutDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RTShortcutDialog)
    , m_data()
{
    ui->setupUi(this);

#ifdef Q_OS_MACOS
    ui->cbxKeyMeta->setText(tr("Command"));
#endif

    QPushButton *pb;
    if ((pb = ui->buttonBox->button(QDialogButtonBox::Ok))) {
        pb->setEnabled(false);
    }

    ui->edKeyShortcut->setEnabled(true);
    ui->edKeyShortcut->setFocus();
}

RTShortcutDialog::~RTShortcutDialog()
{
    delete ui;
}

inline void RTShortcutDialog::update(Qt::KeyboardModifier km, bool state)
{
    Qt::KeyboardModifiers mods;
    mods = m_data.keyCombo.keyboardModifiers();
    mods.setFlag(km, state);
    m_data.keyCombo = QKeyCombination(mods, m_data.keyCombo.key());
    qDebug() << "[SCTDLG]" << m_data.keyCombo;
}

inline void RTShortcutDialog::update(Qt::KeyboardModifiers km, Qt::Key key)
{
    m_data.keyCombo = QKeyCombination(km, key);
    qDebug() << "[SCTDLG]" << m_data.keyCombo;
}

void RTShortcutDialog::on_cbxKeyShift_toggled(bool checked)
{
    update(Qt::ShiftModifier, checked);
}

void RTShortcutDialog::on_cbxKeyControl_toggled(bool checked)
{
    update(Qt::ControlModifier, checked);
}

void RTShortcutDialog::on_cbxKeyAlt_toggled(bool checked)
{
    update(Qt::AltModifier, checked);
}

void RTShortcutDialog::on_cbxKeyPad_toggled(bool checked)
{
    update(Qt::KeypadModifier, checked);
}

void RTShortcutDialog::on_cbxKeyMeta_toggled(bool checked)
{
    update(Qt::MetaModifier, checked);
}

void RTShortcutDialog::on_edKeyShortcut_editingFinished()
{
    QPushButton *pb;
    QKeySequence ks = ui->edKeyShortcut->keySequence();
    if (!ks.isEmpty() && ks[0].key() != Qt::Key_unknown) {
        /* use key modifiers from checkboxes */
        int mods = m_data.keyCombo.keyboardModifiers().toInt();
        /* if nothing selected, try from key sequece edit field */
        if (mods == 0) {
            int ksmod = ks[0].keyboardModifiers().toInt();
            if (ksmod != 0) {
                ui->cbxKeyShift->setChecked(ksmod & Qt::ShiftModifier);
                ui->cbxKeyPad->setChecked(ksmod & Qt::KeypadModifier);
                ui->cbxKeyAlt->setChecked(ksmod & Qt::AltModifier);
#ifdef Q_OS_MACOS /* QT-BUGFIX: CTRL to META (QT does 'Mac-Command' as CTRL and CTRL as META) */
                ui->cbxKeyControl->setChecked(ksmod & Qt::MetaModifier);
                ui->cbxKeyMeta->setChecked(ksmod & Qt::ControlModifier);
#else
                ui->cbxKeyControl->setChecked(ksmod & Qt::ControlModifier);
                ui->cbxKeyMeta->setChecked(ksmod & Qt::MetaModifier);
#endif
                mods = m_data.keyCombo.keyboardModifiers().toInt();
            }
        }
        update(Qt::KeyboardModifiers(mods), ks[0].key());
        if ((pb = ui->buttonBox->button(QDialogButtonBox::Ok))) {
            pb->setEnabled(true);
        }
    } else {
        m_data.keyCombo = QKeyCombination();
        if ((pb = ui->buttonBox->button(QDialogButtonBox::Ok))) {
            pb->setEnabled(false);
        }
    }
}

void RTShortcutDialog::on_edKeyShortcut_keySequenceChanged(const QKeySequence &)
{
    //QPushButton *pb;
    //if ((pb = ui->buttonBox->button(QDialogButtonBox::Ok))) {
    //    pb->setEnabled(false);
    //}
}
