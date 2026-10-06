#include "song-library.hpp"
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTextStream>

static QStringList splitSections(const QString &body) {
    QStringList lines = body.split(QRegularExpression("\\r?\\n"));
    QStringList out; QString currentLabel = "Lyrics"; QStringList current;
    auto flush = [&]() { if (!current.join("\\n").trimmed().isEmpty()) out << currentLabel << current.join("\\n").trimmed(); current.clear(); };
    QRegularExpression marker("^\\s*\\[(.+)\\]\\s*$");
    for (const QString &line : lines) {
        auto m = marker.match(line);
        if (m.hasMatch()) { flush(); currentLabel = m.captured(1).trimmed(); }
        else current << line;
    }
    flush(); return out;
}

bool SongLibraryLoader::loadFile(const QString &fileName, SongLibrary &out, QString &error) {
    out = {};
    QFile f(fileName);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) { error = f.errorString(); return false; }
    const QByteArray raw = f.readAll();
    const QString ext = QFileInfo(fileName).suffix().toLower();
    if (ext == "json") {
        QJsonParseError pe{}; auto doc = QJsonDocument::fromJson(raw, &pe);
        if (pe.error != QJsonParseError::NoError || !doc.isObject()) { error = pe.errorString(); return false; }
        auto root = doc.object(); out.title = root.value("title").toString(QFileInfo(fileName).completeBaseName());
        for (const auto &sv : root.value("songs").toArray()) {
            auto o = sv.toObject(); Song s; s.title = o.value("title").toString(); s.language = o.value("language").toString();
            for (const auto &vv : o.value("sections").toArray()) { auto so=vv.toObject(); s.sections.push_back({so.value("label").toString("Verse"), so.value("text").toString()}); }
            if (!s.title.isEmpty()) out.songs.push_back(s);
        }
        return !out.songs.isEmpty();
    }
    QString text = QString::fromUtf8(raw);
    if (ext == "csv") {
        const auto rows = text.split(QRegularExpression("\\r?\\n"), Qt::SkipEmptyParts);
        for (int i=1;i<rows.size();++i) { auto c=rows[i].split(','); if(c.size()<3) continue; Song s; s.title=c[0].trimmed(); s.language=c[1].trimmed(); s.sections.push_back({c[2].trimmed(), c.mid(3).join(",").trimmed()}); out.songs.push_back(s); }
    } else {
        QString title = QFileInfo(fileName).completeBaseName();
        QString currentTitle; QStringList body;
        auto flush=[&](){ if(currentTitle.isEmpty() && body.isEmpty()) return; Song s; s.title=currentTitle.isEmpty()?title:currentTitle; auto parts=splitSections(body.join("\n")); for(int i=0;i+1<parts.size();i+=2) s.sections.push_back({parts[i],parts[i+1]}); if(s.sections.isEmpty()) s.sections.push_back({"Lyrics",body.join("\n").trimmed()}); out.songs.push_back(s); currentTitle.clear(); body.clear(); };
        const auto lines=text.split(QRegularExpression("\\r?\\n"));
        for(const auto &line: lines) { if(line.startsWith("# ")) { flush(); currentTitle=line.mid(2).trimmed(); } else body << line; } flush();
    }
    out.title = QFileInfo(fileName).completeBaseName();
    if (out.songs.isEmpty()) { error = "No songs were found. Use # Song Title and [Verse]/[Chorus] markers for TXT."; return false; }
    return true;
}
