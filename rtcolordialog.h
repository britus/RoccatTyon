#pragma once

#include "rttypes.h"
#include <QDialog>

namespace Ui {
class RTColorDialog;
}

class RTColorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RTColorDialog(QWidget *parent = nullptr);
    ~RTColorDialog();
    inline const TyonRmpLightInfo &selectedColor() const { return m_selected; }

private:
    Ui::RTColorDialog *ui;
    TyonRmpLightInfo m_selected;
};
