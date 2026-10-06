#pragma once
#include <QWidget>
#include <memory>
#include "content-manager.hpp"

class YarkwanDock : public QWidget {
    Q_OBJECT
public:
    explicit YarkwanDock(QWidget *parent = nullptr);
private:
    void importBible();
    void importSongs();
    void openContentManager();
    void loadBibleLibraryFile(const QString &path);
    void loadSongLibraryFile(const QString &path);
    void addSource(bool lyrics);
    void applyText();
    void navigate(int direction);
    void updateExistingSources();
    QString currentText() const;
    QString currentReference() const;
    QString currentSongText() const;
    void refreshSongUi();
    class QComboBox *libraryBox;
    class QComboBox *bookBox;
    class QSpinBox *chapterBox;
    class QSpinBox *verseBox;
    class QLineEdit *bibleSearch;
    class QComboBox *songBox;
    class QComboBox *sectionBox;
    class QLineEdit *songSearch;
    class QTextEdit *textEdit;
    class QLineEdit *referenceEdit;
    class QComboBox *modeBox;
    class QSpinBox *fontSizeBox;
    class QCheckBox *outlineBox;
    class QLabel *statusLabel;
    ContentManager *contentManager = nullptr;
    std::shared_ptr<void> libraryState;
};
