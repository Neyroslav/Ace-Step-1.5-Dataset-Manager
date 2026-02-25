#pragma once

#include <QWidget>
#include <QList>

class QAudioOutput;
class QCheckBox;
class QComboBox;
class QFrame;
class QLabel;
class QLineEdit;
class PlainTextEdit;
class QPushButton;
class QSlider;
class QTextEdit;
class QMediaPlayer;
class QResizeEvent;

struct TrackData {
    QString id;
    QString audioPath;
    QString filename;
    QString caption;
    QString genre;
    QString lyrics;
    int bpm = 0;
    QString keyscale;
    QString timesignature;
    int duration = 0;
    QString language = "instrumental";
    bool isInstrumental = false;
    QString customTag;
    bool labeled = false;
    QString promptOverride;
};

class AudioItemWidget : public QWidget {
    Q_OBJECT

public:
    explicit AudioItemWidget(int index, const TrackData &data, QWidget *parent = nullptr);

    TrackData data() const;
    void markSaved();
    bool hasUnsavedChanges() const;
    void setIndex(int index);
    void setExpanded(bool expanded);
    bool isExpanded() const;
    void setUiScale(int fontSize);
    void setLanguageValue(const QString &language);
    void setGenreValue(const QString &genre);
    void setInstrumentalValue(bool value);
    void setFieldValue(const QString &field, const QString &value);
    void setCaptionText(const QString &caption);
    void setCaptionLyricsOnlyMode(bool enabled);
    void setStickyViewport(QWidget *viewport);
    void updateStickyPosition();
    bool isPlaying() const;
    void togglePlayback();
    void seekRelativeMs(qint64 deltaMs);

signals:
    void deleteRequested(AudioItemWidget *self);
    void saveRequested();
    void playbackControlActivated(AudioItemWidget *self);
    void languageApplyAllRequested(const QString &language);
    void fieldApplyAllRequested(const QString &field, const QString &value);
    void changed();
    void layoutSizeChanged();

private slots:
    void onPlayPause();
    void onDurationChanged(qint64 durationMs);
    void onPositionChanged(qint64 positionMs);
    void onSliderPressed();
    void onSliderMoved(int value);
    void onSliderReleased();
    void onExpandCaptionClicked();
    void onExpandLyricsClicked();
    void triggerChanged();

private:
    void setupUi();
    void connectSignals();
    void updateDirtyHighlight();
    bool isDirtyComparedToSaved() const;
    void applyDirtyStyle(QWidget *w, bool dirty);
    void updatePlayButtonText();
    void updateHeights();
    void updateExpandButtons();
    void applyDurationIfEmpty();
    void seekToMs(qint64 targetMs);
    int contentHeightFor(QTextEdit *edit, int minHeight, int maxHeight = 5000) const;
    void resizeEvent(QResizeEvent *event) override;

    int m_index = 1;
    bool m_captionExpanded = false;
    bool m_lyricsExpanded = false;
    bool m_updatingSlider = false;
    bool m_userSeeking = false;
    qint64 m_seekTargetMs = -1;
    int m_uiScale = 100;
    bool m_savedInitialized = false;

    TrackData m_data;
    TrackData m_savedData;

    QLabel *m_indexLabel = nullptr;
    QLabel *m_fileNameLabel = nullptr;
    QWidget *m_leftHost = nullptr;
    QFrame *m_leftPanel = nullptr;
    QWidget *m_rightHost = nullptr;
    QFrame *m_rightPanel = nullptr;
    QWidget *m_stickyViewport = nullptr;
    int m_lastStickyOffset = -1;
    QPushButton *m_playPauseButton = nullptr;
    QSlider *m_seekSlider = nullptr;
    QMediaPlayer *m_player = nullptr;
    QAudioOutput *m_audioOutput = nullptr;

    PlainTextEdit *m_captionEdit = nullptr;
    QLineEdit *m_genreEdit = nullptr;
    PlainTextEdit *m_lyricsEdit = nullptr;
    QLineEdit *m_bpmEdit = nullptr;
    QLineEdit *m_keyEdit = nullptr;
    QLineEdit *m_timeSigEdit = nullptr;
    QLineEdit *m_durationEdit = nullptr;
    QComboBox *m_languageCombo = nullptr;
    QComboBox *m_promptOverrideCombo = nullptr;
    QPushButton *m_applyLangAllBtn = nullptr;
    QCheckBox *m_instrumentalCheck = nullptr;
    QPushButton *m_deleteBtn = nullptr;
    QPushButton *m_saveBtn = nullptr;
    QPushButton *m_expandCaptionBtn = nullptr;
    QPushButton *m_expandLyricsBtn = nullptr;
    QList<QWidget *> m_secondaryFieldWidgets;
    bool m_captionLyricsOnlyMode = false;
};
