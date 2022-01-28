/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     lixiang<lixianga@uniontech.com>
 *
 * Maintainer: lixiang<lixianga@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "musicmessageview.h"

#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QUrl>
#include <QResizeEvent>
#include <QMediaPlayer>
#include <QMediaMetaData>
#include <QTime>
#include <QFileInfo>
#include <QTextCodec>

#include <unicode/ucnv.h>
#include <unicode/ucsdet.h>
#include <taglib/tag.h>
#include <taglib/fileref.h>
#include <taglib/taglib.h>
#include <taglib/tpropertymap.h>

PREVIEW_USE_NAMESPACE
MusicMessageView::MusicMessageView(const QString &uri, QWidget *parent)
    : QFrame(parent),
      currentUrl(uri)
{
    initUI();
    localeCodes.insert("zh_CN", "GB18030");
}

void MusicMessageView::initUI()
{
    setFixedSize(600, 300);

    titleLabel = new QLabel(this);
    titleLabel->setObjectName("Title");

    artistLabel = new QLabel(this);
    artistLabel->setObjectName("Artist");
    artistLabel->setText(tr("Artist:"));
    artistValue = new QLabel(this);
    artistValue->setObjectName("artistValue");

    albumLabel = new QLabel(this);
    albumLabel->setObjectName("Album");
    albumLabel->setText(tr("Album:"));
    albumValue = new QLabel(this);
    albumValue->setObjectName("albumValue");

    imgLabel = new QLabel(this);
    imgLabel->setFixedSize(QSize(240, 240));

    mediaPlayer = new QMediaPlayer(this);
    connect(mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, &MusicMessageView::mediaStatusChanged);

    mediaPlayer->setMedia(QUrl::fromUserInput(currentUrl));

    QHBoxLayout *artistLayout = new QHBoxLayout;
    artistLayout->addWidget(artistLabel);
    artistLayout->addSpacing(5);
    artistLayout->addWidget(artistValue, 1);

    QHBoxLayout *albumLayout = new QHBoxLayout;
    albumLayout->addWidget(albumLabel);
    albumLayout->addSpacing(5);
    albumLayout->addWidget(albumValue, 1);

    QVBoxLayout *messageLayout = new QVBoxLayout;
    messageLayout->setSpacing(0);
    messageLayout->addWidget(titleLabel);
    messageLayout->addSpacing(10);
    messageLayout->addLayout(artistLayout);
    messageLayout->addSpacing(3);
    messageLayout->addLayout(albumLayout);
    messageLayout->addStretch();

    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(imgLabel, 0, Qt::AlignTop);
    mainLayout->addSpacing(15);
    mainLayout->addLayout(messageLayout);
    mainLayout->addStretch();

    setLayout(mainLayout);

    setStyleSheet("QLabel#Title{"
                  "font-size: 18px;"
                  "font:demibold;"
                  "}"
                  "QLabel#Artist{"
                  "color: #5b5b5b;"
                  "font-size: 12px;"
                  "}"
                  "QLabel#Album{"
                  "color: #5b5b5b;"
                  "font-size: 12px;"
                  "}"
                  "QLabel#artistValue{"
                  "color: #5b5b5b;"
                  "font-size: 12px;"
                  "}"
                  "QLabel#albumValue{"
                  "color: #5b5b5b;"
                  "font-size: 12px;"
                  "}");
}

void MusicMessageView::updateElidedText()
{
    QFont font;
    font.setPixelSize(16);
    QFontMetrics fm(font);
    titleLabel->setText(fm.elidedText(fileTitle, Qt::ElideRight, width() - imgLabel->width() - 40 - viewMargins));

    font.setPixelSize(12);
    fm = QFontMetrics(font);
    artistValue->setText(fm.elidedText(fileArtist, Qt::ElideRight, width() - imgLabel->width() - 40 - viewMargins));
    albumValue->setText(fm.elidedText(fileAlbum, Qt::ElideRight, width() - imgLabel->width() - 40 - viewMargins));
}

void MusicMessageView::mediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::BufferedMedia || status == QMediaPlayer::LoadedMedia) {
        MediaMeta meta = tagOpenMusicFile(currentUrl);
        fileTitle = meta.title;
        if (fileTitle.isEmpty()) {
            QFileInfo file(currentUrl);
            QString fileName = file.baseName();
            fileTitle = fileName;
        }
        fileArtist = meta.artist;
        if (fileArtist.isEmpty())
            fileArtist = QString(tr("unknown artist"));

        fileAlbum = meta.album;
        if (fileAlbum.isEmpty())
            fileAlbum = QString(tr("unknown album"));

        QImage img = mediaPlayer->metaData(QMediaMetaData::CoverArtImage).value<QImage>();
        if (img.isNull()) {
            img = QImage(":/icons/icons/default_music_cover.png");
        }
        imgLabel->setPixmap(QPixmap::fromImage(img).scaled(imgLabel->size(), Qt::KeepAspectRatio));

        mediaPlayer->deleteLater();

        updateElidedText();
    }
}

void MusicMessageView::resizeEvent(QResizeEvent *event)
{
    QFrame::resizeEvent(event);
    viewMargins = (event->size().height() - imgLabel->height()) / 2;
    if ((event->size().width() - viewMargins - 250) < imgLabel->width()) {
        viewMargins = event->size().width() - 250 - imgLabel->width();
    }
    setContentsMargins(viewMargins, viewMargins, 0, viewMargins);
    updateElidedText();
}

QList<QByteArray> MusicMessageView::detectEncodings(const QByteArray &rawData)
{
    QList<QByteArray> charsets;
    QByteArray charset = QTextCodec::codecForLocale()->name();
    charsets << charset;

    const char *data = rawData.data();
    int32_t len = rawData.size();

    UCharsetDetector *csd;
    const UCharsetMatch **csm;
    int32_t matchCount = 0;

    UErrorCode status = U_ZERO_ERROR;

    csd = ucsdet_open(&status);
    if (status != U_ZERO_ERROR) {
        ucsdet_close(csd);
        return charsets;
    }

    ucsdet_setText(csd, data, len, &status);
    if (status != U_ZERO_ERROR) {
        ucsdet_close(csd);
        return charsets;
    }

    csm = ucsdet_detectAll(csd, &matchCount, &status);
    if (status != U_ZERO_ERROR) {
        ucsdet_close(csd);
        return charsets;
    }

    if (matchCount > 0) {
        charsets.clear();
    }

    for (int32_t match = 0; match < matchCount; match += 1) {
        const char *name = ucsdet_getName(csm[match], &status);
        const char *lang = ucsdet_getLanguage(csm[match], &status);
        if (lang == nullptr || strlen(lang) == 0) {
            lang = "**";
        }
        charsets << name;
    }

    ucsdet_close(csd);
    return charsets;
}

bool MusicMessageView::isChinese(const QChar &c)
{
    return c.unicode() <= 0x9FBF && c.unicode() >= 0x4E00;
}

MediaMeta MusicMessageView::tagOpenMusicFile(const QString &path)
{
    QUrl url(path);
    TagLib::FileRef f(url.toLocalFile().toLocal8Bit());

    TagLib::Tag *tag = f.tag();

    if (!f.file()) {
        qCritical() << "TagLib: open file failed:" << path << f.file();
    }

    if (!tag) {
        qWarning() << "TagLib: no tag for media file" << path;
        return MediaMeta();
    }

    MediaMeta meta;
    characterEncodingTransform(meta, static_cast<void *>(tag));
    return meta;
}

void MusicMessageView::characterEncodingTransform(MediaMeta &meta, void *obj)
{
    TagLib::Tag *tag = static_cast<TagLib::Tag *>(obj);
    bool encode = true;
    encode &= tag->title().isNull() ? true : tag->title().isLatin1();
    encode &= tag->artist().isNull() ? true : tag->artist().isLatin1();
    encode &= tag->album().isNull() ? true : tag->album().isLatin1();

    QByteArray detectByte;
    QByteArray detectCodec;
    if (encode) {
        if (detectCodec.isEmpty()) {
            detectByte += tag->title().toCString();
            detectByte += tag->artist().toCString();
            detectByte += tag->album().toCString();
            auto allDetectCodecs = detectEncodings(detectByte);
            auto localeCode = localeCodes.value(QLocale::system().name());

            auto iter = std::find_if(allDetectCodecs.begin(), allDetectCodecs.end(),
                                     [localeCode](const QByteArray &curDetext) {
                                         return (curDetext == "Big5" || curDetext == localeCode);
                                     });

            if (iter != allDetectCodecs.end()) {
                detectCodec = *iter;
            }

            if (detectCodec.isEmpty())
                detectCodec = allDetectCodecs.value(0);

            QString curStr = QString::fromLocal8Bit(tag->title().toCString());
            if (curStr.isEmpty())
                curStr = QString::fromLocal8Bit(tag->artist().toCString());
            if (curStr.isEmpty())
                curStr = QString::fromLocal8Bit(tag->album().toCString());

            auto ret = std::any_of(curStr.begin(), curStr.end(), [this](const QChar &ch) {
                return isChinese(ch);
            });

            if (ret) {
                detectCodec = "GB18030";
            }
        }

        QString detectCodecStr(detectCodec);
        if (detectCodecStr.compare("utf-8", Qt::CaseInsensitive) == 0) {
            meta.album = TStringToQString(tag->album());
            meta.artist = TStringToQString(tag->artist());
            meta.title = TStringToQString(tag->title());
            meta.codec = "UTF-8";   //info codec
        } else {
            QTextCodec *codec = QTextCodec::codecForName(detectCodec);
            if (codec == nullptr) {
                meta.album = TStringToQString(tag->album());
                meta.artist = TStringToQString(tag->artist());
                meta.title = TStringToQString(tag->title());
            } else {
                meta.album = codec->toUnicode(tag->album().toCString());
                meta.artist = codec->toUnicode(tag->artist().toCString());
                meta.title = codec->toUnicode(tag->title().toCString());
            }
            meta.codec = detectCodec;
        }
    } else {
        meta.album = TStringToQString(tag->album());
        meta.artist = TStringToQString(tag->artist());
        meta.title = TStringToQString(tag->title());
        meta.codec = "UTF-8";
    }

    //empty str
    meta.album = meta.album.simplified();
    meta.artist = meta.artist.simplified();
    meta.title = meta.title.simplified();
}
