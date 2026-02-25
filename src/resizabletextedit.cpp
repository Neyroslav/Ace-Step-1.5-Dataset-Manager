#include "resizabletextedit.h"

#include <QEvent>
#include <QMouseEvent>
#include <QtGlobal>

ResizableTextEdit::ResizableTextEdit(QWidget *parent) : QTextEdit(parent) {
    setMouseTracking(true);
}

void ResizableTextEdit::setResizeLimits(int minHeight, int maxHeight) {
    m_minHeight = qMax(1, minHeight);
    m_maxHeight = qMax(m_minHeight, maxHeight);
    const int h = qBound(m_minHeight, height(), m_maxHeight);
    if (h != height()) {
        setFixedHeight(h);
        emit heightChanged(h);
    }
}

void ResizableTextEdit::setHandleHeight(int px) {
    m_handleHeight = qMax(2, px);
}

void ResizableTextEdit::mousePressEvent(QMouseEvent *event) {
    QPoint localPos;
    int globalY = 0;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    localPos = event->position().toPoint();
    globalY = event->globalPosition().toPoint().y();
#else
    localPos = event->pos();
    globalY = event->globalY();
#endif
    if (event->button() == Qt::LeftButton && isInResizeZone(localPos)) {
        m_resizing = true;
        m_pressGlobalY = globalY;
        m_startHeight = height();
        event->accept();
        return;
    }
    QTextEdit::mousePressEvent(event);
}

void ResizableTextEdit::mouseMoveEvent(QMouseEvent *event) {
    if (m_resizing) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        const int currentGlobalY = event->globalPosition().toPoint().y();
#else
        const int currentGlobalY = event->globalY();
#endif
        const int delta = currentGlobalY - m_pressGlobalY;
        const int newH = qBound(m_minHeight, m_startHeight + delta, m_maxHeight);
        if (newH != height()) {
            setFixedHeight(newH);
            updateGeometry();
            emit heightChanged(newH);
        }
        event->accept();
        return;
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    updateCursorForPos(event->position().toPoint());
#else
    updateCursorForPos(event->pos());
#endif
    QTextEdit::mouseMoveEvent(event);
}

void ResizableTextEdit::mouseReleaseEvent(QMouseEvent *event) {
    if (m_resizing && event->button() == Qt::LeftButton) {
        m_resizing = false;
        emit heightChanged(height());
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        updateCursorForPos(event->position().toPoint());
#else
        updateCursorForPos(event->pos());
#endif
        event->accept();
        return;
    }
    QTextEdit::mouseReleaseEvent(event);
}

void ResizableTextEdit::leaveEvent(QEvent *event) {
    if (!m_resizing) {
        unsetCursor();
    }
    QTextEdit::leaveEvent(event);
}

bool ResizableTextEdit::isInResizeZone(const QPoint &pos) const {
    return pos.y() >= (height() - m_handleHeight);
}

void ResizableTextEdit::updateCursorForPos(const QPoint &pos) {
    if (isInResizeZone(pos)) {
        setCursor(Qt::SizeVerCursor);
    } else {
        unsetCursor();
    }
}
