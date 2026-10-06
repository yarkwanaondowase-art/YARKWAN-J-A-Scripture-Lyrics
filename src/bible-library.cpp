#include "bible-library.hpp"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>

static BibleBook *getBook(BibleLibrary &out, const QString &name)
{
    for (auto &b : out.books)
        if (b.name.compare(name, Qt::CaseInsensitive) == 0)
            return &b;
    BibleBook b;
    b.name = name;
    out.books.append(b);
    return &out.books.last();
}

static bool addVerse(BibleLibrary &out, const QString &book, int chapter, int verse, const QString &text)
{
    if (book.trimmed().isEmpty() || chapter <= 0 || verse <= 0 || text.trimmed().isEmpty())
        return false;
    BibleBook *b = getBook(out, book.trimmed());
    for (auto &existing : b->verses) {
        if (existing.chapter == chapter && existing.verse == verse) {
            existing.text = text.trimmed();
            return true;
        }
    }
    BibleVerse v;
    v.book = b->name;
    v.chapter = chapter;
    v.verse = verse;
    v.text = text.trimmed();
    b->verses.append(v);
    return true;
}

const BibleVerse *BibleLibrary::find(const QString &book, int chapter, int verse) const
{
    for (const auto &b : books) {
        if (b.name.compare(book, Qt::CaseInsensitive) != 0) continue;
        for (const auto &v : b.verses)
            if (v.chapter == chapter && v.verse == verse) return &v;
    }
    return nullptr;
}

bool BibleLibraryLoader::loadJson(const QString &fileName, BibleLibrary &out, QString &error)
{
    QFile f(fileName);
    if (!f.open(QIODevice::ReadOnly)) { error = f.errorString(); return false; }
    QJsonParseError pe;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &pe);
    if (pe.error != QJsonParseError::NoError) { error = pe.errorString(); return false; }
    out = BibleLibrary{};

    QJsonArray verses;
    if (doc.isObject()) {
        const QJsonObject root = doc.object();
        out.format = root.value("format").toString();
        out.title = root.value("title").toString(root.value("name").toString());
        out.language = root.value("language").toString();
        out.copyright = root.value("copyright").toString();
        verses = root.value("verses").toArray();
        // Also accept {books:[{name,verses:[...]}]} for future converters.
        if (verses.isEmpty()) {
            // v1 grouped format: {books:[{name,verses:[...]}]}
            for (const auto &bo : root.value("books").toArray()) {
                const QJsonObject b = bo.toObject();
                const QString bn = b.value("name").toString(b.value("book").toString());
                for (const auto &vo : b.value("verses").toArray()) {
                    const QJsonObject v = vo.toObject();
                    addVerse(out, bn, v.value("chapter").toInt(), v.value("verse").toInt(), v.value("text").toString());
                }
                // Arul John style: {book, chapters:[{chapter, verses:[{verse,text}]}]}
                for (const auto &co : b.value("chapters").toArray()) {
                    const QJsonObject c = co.toObject();
                    const int ch = c.value("chapter").toString().toInt();
                    const int ch2 = ch > 0 ? ch : c.value("chapter").toInt();
                    for (const auto &vo : c.value("verses").toArray()) {
                        const QJsonObject v = vo.toObject();
                        const int vn = v.value("verse").toString().toInt();
                        const int vn2 = vn > 0 ? vn : v.value("verse").toInt();
                        addVerse(out, bn, ch2, vn2, v.value("text").toString());
                    }
                }
            }
        }
        // Also accept a single Arul John book object directly.
        if (verses.isEmpty() && root.contains("chapters")) {
            const QString bn = root.value("book").toString(root.value("name").toString());
            for (const auto &co : root.value("chapters").toArray()) {
                const QJsonObject c = co.toObject();
                int ch = c.value("chapter").toString().toInt();
                if (ch <= 0) ch = c.value("chapter").toInt();
                for (const auto &vo : c.value("verses").toArray()) {
                    const QJsonObject v = vo.toObject();
                    int vn = v.value("verse").toString().toInt();
                    if (vn <= 0) vn = v.value("verse").toInt();
                    addVerse(out, bn, ch, vn, v.value("text").toString());
                }
            }
        }
    } else if (doc.isArray()) {
        verses = doc.array();
    }

    for (const auto &value : verses) {
        const QJsonObject v = value.toObject();
        addVerse(out, v.value("book").toString(), v.value("chapter").toInt(), v.value("verse").toInt(), v.value("text").toString());
    }

    if (out.title.isEmpty()) out.title = QFileInfo(fileName).completeBaseName();
    if (out.isEmpty()) { error = QStringLiteral("No Bible verses were found in the JSON file."); return false; }
    return true;
}

bool BibleLibraryLoader::loadText(const QString &fileName, BibleLibrary &out, QString &error)
{
    QFile f(fileName);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) { error = f.errorString(); return false; }
    QTextStream in(&f);
    in.setEncoding(QStringConverter::Utf8);
    out = BibleLibrary{};
    out.title = QFileInfo(fileName).completeBaseName();

    // Simple interchange format: BOOK|CHAPTER|VERSE|TEXT
    while (!in.atEnd()) {
        const QString line = in.readLine();
        if (line.trimmed().isEmpty() || line.trimmed().startsWith('#')) continue;
        const QStringList p = line.split('|');
        if (p.size() < 4) continue;
        bool ok1 = false, ok2 = false;
        const int chapter = p[1].trimmed().toInt(&ok1);
        const int verse = p[2].trimmed().toInt(&ok2);
        if (ok1 && ok2) addVerse(out, p[0], chapter, verse, p.mid(3).join('|'));
    }
    if (out.isEmpty()) { error = QStringLiteral("No verses found. Expected BOOK|CHAPTER|VERSE|TEXT lines."); return false; }
    return true;
}

bool BibleLibraryLoader::loadFile(const QString &fileName, BibleLibrary &out, QString &error)
{
    const QString ext = QFileInfo(fileName).suffix().toLower();
    if (ext == "json") return loadJson(fileName, out, error);
    if (ext == "txt" || ext == "csv") return loadText(fileName, out, error);
    if (ext == "bib") {
        error = QStringLiteral(".BIB is reserved for the future Tiv/BibleShow-compatible importer. The presentation engine is already prepared for it.");
        return false;
    }
    error = QStringLiteral("Unsupported Bible file type: .%1").arg(ext);
    return false;
}
