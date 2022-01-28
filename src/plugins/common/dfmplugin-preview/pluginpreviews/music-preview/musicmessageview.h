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

#ifndef MUSICMESSAGEVIEW_H
#define MUSICMESSAGEVIEW_H

#include "preview_plugin_global.h"

#include <QFrame>
#include <QMediaPlayer>
class QLabel;

PREVIEW_BEGIN_NAMESPACE
/**
 * @brief MediaMeta 音频文件信息结构体
 */
struct MediaMeta
{
    QString title;
    QString artist;
    QString album;
    QString codec;
};

class MusicMessageView : public QFrame
{
    Q_OBJECT
public:
    explicit MusicMessageView(const QString &uri = "", QWidget *parent = nullptr);
    void initUI();
    void updateElidedText();

signals:

public slots:
    void mediaStatusChanged(QMediaPlayer::MediaStatus status);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    /**
     * @brief detectEncodings 检测字符串字符编码
     * @param rawData         需要检测的字符串
     * @return                返回检测到的字符编码列表
     */
    QList<QByteArray> detectEncodings(const QByteArray &rawData);

    /**
     * @brief isChinese     检测字符是否是中文
     * @param c             需要检测的字符
     * @return              true 为中文字符，反之亦然
     */
    bool isChinese(const QChar &c);

    /**
     * @brief tagOpenMusicFile  使用taglib打开音频文件获取信息
     * @param path              音频文件路径
     * @return                  音频文件信息
     */
    MediaMeta tagOpenMusicFile(const QString &path);

    /**
     * @brief characterEncodingTransform    对音频文件信息字符数据进行转码
     * @param meta                          音频文件信息对象,输出参数
     * @param obj                           taglib打开的文件对象
     */
    void characterEncodingTransform(MediaMeta &meta, void *obj);

private:
    QString currentUrl;
    QLabel *titleLabel { nullptr };
    QLabel *artistLabel { nullptr };
    QLabel *albumLabel { nullptr };
    QLabel *imgLabel { nullptr };
    QLabel *artistValue { nullptr };
    QLabel *albumValue { nullptr };

    QMediaPlayer *mediaPlayer { nullptr };

    QString fileTitle;
    QString fileArtist;
    QString fileAlbum;
    int viewMargins;
    QMap<QString, QByteArray> localeCodes;   //!  区域与编码
};
PREVIEW_END_NAMESPACE
#endif   // MUSICMESSAGEVIEW_H
