#pragma once
#include <QLabel>
#include <QListWidget>
#include <QtMultimedia/QMediaPlayer>
#include <QMainWindow>
#include <QMouseEvent>
#include <QPushButton>
#include <QSlider>
#include <QVector>
#include <QUrl>

// 進度條
class SeekSlider final : public QSlider {
    Q_OBJECT

public:
    using QSlider::QSlider;

protected:
    // 點擊位置直接跳到該位置
    void mousePressEvent(QMouseEvent *e) override {
        if (orientation() == Qt::Horizontal && e->button() == Qt::LeftButton) {
            const double x = e->position().x();
            constexpr double handleHalf = 4.0;
            const double ratio = std::clamp((x - handleHalf) / (width() - 2 * handleHalf), 0.0, 1.0);
            const int value = static_cast<int>(std::round(ratio * (maximum() - minimum()) + minimum()));

            setValue(value);
            emit sliderMoved(value);
            emit sliderReleased();
            e->accept();
        }

        QSlider::mousePressEvent(e);
    }

};

// 音量條
class VolumeSlider final : public QSlider {
    Q_OBJECT

public:
    using QSlider::QSlider;

protected:
    // 點擊位置直接跳到該位置
    void mousePressEvent(QMouseEvent *e) override {
        if (orientation() == Qt::Horizontal && e->button() == Qt::LeftButton) {
            const double x = e->position().x();
            constexpr double handleHalf = 4.0;
            const double ratio = std::clamp((x - handleHalf) / (width() - 2 * handleHalf), 0.0, 1.0);
            const int value = static_cast<int>(std::round(ratio * (maximum() - minimum()) + minimum()));

            setValue(value);
            emit sliderMoved(value);
            emit sliderReleased();
            e->accept();
        }
        QSlider::mousePressEvent(e);
    }
};


class QMediaDevices;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    ~MainWindow() override;

protected:
    // 事件
    void dragEnterEvent(QDragEnterEvent *event) override;

    void dropEvent(QDropEvent *event) override;

private slots:
    // 動作
    void openFiles();

    void clearList();

    void removeSelected();

    void loadM3U();

    void saveM3U();

    void playSelected(int row);

    void playPause();

    void stop() const;

    void next();

    void previous();

    void onPositionChanged(qint64 pos);

    void onDurationChanged(qint64 dur);

    void onStateChanged() const;

    void onErrorChanged();

    void onVolumeChanged(int v) const;

    void onSeek(int v) const;

    void toggleMute() const;

    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);

private:
    void setupUi();

    void setupMenu();

    void setupShortcuts();

    void enqueue(const QList<QUrl> &urls);

    static QString mp3BasePath();

    void playIndex(int idx);

    void updateTimeLabels(qint64 pos, qint64 dur) const;

    static QString formatTime(qint64 ms);

    static bool isAudioUrl(const QUrl &url);

    void seekByMs(qint64 deltaMs) const;

    // 多媒體物件
    QMediaPlayer *m_player;
    QAudioOutput *m_audio;
    QMediaDevices *m_devices = nullptr;

    // UI 控制
    QListWidget *m_list{};
    QPushButton *m_btnPrev{};
    QPushButton *m_btnPlayPause{};
    QPushButton *m_btnStop{};
    QPushButton *m_btnNext{};
    SeekSlider *m_seek{};
    QLabel *m_lblTimeL{};
    QLabel *m_lblTimeR{};
    QSlider *m_volume{};
    QPushButton *m_btnMute{};

    // 播放清單
    QVector<QUrl> m_urls;
    int m_currentIndex = -1;
    qint64 m_durationMs = 0;
    bool m_syncingFromPlayer = false;

    // 動作
    QAction *m_actOpen{};
    QAction *m_actLoadM3U{};
    QAction *m_actSaveM3U{};
    QAction *m_actClear{};
    QAction *m_actRemove{};
};
