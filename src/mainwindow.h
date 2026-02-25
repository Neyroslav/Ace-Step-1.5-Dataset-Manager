#pragma once

#include "audioitemwidget.h"

#include <QDateTime>
#include <QMainWindow>
#include <QUrl>

class QCheckBox;
class QCloseEvent;
class QComboBox;
class QGroupBox;
class QKeySequenceEdit;
class QLabel;
class QLineEdit;
class QResizeEvent;
class QScrollArea;
class QSlider;
class QSpinBox;
class QShortcut;
class QWidget;
class QVBoxLayout;

struct DatasetMetadata {
    QString name = "Dataset";
    QString customTag;
    QString tagPosition = "prepend";
    QDateTime createdAt = QDateTime::currentDateTimeUtc();
    bool allInstrumental = false;
    int genreRatio = 0;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void openDatasetJsonFile();
    void openDatasetFolder();
    void saveDataset();
    void saveDatasetAs();
    void refreshDataset();
    void mergeParagraphs();
    void makeBackup();
    void expandAll();
    void collapseAll();
    void updateStats();
    void onDeleteTrack(AudioItemWidget *item);
    void applyLanguageToAll(const QString &language);
    void applyFieldToAll(const QString &field, const QString &value);
    void onAllInstrumentalToggled(bool checked);
    void onDatasetScrollChanged(int value);
    void showCaptionTutorial();
    void showLyricsTutorial();
    void onAlwaysOnTopChanged();
    void toggleFocusMode();
    void togglePlaybackOnTargetTrack();
    void seekPlaybackBackward();
    void seekPlaybackForward();

private:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void setupUi();
    void clearTracks();
    void rebuildTrackList(const QList<TrackData> &tracks);
    QList<TrackData> collectTracks() const;
    void loadFromFolder(const QString &folderPath);
    QList<TrackData> buildFromAudioFiles(const QString &folderPath) const;
    bool loadFromJson(const QString &jsonPath);
    QString defaultJsonPath() const;
    QString currentTimestampFileSafe() const;
    QString generateId(const QString &source) const;
    static QString sanitizeTagPosition(const QString &value);
    void showTutorialDialog(const QString &title, const QString &markdown, const QUrl &baseUrl = QUrl()) const;
    void showPathToast(const QString &prefix, const QString &filePath);
    void positionToast();
    int unsavedCardsCount() const;
    bool hasUnsavedMetaChanges() const;
    bool hasUnsavedChanges() const;
    void captureMetaSnapshot();
    void updateMainWindowTitle();
    AudioItemWidget *playbackTargetTrack() const;

    DatasetMetadata m_meta;
    QString m_currentFolder;
    QString m_currentJsonPath;
    QString m_lastOpenDir;
    bool m_currentSourceIsExplicitJson = false;

    QWidget *m_datasetContainer = nullptr;
    QScrollArea *m_datasetScroll = nullptr;
    QVBoxLayout *m_trackLayout = nullptr;
    QList<AudioItemWidget *> m_trackWidgets;

    QLineEdit *m_nameEdit = nullptr;
    QLineEdit *m_customTagEdit = nullptr;
    QCheckBox *m_allInstrumentalCheck = nullptr;
    QComboBox *m_tagPositionCombo = nullptr;
    QSlider *m_genreRatioSlider = nullptr;
    QLabel *m_genreRatioLabel = nullptr;

    QSlider *m_fontSlider = nullptr;
    QLabel *m_fontSizeValueLabel = nullptr;
    QCheckBox *m_onTopCheck = nullptr;
    QCheckBox *m_captionLyricsOnlyCheck = nullptr;
    QSpinBox *m_seekStepSecondsSpin = nullptr;
    QKeySequenceEdit *m_focusShortcutEdit = nullptr;
    QShortcut *m_focusShortcut = nullptr;
    QKeySequenceEdit *m_saveShortcutEdit = nullptr;
    QShortcut *m_saveShortcut = nullptr;
    QKeySequenceEdit *m_backupShortcutEdit = nullptr;
    QShortcut *m_backupShortcut = nullptr;
    QKeySequenceEdit *m_playPauseShortcutEdit = nullptr;
    QShortcut *m_playPauseShortcut = nullptr;
    QKeySequenceEdit *m_seekBackwardShortcutEdit = nullptr;
    QKeySequenceEdit *m_seekForwardShortcutEdit = nullptr;
    QShortcut *m_seekBackwardShortcut = nullptr;
    QShortcut *m_seekForwardShortcut = nullptr;
    QGroupBox *m_globalGroup = nullptr;
    QWidget *m_rightPanel = nullptr;
    bool m_focusMode = false;

    QLabel *m_captionedLabel = nullptr;
    QLabel *m_toCaptionLabel = nullptr;
    QLabel *m_lyricsDoneLabel = nullptr;
    QLabel *m_lyricsLeftLabel = nullptr;
    QLabel *m_unsavedCardsLabel = nullptr;
    QWidget *m_saveToast = nullptr;
    AudioItemWidget *m_lastPlaybackActiveTrack = nullptr;

    QString m_savedName;
    QString m_savedCustomTag;
    QString m_savedTagPosition;
    int m_savedGenreRatio = 0;
    bool m_savedAllInstrumental = false;
    bool m_metaSnapshotReady = false;
};
