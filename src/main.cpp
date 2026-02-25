#include "mainwindow.h"

#include <QApplication>
#include <QColor>
#include <QPalette>
#include <QStyleFactory>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setOrganizationName("NEYROSLAV");
    app.setApplicationName("AceStep15DatasetManager");
    app.setStyle(QStyleFactory::create("Fusion"));

    QPalette dark;
    dark.setColor(QPalette::Window, QColor(31, 37, 48));
    dark.setColor(QPalette::WindowText, QColor(232, 238, 248));
    dark.setColor(QPalette::Base, QColor(35, 42, 52));
    dark.setColor(QPalette::AlternateBase, QColor(42, 49, 60));
    dark.setColor(QPalette::ToolTipBase, QColor(42, 49, 60));
    dark.setColor(QPalette::ToolTipText, QColor(232, 238, 248));
    dark.setColor(QPalette::Text, QColor(232, 238, 248));
    dark.setColor(QPalette::Button, QColor(42, 49, 60));
    dark.setColor(QPalette::ButtonText, QColor(223, 231, 243));
    dark.setColor(QPalette::BrightText, QColor(255, 123, 123));
    dark.setColor(QPalette::Highlight, QColor(72, 106, 161));
    dark.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
    dark.setColor(QPalette::Link, QColor(120, 170, 235));
    dark.setColor(QPalette::PlaceholderText, QColor(160, 170, 186));

    dark.setColor(QPalette::Disabled, QPalette::WindowText, QColor(130, 138, 150));
    dark.setColor(QPalette::Disabled, QPalette::Text, QColor(130, 138, 150));
    dark.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(130, 138, 150));
    dark.setColor(QPalette::Disabled, QPalette::Highlight, QColor(70, 76, 88));
    dark.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(180, 186, 196));

    app.setPalette(dark);
    app.setStyleSheet(
        "QToolTip {"
        " color: #e8eef8;"
        " background-color: #2a313c;"
        " border: 1px solid #4e596b;"
        " }");

    MainWindow w;
    w.show();
    return app.exec();
}
