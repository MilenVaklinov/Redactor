#include <Buffer/texteditor.h>
#include <Editor/mytextedit.h>
#include "Shell.h"

#include <QDebug>
#include <QSplitter>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QProcess>
#include <QRegExp>

Shell::Shell(TextEditor *given_editor, QWidget *parent) : QWidget(parent),
    userPrompt(""),
    historySkip(false)
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
    output->setObjectName(tr("*Shell*"));
    output->textEdit->setObjectName("*Shell*");
    output->set_current_file_name(tr("*Shell*"));
    output->textEdit->setReadOnly(false);

    QFont font("DejaVu Sans Mono", 11);
    output->textEdit->setFont(font);

    myProcess = new QProcess(this);
    myProcess->setProcessChannelMode(QProcess::MergedChannels);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    myProcess->setProcessEnvironment(env);
    myProcess->setWorkingDirectory("/home/zak/Qt/Redactor/Shell");
    myProcess->start(QLatin1String("./pty"), QStringList() << "-e" << "/bin/bash");

    connect(myProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(updateOutput()));
    connect(this, SIGNAL(command(QString)), this, SLOT(result(QString)));
    connect(output->textEdit, SIGNAL(shell_handle_enter()), this, SLOT(handleEnter()));
    connect(output->textEdit, SIGNAL(shell_handle_left(QKeyEvent*)), this, SLOT(handleLeft(QKeyEvent*)));
    connect(output->textEdit, SIGNAL(shell_handle_history_up()), this, SLOT(handleHistoryUp()));
    connect(output->textEdit, SIGNAL(shell_handle_history_down()), this, SLOT(handleHistoryDown()));
    connect(output->textEdit, SIGNAL(shell_handle_home()), this, SLOT(handleHome()));

    historyUp.clear();
    historyDown.clear();

    output->textEdit->setFocus();
}

//==========================================================================================
// Destructor.
//==========================================================================================

Shell::~Shell()
{
    myProcess->close();

    delete myProcess;
}

//==========================================================================================
// Start the program / consoleSourceCounter -argument[the folder or file for counting]
//==========================================================================================

void Shell::startExecution(const QString &cmd)
{
    QString command;

    command = cmd + "\n";

    myProcess->write(command.toLocal8Bit());
}

//==========================================================================================
// Update the text from cerr
//==========================================================================================

void Shell::updateOutput()
{
    QByteArray output_text = myProcess->readAllStandardOutput();
    QString converted_text = QString::fromLocal8Bit(output_text);

    if (converted_text.contains(QRegExp("^\\\e\\[0m")))
        converted_text.replace(QRegExp("^\\\e\\[0m"), "");

    if (converted_text.contains(QRegExp("\\\e\\[0m")))
        converted_text.replace(QRegExp("\\\e\\[0m"), "</font>");

    if (converted_text.contains(QRegExp("\\\e\\[01;30m")))
        converted_text.replace(QRegExp("\\\e\\[01;30m"), "<font style=\"color: #000000;\">");

    if (converted_text.contains(QRegExp("\\\e\\[01;31m")))
        converted_text.replace(QRegExp("\\\e\\[01;31m"), "<font style=\"color: #ff0000;\">");

    if (converted_text.contains(QRegExp("\\\e\\[01;32m")))
        converted_text.replace(QRegExp("\\\e\\[01;32m"), "<font style=\"color: #008000;\">");

    if (converted_text.contains(QRegExp("\\\e\\[01;33m")))
        converted_text.replace(QRegExp("\\\e\\[01;33m"), "<font style=\"color: #808000;\">");

    if (converted_text.contains(QRegExp("\\\e\\[01;34m")))
        converted_text.replace(QRegExp("\\\e\\[01;34m"), "<font style=\"color: #0000ff;\">");

    if (converted_text.contains(QRegExp("\\\e\\[01;35m")))
        converted_text.replace(QRegExp("\\\e\\[01;35m"), "<font style=\"color: #800080;\">");

    if (converted_text.contains(QRegExp("\\\e\\[01;36m")))
        converted_text.replace(QRegExp("\\\e\\[01;36m"), "<font style=\"color: #008080;\">");

    if (converted_text.contains(QRegExp("\\\e\\[01;37m")))
        converted_text.replace(QRegExp("\\\e\\[01;37m"), "<font style=\"color: #ffffff;\">");

    if (converted_text.contains(QRegExp("\\\e\\[30;42m")))
        converted_text.replace(QRegExp("\\\e\\[30;42m"), "<font style=\"color: black; background-color: green;\">");

    if (converted_text.contains(QRegExp("\\\e\\[37;41m")))
        converted_text.replace(QRegExp("\\\e\\[37;41m"), "<font style=\"color: white; background-color: red;\">");

    if (converted_text.contains(QRegExp("\\\e\\[30;43m")))
        converted_text.replace(QRegExp("\\\e\\[30;43m"), "<font style=\"color: black; background-color: yellow;\">");

    output->textEdit->appendHtml("<pre>" + converted_text + "</pre>");

    get_userPrompt();

    QScrollBar *vScrollBar = output->textEdit->verticalScrollBar();
    vScrollBar->triggerAction(QScrollBar::SliderToMaximum);
}

void Shell::get_userPrompt()
{
    QTextCursor cursor = output->textEdit->textCursor();
    cursor.select(QTextCursor::LineUnderCursor);
    QString current_line = cursor.selectedText();

    int current_position = cursor.position();

    cursor.movePosition(QTextCursor::StartOfLine);
    int start_of_line = cursor.position();

    if (current_line.startsWith("zak@arch:~")) {

        int index_of_mashine = current_line.indexOf("zak@arch:~");
        cursor.setPosition(start_of_line + index_of_mashine);

        int index_of_dollar = current_line.indexOf("$");
        cursor.setPosition(start_of_line + index_of_dollar + 2, QTextCursor::KeepAnchor);

        userPrompt = cursor.selectedText();
        cursor.clearSelection();

        cursor.setPosition(current_position);
        output->textEdit->setTextCursor(cursor);

    } else if (current_line.startsWith("zak@arch:~")) {

        int index_of_mashine = current_line.indexOf("root@arch:~ ");
        cursor.setPosition(start_of_line + index_of_mashine);

        int index_of_dollar = current_line.indexOf("#");
        cursor.setPosition(start_of_line + index_of_dollar + 2, QTextCursor::KeepAnchor);

        userPrompt = cursor.selectedText();
        cursor.clearSelection();

        cursor.setPosition(current_position);
        output->textEdit->setTextCursor(cursor);

    } else {
        userPrompt = "";
    }
}

/* Filter all key events in the Editor/mytextedit.cpp/keyPressEvent().
  * The keys are filtered and handled manually
  * in order to create a typical shell-like behaviour. For example
  * Up and Down arrows don't move the cursor, but allow the user to
  * browse the last commands that there launched.
  */

//==========================================================================================
// Enter key pressed.
//==========================================================================================

void Shell::handleEnter()
{
    QString cmd = get_command();

    if(0 < cmd.length()) {
        while(historyDown.count() > 0) {
            historyUp.push(historyDown.pop());
        }

        historyUp.push(cmd);
    }

    moveToEndOfLine();

    if(cmd.length() > 0) {

        output->textEdit->setFocus();

        emit command(cmd);
    }
}

//==========================================================================================
// Result received.
//==========================================================================================

void Shell::result(const QString &cmd)
{
    startExecution(cmd);
}

//==========================================================================================
// Arrow up pressed.
//==========================================================================================

void Shell::handleHistoryUp()
{
    if(0 < historyUp.count()) {
        QString cmd = historyUp.pop();
        historyDown.push(cmd);

        clearLine();

        output->textEdit->insertPlainText(cmd);
    }

    historySkip = true;
}

//==========================================================================================
// Arrow down pressed.
//==========================================================================================

void Shell::handleHistoryDown()
{
    if(0 < historyDown.count() && historySkip) {
        historyUp.push(historyDown.pop());
        historySkip = false;
    }

    if(0 < historyDown.count()) {
        QString cmd = historyDown.pop();
        historyUp.push(cmd);

        clearLine();

        output->textEdit->insertPlainText(cmd);

    } else {
        clearLine();
    }
}

//==========================================================================================
//
//==========================================================================================

void Shell::clearLine()
{
    QTextCursor cursor = output->textEdit->textCursor();
    cursor.select(QTextCursor::LineUnderCursor);
    QString current_line = cursor.selectedText();

    cursor.movePosition(QTextCursor::StartOfLine);
    int start_of_line = cursor.position();

    if (current_line.startsWith("zak@arch:~")) {

        int index_of_mashine = current_line.indexOf("zak@arch:~");
        cursor.setPosition(start_of_line + index_of_mashine);

        int index_of_dollar = current_line.indexOf("$");
        cursor.setPosition(start_of_line + index_of_dollar + 2, QTextCursor::KeepAnchor);

        userPrompt = cursor.selectedText();
        cursor.clearSelection();

        cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();

        cursor.setPosition(start_of_line + index_of_dollar + 2);

    } else if (current_line.startsWith("root@arch:~")) {

        int index_of_mashine = current_line.indexOf("zak@arch:~");
        cursor.setPosition(start_of_line + index_of_mashine);

        int index_of_dollar = current_line.indexOf("#");
        cursor.setPosition(start_of_line + index_of_dollar + 2, QTextCursor::KeepAnchor);

        userPrompt = cursor.selectedText();
        cursor.clearSelection();

        cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();

        cursor.setPosition(start_of_line + index_of_dollar + 2);

    } else {
        cursor.select(QTextCursor::LineUnderCursor);
        cursor.removeSelectedText();
    }

    output->textEdit->setTextCursor(cursor);
}

//==========================================================================================
// Select and return the user-input (exclude the prompt).
//==========================================================================================

QString Shell::get_command()
{
    get_userPrompt();

    QTextCursor cursor = output->textEdit->textCursor();
    cursor.select(QTextCursor::LineUnderCursor);
    QString current_line = cursor.selectedText();

    if (current_line.contains(userPrompt))
        current_line.remove(0, userPrompt.length());

    if (current_line.contains(userPrompt))
        current_line.remove(0, userPrompt.length());

    return current_line;
}

void Shell::moveToEndOfLine()
{
    output->textEdit->moveCursor(QTextCursor::EndOfLine);
}

//==========================================================================================
// The text cursor is not allowed to move beyond the prompt.
//==========================================================================================

void Shell::handleLeft(QKeyEvent *event)
{
    QTextCursor cursor = output->textEdit->textCursor();
    int current_position = cursor.position();

    cursor.movePosition(QTextCursor::StartOfLine);
    int user_prompt_pos = cursor.position() + userPrompt.length();

    cursor.setPosition(current_position);
    output->textEdit->setTextCursor(cursor);

    if( current_position > user_prompt_pos)
        output->textEdit->event_from_shell(event);
}

//==========================================================================================
// Home key pressed.
//==========================================================================================

void Shell::handleHome()
{
    QTextCursor c = output->textEdit->textCursor();
    c.movePosition(QTextCursor::StartOfLine);
    c.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, userPrompt.length());
    output->textEdit->setTextCursor(c);
}
