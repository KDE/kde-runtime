/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2009 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (C) 2009-2011 Raphael Kubo da Costa <kubito@gmail.com>
 * Copyright (C) 2012 basysKom GmbH <info@basyskom.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES ( INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION ) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * ( INCLUDING NEGLIGENCE OR OTHERWISE ) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "cliinterface.h"
#include "queries.h"

#ifdef Q_OS_LINUX
# include <KProcess>
#else
# include <KPtyDevice>
# include <KPtyProcess>
#endif

#include <KStandardDirs>
#include <KDebug>
#include <KLocale>

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QProcess>
#include <QThread>
#include <QTimer>
#include <QWaitCondition>

namespace Kerfuffle
{
CliInterface::CliInterface(QObject *parent, const QVariantList & args)
        : ReadWriteArchiveInterface(parent, args),
        m_process(0),
        m_alreadyFailed(false)
{
    if (QMetaType::type("QProcess::ExitStatus") == 0) {
        qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
    }
}

void CliInterface::cacheParameterList()
{
    m_param = parameterList();
    Q_ASSERT(m_param.contains(ExtractProgram));
    Q_ASSERT(m_param.contains(ListProgram));
    Q_ASSERT(m_param.contains(PreservePathSwitch));
    Q_ASSERT(m_param.contains(FileExistsExpression));
    Q_ASSERT(m_param.contains(FileExistsInput));
}

CliInterface::~CliInterface()
{
    Q_ASSERT(!m_process);
}

bool CliInterface::list()
{
    cacheParameterList();
    m_operationMode = List;

    QStringList args = m_param.value(ListArgs).toStringList();
    substituteListVariables(args);

    if (!runProcess(m_param.value(ListProgram).toString(), args)) {
        failOperation();
        return false;
    }

    return true;
}

bool CliInterface::copyFiles(const QList<QVariant> & files, const QString & destinationDirectory, ExtractionOptions options)
{
    kDebug(7109);
    cacheParameterList();

    m_operationMode = Copy;

    //start preparing the argument list
    QStringList args = m_param.value(ExtractArgs).toStringList();

    //now replace the various elements in the list
    for (int i = 0; i < args.size(); ++i) {
        QString argument = args.at(i);
        kDebug(7109) << "Processing argument " << argument;

        if (argument == QLatin1String( "$Archive" )) {
            args[i] = filename();
        }

        if (argument == QLatin1String( "$PreservePathSwitch" )) {
            QStringList replacementFlags = m_param.value(PreservePathSwitch).toStringList();
            Q_ASSERT(replacementFlags.size() == 2);

            bool preservePaths = options.value(QLatin1String( "PreservePaths" )).toBool();
            QString theReplacement;
            if (preservePaths) {
                theReplacement = replacementFlags.at(0);
            } else {
                theReplacement = replacementFlags.at(1);
            }

            if (theReplacement.isEmpty()) {
                args.removeAt(i);
                --i; //decrement to compensate for the variable we removed
            } else {
                //but in this case we don't have to decrement, we just
                //replace it
                args[i] = theReplacement;
            }
        }

        if (argument == QLatin1String( "$PasswordSwitch" )) {
            //if the PasswordSwitch argument has been added, we at least
            //assume that the format of the switch has been added as well
            Q_ASSERT(m_param.contains(PasswordSwitch));

            //we will decrement i afterwards
            args.removeAt(i);

            //if we get a hint about this being a password protected archive, ask about
            //the password in advance.
            if ((options.value(QLatin1String("PasswordProtectedHint")).toBool()) &&
                (password().isEmpty())) {
                kDebug(7109) << "Password hint enabled, querying user";

                Kerfuffle::PasswordNeededQuery query(filename());
                userQuery(&query);
                query.waitForResponse();

                if (query.responseCancelled()) {
                    failOperation();
                    return false;
                }
                setPassword(query.password());
            }

            QString pass = password();

            if (!pass.isEmpty()) {
                QStringList theSwitch = m_param.value(PasswordSwitch).toStringList();
                for (int j = 0; j < theSwitch.size(); ++j) {
                    //get the argument part
                    QString newArg = theSwitch.at(j);

                    //substitute the $Path
                    newArg.replace(QLatin1String( "$Password" ), pass);

                    //put it in the arg list
                    args.insert(i + j, newArg);
                    ++i;

                }
            }
            --i; //decrement to compensate for the variable we replaced
        }

        if (argument == QLatin1String( "$RootNodeSwitch" )) {
            //if the RootNodeSwitch argument has been added, we at least
            //assume that the format of the switch has been added as well
            Q_ASSERT(m_param.contains(RootNodeSwitch));

            //we will decrement i afterwards
            args.removeAt(i);

            QString rootNode;
            if (options.contains(QLatin1String( "RootNode" ))) {
                rootNode = options.value(QLatin1String( "RootNode" )).toString();
                kDebug(7109) << "Set root node " << rootNode;
            }

            if (!rootNode.isEmpty()) {
                QStringList theSwitch = m_param.value(RootNodeSwitch).toStringList();
                for (int j = 0; j < theSwitch.size(); ++j) {
                    //get the argument part
                    QString newArg = theSwitch.at(j);

                    //substitute the $Path
                    newArg.replace(QLatin1String( "$Path" ), rootNode);

                    //put it in the arg list
                    args.insert(i + j, newArg);
                    ++i;

                }
            }
            --i; //decrement to compensate for the variable we replaced
        }

        if (argument == QLatin1String( "$Files" )) {
            args.removeAt(i);
            for (int j = 0; j < files.count(); ++j) {
                args.insert(i + j, escapeFileName(files.at(j).toString()));
                ++i;
            }
            --i;
        }
    }

    kDebug(7109) << "Setting current dir to " << destinationDirectory;
    QDir::setCurrent(destinationDirectory);

    if (!runProcess(m_param.value(ExtractProgram).toString(), args)) {
        failOperation();
        return false;
    }

    return true;
}

bool CliInterface::addFiles(const QStringList & files, const CompressionOptions& options)
{
    cacheParameterList();

    m_operationMode = Add;

    const QString globalWorkDir = options.value(QLatin1String( "GlobalWorkDir" )).toString();
    const QDir workDir = globalWorkDir.isEmpty() ? QDir::current() : QDir(globalWorkDir);
    if (!globalWorkDir.isEmpty()) {
        kDebug(7109) << "GlobalWorkDir is set, changing dir to " << globalWorkDir;
        QDir::setCurrent(globalWorkDir);
    }

    //start preparing the argument list
    QStringList args = m_param.value(AddArgs).toStringList();

    //now replace the various elements in the list
    for (int i = 0; i < args.size(); ++i) {
        const QString argument = args.at(i);
        kDebug(7109) << "Processing argument " << argument;

        if (argument == QLatin1String( "$Archive" )) {
            args[i] = filename();
        }

        if (argument == QLatin1String( "$Files" )) {
            args.removeAt(i);
            for (int j = 0; j < files.count(); ++j) {
                // #191821: workDir must be used instead of QDir::current()
                //          so that symlinks aren't resolved automatically
                // TODO: this kind of call should be moved upwards in the
                //       class hierarchy to avoid code duplication
                const QString relativeName =
                    workDir.relativeFilePath(files.at(j));

                args.insert(i + j, relativeName);
                ++i;
            }
            --i;
        }
    }

    if (!runProcess(m_param.value(AddProgram).toString(), args)) {
        failOperation();
        return false;
    }

    return true;
}

bool CliInterface::deleteFiles(const QList<QVariant> & files)
{
    cacheParameterList();
    m_operationMode = Delete;

    //start preparing the argument list
    QStringList args = m_param.value(DeleteArgs).toStringList();

    //now replace the various elements in the list
    for (int i = 0; i < args.size(); ++i) {
        QString argument = args.at(i);
        kDebug(7109) << "Processing argument " << argument;

        if (argument == QLatin1String( "$Archive" )) {
            args[i] = filename();
        } else if (argument == QLatin1String( "$Files" )) {
            args.removeAt(i);
            for (int j = 0; j < files.count(); ++j) {
                args.insert(i + j, escapeFileName(files.at(j).toString()));
                ++i;
            }
            --i;
        }
    }

    m_removedFiles = files;

    if (!runProcess(m_param.value(DeleteProgram).toString(), args)) {
        failOperation();
        return false;
    }

    return true;
}

bool CliInterface::runProcess(const QString& programName, const QStringList& arguments)
{
    const QString programPath(KStandardDirs::findExe(programName));
    if (programPath.isEmpty()) {
        error(i18nc("@info", "Failed to locate program <filename>%1</filename> in PATH.", programName));
        return false;
    }

    kDebug(7109) << "Executing" << programPath << arguments;

    if (m_process) {
        m_process->waitForFinished();
        delete m_process;
    }

#ifdef Q_OS_LINUX
    m_process = new KProcess();
#else
    m_process = new KPtyProcess();
    m_process->setPtyChannels(KPtyProcess::StdinChannel);
#endif

    m_process->setOutputChannelMode(KProcess::MergedChannels);
    m_process->setNextOpenMode(QIODevice::ReadWrite | QIODevice::Unbuffered | QIODevice::Text);
    m_process->setProgram(programPath, arguments);

    connect(m_process, SIGNAL(readyReadStandardOutput()), SLOT(readStdout()), Qt::DirectConnection);
    connect(m_process, SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(processFinished(int,QProcess::ExitStatus)), Qt::DirectConnection);

    m_stdOutData.clear();

    m_alreadyFailed = false;
    m_process->start();

    if (!m_process->waitForStarted()) {
        m_process->deleteLater();
        m_process = 0;
        return false;
    }

#ifdef Q_OS_LINUX
    bool ret = m_process->waitForFinished(-1);
#else
    QEventLoop loop;
    bool ret = (loop.exec(QEventLoop::WaitForMoreEvents | QEventLoop::ExcludeUserInputEvents) == 0);
#endif

    // in case a second runProcess() has been called and the 'delete m_process' line above is called.
    if (m_process) {
        kDebug(7109) << "ret" << ret << "exitCode" << m_process->exitCode() << "alreadyFailed" << m_alreadyFailed << QThread::currentThread();
        ret = ret && (m_process->exitCode() == 0) && !m_alreadyFailed;
    
        m_process->deleteLater();
        m_process = 0;
    } else {
        kDebug(7109) << "ret" << ret << "m_process" << m_process << "alreadyFailed" << m_alreadyFailed;
        ret = ret && !m_alreadyFailed;
    } 

    return ret;
}

void CliInterface::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode)
    Q_UNUSED(exitStatus)

    kDebug(7109);

    //if the m_process pointer is gone, then there is nothing to worry
    //about here
    if (!m_process) {
        return;
    }

    if (m_operationMode == Delete) {
        foreach(const QVariant& v, m_removedFiles) {
            entryRemoved(v.toString());
        }
    }

    //handle all the remaining data in the process
    readStdout(true);

    progress(1.0);

    if (m_operationMode == Add) {
        list();
        return;
    }
}

void CliInterface::failOperation()
{
    kDebug(7109);

    if (m_alreadyFailed) {
        kDebug(7109) << "already failed";
        return;
    }
    m_alreadyFailed = true;

    doKill();
}

void CliInterface::readStdout(bool handleAll)
{
    //when hacking this function, please remember the following:
    //- standard output comes in unpredictable chunks, this is why
    //you can never know if the last part of the output is a complete line or not
    //- console applications are not really consistent about what
    //characters they send out (newline, backspace, carriage return,
    //etc), so keep in mind that this function is supposed to handle
    //all those special cases and be the lowest common denominator

    Q_ASSERT(m_process);

    if (!m_process->bytesAvailable()) {
        //if process has no more data, we can just bail out
        return;
    }

    if (m_alreadyFailed) {
        kDebug(7109) << "cleaning buffers";
        m_process->readAllStandardOutput();
        m_stdOutData.clear();
    }

    //if the process is still not finished (m_process is appearantly not
    //set to NULL if here), then the operation should definitely not be in
    //the main thread as this would freeze everything. assert this.
    //Q_ASSERT(QThread::currentThread() != QApplication::instance()->thread());

    
    QByteArray dd = m_process->readAllStandardOutput();
    m_stdOutData += dd;

    QList<QByteArray> lines = m_stdOutData.split('\n');

    //The reason for this check is that archivers often do not end
    //queries (such as file exists, wrong password) on a new line, but
    //freeze waiting for input. So we check for errors on the last line in
    //all cases.
    bool foundErrorMessage =
        (checkForErrorMessage(QLatin1String( lines.last() ), WrongPasswordPatterns) ||
         checkForErrorMessage(QLatin1String( lines.last() ), ExtractionFailedPatterns) ||
         checkForPasswordPromptMessage(QLatin1String(lines.last())) ||
         checkForFileExistsMessage(QLatin1String( lines.last() )));

    if (foundErrorMessage) {
        handleAll = true;
    }

    //this is complex, here's an explanation:
    //if there is no newline, then there is no guaranteed full line to
    //handle in the output. The exception is that it is supposed to handle
    //all the data, OR if there's been an error message found in the
    //partial data.
    if (lines.size() == 1 && !handleAll) {
        return;
    }

    if (handleAll) {
        m_stdOutData.clear();
    } else {
        //because the last line might be incomplete we leave it for now
        //note, this last line may be an empty string if the stdoutdata ends
        //with a newline
        m_stdOutData = lines.takeLast();
    }

    foreach(const QByteArray& line, lines) {
        if (m_alreadyFailed) {
            kDebug(7109) << "Breaking loop because operation has already failed";
            break;
        }

        if (!line.isEmpty()) {
            handleLine(QString::fromLocal8Bit(line));
        }
    }
}

void CliInterface::handleLine(const QString& line)
{
    if ((m_operationMode == Copy || m_operationMode == Add) && m_param.contains(CaptureProgress) && m_param.value(CaptureProgress).toBool()) {
        //read the percentage
        int pos = line.indexOf(QLatin1Char( '%' ));
        if (pos != -1 && pos > 1) {
            int percentage = line.mid(pos - 2, 2).toInt();
            progress(float(percentage) / 100);
            return;
        }
    }

    if (m_operationMode == Copy) {
        if (checkForPasswordPromptMessage(line)) {
            kDebug(7109) << "Found a password prompt";

            Kerfuffle::PasswordNeededQuery query(filename());
            userQuery(&query);
            query.waitForResponse();

            if (query.responseCancelled()) {
                failOperation();
                return;
            }

            setPassword(query.password());

            const QString response(password() + QLatin1Char('\n'));
            writeToProcess(response.toLocal8Bit());

            return;
        }

        if (checkForErrorMessage(line, WrongPasswordPatterns)) {
            kDebug(7109) << "Wrong password!";
            error(i18n("Incorrect password."));
            failOperation();
            return;
        }

        if (checkForErrorMessage(line, ExtractionFailedPatterns)) {
            kDebug(7109) << "Error in extraction!!";
            error(i18n("Extraction failed because of an unexpected error."));
            failOperation();
            return;
        }

        if (handleFileExistsMessage(line)) {
            return;
        }
    }

    if (m_operationMode == List) {
        if (checkForPasswordPromptMessage(line)) {
            kDebug(7109) << "Found a password prompt";

            Kerfuffle::PasswordNeededQuery query(filename());
            userQuery(&query);
            query.waitForResponse();

            if (query.responseCancelled()) {
                failOperation();
                return;
            }

            setPassword(query.password());

            const QString response(password() + QLatin1Char('\n'));
            writeToProcess(response.toLocal8Bit());

            return;
        }

        if (checkForErrorMessage(line, WrongPasswordPatterns)) {
            kDebug(7109) << "Wrong password!";
            error(i18n("Incorrect password."));
            failOperation();
            return;
        }

        if (checkForErrorMessage(line, ExtractionFailedPatterns)) {
            kDebug(7109) << "Error in extraction!!";
            error(i18n("Extraction failed because of an unexpected error."));
            failOperation();
            return;
        }

        if (handleFileExistsMessage(line)) {
            return;
        }

        readListLine(line);
        return;
    }
}

bool CliInterface::checkForPasswordPromptMessage(const QString& line)
{
    const QString passwordPromptPattern(m_param.value(PasswordPromptPattern).toString());

    if (passwordPromptPattern.isEmpty())
        return false;

    if (m_passwordPromptPattern.isEmpty()) {
        m_passwordPromptPattern.setPattern(m_param.value(PasswordPromptPattern).toString());
    }

    if (m_passwordPromptPattern.indexIn(line) != -1) {
        return true;
    }

    return false;
}

bool CliInterface::checkForFileExistsMessage(const QString& line)
{
    if (m_existsPattern.isEmpty()) {
        m_existsPattern.setPattern(m_param.value(FileExistsExpression).toString());
    }
    if (m_existsPattern.indexIn(line) != -1) {
        kDebug(7109) << "Detected file existing!! Filename " << m_existsPattern.cap(1);
        return true;
    }

    return false;
}

bool CliInterface::handleFileExistsMessage(const QString& line)
{
    if (!checkForFileExistsMessage(line)) {
        return false;
    }

    const QString filename = m_existsPattern.cap(1);

    Kerfuffle::OverwriteQuery query(QDir::current().path() + QLatin1Char( '/' ) + filename);
    query.setNoRenameMode(true);
    userQuery(&query);
    kDebug(7109) << "Waiting response";
    query.waitForResponse();

    kDebug(7109) << "Finished response";

    QString responseToProcess;
    const QStringList choices = m_param.value(FileExistsInput).toStringList();

    if (query.responseOverwrite()) {
        responseToProcess = choices.at(0);
    } else if (query.responseSkip()) {
        responseToProcess = choices.at(1);
    } else if (query.responseOverwriteAll()) {
        responseToProcess = choices.at(2);
    } else if (query.responseAutoSkip()) {
        responseToProcess = choices.at(3);
    } else if (query.responseCancelled()) {
        if (choices.count() < 5) { // If the program has no way to cancel the extraction, we resort to killing it
            return doKill();
        }
        responseToProcess = choices.at(4);
    }

    Q_ASSERT(!responseToProcess.isEmpty());

    responseToProcess += QLatin1Char( '\n' );

    writeToProcess(responseToProcess.toLocal8Bit());

    return true;
}

bool CliInterface::checkForErrorMessage(const QString& line, int parameterIndex)
{
    static QHash<int, QList<QRegExp> > patternCache;
    QList<QRegExp> patterns;

    if (patternCache.contains(parameterIndex)) {
        patterns = patternCache.value(parameterIndex);
    } else {
        if (!m_param.contains(parameterIndex)) {
            return false;
        }

        foreach(const QString& rawPattern, m_param.value(parameterIndex).toStringList()) {
            patterns << QRegExp(rawPattern);
        }
        patternCache[parameterIndex] = patterns;
    }

    foreach(const QRegExp& pattern, patterns) {
        if (pattern.indexIn(line) != -1) {
            return true;
        }
    }
    return false;
}

bool CliInterface::doKill()
{
    if (m_process) {
        kDebug(7109);
        m_process->terminate();
        QWaitCondition sleep;
        QMutex mutex;
        mutex.lock();
        sleep.wait(&mutex, 2000); // wait two seconds
        m_process->kill(); // this is required or m_process->waitForFinished(-1) never returns
        return true;
    }

    return false;
}

bool CliInterface::doSuspend()
{
    return false;
}

bool CliInterface::doResume()
{
    return false;
}

void CliInterface::substituteListVariables(QStringList& params)
{
    for (int i = 0; i < params.size(); ++i) {
        const QString parameter = params.at(i);

        if (parameter == QLatin1String( "$Archive" )) {
            params[i] = filename();
        }
    }
}

QString CliInterface::escapeFileName(const QString& fileName) const
{
    return fileName;
}

void CliInterface::writeToProcess(const QByteArray& data)
{
    Q_ASSERT(m_process);
    Q_ASSERT(!data.isNull());

    kDebug(7109) << "Writing" << data << "to the process";

#ifdef Q_OS_LINUX
    m_process->write(data);
#else
    m_process->pty()->write(data);
#endif
}

}

#include "cliinterface.moc"
