#include "countingsourcelineswidget.h"
#include "Buffer/texteditor.h"
#include "Editor/mytextedit.h"

#include <QApplication>
#include <QDebug>
#include <QFileInfo>
#include <QList>
#include <QLabel>
#include <QMessageBox>
#include <QTextStream>
#include <QSplitter>
#include <QVBoxLayout>

#include <iostream>

//==========================================================================================
// The constructor
//==========================================================================================

CountingSourceLinesWidget::CountingSourceLinesWidget(const QString &folderName, TextEditor *given_editor, QWidget *parent) : QWidget(parent)
{
    QSplitter *new_splitter = new QSplitter;
    new_splitter->setChildrenCollapsible(false);
    new_splitter->setObjectName("Main Second Splitter");

    QVBoxLayout *container_layout = new QVBoxLayout;
    container_layout->addWidget(new_splitter);
    container_layout->setContentsMargins(0, 0, 0, 0);
    container_layout->setSpacing(0);
    setLayout(container_layout);

    output = given_editor;
    new_splitter->addWidget(output);

    output->createModeLine();
    output->setObjectName(tr("*Counter*"));
    output->set_current_file_name(tr("*Counter*"));
    output->textEdit->setFocus();

    myProcess = new QProcess(this);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    myProcess->setProcessEnvironment(env);
    myProcess->setWorkingDirectory("/home/zak/Qt/Redactor/ConsoleTools");

    name_of_the_source = folderName;

    connect(myProcess, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(processFinished(int,QProcess::ExitStatus)));
    connect(myProcess, SIGNAL(error(QProcess::ProcessError)), this, SLOT(processError(QProcess::ProcessError)));
    connect(myProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(updateOutput()));

    startExecution();
}

CountingSourceLinesWidget::~CountingSourceLinesWidget()
{
    myProcess->close();
    delete myProcess;
}

//==========================================================================================
// Start the program / consoleSourceCounter -argument[the folder or file for counting]
//==========================================================================================

void CountingSourceLinesWidget::startExecution()
{
    output->textEdit->setReadOnly(false);

    QFileInfo checkTheSource(name_of_the_source);

    if(!name_of_the_source.isEmpty() && checkTheSource.exists()) {

        if(checkTheSource.isDir())
            output->textEdit->insertPlainText(tr("Given folder: %1 \n\n").arg(name_of_the_source));
        if(checkTheSource.isFile())
            output->textEdit->insertPlainText(tr("Given file: %1 \n\n").arg(name_of_the_source));
    }

    output->textEdit->insertPlainText("Start counting...\n\n");

    QString program = QLatin1String("./counter");
    QStringList arguments;
    arguments << name_of_the_source;

    //====================================================

#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif

    myProcess->start(program, arguments);

    //===================================================

    // Read only mode
    output->textEdit->document()->setModified(false);
    output->textEdit->setReadOnly(true);
    output->textEdit->setTextInteractionFlags(output->textEdit->textInteractionFlags()
                                              | Qt::TextSelectableByKeyboard);
    QString current_mode = output->chFr->text();
    output->chFr->setText(current_mode.replace(0, 2, "%%"));
    output->textEdit->setFocus();
}

//==========================================================================================
// Update the text from cerr
//==========================================================================================

void CountingSourceLinesWidget::updateOutput()
{
    QByteArray output_text = myProcess->readAllStandardOutput();
    QString text = QString::fromLocal8Bit(output_text);

    output->textEdit->insertPlainText(text);

#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif

    // Read only mode
    output->textEdit->document()->setModified(false);
    output->textEdit->setReadOnly(true);
    output->textEdit->setTextInteractionFlags(output->textEdit->textInteractionFlags()
                                              | Qt::TextSelectableByKeyboard);
    QString current_mode = output->chFr->text();
    output->chFr->setText(current_mode.replace(0, 2, "%%"));
    output->textEdit->setFocus();
}

//==========================================================================================
// Check the exitCode and the process' state
//==========================================================================================

void CountingSourceLinesWidget::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if(exitStatus == QProcess::CrashExit)
        output->textEdit->insertPlainText(tr("The program crashed!"));
    else if(exitCode != 0)
        output->textEdit->insertPlainText(tr("Failed!"));
    else
        output->textEdit->insertPlainText(tr("Successful execution!"));

    // Read only mode
    output->textEdit->document()->setModified(false);
    output->textEdit->setReadOnly(true);
    output->textEdit->setTextInteractionFlags(output->textEdit->textInteractionFlags()
                                              | Qt::TextSelectableByKeyboard);
    QString current_mode = output->chFr->text();
    output->chFr->setText(current_mode.replace(0, 2, "%%"));
    output->textEdit->setFocus();
}

//==========================================================================================
// Checks the type of error that occurred last
//==========================================================================================

void CountingSourceLinesWidget::processError(QProcess::ProcessError error)
{
    switch(error) {
    case 0:
        output->textEdit->insertPlainText(tr("Failed to start!"));
        break;
    case 1:
        output->textEdit->insertPlainText(tr("The application crashed!"));
        break;
    case 2:
        output->textEdit->insertPlainText(tr("Time out!"));
        break;
    case 3:
        output->textEdit->insertPlainText(tr("Read Error!"));
        break;
    case 4:
        output->textEdit->insertPlainText(tr("Write Error!"));
        break;
    case 5:
        output->textEdit->insertPlainText(tr("Unknown Error!"));
    }

    // Read only mode
    output->textEdit->document()->setModified(false);
    output->textEdit->setReadOnly(true);
    output->textEdit->setTextInteractionFlags(output->textEdit->textInteractionFlags()
                                              | Qt::TextSelectableByKeyboard);
    QString current_mode = output->chFr->text();
    output->chFr->setText(current_mode.replace(0, 2, "%%"));
}
