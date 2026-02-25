#include "mainwindow.h"

#include <QCloseEvent>
#include <QCheckBox>
#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialog>
#include <QCryptographicHash>
#include <QCursor>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QEnterEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QKeySequenceEdit>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QSlider>
#include <QSpinBox>
#include <QShortcut>
#include <QTextBrowser>
#include <QTextDocument>
#include <QTimer>
#include <QToolButton>
#include <QPropertyAnimation>
#include <QVBoxLayout>
#include <utility>

namespace {
QStringList audioFilters() {
    return {"*.mp3", "*.wav", "*.flac", "*.m4a", "*.ogg", "*.aac"};
}

QSettings makeAppSettings() {
    const QString iniPath =
        QDir(QCoreApplication::applicationDirPath()).filePath(
            QStringLiteral("AceStep15DatasetManager.ini"));
    return QSettings(iniPath, QSettings::IniFormat);
}

QString resolveHelpMarkdownPath(const QString &fileName) {
    const QStringList candidates = {
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("Help/") + fileName),
        QDir(QCoreApplication::applicationDirPath()).filePath(fileName),
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("../../src/Help/") + fileName),
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("../src/Help/") + fileName),
        QDir(QDir::currentPath()).filePath(QStringLiteral("src/Help/") + fileName),
        QDir(QDir::currentPath()).filePath(QStringLiteral("Help/") + fileName),
    };
    for (const QString &path : candidates) {
        if (QFileInfo::exists(path) && QFileInfo(path).isFile()) {
            return QFileInfo(path).absoluteFilePath();
        }
    }
    return QString();
}

QString readUtf8TextFile(const QString &path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        return QString();
    }
    return QString::fromUtf8(f.readAll());
}

QString formatDateTimeMicros(const QDateTime &dt) {
    const QDateTime local = dt.toLocalTime();
    const QString base = local.toString("yyyy-MM-ddTHH:mm:ss");
    const int micros = local.time().msec() * 1000;
    const QString frac = QString("%1").arg(micros, 6, 10, QChar('0'));
    return base + "." + frac;
}

QString jsonQuoted(const QString &value) {
    QJsonArray arr;
    arr.append(value);
    const QString compact = QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));
    return compact.mid(1, compact.size() - 2);
}

void appendField(QString &out, int indent, const QString &key, const QString &value, bool comma = true) {
    out += QString(indent, ' ') + jsonQuoted(key) + ": " + jsonQuoted(value);
    out += comma ? ",\n" : "\n";
}

void appendField(QString &out, int indent, const QString &key, int value, bool comma = true) {
    out += QString(indent, ' ') + jsonQuoted(key) + ": " + QString::number(value);
    out += comma ? ",\n" : "\n";
}

void appendField(QString &out, int indent, const QString &key, bool value, bool comma = true) {
    out += QString(indent, ' ') + jsonQuoted(key) + ": " + (value ? "true" : "false");
    out += comma ? ",\n" : "\n";
}

void appendNullField(QString &out, int indent, const QString &key, bool comma = true) {
    out += QString(indent, ' ') + jsonQuoted(key) + ": null";
    out += comma ? ",\n" : "\n";
}

QString tagPositionToUi(const QString &raw) {
    if (raw == "append") {
        return "Append (Caption, Tag)";
    }
    if (raw == "replace_caption" || raw == "replace") {
        return "Replace Caption";
    }
    return "Prepend (Tag, Caption)";
}

QString uiToTagPosition(const QString &ui) {
    if (ui.startsWith("Append")) {
        return "append";
    }
    if (ui.startsWith("Replace")) {
        return "replace";
    }
    return "prepend";
}

QByteArray buildOrderedJson(const DatasetMetadata &meta, const QList<TrackData> &tracks) {
    QString out;
    out += "{\n";
    out += "  \"metadata\": {\n";
    appendField(out, 4, "name", meta.name);
    appendField(out, 4, "custom_tag", meta.customTag);
    appendField(out, 4, "tag_position", meta.tagPosition);
    appendField(out, 4, "created_at", formatDateTimeMicros(meta.createdAt));
    appendField(out, 4, "num_samples", static_cast<int>(tracks.size()));
    appendField(out, 4, "all_instrumental", meta.allInstrumental);
    appendField(out, 4, "genre_ratio", meta.genreRatio, false);
    out += "  },\n";
    out += "  \"samples\": [\n";

    for (int i = 0; i < tracks.size(); ++i) {
        const TrackData &t = tracks[i];
        out += "    {\n";
        appendField(out, 6, "id", t.id);
        appendField(out, 6, "audio_path", QDir::toNativeSeparators(t.audioPath));
        appendField(out, 6, "filename", t.filename);
        appendField(out, 6, "caption", t.caption);
        appendField(out, 6, "genre", t.genre);
        appendField(out, 6, "lyrics", t.lyrics);
        appendField(out, 6, "raw_lyrics", QString(""));
        appendField(out, 6, "formatted_lyrics", t.lyrics);
        appendField(out, 6, "bpm", t.bpm);
        appendField(out, 6, "keyscale", t.keyscale);
        appendField(out, 6, "timesignature", t.timesignature);
        appendField(out, 6, "duration", t.duration);
        appendField(out, 6, "language", t.language);
        appendField(out, 6, "is_instrumental", t.isInstrumental);
        appendField(out, 6, "custom_tag", meta.customTag);
        appendField(out, 6, "labeled", !t.caption.trimmed().isEmpty());
        if (t.promptOverride.trimmed().isEmpty()) {
            appendNullField(out, 6, "prompt_override", false);
        } else {
            appendField(out, 6, "prompt_override", t.promptOverride.trimmed().toLower(), false);
        }
        out += (i + 1 < tracks.size()) ? "    },\n" : "    }\n";
    }

    out += "  ]\n";
    out += "}\n";
    return out.toUtf8();
}

class SaveToastWidget : public QFrame {
public:
    explicit SaveToastWidget(QWidget *parent = nullptr) : QFrame(parent) {
        setObjectName("SaveToast");
        setAttribute(Qt::WA_Hover, true);
        setFrameShape(QFrame::NoFrame);
        setWindowFlags(Qt::Widget);

        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins(12, 10, 8, 10);
        layout->setSpacing(10);

        m_textLabel = new QLabel(this);
        m_textLabel->setWordWrap(true);
        m_textLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        layout->addWidget(m_textLabel, 1);

        auto *closeBtn = new QToolButton(this);
        closeBtn->setText("x");
        closeBtn->setAutoRaise(true);
        closeBtn->setCursor(Qt::PointingHandCursor);
        closeBtn->setFixedSize(22, 22);
        layout->addWidget(closeBtn, 0, Qt::AlignTop);
        connect(closeBtn, &QToolButton::clicked, this, [this]() { startFadeOut(); });

        setStyleSheet(
            "QFrame#SaveToast {"
            " border: 1px solid #4f8c5f;"
            " border-radius: 8px;"
            " background-color: #203126;"
            "}"
            "QFrame#SaveToast QLabel { color: #d8ffe0; }"
            "QFrame#SaveToast QToolButton { color: #d8ffe0; }"
            "QFrame#SaveToast QToolButton:hover { background: #2d4636; border-radius: 4px; }");

        m_opacityEffect = new QGraphicsOpacityEffect(this);
        m_opacityEffect->setOpacity(0.0);
        setGraphicsEffect(m_opacityEffect);

        m_fadeAnim = new QPropertyAnimation(m_opacityEffect, "opacity", this);
        m_fadeAnim->setDuration(180);

        m_timer.setSingleShot(true);
        connect(&m_timer, &QTimer::timeout, this, [this]() { startFadeOut(); });
        connect(m_fadeAnim, &QPropertyAnimation::finished, this, [this]() {
            if (m_pendingHide && m_opacityEffect->opacity() <= 0.01) {
                m_pendingHide = false;
                hide();
            }
        });
    }

    void setMaxToastWidth(int px) {
        m_maxWidth = qMax(220, px);
        relayoutToContent();
    }

    void showMessage(const QString &message, int autoHideMs = 4000) {
        m_textLabel->setText(message);
        relayoutToContent();
        m_remainingMs = autoHideMs;
        m_timer.start(m_remainingMs);
        m_elapsed.restart();
        m_pendingHide = false;
        m_fadeAnim->stop();
        m_opacityEffect->setOpacity(0.0);
        show();
        raise();
        m_fadeAnim->setStartValue(0.0);
        m_fadeAnim->setEndValue(1.0);
        m_fadeAnim->start();
    }

protected:
    void enterEvent(QEnterEvent *event) override {
        Q_UNUSED(event);
        if (m_fadeAnim->state() == QAbstractAnimation::Running && m_pendingHide) {
            m_fadeAnim->stop();
            m_pendingHide = false;
            m_opacityEffect->setOpacity(1.0);
        }
        if (m_timer.isActive()) {
            const int spent = static_cast<int>(m_elapsed.elapsed());
            m_remainingMs = qMax(400, m_remainingMs - spent);
            m_timer.stop();
        }
    }

    void leaveEvent(QEvent *event) override {
        QFrame::leaveEvent(event);
        if (isVisible() && m_remainingMs > 0 && !m_timer.isActive()) {
            m_elapsed.restart();
            m_timer.start(m_remainingMs);
        }
    }

private:
    void relayoutToContent() {
        auto *box = qobject_cast<QBoxLayout *>(layout());
        if (!m_textLabel || !box) {
            return;
        }
        const QMargins margins = box->contentsMargins();
        const int spacing = box->spacing();
        const int closeW = 22;
        const int chrome = margins.left() + margins.right() + spacing + closeW;
        const int availableTextW = qMax(120, m_maxWidth - chrome);
        const int rawTextW = m_textLabel->fontMetrics().horizontalAdvance(m_textLabel->text()) + 8;
        const bool wrap = rawTextW > availableTextW;
        m_textLabel->setWordWrap(wrap);
        m_textLabel->setMaximumWidth(availableTextW);
        setFixedWidth(wrap ? m_maxWidth : qMin(m_maxWidth, chrome + rawTextW));
        adjustSize();
    }

    void startFadeOut() {
        if (!isVisible()) {
            return;
        }
        m_timer.stop();
        m_pendingHide = true;
        m_fadeAnim->stop();
        m_fadeAnim->setStartValue(m_opacityEffect->opacity());
        m_fadeAnim->setEndValue(0.0);
        m_fadeAnim->start();
    }

    QLabel *m_textLabel = nullptr;
    QGraphicsOpacityEffect *m_opacityEffect = nullptr;
    QPropertyAnimation *m_fadeAnim = nullptr;
    QTimer m_timer;
    QElapsedTimer m_elapsed;
    int m_remainingMs = 4000;
    int m_maxWidth = 760;
    bool m_pendingHide = false;
};
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUi();
    QSettings s = makeAppSettings();
    m_lastOpenDir = s.value("ui/lastDatasetDir").toString();
    const QByteArray geometry = s.value("ui/windowGeometry").toByteArray();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }
    m_fontSlider->setValue(s.value("ui/fontSize", m_fontSlider->value()).toInt());
    m_onTopCheck->setChecked(s.value("ui/alwaysOnTop", false).toBool());
    if (m_captionLyricsOnlyCheck) {
        m_captionLyricsOnlyCheck->setChecked(s.value("ui/captionLyricsOnlyMode", false).toBool());
    }
    if (m_seekStepSecondsSpin) {
        m_seekStepSecondsSpin->setValue(s.value("ui/seekStepSeconds", 10).toInt());
    }
    if (m_seekBackwardShortcutEdit) {
        const QKeySequence seq(
            s.value("ui/seekBackwardShortcut", QStringLiteral("Alt+Left")).toString());
        m_seekBackwardShortcutEdit->setKeySequence(seq);
        if (m_seekBackwardShortcut) {
            m_seekBackwardShortcut->setKey(seq);
        }
    }
    if (m_seekForwardShortcutEdit) {
        const QKeySequence seq(
            s.value("ui/seekForwardShortcut", QStringLiteral("Alt+Right")).toString());
        m_seekForwardShortcutEdit->setKeySequence(seq);
        if (m_seekForwardShortcut) {
            m_seekForwardShortcut->setKey(seq);
        }
    }
    if (m_focusShortcutEdit) {
        const QKeySequence seq(
            s.value("ui/focusModeShortcut", QStringLiteral("Ctrl+F")).toString());
        m_focusShortcutEdit->setKeySequence(seq);
        if (m_focusShortcut) {
            m_focusShortcut->setKey(seq);
        }
    }
    if (m_saveShortcutEdit) {
        const QKeySequence seq(s.value("ui/saveShortcut", QStringLiteral("Ctrl+S")).toString());
        m_saveShortcutEdit->setKeySequence(seq);
        if (m_saveShortcut) {
            m_saveShortcut->setKey(seq);
        }
    }
    if (m_backupShortcutEdit) {
        const QKeySequence seq(
            s.value("ui/backupShortcut", QStringLiteral("Ctrl+B")).toString());
        m_backupShortcutEdit->setKeySequence(seq);
        if (m_backupShortcut) {
            m_backupShortcut->setKey(seq);
        }
    }
    if (m_playPauseShortcutEdit) {
        const QKeySequence seq(
            s.value("ui/playPauseShortcut", QStringLiteral("Pause")).toString());
        m_playPauseShortcutEdit->setKeySequence(seq);
        if (m_playPauseShortcut) {
            m_playPauseShortcut->setKey(seq);
        }
    }
    captureMetaSnapshot();
    updateStats();
}

void MainWindow::updateMainWindowTitle() {
    QString suffix;
    if (m_currentSourceIsExplicitJson && !m_currentJsonPath.isEmpty()) {
        suffix = QDir::toNativeSeparators(m_currentJsonPath);
    } else if (!m_currentFolder.isEmpty()) {
        suffix = QDir::toNativeSeparators(m_currentFolder);
    }
    setWindowTitle(suffix.isEmpty() ? QStringLiteral("Ace Step 1.5 Dataset Manager")
                                    : QStringLiteral("Ace Step 1.5 Dataset Manager (%1)").arg(suffix));
}

void MainWindow::setupUi() {
    setWindowTitle("Ace Step 1.5 Dataset Manager");
    resize(1650, 940);

    auto *central = new QWidget(this);
    auto *root = new QHBoxLayout(central);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(8);

    auto *leftWrap = new QVBoxLayout();
    m_globalGroup = new QGroupBox("General Properties", central);
    auto *globalLayout = new QGridLayout(m_globalGroup);

    m_nameEdit = new QLineEdit(m_globalGroup);
    m_customTagEdit = new QLineEdit(m_globalGroup);
    m_allInstrumentalCheck = new QCheckBox("All Instrumental", m_globalGroup);
    m_tagPositionCombo = new QComboBox(m_globalGroup);
    m_tagPositionCombo->addItems({"Prepend (Tag, Caption)", "Append (Caption, Tag)", "Replace Caption"});
    m_genreRatioSlider = new QSlider(Qt::Horizontal, m_globalGroup);
    m_genreRatioSlider->setRange(0, 100);
    m_genreRatioLabel = new QLabel("0%", m_globalGroup);

    globalLayout->addWidget(new QLabel("Name"), 0, 0);
    globalLayout->addWidget(m_nameEdit, 0, 1);
    globalLayout->addWidget(new QLabel("Custom Trigger Tag"), 1, 0);
    globalLayout->addWidget(m_customTagEdit, 1, 1);
    globalLayout->addWidget(m_allInstrumentalCheck, 2, 0, 1, 2);
    globalLayout->addWidget(new QLabel("Tag Position"), 3, 0);
    globalLayout->addWidget(m_tagPositionCombo, 3, 1);
    globalLayout->addWidget(new QLabel("Genre Ratio (%)"), 4, 0);
    globalLayout->addWidget(m_genreRatioSlider, 4, 1);
    globalLayout->addWidget(m_genreRatioLabel, 4, 2);

    auto *datasetGroup = new QGroupBox("Dataset", central);
    auto *datasetLayout = new QVBoxLayout(datasetGroup);
    m_datasetScroll = new QScrollArea(datasetGroup);
    m_datasetScroll->setWidgetResizable(true);
    m_datasetContainer = new QWidget(m_datasetScroll);
    m_trackLayout = new QVBoxLayout(m_datasetContainer);
    m_trackLayout->setAlignment(Qt::AlignTop);
    m_trackLayout->setSpacing(10);
    m_datasetContainer->setLayout(m_trackLayout);
    m_datasetScroll->setWidget(m_datasetContainer);
    datasetLayout->addWidget(m_datasetScroll);
    if (m_datasetScroll->verticalScrollBar()) {
        connect(m_datasetScroll->verticalScrollBar(), &QScrollBar::valueChanged,
                this, &MainWindow::onDatasetScrollChanged);
    }

    leftWrap->addWidget(m_globalGroup);
    leftWrap->addWidget(datasetGroup, 1);

    auto *rightScroll = new QScrollArea(central);
    rightScroll->setWidgetResizable(true);
    rightScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    rightScroll->setFixedWidth(336);
    rightScroll->setFrameShape(QFrame::NoFrame);
    m_rightPanel = rightScroll;

    auto *rightPanelContent = new QWidget(rightScroll);
    rightPanelContent->setMinimumWidth(320);
    rightPanelContent->setMaximumWidth(320);
    auto *rightLayout = new QVBoxLayout(rightPanelContent);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(8);

    auto *fileGroup = new QGroupBox("File", rightPanelContent);
    auto *fileLayout = new QVBoxLayout(fileGroup);
    auto *openJsonBtn = new QPushButton("Open .json file", fileGroup);
    auto *openFolderBtn = new QPushButton("Open dataset folder", fileGroup);
    auto *saveBtn = new QPushButton("Save", fileGroup);
    auto *saveAsBtn = new QPushButton("Save As", fileGroup);
    auto *reloadBtn = new QPushButton("Reload", fileGroup);
    auto *backupBtn = new QPushButton("Make backup", fileGroup);
    fileLayout->addWidget(openJsonBtn);
    fileLayout->addWidget(openFolderBtn);
    fileLayout->addWidget(saveBtn);
    fileLayout->addWidget(saveAsBtn);
    fileLayout->addWidget(backupBtn);
    fileLayout->addWidget(reloadBtn);

    auto *controlGroup = new QGroupBox("Controls", rightPanelContent);
    auto *controlLayout = new QVBoxLayout(controlGroup);
    auto *mergeBtn = new QPushButton("Merge paragraphs", controlGroup);
    auto *expandAllBtn = new QPushButton("Expand all", controlGroup);
    auto *collapseAllBtn = new QPushButton("Collapse all", controlGroup);
    controlLayout->addWidget(mergeBtn);
    controlLayout->addWidget(expandAllBtn);
    controlLayout->addWidget(collapseAllBtn);

    auto *helpGroup = new QGroupBox("Help", rightPanelContent);
    auto *helpLayout = new QVBoxLayout(helpGroup);
    auto *captionTutorialBtn = new QPushButton("Caption Tutorial", helpGroup);
    auto *lyricsTutorialBtn = new QPushButton("Lyrics Tutorial", helpGroup);
    helpLayout->addWidget(captionTutorialBtn);
    helpLayout->addWidget(lyricsTutorialBtn);

    auto *settingsGroup = new QGroupBox("Settings", rightPanelContent);
    auto *settingsLayout = new QGridLayout(settingsGroup);
    m_onTopCheck = new QCheckBox("Always on top", settingsGroup);
    m_fontSlider = new QSlider(Qt::Horizontal, settingsGroup);
    m_fontSlider->setRange(8, 20);
    m_fontSlider->setValue(10);
    m_fontSlider->setTracking(false);
    m_fontSizeValueLabel = new QLabel(QString::number(m_fontSlider->value()), settingsGroup);
    m_fontSizeValueLabel->setMinimumWidth(28);
    m_fontSizeValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    settingsLayout->addWidget(new QLabel("Font Size"), 0, 0);
    settingsLayout->addWidget(m_fontSlider, 0, 1);
    settingsLayout->addWidget(m_fontSizeValueLabel, 0, 2);
    settingsLayout->addWidget(m_onTopCheck, 1, 0, 1, 3);
    m_focusShortcutEdit = new QKeySequenceEdit(QKeySequence("Ctrl+F"), settingsGroup);
    settingsLayout->addWidget(new QLabel("Focus Mode Hotkey"), 2, 0);
    settingsLayout->addWidget(m_focusShortcutEdit, 2, 1, 1, 2);
    m_saveShortcutEdit = new QKeySequenceEdit(QKeySequence("Ctrl+S"), settingsGroup);
    settingsLayout->addWidget(new QLabel("Save Hotkey"), 3, 0);
    settingsLayout->addWidget(m_saveShortcutEdit, 3, 1, 1, 2);
    m_backupShortcutEdit = new QKeySequenceEdit(QKeySequence("Ctrl+B"), settingsGroup);
    settingsLayout->addWidget(new QLabel("Backup Hotkey"), 4, 0);
    settingsLayout->addWidget(m_backupShortcutEdit, 4, 1, 1, 2);
    m_playPauseShortcutEdit = new QKeySequenceEdit(QKeySequence("Pause"), settingsGroup);
    settingsLayout->addWidget(new QLabel("Play/Pause Hotkey"), 5, 0);
    settingsLayout->addWidget(m_playPauseShortcutEdit, 5, 1, 1, 2);
    m_seekBackwardShortcutEdit = new QKeySequenceEdit(QKeySequence("Alt+Left"), settingsGroup);
    settingsLayout->addWidget(new QLabel("Seek Backward Hotkey"), 6, 0);
    settingsLayout->addWidget(m_seekBackwardShortcutEdit, 6, 1, 1, 2);
    m_seekForwardShortcutEdit = new QKeySequenceEdit(QKeySequence("Alt+Right"), settingsGroup);
    settingsLayout->addWidget(new QLabel("Seek Forward Hotkey"), 7, 0);
    settingsLayout->addWidget(m_seekForwardShortcutEdit, 7, 1, 1, 2);
    m_seekStepSecondsSpin = new QSpinBox(settingsGroup);
    m_seekStepSecondsSpin->setRange(1, 600);
    m_seekStepSecondsSpin->setValue(10);
    m_seekStepSecondsSpin->setSuffix(" s");
    settingsLayout->addWidget(new QLabel("Seek step"), 8, 0);
    settingsLayout->addWidget(m_seekStepSecondsSpin, 8, 1, 1, 2);
    m_captionLyricsOnlyCheck = new QCheckBox("Caption/Lyrics only in track cards", settingsGroup);
    settingsLayout->addWidget(m_captionLyricsOnlyCheck, 9, 0, 1, 3);
    connect(m_fontSlider, &QSlider::sliderMoved, this, [this](int v) {
        m_fontSizeValueLabel->setText(QString::number(v));
    });
    connect(m_fontSlider, &QSlider::valueChanged, this, [this](int v) {
        m_fontSizeValueLabel->setText(QString::number(v));
        QSettings s = makeAppSettings();
        s.setValue("ui/fontSize", v);
    });
    connect(m_focusShortcutEdit, &QKeySequenceEdit::keySequenceChanged, this,
            [this](const QKeySequence &seq) {
                const QKeySequence finalSeq = seq.isEmpty() ? QKeySequence("Ctrl+F") : seq;
                if (m_focusShortcut && m_focusShortcut->key() != finalSeq) {
                    m_focusShortcut->setKey(finalSeq);
                }
                QSettings s = makeAppSettings();
                s.setValue("ui/focusModeShortcut", finalSeq.toString(QKeySequence::PortableText));
            });
    connect(m_saveShortcutEdit, &QKeySequenceEdit::keySequenceChanged, this,
            [this](const QKeySequence &seq) {
                const QKeySequence finalSeq = seq.isEmpty() ? QKeySequence("Ctrl+S") : seq;
                if (m_saveShortcut && m_saveShortcut->key() != finalSeq) {
                    m_saveShortcut->setKey(finalSeq);
                }
                QSettings s = makeAppSettings();
                s.setValue("ui/saveShortcut", finalSeq.toString(QKeySequence::PortableText));
            });
    connect(m_backupShortcutEdit, &QKeySequenceEdit::keySequenceChanged, this,
            [this](const QKeySequence &seq) {
                const QKeySequence finalSeq = seq.isEmpty() ? QKeySequence("Ctrl+B") : seq;
                if (m_backupShortcut && m_backupShortcut->key() != finalSeq) {
                    m_backupShortcut->setKey(finalSeq);
                }
                QSettings s = makeAppSettings();
                s.setValue("ui/backupShortcut", finalSeq.toString(QKeySequence::PortableText));
            });
    connect(m_playPauseShortcutEdit, &QKeySequenceEdit::keySequenceChanged, this,
            [this](const QKeySequence &seq) {
                const QKeySequence finalSeq = seq.isEmpty() ? QKeySequence("Pause") : seq;
                if (m_playPauseShortcut && m_playPauseShortcut->key() != finalSeq) {
                    m_playPauseShortcut->setKey(finalSeq);
                }
                QSettings s = makeAppSettings();
                s.setValue("ui/playPauseShortcut", finalSeq.toString(QKeySequence::PortableText));
            });
    connect(m_seekBackwardShortcutEdit, &QKeySequenceEdit::keySequenceChanged, this,
            [this](const QKeySequence &seq) {
                const QKeySequence finalSeq = seq.isEmpty() ? QKeySequence("Alt+Left") : seq;
                if (m_seekBackwardShortcut && m_seekBackwardShortcut->key() != finalSeq) {
                    m_seekBackwardShortcut->setKey(finalSeq);
                }
                QSettings s = makeAppSettings();
                s.setValue("ui/seekBackwardShortcut", finalSeq.toString(QKeySequence::PortableText));
            });
    connect(m_seekForwardShortcutEdit, &QKeySequenceEdit::keySequenceChanged, this,
            [this](const QKeySequence &seq) {
                const QKeySequence finalSeq = seq.isEmpty() ? QKeySequence("Alt+Right") : seq;
                if (m_seekForwardShortcut && m_seekForwardShortcut->key() != finalSeq) {
                    m_seekForwardShortcut->setKey(finalSeq);
                }
                QSettings s = makeAppSettings();
                s.setValue("ui/seekForwardShortcut", finalSeq.toString(QKeySequence::PortableText));
            });
    connect(m_seekStepSecondsSpin, qOverload<int>(&QSpinBox::valueChanged), this, [this](int v) {
        QSettings s = makeAppSettings();
        s.setValue("ui/seekStepSeconds", qMax(1, v));
    });
    connect(m_captionLyricsOnlyCheck, &QCheckBox::toggled, this, [this](bool checked) {
        QSettings s = makeAppSettings();
        s.setValue("ui/captionLyricsOnlyMode", checked);
        for (AudioItemWidget *w : std::as_const(m_trackWidgets)) {
            w->setCaptionLyricsOnlyMode(checked);
        }
    });

    auto *authorGroup = new QGroupBox("About", rightPanelContent);
    auto *authorLayout = new QVBoxLayout(authorGroup);
    authorLayout->addWidget(new QLabel("NEYROSLAV"));
    auto *tg = new QLabel("<a href=\"https://t.me/neyroslav\">https://t.me/neyroslav</a>", authorGroup);
    tg->setOpenExternalLinks(true);
    authorLayout->addWidget(tg);
    auto *qtInfo = new QLabel(
        "This software uses Qt 6 (Qt Widgets / Qt Multimedia), licensed under LGPL v3.",
        authorGroup);
    qtInfo->setWordWrap(true);
    authorLayout->addWidget(qtInfo);

    auto *statsGroup = new QGroupBox("Statistics", rightPanelContent);
    auto *statsLayout = new QVBoxLayout(statsGroup);
    m_captionedLabel = new QLabel("Captioned (0/0) (0%)", statsGroup);
    m_toCaptionLabel = new QLabel("To Caption: 0", statsGroup);
    m_lyricsDoneLabel = new QLabel("Lyrics done: 0", statsGroup);
    m_lyricsLeftLabel = new QLabel("Lyrics left: 0", statsGroup);
    m_unsavedCardsLabel = new QLabel("Unsaved cards: 0", statsGroup);
    statsLayout->addWidget(m_captionedLabel);
    statsLayout->addWidget(m_toCaptionLabel);
    statsLayout->addWidget(m_lyricsDoneLabel);
    statsLayout->addWidget(m_lyricsLeftLabel);
    statsLayout->addWidget(m_unsavedCardsLabel);

    auto addCollapsibleSection = [this, rightLayout](const QString &key, const QString &title,
                                                     QGroupBox *group, bool defaultExpanded) {
        group->setTitle(QString());
        auto *header = new QToolButton(group->parentWidget());
        header->setText(title);
        header->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        QSettings s = makeAppSettings();
        const bool expanded =
            s.value(QStringLiteral("ui/rightSections/%1").arg(key), defaultExpanded).toBool();
        header->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
        header->setCheckable(true);
        header->setChecked(expanded);
        header->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        header->setStyleSheet(
            "QToolButton {"
            " text-align: left;"
            " padding: 6px 8px;"
            " font-weight: 600;"
            " border: 1px solid #4e596b;"
            " border-radius: 6px;"
            " background: #2a313c;"
            " color: #dfe7f3;"
            "}"
            "QToolButton:checked {"
            " background: #2a313c;"
            " color: #dfe7f3;"
            "}"
            "QToolButton:hover {"
            " background: #313947;"
            "}"
            "QToolButton:pressed {"
            " background: #252c36;"
            "}");
        connect(header, &QToolButton::toggled, this, [this, key, header, group](bool expanded) {
            header->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
            group->setVisible(expanded);
            QSettings s = makeAppSettings();
            s.setValue(QStringLiteral("ui/rightSections/%1").arg(key), expanded);
            if (m_datasetContainer) {
                m_datasetContainer->updateGeometry();
            }
            if (m_datasetScroll) {
                m_datasetScroll->updateGeometry();
            }
        });
        group->setVisible(expanded);
        rightLayout->addWidget(header);
        rightLayout->addWidget(group);
    };

    addCollapsibleSection("file", "File", fileGroup, true);
    addCollapsibleSection("controls", "Controls", controlGroup, false);
    addCollapsibleSection("help", "Help", helpGroup, false);
    addCollapsibleSection("settings", "Settings", settingsGroup, false);
    addCollapsibleSection("about", "About", authorGroup, true);
    addCollapsibleSection("statistics", "Statistics", statsGroup, true);
    rightLayout->addStretch();
    rightPanelContent->setLayout(rightLayout);
    rightScroll->setWidget(rightPanelContent);

    root->addLayout(leftWrap, 1);
    root->addWidget(m_rightPanel);
    setCentralWidget(central);

    m_focusShortcut = new QShortcut(QKeySequence("Ctrl+F"), this);
    m_focusShortcut->setContext(Qt::ApplicationShortcut);
    connect(m_focusShortcut, &QShortcut::activated, this, &MainWindow::toggleFocusMode);
    m_saveShortcut = new QShortcut(QKeySequence("Ctrl+S"), this);
    m_saveShortcut->setContext(Qt::ApplicationShortcut);
    connect(m_saveShortcut, &QShortcut::activated, this, &MainWindow::saveDataset);
    m_backupShortcut = new QShortcut(QKeySequence("Ctrl+B"), this);
    m_backupShortcut->setContext(Qt::ApplicationShortcut);
    connect(m_backupShortcut, &QShortcut::activated, this, &MainWindow::makeBackup);
    m_playPauseShortcut = new QShortcut(QKeySequence(QStringLiteral("Pause")), this);
    m_playPauseShortcut->setContext(Qt::ApplicationShortcut);
    connect(m_playPauseShortcut, &QShortcut::activated, this,
            &MainWindow::togglePlaybackOnTargetTrack);
    m_seekBackwardShortcut = new QShortcut(QKeySequence(QStringLiteral("Alt+Left")), this);
    m_seekBackwardShortcut->setContext(Qt::ApplicationShortcut);
    connect(m_seekBackwardShortcut, &QShortcut::activated, this, &MainWindow::seekPlaybackBackward);
    m_seekForwardShortcut = new QShortcut(QKeySequence(QStringLiteral("Alt+Right")), this);
    m_seekForwardShortcut->setContext(Qt::ApplicationShortcut);
    connect(m_seekForwardShortcut, &QShortcut::activated, this, &MainWindow::seekPlaybackForward);

    connect(openJsonBtn, &QPushButton::clicked, this, &MainWindow::openDatasetJsonFile);
    connect(openFolderBtn, &QPushButton::clicked, this, &MainWindow::openDatasetFolder);
    connect(saveBtn, &QPushButton::clicked, this, &MainWindow::saveDataset);
    connect(saveAsBtn, &QPushButton::clicked, this, &MainWindow::saveDatasetAs);
    connect(reloadBtn, &QPushButton::clicked, this, &MainWindow::refreshDataset);
    connect(mergeBtn, &QPushButton::clicked, this, &MainWindow::mergeParagraphs);
    connect(backupBtn, &QPushButton::clicked, this, &MainWindow::makeBackup);
    connect(expandAllBtn, &QPushButton::clicked, this, &MainWindow::expandAll);
    connect(collapseAllBtn, &QPushButton::clicked, this, &MainWindow::collapseAll);
    connect(captionTutorialBtn, &QPushButton::clicked, this, &MainWindow::showCaptionTutorial);
    connect(lyricsTutorialBtn, &QPushButton::clicked, this, &MainWindow::showLyricsTutorial);
    connect(m_allInstrumentalCheck, &QCheckBox::toggled, this, &MainWindow::onAllInstrumentalToggled);
    connect(m_genreRatioSlider, &QSlider::valueChanged, this, [this](int v) {
        m_genreRatioLabel->setText(QString::number(v) + "%");
    });
    connect(m_onTopCheck, &QCheckBox::toggled, this, [this](bool) { onAlwaysOnTopChanged(); });
}

void MainWindow::openDatasetFolder() {
    const QString startDir = m_lastOpenDir.isEmpty() ? QDir::homePath() : m_lastOpenDir;
    const QString folder = QFileDialog::getExistingDirectory(this, "Open Dataset Folder", startDir);
    if (folder.isEmpty()) {
        return;
    }
    m_lastOpenDir = folder;
    m_currentSourceIsExplicitJson = false;
    QSettings s = makeAppSettings();
    s.setValue("ui/lastDatasetDir", m_lastOpenDir);
    loadFromFolder(folder);
}

void MainWindow::openDatasetJsonFile() {
    const QString startDir = m_lastOpenDir.isEmpty() ? QDir::homePath() : m_lastOpenDir;
    const QString jsonPath = QFileDialog::getOpenFileName(this, "Open Dataset JSON", startDir,
                                                          "JSON files (*.json)");
    if (jsonPath.isEmpty()) {
        return;
    }

    const QFileInfo fi(jsonPath);
    m_currentFolder = fi.absolutePath();
    m_lastOpenDir = m_currentFolder;
    QSettings s = makeAppSettings();
    s.setValue("ui/lastDatasetDir", m_lastOpenDir);

    if (!loadFromJson(jsonPath)) {
        QMessageBox::warning(this, "Open Dataset JSON", "Failed to load JSON file.");
        return;
    }
    m_currentSourceIsExplicitJson = true;
    updateMainWindowTitle();
    for (AudioItemWidget *w : std::as_const(m_trackWidgets)) {
        w->markSaved();
    }
    captureMetaSnapshot();
    updateStats();
}

void MainWindow::saveDataset() {
    if (m_currentFolder.isEmpty()) {
        QMessageBox::warning(this, "Save", "Open a dataset first.");
        return;
    }

    m_meta.name = m_nameEdit->text().trimmed();
    m_meta.customTag = m_customTagEdit->text().trimmed();
    m_meta.allInstrumental = m_allInstrumentalCheck->isChecked();
    m_meta.tagPosition = uiToTagPosition(m_tagPositionCombo->currentText());
    m_meta.genreRatio = m_genreRatioSlider->value();
    m_meta.createdAt = QDateTime::currentDateTime();

    const QList<TrackData> tracks = collectTracks();
    const QString outPath = !m_currentJsonPath.isEmpty() ? m_currentJsonPath : defaultJsonPath();
    QFile f(outPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::critical(this, "Save", "Failed to write JSON file.");
        return;
    }

    f.write(buildOrderedJson(m_meta, tracks));
    f.close();
    for (AudioItemWidget *w : std::as_const(m_trackWidgets)) {
        w->markSaved();
    }
    captureMetaSnapshot();
    m_currentJsonPath = outPath;
    updateStats();
    showPathToast(QStringLiteral("Saved"), outPath);
}

void MainWindow::saveDatasetAs() {
    if (m_currentFolder.isEmpty()) {
        QMessageBox::warning(this, "Save As", "Open a dataset first.");
        return;
    }

    const QString suggested = !m_currentJsonPath.isEmpty() ? m_currentJsonPath : defaultJsonPath();
    QString outPath =
        QFileDialog::getSaveFileName(this, "Save Dataset As", suggested, "JSON files (*.json)");
    if (outPath.isEmpty()) {
        return;
    }
    if (!outPath.endsWith(".json", Qt::CaseInsensitive)) {
        outPath += ".json";
    }

    m_currentJsonPath = outPath;
    m_currentFolder = QFileInfo(outPath).absolutePath();
    m_lastOpenDir = m_currentFolder;
    m_currentSourceIsExplicitJson = true;
    QSettings s = makeAppSettings();
    s.setValue("ui/lastDatasetDir", m_lastOpenDir);
    updateMainWindowTitle();
    saveDataset();
}

void MainWindow::refreshDataset() {
    if (m_currentSourceIsExplicitJson && !m_currentJsonPath.isEmpty() &&
        QFileInfo::exists(m_currentJsonPath)) {
        if (loadFromJson(m_currentJsonPath)) {
            for (AudioItemWidget *w : std::as_const(m_trackWidgets)) {
                w->markSaved();
            }
            captureMetaSnapshot();
            updateStats();
            showPathToast(QStringLiteral("Reloaded"), m_currentJsonPath);
        }
        return;
    }
    if (m_currentFolder.isEmpty()) {
        return;
    }
    loadFromFolder(m_currentFolder);
    showPathToast(QStringLiteral("Reloaded"), m_currentFolder);
}

void MainWindow::mergeParagraphs() {
    const QRegularExpression re("\\n+");
    for (AudioItemWidget *w : std::as_const(m_trackWidgets)) {
        TrackData d = w->data();
        d.caption = d.caption.replace(re, " ").simplified();
        w->setCaptionText(d.caption);
    }
    updateStats();
}

void MainWindow::makeBackup() {
    if (m_currentFolder.isEmpty()) {
        QMessageBox::warning(this, "Backup", "Open a dataset first.");
        return;
    }
    const QString source = m_currentJsonPath.isEmpty() ? defaultJsonPath() : m_currentJsonPath;
    if (!QFileInfo::exists(source)) {
        saveDataset();
    }
    if (!QFileInfo::exists(source)) {
        QMessageBox::warning(this, "Backup", "No JSON file available for backup.");
        return;
    }
    QDir backupDir(m_currentFolder);
    if (!backupDir.exists("_Backup")) {
        backupDir.mkpath("_Backup");
    }
    const QString base = QFileInfo(source).baseName();
    const QString dst =
        backupDir.filePath("_Backup/" + base + "_" + currentTimestampFileSafe() + ".json");
    if (QFile::copy(source, dst)) {
        showPathToast(QStringLiteral(u"Backup created"), dst);
    } else {
        QMessageBox::critical(this, "Backup", "Failed to create backup.");
    }
}

void MainWindow::expandAll() {
    for (AudioItemWidget *w : std::as_const(m_trackWidgets)) {
        w->setExpanded(true);
    }
}

void MainWindow::collapseAll() {
    for (AudioItemWidget *w : std::as_const(m_trackWidgets)) {
        w->setExpanded(false);
    }
}

void MainWindow::updateStats() {
    const QList<TrackData> tracks = collectTracks();
    const int total = tracks.size();
    int captioned = 0;
    int lyricsDone = 0;
    for (const TrackData &t : tracks) {
        if (!t.caption.trimmed().isEmpty()) {
            ++captioned;
        }
        if (!t.lyrics.trimmed().isEmpty()) {
            ++lyricsDone;
        }
    }
    const int toCaption = total - captioned;
    const int lyricsLeft = total - lyricsDone;
    const int pct = total > 0 ? static_cast<int>((captioned * 100.0) / total + 0.5) : 0;
    const int lyricsPct = total > 0 ? static_cast<int>((lyricsDone * 100.0) / total + 0.5) : 0;
    const int unsaved = unsavedCardsCount();
    m_captionedLabel->setText(QString("Captioned (%1/%2) (%3%)").arg(captioned).arg(total).arg(pct));
    m_toCaptionLabel->setText(QString("To Caption: %1").arg(toCaption));
    m_lyricsDoneLabel->setText(QString("Lyrics done (%1/%2) (%3%)").arg(lyricsDone).arg(total).arg(lyricsPct));
    m_lyricsLeftLabel->setText(QString("Lyrics left: %1").arg(lyricsLeft));
    m_unsavedCardsLabel->setText(QString("Unsaved cards: %1").arg(unsaved));
    m_unsavedCardsLabel->setStyleSheet(
        unsaved > 0 ? "QLabel { color: #ff7b7b; font-weight: 600; }" : "");
}

void MainWindow::onDeleteTrack(AudioItemWidget *item) {
    const int idx = m_trackWidgets.indexOf(item);
    if (idx < 0) {
        return;
    }
    if (m_lastPlaybackActiveTrack == item) {
        m_lastPlaybackActiveTrack = nullptr;
    }
    m_trackWidgets.removeAt(idx);
    item->deleteLater();
    for (int i = 0; i < m_trackWidgets.size(); ++i) {
        m_trackWidgets[i]->setIndex(i + 1);
    }
    updateStats();
}

void MainWindow::applyLanguageToAll(const QString &language) {
    for (AudioItemWidget *w : std::as_const(m_trackWidgets)) {
        w->setLanguageValue(language);
    }
    updateStats();
}

void MainWindow::applyFieldToAll(const QString &field, const QString &value) {
    for (AudioItemWidget *w : std::as_const(m_trackWidgets)) {
        w->setFieldValue(field, value);
    }
    updateStats();
}

void MainWindow::onAllInstrumentalToggled(bool checked) {
    for (AudioItemWidget *w : std::as_const(m_trackWidgets)) {
        w->setInstrumentalValue(checked);
    }
    updateStats();
}

void MainWindow::onAlwaysOnTopChanged() {
    const bool onTop = m_onTopCheck->isChecked();
    Qt::WindowFlags flags = windowFlags();
    if (onTop) {
        flags |= Qt::WindowStaysOnTopHint;
    } else {
        flags &= ~Qt::WindowStaysOnTopHint;
    }
    setWindowFlags(flags);
    show();
    QSettings s = makeAppSettings();
    s.setValue("ui/alwaysOnTop", onTop);
}

AudioItemWidget *MainWindow::playbackTargetTrack() const {
    auto trackFromWidget = [this](QWidget *w) -> AudioItemWidget * {
        QWidget *cur = w;
        while (cur) {
            if (auto *track = qobject_cast<AudioItemWidget *>(cur)) {
                if (m_trackWidgets.contains(track)) {
                    return track;
                }
                return nullptr;
            }
            cur = cur->parentWidget();
        }
        return nullptr;
    };

    if (QWidget *fw = QApplication::focusWidget()) {
        if (AudioItemWidget *focusedTrack = trackFromWidget(fw)) {
            return focusedTrack;
        }
    }

    if (QWidget *hovered = QApplication::widgetAt(QCursor::pos())) {
        if (AudioItemWidget *hoveredTrack = trackFromWidget(hovered)) {
            return hoveredTrack;
        }
    }

    for (AudioItemWidget *w : m_trackWidgets) {
        if (w && w->isPlaying()) {
            return w;
        }
    }
    if (m_lastPlaybackActiveTrack && m_trackWidgets.contains(m_lastPlaybackActiveTrack)) {
        return m_lastPlaybackActiveTrack;
    }
    return nullptr;
}

void MainWindow::togglePlaybackOnTargetTrack() {
    AudioItemWidget *target = playbackTargetTrack();
    if (!target) {
        return;
    }
    target->togglePlayback();
}

void MainWindow::seekPlaybackBackward() {
    AudioItemWidget *target = playbackTargetTrack();
    if (!target) {
        return;
    }
    const int stepSec = m_seekStepSecondsSpin ? m_seekStepSecondsSpin->value() : 10;
    target->seekRelativeMs(-1000LL * qMax(1, stepSec));
}

void MainWindow::seekPlaybackForward() {
    AudioItemWidget *target = playbackTargetTrack();
    if (!target) {
        return;
    }
    const int stepSec = m_seekStepSecondsSpin ? m_seekStepSecondsSpin->value() : 10;
    target->seekRelativeMs(1000LL * qMax(1, stepSec));
}

void MainWindow::toggleFocusMode() {
    m_focusMode = !m_focusMode;
    if (m_globalGroup) {
        m_globalGroup->setVisible(!m_focusMode);
    }
    if (m_rightPanel) {
        m_rightPanel->setVisible(!m_focusMode);
    }
    if (m_datasetContainer) {
        m_datasetContainer->updateGeometry();
        m_datasetContainer->adjustSize();
    }
    if (m_trackLayout) {
        m_trackLayout->invalidate();
    }
    if (m_datasetScroll) {
        m_datasetScroll->updateGeometry();
    }
    for (AudioItemWidget *w : std::as_const(m_trackWidgets)) {
        w->updateStickyPosition();
    }
    showPathToast(m_focusMode ? QStringLiteral("Focus mode ON") : QStringLiteral("Focus mode OFF"),
                  m_currentSourceIsExplicitJson && !m_currentJsonPath.isEmpty() ? m_currentJsonPath
                                                                               : m_currentFolder);
}

void MainWindow::onDatasetScrollChanged(int) {
    for (AudioItemWidget *w : std::as_const(m_trackWidgets)) {
        w->updateStickyPosition();
    }
}

void MainWindow::clearTracks() {
    while (QLayoutItem *item = m_trackLayout->takeAt(0)) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    m_trackWidgets.clear();
}

void MainWindow::rebuildTrackList(const QList<TrackData> &tracks) {
    clearTracks();
    for (int i = 0; i < tracks.size(); ++i) {
        auto *w = new AudioItemWidget(i + 1, tracks[i], m_datasetContainer);
        w->setUiScale(m_fontSlider->value());
        if (m_captionLyricsOnlyCheck) {
            w->setCaptionLyricsOnlyMode(m_captionLyricsOnlyCheck->isChecked());
        }
        if (m_datasetScroll) {
            w->setStickyViewport(m_datasetScroll->viewport());
        }
        connect(w, &AudioItemWidget::deleteRequested, this, &MainWindow::onDeleteTrack);
        connect(w, &AudioItemWidget::saveRequested, this, &MainWindow::saveDataset);
        connect(w, &AudioItemWidget::playbackControlActivated, this, [this](AudioItemWidget *self) {
            m_lastPlaybackActiveTrack = self;
        });
        connect(w, &AudioItemWidget::languageApplyAllRequested, this, &MainWindow::applyLanguageToAll);
        connect(w, &AudioItemWidget::fieldApplyAllRequested, this, &MainWindow::applyFieldToAll);
        connect(w, &AudioItemWidget::changed, this, &MainWindow::updateStats);
        connect(w, &AudioItemWidget::layoutSizeChanged, this, [this]() {
            m_trackLayout->invalidate();
            m_datasetContainer->updateGeometry();
            m_datasetContainer->adjustSize();
            for (AudioItemWidget *tw : std::as_const(m_trackWidgets)) {
                tw->updateStickyPosition();
            }
        });
        connect(m_fontSlider, &QSlider::valueChanged, this, [this, w](int) {
            w->setUiScale(m_fontSlider->value());
        });
        m_trackLayout->addWidget(w);
        m_trackWidgets.append(w);
    }
    m_trackLayout->addStretch();

    for (AudioItemWidget *w : std::as_const(m_trackWidgets)) {
        w->updateStickyPosition();
        w->setInstrumentalValue(m_allInstrumentalCheck->isChecked());
    }
}

QList<TrackData> MainWindow::collectTracks() const {
    QList<TrackData> out;
    out.reserve(m_trackWidgets.size());
    for (AudioItemWidget *w : m_trackWidgets) {
        out.append(w->data());
    }
    return out;
}

void MainWindow::loadFromFolder(const QString &folderPath) {
    m_currentFolder = folderPath;
    updateMainWindowTitle();
    QDir dir(folderPath);
    QFileInfoList jsonFiles = dir.entryInfoList({"*.json"}, QDir::Files | QDir::Readable, QDir::Name);
    bool loadedFromJson = false;
    m_currentJsonPath.clear();
    if (!jsonFiles.isEmpty()) {
        loadedFromJson = loadFromJson(jsonFiles.first().absoluteFilePath());
    }
    if (!loadedFromJson) {
        QList<TrackData> tracks = buildFromAudioFiles(folderPath);
        m_meta = DatasetMetadata{};
        m_meta.name = QFileInfo(folderPath).baseName();
        m_nameEdit->setText(m_meta.name);
        m_customTagEdit->setText("");
        m_allInstrumentalCheck->setChecked(false);
        m_tagPositionCombo->setCurrentText(tagPositionToUi("prepend"));
        m_genreRatioSlider->setValue(0);
        rebuildTrackList(tracks);
    }
    for (AudioItemWidget *w : std::as_const(m_trackWidgets)) {
        w->markSaved();
    }
    captureMetaSnapshot();
    updateStats();
}

QList<TrackData> MainWindow::buildFromAudioFiles(const QString &folderPath) const {
    QDir dir(folderPath);
    const QFileInfoList files = dir.entryInfoList(audioFilters(), QDir::Files, QDir::Name);
    QList<TrackData> tracks;
    for (const QFileInfo &fi : files) {
        TrackData t;
        t.audioPath = fi.absoluteFilePath();
        t.filename = fi.fileName();
        t.id = generateId(t.audioPath);
        t.language = "instrumental";
        tracks.append(t);
    }
    return tracks;
}

bool MainWindow::loadFromJson(const QString &jsonPath) {
    QFile f(jsonPath);
    if (!f.open(QIODevice::ReadOnly)) {
        return false;
    }
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    f.close();
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return false;
    }

    const QJsonObject root = doc.object();
    const QJsonObject meta = root.value("metadata").toObject();
    m_meta.name = meta.value("name").toString("Dataset");
    m_meta.customTag = meta.value("custom_tag").toString();
    m_meta.tagPosition = sanitizeTagPosition(meta.value("tag_position").toString("prepend"));
    m_meta.createdAt = QDateTime::fromString(meta.value("created_at").toString(), Qt::ISODate);
    if (!m_meta.createdAt.isValid()) {
        m_meta.createdAt = QDateTime::currentDateTimeUtc();
    }
    m_meta.allInstrumental = meta.value("all_instrumental").toBool(false);
    m_meta.genreRatio = meta.value("genre_ratio").toInt(0);

    m_nameEdit->setText(m_meta.name);
    m_customTagEdit->setText(m_meta.customTag);
    m_allInstrumentalCheck->setChecked(m_meta.allInstrumental);
    m_tagPositionCombo->setCurrentText(tagPositionToUi(m_meta.tagPosition));
    m_genreRatioSlider->setValue(m_meta.genreRatio);

    QList<TrackData> tracks;
    const QJsonArray samples = root.value("samples").toArray();
    tracks.reserve(samples.size());
    for (const QJsonValue &v : samples) {
        const QJsonObject s = v.toObject();
        TrackData t;
        t.id = s.value("id").toString();
        t.audioPath = s.value("audio_path").toString();
        t.filename = s.value("filename").toString(QFileInfo(t.audioPath).fileName());
        t.caption = s.value("caption").toString();
        t.genre = s.value("genre").toString();
        t.lyrics = s.value("lyrics").toString();
        t.bpm = s.value("bpm").toInt();
        t.keyscale = s.value("keyscale").toString();
        t.timesignature = s.value("timesignature").toString();
        t.duration = s.value("duration").toInt();
        t.language = s.value("language").toString("instrumental");
        t.isInstrumental = s.value("is_instrumental").toBool(false);
        t.customTag = s.value("custom_tag").toString();
        t.labeled = s.value("labeled").toBool(false);
        if (s.contains("prompt_override") && !s.value("prompt_override").isNull()) {
            const QString po = s.value("prompt_override").toString().trimmed().toLower();
            if (po == "caption" || po == "genre") {
                t.promptOverride = po;
            }
        }
        if (t.id.isEmpty()) {
            t.id = generateId(t.audioPath.isEmpty() ? t.filename : t.audioPath);
        }
        if (t.audioPath.isEmpty() && !t.filename.isEmpty()) {
            t.audioPath = QDir(m_currentFolder).filePath(t.filename);
        }
        tracks.append(t);
    }

    m_currentJsonPath = jsonPath;
    rebuildTrackList(tracks);
    return true;
}

QString MainWindow::defaultJsonPath() const {
    const QString baseName = m_nameEdit->text().trimmed().isEmpty() ? "dataset" : m_nameEdit->text().trimmed();
    return QDir(m_currentFolder).filePath(baseName + ".json");
}

QString MainWindow::currentTimestampFileSafe() const {
    return QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
}

QString MainWindow::generateId(const QString &source) const {
    const QByteArray hash = QCryptographicHash::hash(source.toUtf8(), QCryptographicHash::Md5).toHex();
    return QString::fromLatin1(hash.left(8));
}

QString MainWindow::sanitizeTagPosition(const QString &value) {
    if (value == "append" || value == "prepend") {
        return value;
    }
    if (value == "replace_caption" || value == "replace") {
        return "replace";
    }
    return "prepend";
}

int MainWindow::unsavedCardsCount() const {
    int count = 0;
    for (AudioItemWidget *w : m_trackWidgets) {
        if (w->hasUnsavedChanges()) {
            ++count;
        }
    }
    return count;
}

bool MainWindow::hasUnsavedMetaChanges() const {
    if (!m_metaSnapshotReady) {
        return false;
    }
    return m_nameEdit->text().trimmed() != m_savedName ||
           m_customTagEdit->text().trimmed() != m_savedCustomTag ||
           uiToTagPosition(m_tagPositionCombo->currentText()) != m_savedTagPosition ||
           m_genreRatioSlider->value() != m_savedGenreRatio ||
           m_allInstrumentalCheck->isChecked() != m_savedAllInstrumental;
}

bool MainWindow::hasUnsavedChanges() const {
    return hasUnsavedMetaChanges() || unsavedCardsCount() > 0;
}

void MainWindow::captureMetaSnapshot() {
    m_savedName = m_nameEdit->text().trimmed();
    m_savedCustomTag = m_customTagEdit->text().trimmed();
    m_savedTagPosition = uiToTagPosition(m_tagPositionCombo->currentText());
    m_savedGenreRatio = m_genreRatioSlider->value();
    m_savedAllInstrumental = m_allInstrumentalCheck->isChecked();
    m_metaSnapshotReady = true;
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (!hasUnsavedChanges()) {
        QSettings s = makeAppSettings();
        s.setValue("ui/windowGeometry", saveGeometry());
        event->accept();
        return;
    }

    QMessageBox msg(this);
    msg.setWindowTitle("Unsaved changes");
    msg.setText("There are unsaved changes.");
    msg.setInformativeText("Save before exit?");
    msg.setIcon(QMessageBox::Warning);
    QPushButton *saveBtn = msg.addButton("Save", QMessageBox::AcceptRole);
    QPushButton *discardBtn = msg.addButton("Discard", QMessageBox::DestructiveRole);
    QPushButton *cancelBtn = msg.addButton("Cancel", QMessageBox::RejectRole);
    msg.exec();

    if (msg.clickedButton() == saveBtn) {
        saveDataset();
        if (hasUnsavedChanges()) {
            event->ignore();
            return;
        }
        QSettings s = makeAppSettings();
        s.setValue("ui/windowGeometry", saveGeometry());
        event->accept();
        return;
    }
    if (msg.clickedButton() == discardBtn) {
        QSettings s = makeAppSettings();
        s.setValue("ui/windowGeometry", saveGeometry());
        event->accept();
        return;
    }
    Q_UNUSED(cancelBtn);
    event->ignore();
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    positionToast();
}

void MainWindow::showCaptionTutorial() {
    const QString path =
        resolveHelpMarkdownPath(QStringLiteral("About Caption - The Most Important Input.md"));
    if (path.isEmpty()) {
        QMessageBox::warning(this, "Caption Tutorial", "Markdown file not found: src/Help/...");
        return;
    }
    const QString md = readUtf8TextFile(path);
    if (md.isEmpty()) {
        QMessageBox::warning(this, "Caption Tutorial",
                             QString("Failed to read file:\n%1").arg(QDir::toNativeSeparators(path)));
        return;
    }
    showTutorialDialog("Caption Tutorial", md, QUrl::fromLocalFile(QFileInfo(path).absolutePath() + "/"));
}

void MainWindow::showLyricsTutorial() {
    const QString path = resolveHelpMarkdownPath(QStringLiteral("About Lyrics - The Temporal Script.md"));
    if (path.isEmpty()) {
        QMessageBox::warning(this, "Lyrics Tutorial", "Markdown file not found: src/Help/...");
        return;
    }
    const QString md = readUtf8TextFile(path);
    if (md.isEmpty()) {
        QMessageBox::warning(this, "Lyrics Tutorial",
                             QString("Failed to read file:\n%1").arg(QDir::toNativeSeparators(path)));
        return;
    }
    showTutorialDialog("Lyrics Tutorial", md, QUrl::fromLocalFile(QFileInfo(path).absolutePath() + "/"));
}

void MainWindow::showTutorialDialog(const QString &title, const QString &markdown, const QUrl &baseUrl) const {
    auto *dlg = new QDialog(const_cast<MainWindow *>(this));
    dlg->setAttribute(Qt::WA_DeleteOnClose, true);
    dlg->setWindowTitle(title);
    dlg->resize(1120, 800);

    auto *layout = new QVBoxLayout(dlg);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    auto *browser = new QTextBrowser(dlg);
    browser->setOpenExternalLinks(true);
    browser->document()->setDefaultStyleSheet(
        "body { line-height: 1.35; }"
        "table { border-collapse: collapse; }"
        "th, td { border: 1px solid #4a566d; padding: 4px 6px; vertical-align: top; }"
        "th { background: #2f3b52; }"
        "td { background: #242c38; }"
        "code, pre { background: #242c38; }");
    browser->setStyleSheet(
        "QTextBrowser {"
        " background: #1f2530;"
        " color: #e8eef8;"
        " border: 1px solid #3c4a63;"
        " border-radius: 8px;"
        " padding: 8px;"
        "}");
    if (baseUrl.isValid()) {
        browser->document()->setBaseUrl(baseUrl);
    }
    QTextDocument mdDoc;
    if (baseUrl.isValid()) {
        mdDoc.setBaseUrl(baseUrl);
    }
    mdDoc.setMarkdown(markdown);
    browser->setHtml(mdDoc.toHtml());
    layout->addWidget(browser, 1);

    auto *closeBtn = new QPushButton("Close", dlg);
    closeBtn->setFixedWidth(120);
    connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);
    layout->addWidget(closeBtn, 0, Qt::AlignRight);

    dlg->show();
}

void MainWindow::showPathToast(const QString &prefix, const QString &filePath) {
    if (!m_saveToast) {
        m_saveToast = new SaveToastWidget(this);
    }
    const QString msg =
        QStringLiteral("%1 - %2").arg(prefix, QDir::toNativeSeparators(filePath));
    static_cast<SaveToastWidget *>(m_saveToast)->showMessage(msg, 4000);
    positionToast();
}

void MainWindow::positionToast() {
    if (!m_saveToast || !m_saveToast->isVisible()) {
        return;
    }
    const int margin = 14;
    const int maxW = qMax(220, width() - 2 * margin);
    static_cast<SaveToastWidget *>(m_saveToast)->setMaxToastWidth(maxW);
    const int x = qMax(margin, (width() - m_saveToast->width()) / 2);
    const int y = qMax(margin, height() - m_saveToast->height() - margin);
    m_saveToast->move(x, y);
}
