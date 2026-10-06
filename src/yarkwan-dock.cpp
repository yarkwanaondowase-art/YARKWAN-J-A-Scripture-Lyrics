#include "yarkwan-dock.hpp"
#include "bible-library.hpp"
#include "song-library.hpp"
#include "content-manager.hpp"
#include <obs-frontend-api.h>
#include <obs.h>
#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QFileInfo>
#include <QCheckBox>
#include <memory>
#include <QDialog>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QScrollArea>
#include <QTabWidget>

struct DockState { BibleLibrary bible; SongLibrary songs; bool showingSong=false; };

YarkwanDock::YarkwanDock(QWidget *parent) : QWidget(parent), libraryState(std::make_shared<DockState>()) {
    contentManager = new ContentManager(this);
    setMinimumWidth(370);
    auto *root=new QVBoxLayout(this);
    auto *brand=new QLabel("<b>YARKWAN J. A.</b><br>Scripture &amp; Lyrics"); root->addWidget(brand);

    auto *bibleGroup=new QGroupBox("Bible"); auto *bl=new QVBoxLayout(bibleGroup);
    auto *bi=new QHBoxLayout; auto *downloadBibleBtn=new QPushButton("Bible Library…"); auto *importBibleBtn=new QPushButton("Import Bible…"); libraryBox=new QComboBox; libraryBox->setEnabled(false); bi->addWidget(downloadBibleBtn); bi->addWidget(importBibleBtn); bi->addWidget(libraryBox,1); bl->addLayout(bi);
    bibleSearch=new QLineEdit; bibleSearch->setPlaceholderText("Search Bible text…"); bl->addWidget(bibleSearch);
    auto *bf=new QFormLayout; bookBox=new QComboBox; chapterBox=new QSpinBox; verseBox=new QSpinBox; chapterBox->setRange(1,9999); verseBox->setRange(1,9999); bf->addRow("Book",bookBox); bf->addRow("Chapter",chapterBox); bf->addRow("Verse",verseBox); bl->addLayout(bf); root->addWidget(bibleGroup);

    auto *songGroup=new QGroupBox("Songs"); auto *sl=new QVBoxLayout(songGroup); auto *si=new QHBoxLayout; auto *downloadSongsBtn=new QPushButton("Hymn Library…"); auto *importSongsBtn=new QPushButton("Import Songs…"); songBox=new QComboBox; si->addWidget(downloadSongsBtn); si->addWidget(importSongsBtn); si->addWidget(songBox,1); sl->addLayout(si); songSearch=new QLineEdit; songSearch->setPlaceholderText("Search songs…"); sl->addWidget(songSearch); auto *sf=new QFormLayout; sectionBox=new QComboBox; sf->addRow("Section",sectionBox); sl->addLayout(sf); root->addWidget(songGroup);

    auto *presentation=new QGroupBox("Preview / Presentation"); auto *pl=new QVBoxLayout(presentation); textEdit=new QTextEdit; textEdit->setMinimumHeight(120); textEdit->setPlaceholderText("Selected Scripture or song lyrics…"); referenceEdit=new QLineEdit; referenceEdit->setPlaceholderText("Reference / song section"); pl->addWidget(textEdit); pl->addWidget(referenceEdit); auto *style=new QFormLayout; modeBox=new QComboBox; modeBox->addItem("Lower Third","lower-third"); modeBox->addItem("Full Screen","full-screen"); fontSizeBox=new QSpinBox; fontSizeBox->setRange(12,180); fontSizeBox->setValue(48); outlineBox=new QCheckBox("Text outline"); outlineBox->setChecked(true); style->addRow("Mode",modeBox); style->addRow("Font size",fontSizeBox); style->addRow("",outlineBox); pl->addLayout(style); root->addWidget(presentation);
    auto *nav=new QHBoxLayout; auto *prev=new QPushButton("◀ Previous"); auto *next=new QPushButton("Next ▶"); nav->addWidget(prev); nav->addWidget(next); root->addLayout(nav);
    auto *actions=new QHBoxLayout; auto *addScripture=new QPushButton("Add Scripture Source"); auto *addLyrics=new QPushButton("Add Lyrics Source"); actions->addWidget(addScripture); actions->addWidget(addLyrics); root->addLayout(actions);
    auto *show=new QPushButton("SHOW / UPDATE"); auto *blank=new QPushButton("BLANK"); auto *row=new QHBoxLayout; row->addWidget(show); row->addWidget(blank); root->addLayout(row);
    statusLabel=new QLabel("Ready — install a Bible/hymnal or import your own files."); statusLabel->setWordWrap(true); root->addWidget(statusLabel); root->addStretch();

    connect(importBibleBtn,&QPushButton::clicked,this,&YarkwanDock::importBible);
    connect(downloadBibleBtn,&QPushButton::clicked,this,&YarkwanDock::openContentManager);
    connect(downloadSongsBtn,&QPushButton::clicked,this,&YarkwanDock::openContentManager);
    connect(importSongsBtn,&QPushButton::clicked,this,&YarkwanDock::importSongs);
    connect(bookBox,&QComboBox::currentTextChanged,this,[this](){chapterBox->setValue(1);verseBox->setValue(1);if(!std::static_pointer_cast<DockState>(libraryState)->showingSong)textEdit->setPlainText(currentText());});
    connect(chapterBox,qOverload<int>(&QSpinBox::valueChanged),this,[this](int){if(!std::static_pointer_cast<DockState>(libraryState)->showingSong)textEdit->setPlainText(currentText());});
    connect(verseBox,qOverload<int>(&QSpinBox::valueChanged),this,[this](int){if(!std::static_pointer_cast<DockState>(libraryState)->showingSong)textEdit->setPlainText(currentText());});
    connect(sectionBox,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int){auto st=std::static_pointer_cast<DockState>(libraryState);if(st->showingSong){textEdit->setPlainText(currentSongText());referenceEdit->setText(sectionBox->currentText());}});
    connect(songBox,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int){refreshSongUi();});
    connect(bibleSearch,&QLineEdit::returnPressed,this,[this](){auto st=std::static_pointer_cast<DockState>(libraryState);QString q=bibleSearch->text().trimmed();if(q.isEmpty())return;for(int bi=0;bi<st->bible.books.size();++bi)for(const auto &v:st->bible.books[bi].verses)if(v.text.contains(q,Qt::CaseInsensitive)){st->showingSong=false;bookBox->setCurrentIndex(bi);chapterBox->setValue(v.chapter);verseBox->setValue(v.verse);textEdit->setPlainText(v.text);referenceEdit->setText(v.book+" "+QString::number(v.chapter)+":"+QString::number(v.verse));return;}statusLabel->setText("No Bible verse matched the search.");});
    connect(songSearch,&QLineEdit::returnPressed,this,[this](){QString q=songSearch->text().trimmed();auto st=std::static_pointer_cast<DockState>(libraryState);if(q.isEmpty())return;for(int i=0;i<st->songs.songs.size();++i)if(st->songs.songs[i].title.contains(q,Qt::CaseInsensitive)){songBox->setCurrentIndex(i);refreshSongUi();return;}statusLabel->setText("No song matched the search.");});
    connect(prev,&QPushButton::clicked,this,[this](){navigate(-1);}); connect(next,&QPushButton::clicked,this,[this](){navigate(1);});
    connect(addScripture,&QPushButton::clicked,this,[this](){addSource(false);}); connect(addLyrics,&QPushButton::clicked,this,[this](){addSource(true);}); connect(show,&QPushButton::clicked,this,&YarkwanDock::applyText);
    connect(blank,&QPushButton::clicked,this,[this](){textEdit->clear();referenceEdit->clear();applyText();statusLabel->setText("Display blanked.");});
}

void YarkwanDock::openContentManager(){
    QDialog dlg(this); dlg.setWindowTitle("YARKWAN J. A. — Content Library"); dlg.resize(720,560);
    auto *root=new QVBoxLayout(&dlg);
    auto *tabs=new QTabWidget(&dlg); root->addWidget(tabs);
    auto makeTab=[&](const QVector<ContentItem> &items, bool bible){
        QWidget *w=new QWidget; auto *v=new QVBoxLayout(w); auto *list=new QListWidget; v->addWidget(list,1);
        for(const auto &it:items){ auto *row=new QWidget; auto *h=new QHBoxLayout(row); h->setContentsMargins(4,4,4,4); auto *name=new QLabel(QString("<b>%1</b><br><small>%2 · %3</small>").arg(it.name.toHtmlEscaped(),it.language.toUpper(),it.license.toHtmlEscaped())); h->addWidget(name,1); auto *b=new QPushButton("Install / Update"); h->addWidget(b); QListWidgetItem *li=new QListWidgetItem; li->setSizeHint(QSize(0,60)); list->addItem(li); list->setItemWidget(li,row);
            QString existing=ContentManager::libraryDirectory()+"/"+it.fileName;
            if(QFileInfo::exists(existing)){b->setText("Installed — Update");}
            connect(b,&QPushButton::clicked,&dlg,[&,it,b,bible,existing](){QString path,err; b->setEnabled(false); statusLabel->setText("Preparing "+it.name+"…"); bool ok=bible?contentManager->installBible(it,path,err):contentManager->installHymnCollection(it,path,err); b->setEnabled(true); if(!ok){QMessageBox::warning(&dlg,"Installation failed",it.name+"\n\n"+err); statusLabel->setText("Installation failed: "+err); return;} if(bible)loadBibleLibraryFile(path); else loadSongLibraryFile(path); b->setText("Installed — Update"); statusLabel->setText("Installed: "+it.name); });
        }
        return w;
    };
    tabs->addTab(makeTab(ContentManager::bibleCatalogue(),true),"Bible Versions");
    tabs->addTab(makeTab(ContentManager::hymnCatalogue(),false),"Hymn Collections");
    auto *info=new QLabel("Downloads are stored in your Windows user data folder and remain available offline. Manual Import is still available for your own JSON/TXT/CSV files."); info->setWordWrap(true); root->addWidget(info);
    connect(contentManager,&ContentManager::progress,this,[this](const QString &m){statusLabel->setText(m);});
    dlg.exec();
}

void YarkwanDock::loadBibleLibraryFile(const QString &path){BibleLibrary lib;QString err;if(!BibleLibraryLoader::loadFile(path,lib,err)){QMessageBox::warning(this,"Bible",err);return;}auto st=std::static_pointer_cast<DockState>(libraryState);st->bible=lib;st->showingSong=false;libraryBox->clear();libraryBox->addItem(lib.title.isEmpty()?QFileInfo(path).completeBaseName():lib.title);libraryBox->setEnabled(true);bookBox->clear();for(const auto &b:lib.books)bookBox->addItem(b.name);chapterBox->setValue(1);verseBox->setValue(1);referenceEdit->setText(bookBox->currentText()+" 1:1");textEdit->setPlainText(currentText());statusLabel->setText(QString("Bible loaded: %1 books.").arg(lib.books.size()));}
void YarkwanDock::loadSongLibraryFile(const QString &path){SongLibrary lib;QString err;if(!SongLibraryLoader::loadFile(path,lib,err)){QMessageBox::warning(this,"Hymns",err);return;}auto st=std::static_pointer_cast<DockState>(libraryState);st->songs=lib;st->showingSong=true;songBox->clear();for(const auto &s:lib.songs)songBox->addItem(s.title);refreshSongUi();statusLabel->setText(QString("Hymns loaded: %1.").arg(lib.songs.size()));}

void YarkwanDock::importBible(){QString f=QFileDialog::getOpenFileName(this,"Import Bible",{},"Bible files (*.json *.txt *.csv *.bib);;All files (*.*)");if(f.isEmpty())return;loadBibleLibraryFile(f);}
void YarkwanDock::importSongs(){QString f=QFileDialog::getOpenFileName(this,"Import Songs",{},"Song files (*.json *.txt *.csv);;All files (*.*)");if(f.isEmpty())return;loadSongLibraryFile(f);}
QString YarkwanDock::currentText()const{auto st=std::static_pointer_cast<DockState>(libraryState);if(bookBox->currentIndex()<0||bookBox->currentIndex()>=st->bible.books.size())return textEdit->toPlainText();const auto &b=st->bible.books[bookBox->currentIndex()];for(const auto &v:b.verses)if(v.chapter==chapterBox->value()&&v.verse==verseBox->value())return v.text;return textEdit->toPlainText();}
QString YarkwanDock::currentReference()const{auto st=std::static_pointer_cast<DockState>(libraryState);if(!st->showingSong&&bookBox->currentIndex()>=0&&bookBox->currentIndex()<st->bible.books.size())return bookBox->currentText()+" "+QString::number(chapterBox->value())+":"+QString::number(verseBox->value());return referenceEdit->text();}
QString YarkwanDock::currentSongText()const{auto st=std::static_pointer_cast<DockState>(libraryState);int i=songBox->currentIndex(),j=sectionBox->currentIndex();if(i<0||i>=st->songs.songs.size())return textEdit->toPlainText();if(j<0||j>=st->songs.songs[i].sections.size())return textEdit->toPlainText();return st->songs.songs[i].sections[j].text;}
void YarkwanDock::refreshSongUi(){auto st=std::static_pointer_cast<DockState>(libraryState);int i=songBox->currentIndex();sectionBox->clear();if(i<0||i>=st->songs.songs.size())return;st->showingSong=true;for(const auto &s:st->songs.songs[i].sections)sectionBox->addItem(s.label);if(sectionBox->count()){sectionBox->setCurrentIndex(0);textEdit->setPlainText(currentSongText());referenceEdit->setText(songBox->currentText()+" — "+sectionBox->currentText());}}
void YarkwanDock::applyText(){auto st=std::static_pointer_cast<DockState>(libraryState);if(st->showingSong){textEdit->setPlainText(currentSongText());referenceEdit->setText(songBox->currentText()+" — "+sectionBox->currentText());}else{ textEdit->setPlainText(currentText());referenceEdit->setText(currentReference()); }updateExistingSources();statusLabel->setText("SHOW / UPDATE sent to YARKWAN sources in the current scene.");}
void YarkwanDock::navigate(int d){auto st=std::static_pointer_cast<DockState>(libraryState);if(st->showingSong){int i=qBound(0,sectionBox->currentIndex()+d,sectionBox->count()-1);sectionBox->setCurrentIndex(i);}else verseBox->setValue(qMax(1,verseBox->value()+d));applyText();}
void YarkwanDock::updateExistingSources(){
    obs_source_t *sceneSource=obs_frontend_get_current_scene(); if(!sceneSource)return;
    obs_scene_t *scene=obs_scene_from_source(sceneSource);
    if(scene){
        struct UpdateData { QString text, reference, mode; int fontSize; bool outline; };
        UpdateData data{textEdit->toPlainText(),referenceEdit->text(),modeBox->currentData().toString(),fontSizeBox->value(),outlineBox->isChecked()};
        auto cb=[](obs_scene_t *, obs_sceneitem_t *item, void *ptr)->bool {
            auto *d=static_cast<UpdateData *>(ptr); obs_source_t *s=obs_sceneitem_get_source(item);
            if(s && strcmp(obs_source_get_id(s),"yarkwan_scripture_lyrics_source")==0){
                obs_data_t *x=obs_source_get_settings(s);
                obs_data_set_string(x,"text",d->text.toUtf8().constData()); obs_data_set_string(x,"reference",d->reference.toUtf8().constData());
                obs_data_set_string(x,"mode",d->mode.toUtf8().constData()); obs_data_set_int(x,"font_size",d->fontSize); obs_data_set_bool(x,"outline",d->outline);
                obs_source_update(s,x); obs_data_release(x);
            } return true;
        };
        obs_scene_enum_items(scene,cb,&data); obs_scene_release(scene);
    }
    obs_source_release(sceneSource);
}
void YarkwanDock::addSource(bool lyrics){obs_source_t *source=obs_source_create("yarkwan_scripture_lyrics_source",lyrics?"YARKWAN Lyrics":"YARKWAN Scripture",nullptr,nullptr);if(!source){statusLabel->setText("Could not create OBS source.");return;}obs_data_t *settings=obs_data_create();obs_data_set_string(settings,"text",textEdit->toPlainText().toUtf8().constData());obs_data_set_string(settings,"reference",referenceEdit->text().toUtf8().constData());obs_data_set_string(settings,"mode",modeBox->currentData().toString().toUtf8().constData());obs_data_set_int(settings,"font_size",fontSizeBox->value());obs_data_set_bool(settings,"outline",outlineBox->isChecked());obs_source_update(source,settings);obs_data_release(settings);obs_source_t *sceneSource=obs_frontend_get_current_scene();if(sceneSource){obs_scene_t *scene=obs_scene_from_source(sceneSource);if(scene){obs_scene_add(scene,source);obs_scene_release(scene);}obs_source_release(sceneSource);}obs_source_release(source);statusLabel->setText(lyrics?"Lyrics source added.":"Scripture source added.");}
void YarkwanDock::setMode(const QString &){} void YarkwanDock::chooseColor(bool){}
