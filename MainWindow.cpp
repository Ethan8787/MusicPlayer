#include "MainWindow.h"

#include <QtMultimedia/QMediaPlayer>
#include <QtMultimedia/QAudioOutput>
#include <QtMultimedia/QMediaMetaData>
#include <QListWidget>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QFileDialog>
#include <QMouseEvent>
#include <QMimeData>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QBoxLayout>
#include <QShortcut>
#include <QKeySequence>
#include <QFileInfo>
#include <QTextStream>
#include <QMessageBox>
#include <QtMultimedia/QMediaDevices>
#include <QtMultimedia/QAudioDevice>
#include <QIcon>

QString exeDir = QCoreApplication::applicationDirPath();

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_player(new QMediaPlayer(this)), // 播放器物件
      m_audio(new QAudioOutput(this)) {
    // 音訊輸出物件
    m_player->setAudioOutput(m_audio); // 連接
    m_devices = new QMediaDevices(this); // 媒體裝置物件

    auto applyDefaultOutput = [this] {
        const bool wasMuted = m_audio->isMuted();
        const float vol = m_audio->volume();
        m_audio->setDevice(QMediaDevices::defaultAudioOutput());
        m_audio->setVolume(vol);
        m_audio->setMuted(wasMuted);
    };

    applyDefaultOutput(); // 初始設定

    connect(m_devices, &QMediaDevices::audioOutputsChanged, this, applyDefaultOutput); // 輸出裝置變更

    setupUi();
    setupMenu();
    setupShortcuts();

    connect(m_player, &QMediaPlayer::positionChanged, this, &MainWindow::onPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &MainWindow::onDurationChanged);
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, &MainWindow::onStateChanged);
    connect(m_player, &QMediaPlayer::errorOccurred, this, &MainWindow::onErrorChanged);
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &MainWindow::onMediaStatusChanged);
    connect(m_audio, &QAudioOutput::volumeChanged, this, &MainWindow::onVolumeChanged);

    setAcceptDrops(true);
    statusBar()->showMessage("Ready"); // 就緒
}

MainWindow::~MainWindow() = default;

// UI 設定
void MainWindow::setupUi() {
    m_list = new QListWidget(this);

    // highlight always blue (even when sliders are clicked)
    m_list->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setFocusPolicy(Qt::NoFocus);
    m_list->setStyleSheet("QListWidget::item:selected { color: #5CC8FF; background: transparent; font-weight: bold; }");

    connect(m_list, &QListWidget::itemClicked, this, [this](QListWidgetItem *) {
        playSelected(m_list->currentRow());
    });

    m_btnPrev = new QPushButton(this);
    m_btnPlayPause = new QPushButton(this);
    m_btnNext = new QPushButton(this);
    m_btnStop = new QPushButton(this);
    m_btnMute = new QPushButton(this);

    constexpr QSize iconSize(12, 12);
    m_btnPrev->setIconSize(iconSize);
    m_btnPlayPause->setIconSize(iconSize);
    m_btnStop->setIconSize(iconSize);
    m_btnNext->setIconSize(iconSize);
    m_btnMute->setIconSize(iconSize);

    m_btnPrev->setIcon(QIcon(":/icons/prev.svg"));
    m_btnPlayPause->setIcon(QIcon(":/icons/play.svg"));
    m_btnStop->setIcon(QIcon(":/icons/stop.svg"));
    m_btnNext->setIcon(QIcon(":/icons/next.svg"));
    m_btnMute->setIcon(QIcon(":/icons/volume.svg"));

    for (auto *b: {m_btnPrev, m_btnPlayPause, m_btnNext, m_btnStop, m_btnMute}) {
        b->setIconSize(iconSize);
        b->setFlat(true);
        b->setMinimumSize(30, 30);
    }

    connect(m_btnPrev, &QPushButton::clicked, this, &MainWindow::previous);
    connect(m_btnPlayPause, &QPushButton::clicked, this, &MainWindow::playPause);
    connect(m_btnNext, &QPushButton::clicked, this, &MainWindow::next);
    connect(m_btnStop, &QPushButton::clicked, this, &MainWindow::stop);
    connect(m_btnMute, &QPushButton::clicked, this, &MainWindow::toggleMute);

    m_seek = new SeekSlider(Qt::Horizontal, this);
    m_seek->setRange(0, 1000);
    m_seek->setTracking(true);

    connect(m_seek, &QSlider::valueChanged, this, &MainWindow::onSeek);

    connect(m_seek, &QSlider::sliderMoved, this, &MainWindow::onSeek);
    connect(m_seek, &QAbstractSlider::sliderReleased, this, [this] { onSeek(m_seek->value()); });


    m_lblTimeL = new QLabel("00:00", this);
    m_lblTimeR = new QLabel("00:00", this);

    m_volume = new VolumeSlider(Qt::Horizontal, this);
    m_volume->setRange(0, 100);
    m_volume->setValue(100); // default 100%
    m_audio->setVolume(1.0f); // 100% = 1.0

    connect(m_volume, &QSlider::valueChanged, this, [this](int v) {
        const float scaled = std::clamp(static_cast<float>(v) / 100.0f, 0.0f, 1.0f);
        m_audio->setVolume(scaled);
    });

    m_seek->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_seek->setMinimumWidth(100); // optional floor

    const auto row = new QHBoxLayout();
    row->setContentsMargins(12, 6, 12, 8);
    row->setSpacing(10);

    row->addWidget(m_btnPrev);
    row->addWidget(m_btnPlayPause);
    row->addWidget(m_btnNext);
    row->addWidget(m_btnStop);
    row->addSpacing(12);

    row->addWidget(m_lblTimeL);
    row->addWidget(m_seek, 1);
    row->addWidget(m_lblTimeR);

    row->addSpacing(12);

    row->addWidget(m_btnMute);
    m_volume->setFixedWidth(100);
    row->addWidget(m_volume);

    const auto bottom = new QWidget(this);
    const auto bottomH = new QHBoxLayout(bottom);
    bottomH->setContentsMargins(0, 0, 0, 0);
    bottomH->addLayout(row);

    const auto central = new QWidget(this);
    const auto rootV = new QVBoxLayout(central);
    rootV->setContentsMargins(0, 0, 0, 0);
    rootV->setSpacing(0);
    rootV->addWidget(m_list, 1);
    rootV->addWidget(bottom, 0);
    setCentralWidget(central);

    const auto tb = addToolBar("Controls");
    tb->addAction(QIcon(":/icons/open.svg"), "Open", this, &MainWindow::openFiles);
    tb->addAction("Clear", this, &MainWindow::clearList);
    tb->addAction("Remove", this, &MainWindow::removeSelected);

    auto *label = new QLabel("Made by Ethan");
    statusBar()->addPermanentWidget(label);
    label->setStyleSheet("color: #888888; font-size: 10pt;");
    statusBar()->addPermanentWidget(label);
}

// 媒體狀態變更
void MainWindow::onMediaStatusChanged(QMediaPlayer::MediaStatus status) {
    using MS = QMediaPlayer::MediaStatus;
    if (status == MS::LoadedMedia || status == MS::BufferedMedia) {
        qint64 d = m_player->duration();
        if (d > 0) {
            m_durationMs = d;
            updateTimeLabels(m_player->position(), m_durationMs);
        }
    }
}

// 選單設定
void MainWindow::setupMenu() {
    const auto file = menuBar()->addMenu("&File");
    m_actOpen = file->addAction("Open…", QKeySequence::Open, this, &MainWindow::openFiles);
    m_actLoadM3U = file->addAction("Load M3U…", this, &MainWindow::loadM3U);
    m_actSaveM3U = file->addAction("Save M3U…", this, &MainWindow::saveM3U);
    file->addSeparator();
    file->addAction("E&xit", QKeySequence::Quit, this, &QWidget::close);

    const auto edit = menuBar()->addMenu("&Edit");
    m_actRemove = edit->addAction("Remove Selected", QKeySequence::Delete, this, &MainWindow::removeSelected);
    m_actClear = edit->addAction("Clear All", this, &MainWindow::clearList);

    const auto help = menuBar()->addMenu("&Help");
    help->addAction("About", this, [this] {
        QMessageBox::about(this, "MusicPlayer",
                           "A simple Qt 6 Music Player demo.\n"
                           "• Space: Play/Pause\n"
                           "• Enter/Double-click: Play selected\n"
                           "• Ctrl+Right/Left: Next/Prev\n"
                           "• +/-: Volume\n"
                           "• Drag & drop audio files into the window");
    });
}

// 快捷鍵設定
void MainWindow::setupShortcuts() {
    (void) new QShortcut(QKeySequence(Qt::Key_Space), this, SLOT(playPause()));
    (void) new QShortcut(QKeySequence(Qt::Key_Return), this, [this] { playSelected(m_list->currentRow()); });
    (void) new QShortcut(QKeySequence(Qt::Key_Enter), this, [this] { playSelected(m_list->currentRow()); });
    (void) new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Right), this, SLOT(next()));
    (void) new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Left), this, SLOT(previous()));
    (void) new QShortcut(QKeySequence(Qt::Key_Plus), this,
                         [this] { m_volume->setValue(std::min(100, m_volume->value() + 5)); });
    (void) new QShortcut(QKeySequence(Qt::Key_Minus), this,
                         [this] { m_volume->setValue(std::max(0, m_volume->value() - 5)); });
    (void) new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_M), this, SLOT(toggleMute()));

    const auto scPlayPause = new QShortcut(QKeySequence(Qt::Key_MediaTogglePlayPause), this);
    scPlayPause->setContext(Qt::ApplicationShortcut);
    connect(scPlayPause, &QShortcut::activated, this, &MainWindow::playPause);

    const auto scPlay = new QShortcut(QKeySequence(Qt::Key_MediaPlay), this);
    scPlay->setContext(Qt::ApplicationShortcut);
    connect(scPlay, &QShortcut::activated, this, &MainWindow::playPause);

    const auto scPause = new QShortcut(QKeySequence(Qt::Key_MediaPause), this);
    scPause->setContext(Qt::ApplicationShortcut);
    connect(scPause, &QShortcut::activated, this, &MainWindow::playPause);

    const auto scNext = new QShortcut(QKeySequence(Qt::Key_MediaNext), this);
    scNext->setContext(Qt::ApplicationShortcut);
    connect(scNext, &QShortcut::activated, this, &MainWindow::next);

    const auto scPrev = new QShortcut(QKeySequence(Qt::Key_MediaPrevious), this);
    scPrev->setContext(Qt::ApplicationShortcut);
    connect(scPrev, &QShortcut::activated, this, &MainWindow::previous);

    const auto scStop = new QShortcut(QKeySequence(Qt::Key_MediaStop), this);
    scStop->setContext(Qt::ApplicationShortcut);
    connect(scStop, &QShortcut::activated, this, &MainWindow::stop);

    const auto scSeekLeft = new QShortcut(QKeySequence(Qt::Key_Left), this);
    scSeekLeft->setContext(Qt::ApplicationShortcut);
    connect(scSeekLeft, &QShortcut::activated, this, [this] { seekByMs(-5000); });

    const auto scSeekRight = new QShortcut(QKeySequence(Qt::Key_Right), this);
    scSeekRight->setContext(Qt::ApplicationShortcut);
    connect(scSeekRight, &QShortcut::activated, this, [this] { seekByMs(+5000); });
}

// 快進/快退
void MainWindow::seekByMs(qint64 deltaMs) const {
    if (m_durationMs <= 0) return;
    const qint64 cur = m_player->position();
    const qint64 tgt = std::clamp(cur + deltaMs, 0LL, m_durationMs);
    m_player->setPosition(tgt);
}

// 開啟檔案
void MainWindow::openFiles() {
    const QStringList filters = {
        "Audio Files (*.mp3 *.wav *.flac *.m4a *.aac *.ogg *.opus)",
        "All Files (*)"
    };
    if (const auto urls = QFileDialog::getOpenFileUrls(this, "Open Audio Files", QUrl(), filters.join(";;")); !urls.
        isEmpty()) enqueue(urls);
}

// 取得 mp3 資料夾路徑
QString MainWindow::mp3BasePath() {
    QDir base(QCoreApplication::applicationDirPath());
    base.cdUp(); // 回到 ../
    base.cd("mp3");
    if (!base.exists()) {
        qWarning() << "mp3 folder not found:" << base.absolutePath();
    }
    return base.absolutePath();
}

// 加入播放清單
void MainWindow::enqueue(const QList<QUrl> &urls) {
    int added = 0;
    for (const QUrl &url: urls) {
        if (!isAudioUrl(url)) continue;
        m_urls.push_back(url);

        QString name = url.fileName();
        if (!name.isEmpty()) {
            QFileInfo fi(name);
            name = fi.completeBaseName();
        } else {
            name = url.toString();
        }
        m_list->addItem(name);
        ++added;
    }
    if (added > 0 && m_currentIndex < 0) playIndex(0);
    statusBar()->showMessage(QString("Added %1 item(s)").arg(added), 3000);
}

// 清空播放清單
void MainWindow::clearList() {
    m_player->stop();
    m_urls.clear();
    m_list->clear();
    m_currentIndex = -1;
    m_durationMs = 0;
    m_seek->setValue(0);
    updateTimeLabels(0, 0);
}

// 移除選取項目
void MainWindow::removeSelected() {
    auto rows = m_list->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;

    std::ranges::sort(rows, [](const QModelIndex &a, const QModelIndex &b) { return a.row() > b.row(); });
    for (const auto &idx: rows) {
        if (const int r = idx.row(); r >= 0 && r < m_urls.size()) {
            m_urls.remove(r);
            delete m_list->takeItem(r);
            if (r == m_currentIndex) {
                stop();
                m_currentIndex = -1;
            } else if (r < m_currentIndex) m_currentIndex--;
        }
    }
    if (m_currentIndex < 0 && !m_urls.isEmpty()) playIndex(0);
}

// 儲存播放清單
void MainWindow::saveM3U() {
    if (m_urls.isEmpty()) {
        QMessageBox::information(this, "Save M3U", "Playlist is empty.");
        return;
    }

    const QString binPath = QCoreApplication::applicationDirPath();
    const QString basePath = mp3BasePath();
    const QString file = QFileDialog::getSaveFileName(
        this, "Save M3U Playlist",
        basePath + "/playlist.m3u",
        "Playlists (*.m3u *.m3u8)");
    if (file.isEmpty()) return;

    QFile f(file);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Failed to write playlist.");
        return;
    }

    QTextStream out(&f);
    out << "#EXTM3U\n";

    for (const QUrl &u: m_urls) {
        QString absPath = u.toLocalFile();
        QString relPath = QDir(binPath).relativeFilePath(absPath);
        out << relPath << "\n";
    }

    statusBar()->showMessage("Playlist saved (relative to /bin).", 3000);
}

// 載入播放清單
void MainWindow::loadM3U() {
    const QString basePath = mp3BasePath();
    const QString binPath = QCoreApplication::applicationDirPath();

    const auto file = QFileDialog::getOpenFileName(
        this, "Load M3U Playlist", basePath,
        "Playlists (*.m3u *.m3u8);;All Files (*)");
    if (file.isEmpty()) return;

    QFile f(file);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Failed to open playlist.");
        return;
    }

    QList<QUrl> urls;
    QTextStream in(&f);
    const QString playlistDir = QFileInfo(file).absolutePath();

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith("#"))
            continue;

        QString fullPath;
        QFileInfo fi(line);

        if (fi.isAbsolute()) {
            fullPath = line;
        } else {
            fullPath = QDir(playlistDir).absoluteFilePath(line);
            if (!QFile::exists(fullPath))
                fullPath = QDir(binPath).absoluteFilePath(line);
            if (!QFile::exists(fullPath))
                fullPath = QDir(basePath).absoluteFilePath(line);
        }

        if (QFile::exists(fullPath))
            urls.push_back(QUrl::fromLocalFile(fullPath));
    }

    enqueue(urls);
    statusBar()->showMessage("Playlist loaded (relative paths supported).", 3000);
}

// 播放選取項目
void MainWindow::playSelected(int row) {
    if (row < 0 || row >= m_urls.size()) return;
    playIndex(row);
}

// 播放 | 暫停
void MainWindow::playPause() {
    using S = QMediaPlayer::PlaybackState;
    if (m_player->playbackState() == S::PlayingState) {
        m_player->pause();
    } else if (m_player->playbackState() == S::PausedState) {
        m_player->play();
    } else {
        if (m_currentIndex < 0 && !m_urls.isEmpty()) playIndex(0);
        else m_player->play();
    }
}

// 停
void MainWindow::stop() const {
    m_player->stop();
    m_btnPlayPause->setIcon(QIcon(":/icons/play.svg"));
}

// 下一個
void MainWindow::next() {
    if (m_urls.isEmpty()) return;
    const qsizetype nextIdx = (m_currentIndex + 1) % m_urls.size();
    playIndex(static_cast<int>(nextIdx));
}

// 上一個
void MainWindow::previous() {
    if (m_urls.isEmpty()) return;
    const qsizetype prevIdx = (m_currentIndex - 1 + m_urls.size()) % m_urls.size();
    playIndex(static_cast<int>(prevIdx));
}

// 播放位置變更
void MainWindow::onPositionChanged(qint64 pos) {
    if (m_durationMs > 0) {
        const int v = static_cast<int>((pos * 1000) / m_durationMs);
        if (!m_seek->isSliderDown()) {
            m_syncingFromPlayer = true;
            m_seek->setValue(std::clamp(v, 0, 1000));
            m_syncingFromPlayer = false;
        }
    }
    updateTimeLabels(pos, m_durationMs);

    if (m_durationMs > 0 && pos >= m_durationMs - 10) {
        if (m_player->playbackState() == QMediaPlayer::PlayingState) next();
    }
}

// 總長度變更
void MainWindow::onDurationChanged(const qint64 dur) {
    m_durationMs = dur;
    updateTimeLabels(m_player->position(), dur);
}

// 播放狀態變更
void MainWindow::onStateChanged() const {
    using S = QMediaPlayer::PlaybackState;
    if (m_player->playbackState() == S::PlayingState) {
        m_btnPlayPause->setIcon(QIcon(":/icons/pause.svg"));
        statusBar()->showMessage("Playing");
    } else if (m_player->playbackState() == S::PausedState) {
        m_btnPlayPause->setIcon(QIcon(":/icons/play.svg"));
        statusBar()->showMessage("Paused");
    } else {
        m_btnPlayPause->setIcon(QIcon(":/icons/play.svg"));
        statusBar()->showMessage("Stopped");
    }
}

// 錯誤處理
void MainWindow::onErrorChanged() {
    if (const auto err = m_player->error(); err != QMediaPlayer::NoError)
        QMessageBox::warning(
            this, "Playback Error", m_player->errorString());
}

// 音量變更
void MainWindow::onVolumeChanged(int) const {
    int v = static_cast<int>(std::lround(m_audio->volume() * 100.0));
    v = std::clamp(v, 0, 100);
    if (m_volume->value() != v)
        m_volume->setValue(v);

    m_btnMute->setIcon(QIcon(m_audio->isMuted()
                                 ? ":/icons/volume-mute.svg"
                                 : ":/icons/volume.svg"));
}

// 拖曳進度條
void MainWindow::onSeek(int v) const {
    if (m_durationMs <= 0) return;
    if (m_syncingFromPlayer) return;
    const qint64 target = (m_durationMs * v) / 1000;
    m_player->setPosition(target);
}

// 靜音切換
void MainWindow::toggleMute() const {
    m_audio->setMuted(!m_audio->isMuted());
    onVolumeChanged(0);
}

// 播放指定索引
void MainWindow::playIndex(int idx) {
    if (idx < 0 || idx >= m_urls.size()) return;

    for (int i = 0; i < m_list->count(); ++i) {
        QFont f = m_list->item(i)->font();
        f.setBold(false);
        m_list->item(i)->setFont(f);
    }

    m_currentIndex = idx;
    m_list->setCurrentRow(idx);

    if (auto *item = m_list->item(idx)) {
        QFont f = item->font();
        f.setBold(true);
        item->setFont(f);
    }

    m_durationMs = 0;
    updateTimeLabels(0, 0);

    m_player->setSource(m_urls[idx]);
    m_player->play();
    setWindowTitle(QString("MusicPlayer"));
}

// 更新時間標籤
void MainWindow::updateTimeLabels(const qint64 pos, const qint64 dur) const {
    m_lblTimeL->setText(formatTime(pos));
    m_lblTimeR->setText(formatTime(dur));
}

// 格式化時間
QString MainWindow::formatTime(qint64 ms) {
    if (ms < 0) ms = 0;
    const qint64 sec = ms / 1000;
    const int h = static_cast<int>(sec / 3600);
    const int m = static_cast<int>((sec % 3600) / 60);
    const int s = static_cast<int>(sec % 60);
    if (h > 0)
        return QString("%1:%2:%3")
                .arg(h, 1, 10, QLatin1Char('0'))
                .arg(m, 2, 10, QLatin1Char('0'))
                .arg(s, 2, 10, QLatin1Char('0'));
    return QString("%1:%2")
            .arg(m, 2, 10, QLatin1Char('0'))
            .arg(s, 2, 10, QLatin1Char('0'));
}

// 判斷是否為音訊檔案
bool MainWindow::isAudioUrl(const QUrl &url) {
    const QString f = url.fileName().toLower();
    return f.endsWith(".mp3") || f.endsWith(".wav") || f.endsWith(".flac") ||
           f.endsWith(".m4a") || f.endsWith(".aac") || f.endsWith(".ogg") ||
           f.endsWith(".opus");
}

// 事件
void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) event->acceptProposedAction();
}

// 事件
void MainWindow::dropEvent(QDropEvent *event) {
    if (const QList<QUrl> urls = event->mimeData()->urls(); !urls.isEmpty()) enqueue(urls);
}
