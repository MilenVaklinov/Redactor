#include "dired.h"
#include "Buffer/texteditor.h"
#include "Editor/mytextedit.h"

#include <QDir>
#include <QDebug>
#include <QFileInfo>
#include <QMessageBox>
#include <QTextStream>
#include <QList>
#include <QLabel>
#include <QSplitter>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QProcess>

#include <iostream>

Dired::Dired(const QString &folderName, TextEditor *given_editor, QWidget *parent) : QWidget(parent),
    is_it_file(false)
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
    output->setObjectName("*Dired*");
    output->set_current_file_name(tr("*Dired*"));
    output->textEdit->setObjectName("*Dired*");
    output->textEdit->setFocus();

    QFont font("DejaVu Sans Mono", 10);
    output->textEdit->setFont(font);

    myProcess = new QProcess(this);
    myProcess->setProcessChannelMode(QProcess::MergedChannels);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    myProcess->setProcessEnvironment(env);

    name_of_the_source = folderName;

    if (name_of_the_source != "/" && name_of_the_source.endsWith("/"))
        name_of_the_source.chop(1);

    connect(myProcess, SIGNAL(error(QProcess::ProcessError)), this, SLOT(processError(QProcess::ProcessError)));
    connect(myProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(updateOutput()));

    output->textEdit->document()->setModified(false);
    output->textEdit->setReadOnly(true);
    output->textEdit->setTextInteractionFlags(output->textEdit->textInteractionFlags()
                                              | Qt::TextSelectableByKeyboard);
    QString current_mode = output->chFr->text();
    output->chFr->setText(current_mode.replace(0, 2, "%%"));

    connect(output->textEdit, SIGNAL(pushed_enter(int)), SLOT(show_next(int)));

    startExecution();
}

Dired::~Dired() { }



//==========================================================================================
// Start the program / consoleSourceCounter -argument[the folder or file for counting]
//==========================================================================================

void Dired::startExecution()
{   
    myProcess->setWorkingDirectory("/home/zak/Qt/Redactor/Dired");
    myProcess->start(QLatin1String("sh"));

    if (name_of_the_source.contains(" "))
        name_of_the_source.replace(" ", "\\ ");

    QString command = "ls --color=always -lap " + name_of_the_source + " | ./aha -n\n";

    if (name_of_the_source.contains("\\ "))
        name_of_the_source.replace("\\ ", " ");

    myProcess->write(command.toLocal8Bit());
    myProcess->closeWriteChannel();
}

//==========================================================================================
// Update the text from cerr
//==========================================================================================

void Dired::updateOutput()
{
    QFileInfo checkTheSource(name_of_the_source);
    QString current_dir;

    if(!name_of_the_source.isEmpty() && checkTheSource.exists()) {

        if(checkTheSource.isDir())
            current_dir = "<font color=#008000>" + name_of_the_source + "</font>\n";
        if(checkTheSource.isFile()) {
            int index_of_last_slash = name_of_the_source.lastIndexOf('/');
            QString name_of_current_dir = name_of_the_source.left(index_of_last_slash);
            current_dir = "<font color=#008000>" + name_of_current_dir + "</font>";
            is_it_file = true;
        }
    }

    QByteArray errorOutput = myProcess->readAllStandardOutput();
    QString converted_text = QString::fromLocal8Bit(errorOutput);
    QTextCursor cursor = output->textEdit->textCursor();

    if (!converted_text.contains("total"))
        return;

    output->textEdit->setReadOnly(false);

    if (is_it_file) {

        int index_of_last_slash = name_of_the_source.lastIndexOf('/');
        QString name_of_file = name_of_the_source.mid(index_of_last_slash + 1);
        converted_text.replace(name_of_the_source, name_of_file);

        converted_text.prepend(current_dir);
        output->textEdit->appendHtml("<pre>" + converted_text + "</pre>");

        QString current_text = output->textEdit->toPlainText();
        int index_of_file_name = current_text.indexOf(name_of_file);
        cursor.setPosition(index_of_file_name);

        output->textEdit->setTextCursor(cursor);

    } else {

        converted_text.prepend(current_dir);
        output->textEdit->appendHtml("<pre>" + converted_text + "</pre>");

        QString text_for_manipulation = output->textEdit->toPlainText();
        folders = text_for_manipulation.split("\n", QString::SkipEmptyParts);

        output->textEdit->moveCursor(QTextCursor::Start);
        output->textEdit->moveCursor(QTextCursor::Down);
        output->textEdit->moveCursor(QTextCursor::Down);
        output->textEdit->moveCursor(QTextCursor::EndOfLine);
        output->textEdit->moveCursor(QTextCursor::PreviousCharacter);
        output->textEdit->moveCursor(QTextCursor::PreviousCharacter);
    }

    output->textEdit->document()->setModified(false);
    output->textEdit->setReadOnly(true);
    output->textEdit->setTextInteractionFlags(output->textEdit->textInteractionFlags()
                                              | Qt::TextSelectableByKeyboard);
    QString current_mode = output->chFr->text();
    output->chFr->setText(current_mode.replace(0, 2, "%%"));

    output->textEdit->setFocus();

    QScrollBar *vScrollBar = output->textEdit->verticalScrollBar();
    vScrollBar->triggerAction(QScrollBar::SliderToMinimum);
}

//==========================================================================================
// Checks the type of error that occurred last
//==========================================================================================

void Dired::processError(QProcess::ProcessError error)
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

    output->textEdit->document()->setModified(false);
    output->textEdit->setReadOnly(true);
    output->textEdit->setTextInteractionFlags(output->textEdit->textInteractionFlags()
                                              | Qt::TextSelectableByKeyboard);
    QString current_mode = output->chFr->text();
    output->chFr->setText(current_mode.replace(0, 2, "%%"));
}

void Dired::show_next(int current_line)
{
    if (current_line > 2) {

        for (int i = 0; i != folders.count(); ++i) {
            if(folders.at(i).endsWith("/") && i == (current_line-1)) {

                QString first_line = folders.at(2);
                int index_of_dot = first_line.indexOf("./");
                QString next_folder = "";
                QString old_part = folders.at(i).mid(index_of_dot);

                old_part.chop(1);

                if (old_part == "..") {

                    if (name_of_the_source.count("/") > 1) {

                        QString part_for_deletion = "/" + (name_of_the_source.section('/', name_of_the_source.count("/")));

                        name_of_the_source.remove(part_for_deletion);
                        next_folder = name_of_the_source;

                        emit folder_ready(next_folder);
                        return;

                    } else if (name_of_the_source.count("/") == 1){

                        if (name_of_the_source == "/") {
                            output->textEdit->moveCursor(QTextCursor::Down);
                            return;
                        }

                        QString part_for_deletion = name_of_the_source.section('/', name_of_the_source.count("/"));

                        name_of_the_source.remove(part_for_deletion);

                        next_folder = name_of_the_source;

                        emit folder_ready(next_folder);
                        return;
                    }

                } else if (old_part == ".")
                    return;

                else {
                    if (name_of_the_source == "/")
                        next_folder = name_of_the_source + old_part;
                    else
                        next_folder = name_of_the_source + "/" + old_part;

                    emit folder_ready(next_folder);

                    return;
                }

            } else if (i == (current_line - 1)){
                // Here starts the case if the line is a file, not a directory
                // but *Dired* was open using a directory as: dired /home/x/dir

                QString next_file;
                QString first_line = folders.at(2);

                int index_of_dot = first_line.indexOf("./");

                QString old_part = folders.at(i).mid(index_of_dot);

                if (name_of_the_source == "/")
                    next_file = name_of_the_source + old_part;
                else
                    next_file = name_of_the_source + "/" + old_part;

                emit file_ready(next_file);


            } else { }
        }
    }

    if (is_it_file) {
        // Here starts the case if the line is a file, not a directory
        // but *Dired* was open using a file as: dired /home/x/file.
        // Here we don't have "./" so the method is different.

        emit file_ready(name_of_the_source);
    }
}
