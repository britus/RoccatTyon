#pragma once
#include <QDialog>
#include <QKeyCombination>
#include <QKeySequence>

namespace Ui {
class RTShortcutDialog;
}

class RTShortcutDialog : public QDialog
{
    Q_OBJECT

public:
    typedef struct
    {
        QKeyCombination keyCombo;
    } TDialogData;

    explicit RTShortcutDialog(QWidget *parent = nullptr);
    ~RTShortcutDialog();
    inline const TDialogData &data() const { return m_data; }

private slots:
    void on_edKeyShortcut_editingFinished();
    void on_cbxKeyShift_toggled(bool checked);
    void on_cbxKeyControl_toggled(bool checked);
    void on_cbxKeyAlt_toggled(bool checked);
    void on_cbxKeyMeta_toggled(bool checked);
    void on_cbxKeyPad_toggled(bool checked);

    void on_edKeyShortcut_keySequenceChanged(const QKeySequence &keySequence);

private:
    Ui::RTShortcutDialog *ui;
    TDialogData m_data;
    inline void update(Qt::KeyboardModifier km, bool state);
    inline void update(Qt::KeyboardModifiers mods, Qt::Key key);
};
