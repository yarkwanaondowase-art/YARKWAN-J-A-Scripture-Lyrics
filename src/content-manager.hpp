#pragma once

#include <QObject>
#include <QString>
#include <QVector>

struct ContentItem {
    QString id;
    QString name;
    QString kind;       // bible or hymns
    QString language;
    QString license;
    QString sourceUrl;
    QString downloadUrl;
    QString fileName;
    int count = 0;
};

class ContentManager : public QObject {
    Q_OBJECT
public:
    explicit ContentManager(QObject *parent = nullptr);
    static QVector<ContentItem> bibleCatalogue();
    static QVector<ContentItem> hymnCatalogue();
    static QString libraryDirectory();
    bool installBible(const ContentItem &item, QString &path, QString &error);
    bool installHymnCollection(const ContentItem &item, QString &path, QString &error);

signals:
    void progress(const QString &message);

private:
    bool downloadToFile(const QString &url, const QString &path, QString &error);
    bool installHymnaryPages(const ContentItem &item, QString &path, QString &error);
};
