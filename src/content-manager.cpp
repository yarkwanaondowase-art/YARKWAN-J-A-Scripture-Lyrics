#include "content-manager.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QTextStream>

ContentManager::ContentManager(QObject *parent) : QObject(parent) {}

QVector<ContentItem> ContentManager::bibleCatalogue()
{
    // These are the public-domain/free-redistribution versions published by the
    // Midvash bible-data project. URLs intentionally point to raw source data.
    const QString base = "https://raw.githubusercontent.com/midvash/bible-data/main/versions/";
    struct V { const char *id,*name,*lang,*file; } v[] = {
        {"kjv","King James Version","en","kjv"},
        {"asv","American Standard Version","en","asv"},
        {"web","World English Bible","en","web"},
        {"geneva1599","Geneva Bible 1599","en","geneva1599"},
        {"dra","Douay-Rheims American Edition","en","dra"},
        {"lsg","Louis Segond 1910","fr","lsg"},
        {"darby-fr","Bible Darby Française","fr","darby-fr"},
        {"martin1744","Bible David Martin 1744","fr","martin1744"},
        {"luth1912","Lutherbibel 1912","de","luth1912"},
        {"elb1905","Elberfelder Bibel 1905","de","elb1905"},
        {"diodati","Bibbia Diodati 1649","it","diodati"},
        {"riveduta","Bibbia Riveduta 1927","it","riveduta"},
        {"dutch1917","De Heilige Schrift 1917","nl","dutch1917"},
        {"synodal","Синодальный перевод","ru","synodal"},
        {"kp","Куліш-Пулюй (1905)","uk","kp"},
        {"bg","Biblia Gdańska","pl","bg"},
        {"bkr","Bible kralická","cs","bkr"},
        {"kar","Károli Biblia","hu","kar"},
        {"vdc","Biblia Cornilescu","ro","vdc"},
        {"dansk1931","Dansk Bibel 1931","da","dansk1931"},
        {"sv1917","Bibeln 1917","sv","sv1917"},
        {"nb1930","Norsk Bibel 1930","nb","nb1930"},
        {"cuv","Chinese Union Version Traditional","zh","cuv"},
        {"cuvs","Chinese Union Version Simplified","zh","cuvs"},
        {"svd","Smith-Van Dyck","ar","svd"},
        {"vi1934","Kinh Thánh 1934","vi","vi1934"},
        {"almeida-livre","Almeida 1819","pt","almeida-livre"},
        {"lsb","La Sankta Biblio","eo","lsb"}
    };
    QVector<ContentItem> out;
    for (const auto &x : v) {
        ContentItem i;
        i.id=x.id; i.name=x.name; i.kind="bible"; i.language=x.lang;
        i.license="Public domain / free redistribution";
        i.sourceUrl="https://github.com/midvash/bible-data";
        i.downloadUrl=base + x.lang + "/" + x.file + "/" + x.file + ".json";
        i.fileName=x.id + ".json";
        out.push_back(i);
    }
    return out;
}

QVector<ContentItem> ContentManager::hymnCatalogue()
{
    QVector<ContentItem> out;
    ContentItem ss;
    ss.id="ssands"; ss.name="Sacred Songs and Solos — 1,200 hymns"; ss.kind="hymns";
    ss.language="en"; ss.license="Verify source/licensing before redistribution";
    ss.sourceUrl="https://www.hymnaryapps.com/hymnbook/sacred-songs-and-solos";
    ss.fileName="sacred-songs-and-solos-1200.json"; ss.count=1200;
    out.push_back(ss);

    ContentItem mapm;
    mapm.id="hymns-ancient-modern-1861"; mapm.name="Hymns Ancient and Modern — 1861 edition"; mapm.kind="hymns";
    mapm.language="en"; mapm.license="Public domain";
    mapm.sourceUrl="https://commons.wikimedia.org/wiki/File:Hymns_ancient_and_modern_.._(IA_hymnsancientmode00chur).pdf";
    mapm.downloadUrl="https://archive.org/download/hymnsancientmode00chur/hymnsancientmode00chur.pdf";
    mapm.fileName="hymns-ancient-and-modern-1861.pdf"; mapm.count=0;
    out.push_back(mapm);
    return out;
}

QString ContentManager::libraryDirectory()
{
    QString p=QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)+"/libraries";
    QDir().mkpath(p);
    return p;
}

bool ContentManager::downloadToFile(const QString &url, const QString &path, QString &error)
{
    QNetworkAccessManager manager;
    QNetworkRequest request{QUrl(url)};
    request.setHeader(QNetworkRequest::UserAgentHeader, "YARKWAN-J-A-Scripture-Lyrics/0.5");
    QNetworkReply *reply=manager.get(request);
    QEventLoop loop;
    QObject::connect(reply,&QNetworkReply::finished,&loop,&QEventLoop::quit);
    loop.exec();
    if (reply->error()!=QNetworkReply::NoError) { error=reply->errorString(); reply->deleteLater(); return false; }
    QFile f(path);
    if(!f.open(QIODevice::WriteOnly)){error=f.errorString();reply->deleteLater();return false;}
    f.write(reply->readAll()); f.close(); reply->deleteLater(); return true;
}

static QString stripHtml(const QString &s)
{
    QString x=s;
    x.replace(QRegularExpression("<br\\s*/?>",QRegularExpression::CaseInsensitiveOption),"\n");
    x.replace(QRegularExpression("</(?:p|div|li|h[1-6]|tr)>",QRegularExpression::CaseInsensitiveOption),"\n");
    x.remove(QRegularExpression("<[^>]+>"));
    return x.toHtmlEscaped().isEmpty() ? QString() : x;
}

bool ContentManager::installHymnaryPages(const ContentItem &item, QString &path, QString &error)
{
    const QString temp=path+".part";
    QFile::remove(temp);
    QJsonArray songs;
    for(int n=1;n<=item.count;++n){
        emit progress(QString("Downloading %1: hymn %2/%3…").arg(item.name).arg(n).arg(item.count));
        QNetworkAccessManager manager; QNetworkRequest req(QUrl(QString("https://www.hymnaryapps.com/hymnbook/sacred-songs/no/%1").arg(n)));
        req.setHeader(QNetworkRequest::UserAgentHeader,"YARKWAN-J-A-Scripture-Lyrics/0.5");
        QNetworkReply *r=manager.get(req); QEventLoop loop; QObject::connect(r,&QNetworkReply::finished,&loop,&QEventLoop::quit); loop.exec();
        if(r->error()!=QNetworkReply::NoError){error=r->errorString();r->deleteLater();QFile::remove(temp);return false;}
        const QString html=QString::fromUtf8(r->readAll()); r->deleteLater();
        QRegularExpression titleRe("<h[1-3][^>]*>\\s*([^<]+)",QRegularExpression::CaseInsensitiveOption); auto tm=titleRe.match(html);
        QString title=tm.hasMatch()?tm.captured(1).trimmed():QString("Hymn %1").arg(n);
        int start=tm.hasMatch()?tm.capturedEnd():0;
        QString body=html.mid(start);
        int stop=body.indexOf(QRegularExpression("<footer|Download Sacred Songs",QRegularExpression::CaseInsensitiveOption)); if(stop>=0)body=body.left(stop);
        body=stripHtml(body);
        body.replace(QRegularExpression("\\n{3,}"),"\n\n"); body=body.trimmed();
        if(body.isEmpty()) { error=QString("No lyrics could be extracted for hymn %1").arg(n); QFile::remove(temp); return false; }
        QJsonObject song; song["number"]=n; song["title"]=title; song["language"]="en";
        QJsonArray sections; QJsonObject sec; sec["label"]="Lyrics"; sec["text"]=body; sections.append(sec); song["sections"]=sections; songs.append(song);
    }
    QJsonObject root; root["title"]=item.name; root["songs"]=songs;
    QFile f(temp); if(!f.open(QIODevice::WriteOnly)){error=f.errorString();return false;}
    f.write(QJsonDocument(root).toJson(QJsonDocument::Compact)); f.close();
    QFile::remove(path); if(!QFile::rename(temp,path)){error="Could not finalize hymn library file.";return false;} return true;
}

bool ContentManager::installBible(const ContentItem &item, QString &path, QString &error)
{
    if(item.downloadUrl.isEmpty()){error="No download source is configured for this Bible.";return false;}
    path=libraryDirectory()+"/"+item.fileName;
    emit progress("Downloading "+item.name+"…");
    return downloadToFile(item.downloadUrl,path,error);
}

bool ContentManager::installHymnCollection(const ContentItem &item, QString &path, QString &error)
{
    path=libraryDirectory()+"/"+item.fileName;
    if(item.id=="ssands") return installHymnaryPages(item,path,error);
    if(!item.downloadUrl.isEmpty()){
        emit progress("Downloading "+item.name+"…");
        return downloadToFile(item.downloadUrl,path,error);
    }
    error="No download source is configured for this collection."; return false;
}
