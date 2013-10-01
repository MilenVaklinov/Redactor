#ifndef SHELLWIDGET_H
#define SHELLWIDGET_H

#include <QKeyEvent>
#include <QTextBlock>
#include <QTextBrowser>
#include <QTextCursor>
#include <QTextDocumentFragment>
#include <QStack>
#include <QProcess>

class TextEditor;
class QVBoxLayout;
class QRegExp;
class QTextCharFormat;

class Shell : public QWidget
{
  Q_OBJECT

public:
  explicit Shell(TextEditor *given_editor, QWidget *parent = 0);
    ~Shell();

private:
    // Functions
    void moveToEndOfLine();
    void clearLine();
    QString get_command();
    void get_userPrompt();

private:
    // Variables
    QString userPrompt;
    QStack<QString> historyUp;
    QStack<QString> historyDown;
    bool historySkip;

    QProcess *myProcess;
    TextEditor *editor;
    TextEditor *output;
    QVBoxLayout *mainLayout;

// The command signal is fired when a user input is entered
signals:
  void command(const QString &cmd);

// The result slot displays the result of a command in the Shell
private slots:
  void handleLeft(QKeyEvent *event);
  void handleEnter();
  void handleHistoryUp();
  void handleHistoryDown();
  void handleHome();

  void result(const QString &cmd);
  void updateOutput();
  void startExecution(const QString &cmd);
};

#endif
