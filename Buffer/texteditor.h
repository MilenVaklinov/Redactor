#ifndef TEXTEDITOR_H
#define TEXTEDITOR_H

#include <QWidget>

class QAction;
class ClockLabel;
class Highlighter;
class QLabel;
class QToolBar;
class QTextLine;
class QVBoxLayout;
class MyTextEdit;

class TextEditor : public QWidget
{
    Q_OBJECT
    
public:
    TextEditor(QWidget *parent = 0);
    MyTextEdit *textEdit;

    void newFile();
    void insertImage();
    void setTitleFromMainWindow(const QString& fileName);
    bool loadFile(const QString &fileName);
    bool maybeSave();
    bool save();
    bool saveAs();

    bool saveFile(const QString &fileName);
    QString saveFileNonGui(const QString &fileName);

    void set_current_file_name(const QString& name);
    QString currentFile() { return curFile; }
    QString currentFilePath() { return path; }

    void createModeLine();

    QLabel *chFr;
    QString get_majorMinor();

    void create_clock();
    void delete_clock();

signals:
    void pushed_x();
    void pushed_q();
    void pushed_g();
    void pushed_ctrl_g();
    void pushed_read_only();
    void curFile_changed();

public slots:
    void change_the_highlighter();

private slots:
    void updateCursorPosition();
    void documentIsModified(bool mode);
    void update_modeline_color(bool status);

private:
    void set_majorMinor(const QString& given_text);
    void init_variables();
    void create_buffer();
    void create_shortcuts();
    void addWidgetsToLayout();
    void createLabels();
    void addLabelsToModeLine();
    void setCurrentFile(const QString &fileName);

private:
    Highlighter *highlighter;
    QLabel *cs;
    QLabel *buf;
    QLabel *posLine;
    QLabel *majorMinor;
    QVBoxLayout *layout;
    QToolBar *modeLine;
    QString curFile;
    QString path;

    // Clock
    ClockLabel *clock;
    QAction *visible_clock_label;

    bool isUntitled;
    bool alt;
    bool ctrl;
    bool shift;

};

#endif // TEXTEDITOR_H
