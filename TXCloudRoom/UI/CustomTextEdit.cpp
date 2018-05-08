#include "CustomTextEdit.h"

CustomTextEdit::CustomTextEdit(QWidget *parent)
    : QTextEdit(parent)
{
}

CustomTextEdit::~CustomTextEdit()
{
}

void CustomTextEdit::keyPressEvent(QKeyEvent * ev)
{
    if (ev->key() == Qt::Key_Return || ev->key() == Qt::Key_Enter)
    {
        if (Qt::ControlModifier == (ev->modifiers() & Qt::ControlModifier))
        {
            emit ctrlEnterPressed();
        }
        else
        {
            emit enterPressed();
        }

        ev->accept();
    }
    else
    {
        QTextEdit::keyPressEvent(ev);
    }
}
