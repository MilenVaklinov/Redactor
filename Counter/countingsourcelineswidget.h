#ifndef COUNTINGSOURCELINESWIDGET_H
#define COUNTINGSOURCELINESWIDGET_H

#include <QWidget>
#include <QProcess>

class QString;
class TextEditor;
class QVBoxLayout;

class CountingSourceLinesWidget : public QWidget
{
    Q_OBJECT
    
public:
    CountingSourceLinesWidget(const QString &folderName, TextEditor *given_editor, QWidget *parent = 0);
    ~CountingSourceLinesWidget();

private:
    QProcess *myProcess;
    QString name_of_the_source;
    TextEditor *output;
    QVBoxLayout *mainLayout;

private slots:
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void processError(QProcess::ProcessError error);
    void updateOutput();
    void startExecution();

};

#endif
