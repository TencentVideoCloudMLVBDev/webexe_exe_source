#pragma once

#include <QTextEdit>
#include <QKeyEvent>

class CustomTextEdit : public QTextEdit
{
    Q_OBJECT

public:
    CustomTextEdit(QWidget *parent);
    ~CustomTextEdit();
signals:
    void enterPressed();
    void ctrlEnterPressed();
protected:
    virtual void keyPressEvent(QKeyEvent * ev);
};
