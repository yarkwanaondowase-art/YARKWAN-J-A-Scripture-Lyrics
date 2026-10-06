#pragma once
#include <QString>
#include <QVector>

struct SongSection {
    QString label;
    QString text;
};
struct Song {
    QString title;
    QString language;
    QVector<SongSection> sections;
};
struct SongLibrary {
    QString title;
    QVector<Song> songs;
};
class SongLibraryLoader {
public:
    static bool loadFile(const QString &fileName, SongLibrary &out, QString &error);
};
