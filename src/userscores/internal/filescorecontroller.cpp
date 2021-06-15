/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore BVBA and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "filescorecontroller.h"

#include <QObject>
#include <QBuffer>

#include "log.h"
#include "translation.h"

#include "userscoresconfiguration.h"

using namespace mu;
using namespace mu::userscores;
using namespace mu::notation;
using namespace mu::framework;
using namespace mu::actions;

void FileScoreController::init()
{
    dispatcher()->reg(this, "file-open", this, &FileScoreController::openScore);
    dispatcher()->reg(this, "file-import", this, &FileScoreController::importScore);
    dispatcher()->reg(this, "file-new", this, &FileScoreController::newScore);
    dispatcher()->reg(this, "file-close", this, &FileScoreController::closeScore);

    dispatcher()->reg(this, "file-save", this, &FileScoreController::saveScore);
    dispatcher()->reg(this, "file-save-as", this, &FileScoreController::saveScoreAs);
    dispatcher()->reg(this, "file-save-a-copy", this, &FileScoreController::saveScoreCopy);
    dispatcher()->reg(this, "file-save-selection", this, &FileScoreController::saveSelection);
    dispatcher()->reg(this, "file-save-online", this, &FileScoreController::saveOnline);

    dispatcher()->reg(this, "file-export", this, &FileScoreController::exportScore);

    dispatcher()->reg(this, "file-import-pdf", this, &FileScoreController::importPdf);

    dispatcher()->reg(this, "clear-recent", this, &FileScoreController::clearRecentScores);

    dispatcher()->reg(this, "continue-last-session", this, &FileScoreController::continueLastSession);
}

IMasterNotationPtr FileScoreController::currentMasterNotation() const
{
    return globalContext()->currentMasterNotation();
}

INotationPtr FileScoreController::currentNotation() const
{
    return currentMasterNotation() ? currentMasterNotation()->notation() : nullptr;
}

INotationInteractionPtr FileScoreController::currentInteraction() const
{
    return currentNotation() ? currentNotation()->interaction() : nullptr;
}

INotationSelectionPtr FileScoreController::currentNotationSelection() const
{
    return currentNotation() ? currentInteraction()->selection() : nullptr;
}

Ret FileScoreController::openScore(const io::path& scorePath)
{
    return doOpenScore(scorePath);
}

bool FileScoreController::isScoreOpened(const io::path& scorePath) const
{
    auto notation = globalContext()->currentMasterNotation();
    if (!notation) {
        return false;
    }

    LOGD() << "notation->path: " << notation->path() << ", check path: " << scorePath;
    if (notation->path() == scorePath) {
        return true;
    }

    return false;
}

void FileScoreController::openScore(const actions::ActionData& args)
{
    io::path scorePath = args.count() > 0 ? args.arg<io::path>(0) : "";

    if (scorePath.empty()) {
        QStringList filter;
        filter << QObject::tr("MuseScore Files") + " (*.mscz *.mscx)";
        scorePath = selectScoreOpenningFile(filter);
        if (scorePath.empty()) {
            return;
        }
    }

    doOpenScore(scorePath);
}

void FileScoreController::importScore()
{
    QString allExt = "*.mscz *.mscx *.mxl *.musicxml *.xml *.mid *.midi *.kar *.md *.mgu *.sgu *.cap *.capx"
                     "*.ove *.scw *.bmw *.bww *.gtp *.gp3 *.gp4 *.gp5 *.gpx *.gp *.ptb *.mscz, *.mscx,";

    QStringList filter;
    filter << QObject::tr("All Supported Files") + " (" + allExt + ")"
           << QObject::tr("MuseScore Files") + " (*.mscz *.mscx)"
           << QObject::tr("MusicXML Files") + " (*.mxl *.musicxml *.xml)"
           << QObject::tr("MIDI Files") + " (*.mid *.midi *.kar)"
           << QObject::tr("MuseData Files") + " (*.md)"
           << QObject::tr("Capella Files") + " (*.cap *.capx)"
           << QObject::tr("BB Files (experimental)") + " (*.mgu *.sgu)"
           << QObject::tr("Overture / Score Writer Files (experimental)") + " (*.ove *.scw)"
           << QObject::tr("Bagpipe Music Writer Files (experimental)") + " (*.bmw *.bww)"
           << QObject::tr("Guitar Pro Files") + " (*.gtp *.gp3 *.gp4 *.gp5 *.gpx *.gp)"
           << QObject::tr("Power Tab Editor Files (experimental)") + " (*.ptb)"
           << QObject::tr("MuseScore Backup Files") + " (*.mscz, *.mscx,)";

    io::path scorePath = selectScoreOpenningFile(filter);

    if (scorePath.empty()) {
        return;
    }

    doOpenScore(scorePath);
}

void FileScoreController::newScore()
{
    Ret ret = interactive()->open("musescore://userscores/newscore").ret;

    if (ret) {
        ret = interactive()->open("musescore://notation").ret;
    }

    if (!ret) {
        LOGE() << ret.toString();
    }
}

void FileScoreController::closeScore()
{
    globalContext()->setCurrentMasterNotation(nullptr);
}

void FileScoreController::saveScore()
{
    if (!currentMasterNotation()->created().val) {
        doSaveScore();
        return;
    }

    io::path defaultFilePath = defaultSavingFilePath();

    io::path filePath = selectScoreSavingFile(defaultFilePath, qtrc("userscores", "Save Score"));
    if (filePath.empty()) {
        return;
    }

    if (io::syffix(filePath).empty()) {
        filePath = filePath + UserScoresConfiguration::DEFAULT_FILE_SUFFIX;
    }

    doSaveScore(filePath);
}

void FileScoreController::saveScoreAs()
{
    io::path defaultFilePath = defaultSavingFilePath();
    io::path selectedFilePath = selectScoreSavingFile(defaultFilePath, qtrc("userscores", "Save Score"));
    if (selectedFilePath.empty()) {
        return;
    }

    doSaveScore(selectedFilePath, SaveMode::SaveAs);
}

void FileScoreController::saveScoreCopy()
{
    io::path defaultFilePath = defaultSavingFilePath();
    io::path selectedFilePath = selectScoreSavingFile(defaultFilePath, qtrc("userscores", "Save a Copy"));
    if (selectedFilePath.empty()) {
        return;
    }

    doSaveScore(selectedFilePath, SaveMode::SaveCopy);
}

void FileScoreController::saveSelection()
{
    io::path defaultFilePath = defaultSavingFilePath();
    io::path selectedFilePath = selectScoreSavingFile(defaultFilePath, qtrc("userscores", "Save Selection"));
    if (selectedFilePath.empty()) {
        return;
    }

    Ret ret = currentMasterNotation()->save(selectedFilePath, SaveMode::SaveSelection);
    if (!ret) {
        LOGE() << ret.toString();
    }
}

void FileScoreController::saveOnline()
{
    IMasterNotationPtr master = globalContext()->currentMasterNotation();
    if (!master) {
        return;
    }

    /*
    QBuffer* scoreData = new QBuffer();
    scoreData->open(QIODevice::ReadWrite);

    Ret ret = master->writeToDevice(*scoreData);

    if (!ret) {
        LOGE() << ret.toString();
        return;
    }*/

    Meta meta = master->metaInfo();

    QFile* file = new QFile(meta.filePath.toQString());
    file->open(QIODevice::ReadOnly);

    ProgressChannel progressCh = uploadingService()->progressChannel();
    progressCh.onReceive(this, [](const Progress& progress) {
        LOGD() << "Uploading progress: " << progress.current << "/" << progress.total;
    });

    uploadingService()->uploadScore(*file, meta.title, meta.source);
}

void FileScoreController::importPdf()
{
    interactive()->openUrl("https://musescore.com/import");
}

void FileScoreController::clearRecentScores()
{
    configuration()->setRecentScorePaths({});
    platformRecentFilesController()->clearRecentFiles();
}

void FileScoreController::continueLastSession()
{
    io::paths recentScorePaths = configuration()->recentScorePaths().val;

    if (recentScorePaths.empty()) {
        return;
    }

    io::path lastScorePath = recentScorePaths.front();
    openScore(lastScorePath);
}

void FileScoreController::exportScore()
{
    interactive()->open("musescore://userscores/export");
}

io::path FileScoreController::selectScoreOpenningFile(const QStringList& filter)
{
    QString filterStr = filter.join(";;");
    return interactive()->selectOpeningFile(qtrc("userscores", "Score"), "", filterStr);
}

io::path FileScoreController::selectScoreSavingFile(const io::path& defaultFilePath, const QString& saveTitle)
{
    QString filter = QObject::tr("MuseScore Files") + " (*.mscz *.mscx)";
    io::path filePath = interactive()->selectSavingFile(saveTitle, defaultFilePath, filter);

    return filePath;
}

Ret FileScoreController::doOpenScore(const io::path& filePath)
{
    TRACEFUNC;

    if (multiInstancesProvider()->isScoreAlreadyOpened(filePath)) {
        multiInstancesProvider()->activateWindowWithScore(filePath);
        return make_ret(Ret::Code::Ok);
    }

    auto notation = notationCreator()->newMasterNotation();
    IF_ASSERT_FAILED(notation) {
        return make_ret(Ret::Code::InternalError);
    }

    Ret ret = notation->load(filePath);
    if (!ret) {
        LOGE() << "failed load: " << filePath << ", ret: " << ret.toString();
        //! TODO Show dialog about error
        return make_ret(Ret::Code::InternalError);
    }

    if (!globalContext()->containsMasterNotation(filePath)) {
        globalContext()->addMasterNotation(notation);
    }

    globalContext()->setCurrentMasterNotation(notation);

    prependToRecentScoreList(filePath);

    interactive()->open("musescore://notation");

    return make_ret(Ret::Code::Ok);
}

void FileScoreController::doSaveScore(const io::path& filePath, SaveMode saveMode)
{
    io::path oldPath = currentMasterNotation()->metaInfo().filePath;

    Ret ret = currentMasterNotation()->save(filePath, saveMode);
    if (!ret) {
        LOGE() << ret.toString();
        return;
    }

    if (saveMode == SaveMode::SaveAs && oldPath != filePath) {
        globalContext()->currentMasterNotationChanged().notify();
    }

    prependToRecentScoreList(filePath);
}

io::path FileScoreController::defaultSavingFilePath() const
{
    Meta scoreMetaInfo = currentMasterNotation()->metaInfo();

    io::path fileName = scoreMetaInfo.title;
    if (fileName.empty()) {
        fileName = scoreMetaInfo.fileName;
    }

    return configuration()->defaultSavingFilePath(fileName);
}

void FileScoreController::prependToRecentScoreList(const io::path& filePath)
{
    if (filePath.empty()) {
        return;
    }

    io::paths recentScorePaths = configuration()->recentScorePaths().val;

    auto it = std::find(recentScorePaths.begin(), recentScorePaths.end(), filePath);
    if (it != recentScorePaths.end()) {
        recentScorePaths.erase(it);
    }

    recentScorePaths.insert(recentScorePaths.begin(), filePath);
    configuration()->setRecentScorePaths(recentScorePaths);
    platformRecentFilesController()->addRecentFile(filePath);
}

bool FileScoreController::isScoreOpened() const
{
    return currentMasterNotation() != nullptr;
}

bool FileScoreController::isNeedSaveScore() const
{
    return currentMasterNotation() && currentMasterNotation()->needSave().val;
}

bool FileScoreController::hasSelection() const
{
    return currentNotationSelection() ? !currentNotationSelection()->isNone() : false;
}
