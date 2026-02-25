#include "audioitemwidget.h"
#include "plaintextedit.h"

#include <QAudioOutput>
#include <QCheckBox>
#include <QComboBox>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMediaPlayer>
#include <QMenu>
#include <QMouseEvent>
#include <QPushButton>
#include <QFrame>
#include <QSignalBlocker>
#include <QScrollBar>
#include <QSizePolicy>
#include <QSlider>
#include <QStyle>
#include <QTimer>
#include <QTextEdit>
#include <QTextDocument>
#include <QFontMetrics>
#include <QUrl>
#include <QVBoxLayout>
#include <cmath>
#include <memory>

namespace {
QStringList languages() {
    return {"instrumental", "en", "zh", "ja", "ko", "es", "fr", "de", "pt", "ru"};
}

constexpr const char *kFieldCaption = "caption";
constexpr const char *kFieldGenre = "genre";
constexpr const char *kFieldLyrics = "lyrics";
constexpr const char *kFieldBpm = "bpm";
constexpr const char *kFieldKey = "keyscale";
constexpr const char *kFieldTimeSig = "timesignature";
constexpr const char *kFieldDuration = "duration";

class ClickSeekSlider : public QSlider {
public:
    explicit ClickSeekSlider(Qt::Orientation orientation, QWidget *parent = nullptr)
        : QSlider(orientation, parent) {}

protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            const int pos =
                orientation() == Qt::Horizontal ? qRound(event->position().x())
                                                : qRound(event->position().y());
#else
            const int pos = orientation() == Qt::Horizontal ? event->pos().x() : event->pos().y();
#endif
            const int span = qMax(1, orientation() == Qt::Horizontal ? width() : height());
            const bool upsideDown =
                (orientation() == Qt::Horizontal)
                    ? (invertedAppearance() != (layoutDirection() == Qt::RightToLeft))
                    : !invertedAppearance();
            const int value = QStyle::sliderValueFromPosition(minimum(), maximum(), pos, span,
                                                              upsideDown);
            setSliderPosition(value);
            setValue(value);
        }
        QSlider::mousePressEvent(event);
    }
};
}

AudioItemWidget::AudioItemWidget(int index, const TrackData &data, QWidget *parent)
    : QWidget(parent), m_index(index), m_data(data) {
    setupUi();
    connectSignals();
    setExpanded(false);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    m_player->setSource(QUrl::fromLocalFile(m_data.audioPath));
    applyDurationIfEmpty();
    markSaved();
}

void AudioItemWidget::setupUi() {
    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(10, 10, 10, 10);
    root->setSpacing(10);

    setObjectName("TrackCard");
    setAttribute(Qt::WA_StyledBackground, true);

    m_leftHost = new QWidget(this);
    m_leftHost->setFixedWidth(280);
    m_leftHost->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_leftHost->setMinimumHeight(1);

    m_leftPanel = new QFrame(m_leftHost);
    m_leftPanel->setObjectName("TrackHeader");
    m_leftPanel->setFixedWidth(280);
    auto *leftPanelLayout = new QVBoxLayout(m_leftPanel);
    leftPanelLayout->setContentsMargins(8, 8, 8, 8);
    leftPanelLayout->setSpacing(6);

    auto *playerTop = new QHBoxLayout();
    playerTop->setSpacing(8);

    m_indexLabel = new QLabel(QString::number(m_index), m_leftPanel);
    QFont idxFont = m_indexLabel->font();
    idxFont.setPointSize(idxFont.pointSize() + 3);
    idxFont.setBold(true);
    m_indexLabel->setFont(idxFont);
    m_indexLabel->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    m_indexLabel->setFixedWidth(48);
    playerTop->addWidget(m_indexLabel);

    m_fileNameLabel = new QLabel(m_data.filename.isEmpty() ? QFileInfo(m_data.audioPath).fileName() : m_data.filename, m_leftPanel);
    m_fileNameLabel->setWordWrap(true);
    m_fileNameLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_fileNameLabel->setToolTip(m_data.filename.isEmpty() ? QFileInfo(m_data.audioPath).fileName() : m_data.filename);
    m_fileNameLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_playPauseButton = new QPushButton("Play", m_leftPanel);
    m_seekSlider = new ClickSeekSlider(Qt::Horizontal, m_leftPanel);
    m_seekSlider->setRange(0, 0);
    m_playPauseButton->setFixedWidth(90);
    playerTop->addWidget(m_playPauseButton, 0, Qt::AlignRight);
    leftPanelLayout->addLayout(playerTop);
    leftPanelLayout->addWidget(m_fileNameLabel);
    leftPanelLayout->addWidget(m_seekSlider);
    const int initialPanelHeight = qMax(96, m_leftPanel->sizeHint().height());
    m_leftPanel->setFixedHeight(initialPanelHeight);
    m_leftHost->setMinimumHeight(initialPanelHeight);
    root->addWidget(m_leftHost, 0);

    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(m_audioOutput);

    auto *contentRow = new QHBoxLayout();
    contentRow->setSpacing(10);

    auto *fields = new QGridLayout();
    fields->setHorizontalSpacing(6);
    fields->setVerticalSpacing(4);

    m_captionEdit = new PlainTextEdit(this);
    m_captionEdit->setPlaceholderText("Caption");
    m_captionEdit->setPlainText(m_data.caption);
    m_captionEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_captionEdit->setContextMenuPolicy(Qt::CustomContextMenu);

    m_genreEdit = new QLineEdit(this);
    m_genreEdit->setPlaceholderText("Genre");
    m_genreEdit->setText(m_data.genre);
    m_genreEdit->setContextMenuPolicy(Qt::CustomContextMenu);

    m_lyricsEdit = new PlainTextEdit(this);
    m_lyricsEdit->setPlaceholderText("Lyrics");
    m_lyricsEdit->setPlainText(m_data.lyrics);
    m_lyricsEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_lyricsEdit->setContextMenuPolicy(Qt::CustomContextMenu);

    m_bpmEdit = new QLineEdit(QString::number(m_data.bpm), this);
    m_bpmEdit->setPlaceholderText("BPM");
    m_bpmEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    m_keyEdit = new QLineEdit(m_data.keyscale, this);
    m_keyEdit->setPlaceholderText("Key");
    m_keyEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    m_timeSigEdit = new QLineEdit(m_data.timesignature, this);
    m_timeSigEdit->setPlaceholderText("Time Sig");
    m_timeSigEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    m_durationEdit = new QLineEdit(QString::number(m_data.duration), this);
    m_durationEdit->setPlaceholderText("Duration(s)");
    m_durationEdit->setContextMenuPolicy(Qt::CustomContextMenu);

    m_languageCombo = new QComboBox(this);
    m_languageCombo->addItems(languages());
    const int langIndex = m_languageCombo->findText(m_data.language);
    m_languageCombo->setCurrentIndex(langIndex >= 0 ? langIndex : 0);

    m_promptOverrideCombo = new QComboBox(this);
    m_promptOverrideCombo->addItems({"Use Global Ratio", "Caption", "Genre"});
    if (m_data.promptOverride == "caption") {
        m_promptOverrideCombo->setCurrentText("Caption");
    } else if (m_data.promptOverride == "genre") {
        m_promptOverrideCombo->setCurrentText("Genre");
    } else {
        m_promptOverrideCombo->setCurrentText("Use Global Ratio");
    }
    const QString promptOverrideTip =
        "Prompt Override (This Sample) - Override global ratio for this sample";
    m_promptOverrideCombo->setToolTip(promptOverrideTip);

    m_applyLangAllBtn = new QPushButton("Apply language to all", this);
    m_instrumentalCheck = new QCheckBox("Instrumental", this);
    m_instrumentalCheck->setChecked(m_data.isInstrumental);

    auto *captionLabel = new QLabel("Caption", this);
    auto *genreLabel = new QLabel("Genre", this);
    auto *bpmLabel = new QLabel("BPM", this);
    auto *keyLabel = new QLabel("Key", this);
    auto *timeSigLabel = new QLabel("Time Sig", this);
    auto *durationLabel = new QLabel("Duration(s)", this);
    auto *lyricsLabel = new QLabel("Lyrics", this);
    auto *languageLabel = new QLabel("Language", this);
    auto *promptOverrideLabel = new QLabel("Prompt Override", this);

    fields->addWidget(captionLabel, 0, 0);
    fields->addWidget(m_captionEdit, 1, 0, 1, 5);
    fields->addWidget(genreLabel, 2, 0);
    fields->addWidget(m_genreEdit, 3, 0);
    fields->addWidget(bpmLabel, 2, 1);
    fields->addWidget(m_bpmEdit, 3, 1);
    fields->addWidget(keyLabel, 2, 2);
    fields->addWidget(m_keyEdit, 3, 2);
    fields->addWidget(timeSigLabel, 2, 3);
    fields->addWidget(m_timeSigEdit, 3, 3);
    fields->addWidget(durationLabel, 2, 4);
    fields->addWidget(m_durationEdit, 3, 4);
    fields->addWidget(lyricsLabel, 4, 0);
    fields->addWidget(m_lyricsEdit, 5, 0, 1, 5);
    fields->addWidget(languageLabel, 6, 0);
    fields->addWidget(m_languageCombo, 6, 1);
    fields->addWidget(m_applyLangAllBtn, 6, 2, 1, 2);
    fields->addWidget(m_instrumentalCheck, 6, 4);
    promptOverrideLabel->setToolTip(promptOverrideTip);
    fields->addWidget(promptOverrideLabel, 7, 0);
    fields->addWidget(m_promptOverrideCombo, 7, 1, 1, 2);
    m_secondaryFieldWidgets = {genreLabel,      m_genreEdit,        bpmLabel,      m_bpmEdit,
                               keyLabel,        m_keyEdit,          timeSigLabel,  m_timeSigEdit,
                               durationLabel,   m_durationEdit,     languageLabel, m_languageCombo,
                               m_applyLangAllBtn, m_instrumentalCheck, promptOverrideLabel,
                               m_promptOverrideCombo};
    fields->setRowStretch(1, 1);
    fields->setRowStretch(5, 2);
    fields->setRowStretch(6, 0);
    fields->setRowStretch(7, 0);

    contentRow->addLayout(fields, 1);

    m_rightHost = new QWidget(this);
    m_rightHost->setFixedWidth(180);
    m_rightHost->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_rightHost->setMinimumHeight(1);

    m_rightPanel = new QFrame(m_rightHost);
    m_rightPanel->setObjectName("TrackActions");
    m_rightPanel->setFixedWidth(180);
    auto *buttons = new QVBoxLayout(m_rightPanel);
    buttons->setContentsMargins(8, 8, 8, 8);
    buttons->setSpacing(6);
    m_deleteBtn = new QPushButton("Delete", m_rightPanel);
    m_saveBtn = new QPushButton("Save", m_rightPanel);
    m_expandCaptionBtn = new QPushButton("Expand Caption", m_rightPanel);
    m_expandLyricsBtn = new QPushButton("Expand Lyrics", m_rightPanel);
    buttons->addWidget(m_deleteBtn);
    buttons->addWidget(m_saveBtn);
    buttons->addWidget(m_expandCaptionBtn);
    buttons->addWidget(m_expandLyricsBtn);
    buttons->addStretch();
    const int initialButtonsH = qMax(110, m_rightPanel->sizeHint().height());
    m_rightPanel->setFixedHeight(initialButtonsH);
    m_rightHost->setMinimumHeight(initialButtonsH);
    contentRow->addWidget(m_rightHost, 0);
    root->addLayout(contentRow, 1);

    setStyleSheet(
        "QWidget#TrackCard {"
        "  border: 2px solid #5f6876;"
        "  border-radius: 8px;"
        "  background-color: #1f252e;"
        "}"
        "QFrame#TrackHeader {"
        "  border: 1px solid #5f6876;"
        "  border-radius: 6px;"
        "  background-color: #242b35;"
        "}"
        "QFrame#TrackActions {"
        "  border: 1px solid #5f6876;"
        "  border-radius: 6px;"
        "  background-color: #242b35;"
        "}"
        "QWidget#TrackCard QTextEdit, QWidget#TrackCard QLineEdit, QWidget#TrackCard QComboBox {"
        "  border: 1px solid #5b6370;"
        "  background-color: #2a313c;"
        "}"
    );
}

void AudioItemWidget::connectSignals() {
    connect(m_playPauseButton, &QPushButton::clicked, this, &AudioItemWidget::onPlayPause);
    connect(m_player, &QMediaPlayer::durationChanged, this, &AudioItemWidget::onDurationChanged);
    connect(m_player, &QMediaPlayer::positionChanged, this, &AudioItemWidget::onPositionChanged);
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState) {
        updatePlayButtonText();
    });
    connect(m_seekSlider, &QSlider::sliderPressed, this, &AudioItemWidget::onSliderPressed);
    connect(m_seekSlider, &QSlider::sliderMoved, this, &AudioItemWidget::onSliderMoved);
    connect(m_seekSlider, &QSlider::sliderReleased, this, &AudioItemWidget::onSliderReleased);
    connect(m_expandCaptionBtn, &QPushButton::clicked, this, &AudioItemWidget::onExpandCaptionClicked);
    connect(m_expandLyricsBtn, &QPushButton::clicked, this, &AudioItemWidget::onExpandLyricsClicked);
    connect(m_deleteBtn, &QPushButton::clicked, this, [this]() { emit deleteRequested(this); });
    connect(m_saveBtn, &QPushButton::clicked, this, [this]() { emit saveRequested(); });
    connect(m_applyLangAllBtn, &QPushButton::clicked, this, [this]() {
        emit languageApplyAllRequested(m_languageCombo->currentText());
    });

    connect(m_captionEdit, &QTextEdit::textChanged, this, &AudioItemWidget::triggerChanged);
    connect(m_genreEdit, &QLineEdit::textChanged, this, &AudioItemWidget::triggerChanged);
    connect(m_lyricsEdit, &QTextEdit::textChanged, this, &AudioItemWidget::triggerChanged);
    connect(m_bpmEdit, &QLineEdit::textChanged, this, &AudioItemWidget::triggerChanged);
    connect(m_keyEdit, &QLineEdit::textChanged, this, &AudioItemWidget::triggerChanged);
    connect(m_timeSigEdit, &QLineEdit::textChanged, this, &AudioItemWidget::triggerChanged);
    connect(m_durationEdit, &QLineEdit::textChanged, this, &AudioItemWidget::triggerChanged);
    connect(m_languageCombo, &QComboBox::currentTextChanged, this, &AudioItemWidget::triggerChanged);
    connect(m_promptOverrideCombo, &QComboBox::currentTextChanged, this, &AudioItemWidget::triggerChanged);
    connect(m_instrumentalCheck, &QCheckBox::toggled, this, [this](bool) { emit changed(); });

    auto connectLineContextMenu = [this](QLineEdit *edit, const QString &field) {
        connect(edit, &QWidget::customContextMenuRequested, this, [this, edit, field](const QPoint &pos) {
            std::unique_ptr<QMenu> menu(edit->createStandardContextMenu());
            menu->addSeparator();
            QAction *applyAct = menu->addAction("Apply to all tracks");
            connect(applyAct, &QAction::triggered, this, [this, edit, field]() {
                emit fieldApplyAllRequested(field, edit->text());
            });
            menu->exec(edit->mapToGlobal(pos));
        });
    };

    auto connectTextContextMenu = [this](QTextEdit *edit, const QString &field) {
        connect(edit, &QWidget::customContextMenuRequested, this, [this, edit, field](const QPoint &pos) {
            std::unique_ptr<QMenu> menu(edit->createStandardContextMenu());
            menu->addSeparator();
            QAction *applyAct = menu->addAction("Apply to all tracks");
            connect(applyAct, &QAction::triggered, this, [this, edit, field]() {
                emit fieldApplyAllRequested(field, edit->toPlainText());
            });
            menu->exec(edit->mapToGlobal(pos));
        });
    };

    connectTextContextMenu(m_captionEdit, QString::fromLatin1(kFieldCaption));
    connectLineContextMenu(m_genreEdit, QString::fromLatin1(kFieldGenre));
    connectTextContextMenu(m_lyricsEdit, QString::fromLatin1(kFieldLyrics));
    connectLineContextMenu(m_bpmEdit, QString::fromLatin1(kFieldBpm));
    connectLineContextMenu(m_keyEdit, QString::fromLatin1(kFieldKey));
    connectLineContextMenu(m_timeSigEdit, QString::fromLatin1(kFieldTimeSig));
    connectLineContextMenu(m_durationEdit, QString::fromLatin1(kFieldDuration));
}

TrackData AudioItemWidget::data() const {
    TrackData out = m_data;
    out.caption = m_captionEdit->toPlainText();
    out.genre = m_genreEdit->text();
    out.lyrics = m_lyricsEdit->toPlainText();
    out.bpm = m_bpmEdit->text().toInt();
    out.keyscale = m_keyEdit->text();
    out.timesignature = m_timeSigEdit->text();
    out.duration = m_durationEdit->text().toInt();
    out.language = m_languageCombo->currentText();
    out.isInstrumental = m_instrumentalCheck->isChecked();
    if (m_promptOverrideCombo->currentText() == "Caption") {
        out.promptOverride = "caption";
    } else if (m_promptOverrideCombo->currentText() == "Genre") {
        out.promptOverride = "genre";
    } else {
        out.promptOverride.clear();
    }
    out.labeled = !out.caption.trimmed().isEmpty();
    return out;
}

void AudioItemWidget::setIndex(int index) {
    m_index = index;
    m_indexLabel->setText(QString::number(index));
}

void AudioItemWidget::setExpanded(bool expanded) {
    m_captionExpanded = expanded;
    m_lyricsExpanded = expanded;
    updateExpandButtons();
    updateHeights();
}

bool AudioItemWidget::isExpanded() const {
    return m_captionExpanded && m_lyricsExpanded;
}

void AudioItemWidget::setUiScale(int fontSize) {
    if (m_captionEdit) {
        QFont captionFont = m_captionEdit->font();
        if (captionFont.pointSize() != fontSize) {
            captionFont.setPointSize(fontSize);
            m_captionEdit->setFont(captionFont);
            m_captionEdit->document()->setDefaultFont(captionFont);
        }
    }
    if (m_lyricsEdit) {
        QFont lyricsFont = m_lyricsEdit->font();
        if (lyricsFont.pointSize() != fontSize) {
            lyricsFont.setPointSize(fontSize);
            m_lyricsEdit->setFont(lyricsFont);
            m_lyricsEdit->document()->setDefaultFont(lyricsFont);
        }
    }
    m_uiScale = 100;
    updateHeights();
}

void AudioItemWidget::setLanguageValue(const QString &language) {
    const int idx = m_languageCombo->findText(language);
    if (idx >= 0) {
        m_languageCombo->setCurrentIndex(idx);
    }
    updateDirtyHighlight();
}

void AudioItemWidget::setGenreValue(const QString &genre) {
    m_genreEdit->setText(genre);
    updateDirtyHighlight();
}

void AudioItemWidget::setInstrumentalValue(bool value) {
    m_instrumentalCheck->setChecked(value);
    updateDirtyHighlight();
}

void AudioItemWidget::setFieldValue(const QString &field, const QString &value) {
    if (field == QLatin1String(kFieldCaption)) {
        m_captionEdit->setPlainText(value);
    } else if (field == QLatin1String(kFieldGenre)) {
        m_genreEdit->setText(value);
    } else if (field == QLatin1String(kFieldLyrics)) {
        m_lyricsEdit->setPlainText(value);
    } else if (field == QLatin1String(kFieldBpm)) {
        m_bpmEdit->setText(value);
    } else if (field == QLatin1String(kFieldKey)) {
        m_keyEdit->setText(value);
    } else if (field == QLatin1String(kFieldTimeSig)) {
        m_timeSigEdit->setText(value);
    } else if (field == QLatin1String(kFieldDuration)) {
        m_durationEdit->setText(value);
    }
    updateDirtyHighlight();
}

void AudioItemWidget::setCaptionText(const QString &caption) {
    m_captionEdit->setPlainText(caption);
    updateDirtyHighlight();
}

void AudioItemWidget::setCaptionLyricsOnlyMode(bool enabled) {
    if (m_captionLyricsOnlyMode == enabled) {
        return;
    }
    m_captionLyricsOnlyMode = enabled;
    for (QWidget *w : m_secondaryFieldWidgets) {
        if (w) {
            w->setVisible(!enabled);
        }
    }
    updateHeights();
}

void AudioItemWidget::setStickyViewport(QWidget *viewport) {
    m_stickyViewport = viewport;
    updateStickyPosition();
}

void AudioItemWidget::updateStickyPosition() {
    if (!m_leftHost || !m_leftPanel) {
        return;
    }
    int offset = 0;
    if (m_stickyViewport && isVisible()) {
        const QPoint topInViewport = mapTo(m_stickyViewport, QPoint(0, 0));
        const int viewportTopOverlap = qMax(0, -topInViewport.y());
        int maxOffset = qMax(0, m_leftHost->height() - m_leftPanel->height());
        if (m_rightHost && m_rightPanel) {
            maxOffset = qMin(maxOffset, qMax(0, m_rightHost->height() - m_rightPanel->height()));
        }
        offset = qBound(0, viewportTopOverlap, maxOffset);
    }
    if (m_leftPanel->width() != m_leftHost->width()) {
        m_leftPanel->setFixedWidth(m_leftHost->width());
    }
    if (m_rightPanel && m_rightHost && m_rightPanel->width() != m_rightHost->width()) {
        m_rightPanel->setFixedWidth(m_rightHost->width());
    }
    if (offset != m_lastStickyOffset) {
        m_leftPanel->move(0, offset);
        if (m_rightPanel) {
            m_rightPanel->move(0, offset);
        }
        m_lastStickyOffset = offset;
    }
}

void AudioItemWidget::onPlayPause() {
    emit playbackControlActivated(this);
    if (m_player->playbackState() == QMediaPlayer::PlayingState) {
        m_player->pause();
    } else {
        m_player->play();
    }
    updatePlayButtonText();
}

void AudioItemWidget::onDurationChanged(qint64 durationMs) {
    m_seekSlider->setRange(0, static_cast<int>(durationMs));
    if (m_durationEdit->text().trimmed().isEmpty() || m_durationEdit->text().toInt() <= 0) {
        const int sec = static_cast<int>(durationMs / 1000);
        if (sec > 0) {
            m_durationEdit->setText(QString::number(sec));
        }
    }
}

void AudioItemWidget::onPositionChanged(qint64 positionMs) {
    if (m_updatingSlider || m_userSeeking) {
        return;
    }
    if (m_seekTargetMs >= 0) {
        const qint64 delta = positionMs - m_seekTargetMs;
        if (delta < -250 || delta > 250) {
            return;
        }
        m_seekTargetMs = -1;
    }
    m_seekSlider->setValue(static_cast<int>(positionMs));
    updatePlayButtonText();
}

void AudioItemWidget::onSliderPressed() {
    emit playbackControlActivated(this);
    m_userSeeking = true;
}

void AudioItemWidget::onSliderMoved(int value) {
    emit playbackControlActivated(this);
    m_seekTargetMs = value;
}

void AudioItemWidget::onSliderReleased() {
    emit playbackControlActivated(this);
    m_userSeeking = false;
    m_updatingSlider = true;
    m_seekTargetMs = m_seekSlider->value();
    seekToMs(m_seekTargetMs);
    m_updatingSlider = false;
}

void AudioItemWidget::onExpandCaptionClicked() {
    m_captionExpanded = !m_captionExpanded;
    updateExpandButtons();
    updateHeights();
}

void AudioItemWidget::onExpandLyricsClicked() {
    m_lyricsExpanded = !m_lyricsExpanded;
    updateExpandButtons();
    updateHeights();
}

void AudioItemWidget::triggerChanged() {
    const bool captionChanged = sender() == m_captionEdit;
    const bool lyricsChanged = sender() == m_lyricsEdit;
    if ((captionChanged && m_captionExpanded) || (lyricsChanged && m_lyricsExpanded)) {
        updateHeights();
    }
    updateDirtyHighlight();
    emit changed();
}

void AudioItemWidget::updatePlayButtonText() {
    m_playPauseButton->setText(m_player->playbackState() == QMediaPlayer::PlayingState ? "Pause" : "Play");
}

bool AudioItemWidget::isPlaying() const {
    return m_player && m_player->playbackState() == QMediaPlayer::PlayingState;
}

void AudioItemWidget::togglePlayback() {
    onPlayPause();
}

void AudioItemWidget::seekRelativeMs(qint64 deltaMs) {
    if (!m_player) {
        return;
    }
    emit playbackControlActivated(this);
    const qint64 duration = m_player->duration();
    qint64 target = m_player->position() + deltaMs;
    if (duration > 0) {
        target = qBound<qint64>(0, target, duration);
    } else {
        target = qMax<qint64>(0, target);
    }
    seekToMs(target);
    if (m_seekSlider) {
        m_seekSlider->setValue(static_cast<int>(target));
    }
    updatePlayButtonText();
}

void AudioItemWidget::seekToMs(qint64 targetMs) {
    if (!m_player) {
        return;
    }
    m_seekTargetMs = qMax<qint64>(0, targetMs);

    // Reduce decoder/output click at seek boundaries by briefly muting output.
    const bool canMute = (m_audioOutput != nullptr);
    const qreal prevVolume = canMute ? m_audioOutput->volume() : 1.0;
    if (canMute) {
        m_audioOutput->setVolume(0.0);
    }
    m_player->setPosition(m_seekTargetMs);
    if (canMute) {
        QTimer::singleShot(45, this, [this, prevVolume]() {
            if (m_audioOutput) {
                m_audioOutput->setVolume(prevVolume);
            }
        });
    }
}

void AudioItemWidget::updateHeights() {
    const int captionBase = m_captionExpanded ? 140 : 70;
    const int lyricsBase = m_lyricsExpanded ? 240 : 120;
    const int captionPreset = qMax(50, (captionBase * m_uiScale) / 100);
    const int lyricsPreset = qMax(100, (lyricsBase * m_uiScale) / 100);
    const int smallLineH = qMax(24, m_bpmEdit->sizeHint().height());

    const int captionH = m_captionExpanded ? contentHeightFor(m_captionEdit, captionPreset, 5000) : captionPreset;
    const int lyricsH = m_lyricsExpanded ? contentHeightFor(m_lyricsEdit, lyricsPreset, 7000) : lyricsPreset;

    {
        const QSignalBlocker b1(m_captionEdit);
        const QSignalBlocker b2(m_lyricsEdit);
        m_captionEdit->setFixedHeight(captionH);
        m_lyricsEdit->setFixedHeight(lyricsH);
    }
    m_genreEdit->setFixedHeight(smallLineH);
    m_bpmEdit->setFixedHeight(smallLineH);
    m_keyEdit->setFixedHeight(smallLineH);
    m_timeSigEdit->setFixedHeight(smallLineH);
    m_durationEdit->setFixedHeight(smallLineH);
    m_captionEdit->setVerticalScrollBarPolicy(m_captionExpanded ? Qt::ScrollBarAlwaysOff : Qt::ScrollBarAsNeeded);
    m_lyricsEdit->setVerticalScrollBarPolicy(m_lyricsExpanded ? Qt::ScrollBarAlwaysOff : Qt::ScrollBarAsNeeded);
    if (m_leftPanel && m_leftHost) {
        const int panelH = qMax(96, m_leftPanel->sizeHint().height());
        m_leftPanel->setFixedHeight(panelH);
        m_leftHost->setMinimumHeight(panelH);
    }
    if (m_rightPanel && m_rightHost) {
        const int panelH = qMax(110, m_rightPanel->sizeHint().height());
        m_rightPanel->setFixedHeight(panelH);
        m_rightHost->setMinimumHeight(panelH);
    }
    if (m_fileNameLabel) {
        const QString name = m_data.filename.isEmpty() ? QFileInfo(m_data.audioPath).fileName() : m_data.filename;
        m_fileNameLabel->setText(name);
        m_fileNameLabel->setToolTip(name);
    }
    updateStickyPosition();
    updateGeometry();
    emit layoutSizeChanged();
}

void AudioItemWidget::updateExpandButtons() {
    m_expandCaptionBtn->setText(m_captionExpanded ? "Collapse Caption" : "Expand Caption");
    m_expandLyricsBtn->setText(m_lyricsExpanded ? "Collapse Lyrics" : "Expand Lyrics");
}

void AudioItemWidget::applyDurationIfEmpty() {
    if (m_data.duration > 0) {
        m_durationEdit->setText(QString::number(m_data.duration));
    } else if (!m_data.audioPath.isEmpty() && QFileInfo::exists(m_data.audioPath)) {
        m_player->setSource(QUrl::fromLocalFile(m_data.audioPath));
    }
}

int AudioItemWidget::contentHeightFor(QTextEdit *edit, int minHeight, int maxHeight) const {
    QTextDocument doc;
    doc.setDefaultFont(edit->font());
    doc.setPlainText(edit->toPlainText());
    const int viewportW = qMax(1, edit->viewport()->width() - 2);
    doc.setTextWidth(viewportW);
    const int textH = static_cast<int>(std::ceil(doc.size().height()));
    const int frame = static_cast<int>(edit->frameWidth() * 2);
    const int margins = edit->contentsMargins().top() + edit->contentsMargins().bottom();
    const int padding = 8;
    return qBound(minHeight, textH + frame + margins + padding, maxHeight);
}

void AudioItemWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    updateStickyPosition();
}

void AudioItemWidget::markSaved() {
    m_savedData = data();
    m_savedInitialized = true;
    updateDirtyHighlight();
}

bool AudioItemWidget::hasUnsavedChanges() const {
    if (!m_savedInitialized) {
        return false;
    }
    return isDirtyComparedToSaved();
}

void AudioItemWidget::updateDirtyHighlight() {
    if (!m_savedInitialized) {
        return;
    }
    const TrackData cur = data();
    applyDirtyStyle(m_captionEdit, cur.caption != m_savedData.caption);
    applyDirtyStyle(m_genreEdit, cur.genre != m_savedData.genre);
    applyDirtyStyle(m_lyricsEdit, cur.lyrics != m_savedData.lyrics);
    applyDirtyStyle(m_bpmEdit, cur.bpm != m_savedData.bpm);
    applyDirtyStyle(m_keyEdit, cur.keyscale != m_savedData.keyscale);
    applyDirtyStyle(m_timeSigEdit, cur.timesignature != m_savedData.timesignature);
    applyDirtyStyle(m_durationEdit, cur.duration != m_savedData.duration);
    applyDirtyStyle(m_languageCombo, cur.language != m_savedData.language);
    applyDirtyStyle(m_promptOverrideCombo, cur.promptOverride != m_savedData.promptOverride);
    applyDirtyStyle(m_instrumentalCheck, cur.isInstrumental != m_savedData.isInstrumental);
}

bool AudioItemWidget::isDirtyComparedToSaved() const {
    const TrackData cur = data();
    return cur.caption != m_savedData.caption ||
           cur.genre != m_savedData.genre ||
           cur.lyrics != m_savedData.lyrics ||
           cur.bpm != m_savedData.bpm ||
           cur.keyscale != m_savedData.keyscale ||
           cur.timesignature != m_savedData.timesignature ||
           cur.duration != m_savedData.duration ||
           cur.language != m_savedData.language ||
           cur.promptOverride != m_savedData.promptOverride ||
           cur.isInstrumental != m_savedData.isInstrumental;
}

void AudioItemWidget::applyDirtyStyle(QWidget *w, bool dirty) {
    if (!w) {
        return;
    }
    if (!dirty) {
        w->setStyleSheet("");
        return;
    }
    if (qobject_cast<QCheckBox *>(w)) {
        w->setStyleSheet("QCheckBox { color: #ff7b7b; font-weight: 600; }");
        return;
    }
    if (qobject_cast<QComboBox *>(w)) {
        w->setStyleSheet("QComboBox { border: 2px solid #d14a4a; background-color: #3b2323; }");
        return;
    }
    if (qobject_cast<QLineEdit *>(w)) {
        w->setStyleSheet("QLineEdit { border: 2px solid #d14a4a; background-color: #3b2323; }");
        return;
    }
    if (qobject_cast<QTextEdit *>(w)) {
        w->setStyleSheet("QTextEdit { border: 2px solid #d14a4a; background-color: #3b2323; }");
        return;
    }
}
