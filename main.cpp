#include <QApplication>
#include <QColor>
#include "MainWindow.h"
#include <QStyleFactory>
#include <QPalette>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv); // Qt 應用程式物件
    QApplication::setApplicationName("MusicPlayer");
    QApplication::setOrganizationName("Ethan");
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    // 調色盤
    QPalette dark;
    dark.setColor(QPalette::WindowText, QColor(230, 230, 230));
    dark.setColor(QPalette::Window, QColor(18, 18, 18));
    dark.setColor(QPalette::Base, QColor(24, 24, 24));
    dark.setColor(QPalette::AlternateBase, QColor(45, 45, 45));
    dark.setColor(QPalette::ToolTipBase, QColor(230, 230, 230));
    dark.setColor(QPalette::ToolTipText, QColor(25, 25, 25));
    dark.setColor(QPalette::Text, QColor(230, 230, 230));
    dark.setColor(QPalette::Button, QColor(45, 45, 45));
    dark.setColor(QPalette::ButtonText, QColor(230, 230, 230));
    dark.setColor(QPalette::BrightText, QColor(255, 80, 80));
    dark.setColor(QPalette::Highlight, QColor(255, 255, 255));
    dark.setColor(QPalette::HighlightedText, QColor(25, 25, 25));

    QApplication::setPalette(dark); // 套用

    const QFont font("SF Pro Display", 12);
    QApplication::setFont(font);

    // StyleSheet
    app.setStyleSheet(R"(
    QWidget {
        color: #E6E6E6;
        font-size: 13px;
    }

    QPushButton {
        background: rgba(255,255,255,0.08);
        border: none;
        border-radius: 9px;
        padding: 2px 4px;
        min-width: 14px;
        min-height: 14px;
    }

    QPushButton:hover  {
        background: rgba(255,255,255,0.20);
    }

    QPushButton:pressed {
        background: rgba(255,255,255,0.35);
    }

    QPushButton#primaryCtrl {
        padding: 2px 4px;
        border-radius: 9px;
        font-weight: 600;
    }

    QPushButton:hover {
        background: rgba(255,255,255,0.15);
    }

    QListWidget {
        outline: none;
        selection-background-color: transparent;
        color: #FFFFFF;
        selection-color: #5CC8FF;
    }

    QListWidget::item:selected, QListWidget::item:selected:!active, QListWidget::item:selected:active, QListWidget::item:selected:focus, QListWidget::item:selected:!focus {
        background: transparent;
        color: #5CC8FF;
        font-weight: bold;
    }

    QListWidget::item:hover {
        background: rgba(255,255,255,0.08);
    }

    QListWidget::item:selected:hover, QListWidget::item:selected:!active:hover, QListWidget::item:selected:active:hover, QListWidget::item:selected:focus:hover, QListWidget::item:selected:!focus:hover {
        background: rgba(255,255,255,0.08);
    }

    QSlider::groove:horizontal {
        height: 6px;
        background: rgba(255,255,255,0.10);
        border-radius: 8px;
    }

    QSlider::handle:horizontal {
        width: 8px;
        margin: -2px 0;
        border-radius: 4px;
        background: #FFFFFF;
        border: 1px solid rgba(0,0,0,0.25);
    }

    QSlider::sub-page:horizontal {
        background: #5CC8FF;
        border-radius: 3px;
    }

    QToolBar, QMenuBar, QStatusBar {
        background: #252526;
        border: none;
    }

    QMenu {
        background: #2B2B2C;
        border: 1px solid rgba(255,255,255,0.12);
    }

    QMenu::item:selected {
        background: rgba(255,255,255,0.12);
    }

    QStatusBar {
        color:#DDDDDD;
    }
)");

    MainWindow w; // 主視窗物件
    w.resize(900, 520); // 視窗大小
    w.show();
    return QApplication::exec();
}
