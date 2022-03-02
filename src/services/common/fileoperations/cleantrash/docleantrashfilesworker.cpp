/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     liyigang<liyigang@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             lanxuesong<lanxuesong@uniontech.com>
 *             xushitong<xushitong@uniontech.com>
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
#include "docleantrashfilesworker.h"
#include "dfm-base/base/schemefactory.h"
#include "dfm-base/base/standardpaths.h"

#include "dfm-io/core/denumerator.h"
#include "dfm-io/core/diofactory.h"

#include <QUrl>
#include <QDebug>
#include <QWaitCondition>

DFMBASE_USE_NAMESPACE
DSC_USE_NAMESPACE
DoCleanTrashFilesWorker::DoCleanTrashFilesWorker(QObject *parent)
    : AbstractWorker(parent)
{
    jobType = AbstractJobHandler::JobType::kCleanTrashType;
}

DoCleanTrashFilesWorker::~DoCleanTrashFilesWorker()
{
    stop();
}

bool DoCleanTrashFilesWorker::doWork()
{
    // The endcopy interface function has been called here
    if (!AbstractWorker::doWork())
        return false;
    // ToDo::执行删除的业务逻辑
    cleanAllTrashFiles();
    // 完成
    endWork();

    return true;
}

void DoCleanTrashFilesWorker::onUpdateProccess()
{
    emitProccessChangedNotify(cleanTrashFilesCount);
}

bool DoCleanTrashFilesWorker::statisticsFilesSize()
{
    if (sourceUrls.count() == 0) {
        qWarning() << "sources files list is empty!";
        return false;
    }

    QString path = sourceUrls.first().path();
    if (path.endsWith("/"))
        path.chop(1);

    if (sourceUrls.count() == 1 && path == StandardPaths::location(StandardPaths::kTrashFilesPath))
        FileOperationsUtils::getDirFiles(sourceUrls.first(), allFilesList);

    return true;
}

bool DoCleanTrashFilesWorker::initArgs()
{
    AbstractWorker::initArgs();
    trashInfoPath = StandardPaths::location(StandardPaths::kTrashInfosPath);
    trashInfoPath = trashInfoPath.endsWith("/") ? trashInfoPath : trashInfoPath + "/";
    trashFilePath = StandardPaths::location(StandardPaths::kTrashFilesPath);
    return true;
}

void DoCleanTrashFilesWorker::doOperateWork(AbstractJobHandler::SupportActions actions)
{
    AbstractWorker::doOperateWork(actions);
    resume();
}

AbstractJobHandler::SupportActions DoCleanTrashFilesWorker::supportActions(const AbstractJobHandler::JobErrorType &error)
{
    return AbstractWorker::supportActions(error);
}
/*!
 * \brief DoCleanTrashFilesWorker::deleteAllFiles delete All files
 * \return delete all files success
 */
bool DoCleanTrashFilesWorker::cleanAllTrashFiles()
{
    QList<QUrl>::iterator it = sourceUrls.begin();
    QList<QUrl>::iterator itend = sourceUrls.end();
    if (!allFilesList.isEmpty()) {
        it = allFilesList.begin();
        itend = allFilesList.end();
    }
    while (it != itend) {
        if (!stateCheck())
            return false;
        const QUrl &url = *it;
        emitCurrentTaskNotify(url, QUrl());

        if (!url.path().startsWith(trashFilePath)) {
            // pause and emit error msg
            AbstractJobHandler::SupportAction action = doHandleErrorAndWait(url, AbstractJobHandler::JobErrorType::kIsNotTrashFileError);
            if (AbstractJobHandler::SupportAction::kSkipAction != action) {
                return false;
            } else {
                continue;
            }
        }

        const auto &fileInfo = InfoFactory::create<AbstractFileInfo>(url);
        if (!fileInfo) {
            // pause and emit error msg
            AbstractJobHandler::SupportAction action = doHandleErrorAndWait(url, AbstractJobHandler::JobErrorType::kProrogramError);
            if (AbstractJobHandler::SupportAction::kSkipAction != action) {
                return false;
            } else {
                continue;
            }
        }
        if (!clearTrashFile(fileInfo))
            return false;
        cleanTrashFilesCount++;
        ++it;
    }
    return true;
}
/*!
 * \brief DoCleanTrashFilesWorker::clearTrashFile
 * \param fromUrl URL of the source file
 * \param toUrl Destination URL
 * \param trashInfo File information in Recycle Bin
 * \return Is the execution successful
 */
bool DoCleanTrashFilesWorker::clearTrashFile(const AbstractFileInfoPointer &trashInfo)
{
    AbstractJobHandler::SupportAction action = AbstractJobHandler::SupportAction::kNoAction;
    QString location(trashInfoPath + trashInfo->fileName() + ".trashinfo");
    bool resultFile = false;
    bool resultInfo = false;
    do {
        if (!resultFile) {
            if (trashInfo->isFile() || trashInfo->isSymLink()) {
                resultFile = handler->deleteFile(trashInfo->url());
            } else {
                // dir
                resultFile = deleteDir(trashInfo->url());
            }
        }
        if (!resultInfo)
            resultInfo = handler->deleteFile(QUrl::fromLocalFile(location));
        if (!resultInfo || !resultFile)
            action = doHandleErrorAndWait(trashInfo->url(),
                                          AbstractJobHandler::JobErrorType::kDeleteTrashFileError, handler->errorString());
    } while (isStopped() && action == AbstractJobHandler::SupportAction::kRetryAction);

    return action == AbstractJobHandler::SupportAction::kNoAction || AbstractJobHandler::SupportAction::kSkipAction == action;
}
/*!
 * \brief DoCleanTrashFilesWorker::doHandleErrorAndWait Blocking handles errors and returns
 * actions supported by the operation
 * \param from source information
 * \param to target information
 * \param error error type
 * \param needRetry is neef retry action
 * \param errorMsg error message
 * \return support action
 */
AbstractJobHandler::SupportAction DoCleanTrashFilesWorker::doHandleErrorAndWait(const QUrl &from, const AbstractJobHandler::JobErrorType &error, const QString &errorMsg)
{
    setStat(AbstractJobHandler::JobState::kPauseState);
    emitErrorNotify(from, QUrl(), error, errorMsg);

    handlingErrorQMutex.lock();
    handlingErrorCondition.wait(&handlingErrorQMutex);
    handlingErrorQMutex.unlock();

    return currentAction;
}

bool DoCleanTrashFilesWorker::deleteDir(const QUrl &url) const
{
    QSharedPointer<dfmio::DIOFactory> factory = produceQSharedIOFactory(url.scheme(), static_cast<QUrl>(url));
    if (!factory) {
        return false;
    }

    QSharedPointer<dfmio::DEnumerator> enumerator = factory->createEnumerator();
    if (!enumerator) {
        return false;
    }

    bool succ = false;
    while (enumerator->hasNext()) {
        const QString &path = enumerator->next();

        const QUrl &urlNext = QUrl::fromLocalFile(path);
        AbstractFileInfoPointer info = InfoFactory::create<AbstractFileInfo>(urlNext);
        if (info->isDir()) {
            succ = deleteDir(urlNext);
        } else {
            succ = handler->deleteFile(urlNext);
        }
    }
    succ = handler->deleteFile(url);
    return succ;
}
