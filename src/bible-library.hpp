#pragma once

#include <QString>
#include <QVector>

struct BibleVerse {
    QString book;
    int chapter = 0;
    int verse = 0;
    QString text;
};

struct BibleBook {
    QString name;
    QVector<BibleVerse> verses;
};

struct BibleLibrary {
    QString format;
    QString title;
    QString language;
    QString copyright;
    QVector<BibleBook> books;
    bool isEmpty() const { return books.isEmpty(); }
    const BibleVerse *find(const QString &book, int chapter, int verse) const;
};

class BibleLibraryLoader {
public:
    static bool loadJson(const QString &fileName, BibleLibrary &out, QString &error);
    static bool loadText(const QString &fileName, BibleLibrary &out, QString &error);
    static bool loadFile(const QString &fileName, BibleLibrary &out, QString &error);
};
