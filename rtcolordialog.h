#pragma once

#include "rthiddevice.h"
#include "rttypes.h"
#include <QDialog>

namespace Ui {
class RTColorDialog;
}

class RTColorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RTColorDialog(const RTHidDevice::TDeviceColors &colors, QWidget *parent = nullptr);
    ~RTColorDialog();
    inline const TyonLight &selectedColor() const { return m_selected; }

private:
    Ui::RTColorDialog *ui;
    TyonLight m_selected;
};
