#include "plaintextedit.h"

#include <QMimeData>

PlainTextEdit::PlainTextEdit(QWidget *parent) : QTextEdit(parent) {
}

void PlainTextEdit::insertFromMimeData(const QMimeData *source) {
    if (!source) {
        return;
    }
    insertPlainText(source->text());
}

