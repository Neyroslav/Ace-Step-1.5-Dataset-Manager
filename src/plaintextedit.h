#pragma once

#include <QTextEdit>

class QMimeData;

class PlainTextEdit : public QTextEdit {
    Q_OBJECT

public:
    explicit PlainTextEdit(QWidget *parent = nullptr);

protected:
    void insertFromMimeData(const QMimeData *source) override;
};

