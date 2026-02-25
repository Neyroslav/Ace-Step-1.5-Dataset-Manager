#pragma once

#include <QTextEdit>

class ResizableTextEdit : public QTextEdit {
    Q_OBJECT

public:
    explicit ResizableTextEdit(QWidget *parent = nullptr);

    void setResizeLimits(int minHeight, int maxHeight);
    void setHandleHeight(int px);

signals:
    void heightChanged(int newHeight);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    bool isInResizeZone(const QPoint &pos) const;
    void updateCursorForPos(const QPoint &pos);

    bool m_resizing = false;
    int m_pressGlobalY = 0;
    int m_startHeight = 0;
    int m_minHeight = 50;
    int m_maxHeight = 1200;
    int m_handleHeight = 8;
};

