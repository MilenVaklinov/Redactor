#ifndef DIRED_H
#define DIRED_H

#include <QWidget>
#include <QProcess>

class QString;
class TextEditor;
class QVBoxLayout;

class Dired : public QWidget
{
    Q_OBJECT
public:
    explicit Dired(const QString &folderName, TextEditor *given_editor, QWidget *parent = 0);
    ~Dired();

signals:
    void folder_ready(QString folder);
    void file_ready(QString file);

private:

    QProcess *myProcess;
    QString name_of_the_source;
    TextEditor *editor;
    TextEditor *output;
    QVBoxLayout *mainLayout;
    QStringList folders;

    bool is_it_file;

private slots:
    void processError(QProcess::ProcessError error);
    void updateOutput();
    void startExecution();
    void show_next(int current_line);
    
};

#endif // DIRED_H
