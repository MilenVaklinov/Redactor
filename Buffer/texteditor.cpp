#include "mainwindow.h"
#include "texteditor.h"
#include "Editor/mytextedit.h"
#include "Clock/clocklabel.h"
#include "Highlighter/highlighter.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QString>
#include <QShortcut>
#include <QScrollBar>
#include <QToolBar>
#include <QTextCursor>

#include <QTextStream>
#include <QVBoxLayout>
#include <QUrl>
#include <QImageReader>


TextEditor::TextEditor(QWidget *parent) : QWidget(parent)
{
    init_variables();
    create_buffer();
    create_shortcuts();
}

void TextEditor::init_variables()
{
    // Int variables

    // Bool variables
    isUntitled = true;
    ctrl = false;
    alt = false;
    shift = false;

    // Pointer variables
    highlighter = 0;
    clock = 0;
    visible_clock_label = 0;
}

void TextEditor::create_buffer()
{
    textEdit = new MyTextEdit;

    QFont font("DejaVu Sans Mono", 10);

    textEdit->setFont(font);
    textEdit->setStyleSheet("color:black;");

    layout = new QVBoxLayout(this);
    layout->addWidget(textEdit);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
}

void TextEditor::newFile()
{
    static int sequanceNumber = 1;

    isUntitled = true;

    curFile = tr("document%1.txt").arg(sequanceNumber++);
    emit curFile_changed();

    textEdit->document()->setModified(false);
    path = QDir::homePath() + "/" + curFile;
}

void TextEditor::setTitleFromMainWindow(const QString& fileName)
{
    path = fileName;

    QStringList pathList = fileName.split("/");
    curFile = pathList.at(pathList.count()-1);
    emit curFile_changed();

    isUntitled = false;
    textEdit->document()->setModified(false);
}

void TextEditor::set_current_file_name(const QString& name)
{
    QString newPath = path;
    newPath.remove(curFile);

    curFile = name;
    emit curFile_changed();

    path = newPath + curFile;
    isUntitled = false;
    buf->setText(QString(8, ' ') + curFile);
}

bool TextEditor::loadFile(const QString &fileName)
{
    QFile file(fileName);

    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        QMessageBox::warning(this,
                             tr("Redactor"),
                             tr("Cannot read file %1: \n%2.").arg(fileName).arg(file.errorString()));

        return false;
    }

    QTextStream in(&file);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    textEdit->setPlainText(in.readAll());
    QApplication::restoreOverrideCursor();

    textEdit->document()->setModified(false);

    path = fileName;
    setCurrentFile(fileName);

    return true;
}

bool TextEditor::save()
{
    if(isUntitled)
        return saveAs();
    else
        return saveFile(curFile);
}

bool TextEditor::saveAs()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"), curFile);

    if(fileName.isEmpty())
        return false;

    return saveFile(fileName);
}

bool TextEditor::saveFile(const QString &fileName)
{
    QFile file(fileName);

    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, tr("Redactor"),
                             tr("Connot write file %1: \n%2").arg(fileName).arg(file.errorString()));

        return false;
    }

    QTextStream out(&file);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    out << textEdit->toPlainText();
    QApplication::restoreOverrideCursor();

    setCurrentFile(fileName);
    textEdit->document()->setModified(false);

    return true;
}

QString TextEditor::saveFileNonGui(const QString &fileName)
{
    QFile file(fileName);

    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QString errorMsg = (tr("Connot write the file: %1 %2").arg(fileName).arg(file.errorString()));
        return errorMsg;
    }

    QTextStream out(&file);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    out << textEdit->toPlainText();
    QApplication::restoreOverrideCursor();

    setCurrentFile(fileName);
    textEdit->document()->setModified(false);

    return "";
}

bool TextEditor::maybeSave()
{
    if(textEdit->document()->isModified())
    {


        switch(QMessageBox::warning(this, tr("Redactor"),
                                   tr("%1 has been modified.\n"
                                      "Do you want to save the changes?").arg(currentFile()),
                                   QMessageBox::Save | QMessageBox::Default,
                                   QMessageBox::Discard,
                                   QMessageBox::Cancel))

        {
        case  QMessageBox::Save:
            return save();
        case QMessageBox::Discard:
            return true;
        default:
               return false;
        }
    }

    return true;
}

void TextEditor::setCurrentFile(const QString &fileName)
{
    curFile = QFileInfo(QFileInfo(fileName).canonicalFilePath()).fileName();
    isUntitled = false;
    textEdit->document()->setModified(false);
}

void TextEditor::createModeLine()
{   
    modeLine = new QToolBar(this);
    modeLine->setFixedHeight(16);
    modeLine->setObjectName(tr("mode-line"));
    modeLine->setStyleSheet("color: #000000; background-color: #BFBFBF;");
    modeLine->setMovable(false);
    modeLine->setAllowedAreas(Qt::BottomToolBarArea);

    addWidgetsToLayout();
    createLabels();
    addLabelsToModeLine();

    connect(textEdit, SIGNAL(modeline_focus(bool)), this, SLOT(update_modeline_color(bool)));
    connect(this, SIGNAL(curFile_changed()), this, SLOT(change_the_highlighter()));
}

void TextEditor::update_modeline_color(bool status)
{
    if (status)
        modeLine->setStyleSheet("color: #000000; background-color: #BFBFBF;");
    else
        modeLine->setStyleSheet("color: #000000; background-color: #E5E5E5;");
}

void TextEditor::addWidgetsToLayout()
{
    layout->addWidget(modeLine);
}

//==========================================================================================
// Create all the labels for mode-line
//==========================================================================================

void TextEditor::createLabels()
{

    /* ch shows two dashes (‘--’) if the buffer displayed in the window
     has the same contents as the corresponding file on the disk; i.e.,
     if the buffer is “unmodified”. If the buffer is modified, it shows
     two stars (‘**’). For a read-only buffer, it shows ‘%*’ if the buffer
     is modified, and ‘%%’ otherwise.

    The text displayed in the mode line has the following format:
              cs:ch-fr  buf      pos line   (major minor)
    */

    QFont font("DejaVu Sans Mono", 9);

    cs = new QLabel;
    cs->setText("--:");
    cs->setFont(font);

    chFr = new QLabel;
    chFr->setText("---");
    chFr->setFont(font);

    buf = new QLabel;
    QString fileName = QString(8, ' ') + currentFile();
    buf->setText(fileName);
    buf->setFont(QFont("DejaVu Sans Mono", 9, QFont::Bold));

    posLine = new QLabel;
    posLine->setText(QString(8, ' ') + "Top L1");
    posLine->setFont(font);

    majorMinor = new QLabel;
    majorMinor->setText(QString(4, ' ') + "(Text)");
    majorMinor->setFont(font);

    connect(textEdit->document(), SIGNAL(modificationChanged(bool)), this, SLOT(documentIsModified(bool)));
    connect(textEdit, SIGNAL(cursorPositionChanged()), this, SLOT(updateCursorPosition()));
}

void TextEditor::addLabelsToModeLine()
{
    modeLine->addWidget(cs);
    modeLine->addWidget(chFr);
    modeLine->addWidget(buf);
    modeLine->addWidget(posLine);
    modeLine->addWidget(majorMinor);
}

void TextEditor::set_majorMinor(const QString &given_text)
{
    QString text = QString(4, ' ') + "(" + given_text + ")";
    majorMinor->setText(text);
}

QString TextEditor::get_majorMinor()
{
    return majorMinor->text();
}

void TextEditor::documentIsModified(bool mode)
{
    if(mode)
        chFr->setText("**-");

    else
       chFr->setText("---");

    QString fileName = QString(8, ' ') + currentFile();
    buf->setText(fileName);
}

void TextEditor::updateCursorPosition()
{
    QTextCursor cursor = textEdit->textCursor();
    QScrollBar *vertical_bar = textEdit->verticalScrollBar();

    if (!vertical_bar)
        return;

    int minimum = vertical_bar->minimum();
    int maximum = vertical_bar->maximum();

    int page_step = vertical_bar->pageStep();
    int single_step = vertical_bar->singleStep();
    int page_count = maximum / page_step;

    // The count of all lines in the document.
    int line_count = textEdit->blockCount();

    // The number of the current line.
    int line_number = cursor.blockNumber() + 1;

    int lines_per_page = 0;
    if (page_count > 0)
        lines_per_page = line_count / page_count;

    QVariant currentLine = line_number;
    QString line = "";

    // If minimum == maximum then all the text is only in one page.
    if (minimum == maximum)
        line = QString(8, ' ') + "All L" + currentLine.toString();
    else {

        // If lines_per_page = 0, then we have scroll bar but we do not have a whole page to scroll.
        if (lines_per_page == 0) {

            if ( line_number < (page_step / single_step) )
                line = QString(8, ' ') + "Top L" + currentLine.toString();
            else
                line = QString(8, ' ') + "Bot L" + currentLine.toString();

        } else {
            if ( line_number < lines_per_page - 1 ) {
                line = QString(8, ' ') + "Top L" + currentLine.toString();
            } else if ( line_number > (line_count - lines_per_page) ) {
                line = QString(8, ' ') + "Bot L" + currentLine.toString();
            } else {
                float percent_f = currentLine.toFloat() / QVariant(line_count).toFloat();
                int percent = percent_f * 100;
                line = QString(8, ' ') + QString::number(percent) + "% L" + currentLine.toString();
            }
        }
    }

    posLine->setText(line);
}

//==========================================================================================
// Create and destoy the clock in the mode line.
//==========================================================================================

void TextEditor::create_clock()
{
    clock = new ClockLabel;
    visible_clock_label = modeLine->addWidget(clock);
    visible_clock_label->setVisible(true);
}

//==========================================================================================

void TextEditor::delete_clock()
{
    visible_clock_label->setVisible(false);
    delete clock;
}

//==========================================================================================
// Insert image in the current document.
//==========================================================================================

void TextEditor::insertImage()
{

    QString file = "/home/zak/Qt/Redactor/Documentation/images/main_image.png";
    QUrl Uri ( QString ( "file://%1" ).arg ( file ) );
    QImage image = QImageReader ( file ).read();

    QTextDocument * textDocument = textEdit->document();
    textDocument->addResource( QTextDocument::ImageResource, Uri, QVariant ( image ) );
    QTextCursor cursor = textEdit->textCursor();
    QTextImageFormat imageFormat;
    imageFormat.setWidth( image.width() );
    imageFormat.setHeight( image.height() );
    imageFormat.setName( Uri.toString() );
    cursor.insertImage(imageFormat);
 }

void TextEditor::create_shortcuts()
{

}

void TextEditor::change_the_highlighter()
{
    if (highlighter)
        delete highlighter;

    int dots = curFile.count(".");
    QString suffix = curFile.section(".", dots);

    if (suffix == "cpp" || suffix == "cc" || suffix == "C"
            || suffix == "cxx" || suffix == "pcc" || suffix == "H" || suffix == "h"
            || suffix == "hh" || suffix == "hpp") {

        QString path = "/home/zak/Qt/Redactor/Highlighter/syntax/cplusplus.txt";
        QFile file(path);

        if (file.exists()) {
            highlighter = new Highlighter(textEdit->document());
            highlighter->open_file(path);
        }

        set_majorMinor("C/C++");

    } else if (suffix == "txt" || suffix == "")
        set_majorMinor(tr("Text"));

    else if (curFile == "*Counter*" || curFile == "*Dired*" || curFile == "*Home Page*" || curFile == "*Shell*")
        set_majorMinor(tr("Fundamental"));

    else if(suffix == "c" || suffix == "ec" || suffix == "pgc")
        set_majorMinor("C");

    else if(suffix == "py")
        set_majorMinor("Python");

    else if(suffix == "XML" || suffix == "xml")
        set_majorMinor("XML");

    else if(suffix == "html" || suffix == "htm")
        set_majorMinor("HTML");

    else if(suffix == "java")
        set_majorMinor("Java");

    else if(suffix == "js")
        set_majorMinor("JavaScript");

    else if(suffix == "pyx")
        set_majorMinor("Cython");

    else if(suffix == "el" || suffix == "jl" || suffix == "lisp"
            || suffix == "lsp" || suffix == "sc" || suffix == "scm")
        set_majorMinor("Lisp");

    else if(suffix == "perl" || suffix == "PL" || suffix == "pl"
            || suffix == "plh" || suffix == "plx" || suffix == "pm")
        set_majorMinor("Perl");

    else if(suffix == "php" || suffix == "php3" || suffix == "php4" || suffix == "php5")
        set_majorMinor("PHP");

    else if(suffix == "rb")
        set_majorMinor("Ruby");

    else if(suffix == "psql" || suffix == "SQL" || suffix == "sql")
        set_majorMinor("SQL");

    else if(suffix == "csh" || suffix == "tcsh")
        set_majorMinor("C Shell");

    else if(suffix == "abap")
        set_majorMinor("ABAP");

    else if(suffix == "as")
        set_majorMinor("ActionScript");

    else if(suffix == "ada" || suffix == "adb" || suffix == "ads" || suffix == "pad")
        set_majorMinor("Ada");
    else if(suffix == "adso")
        set_majorMinor("ADSO/IDSM");

    else if(suffix == "ample" || suffix == "dofile" || suffix == "startup")
        set_majorMinor("AMPLE");

    else if(suffix == "asa" || suffix == "asp")
        set_majorMinor("ASP");

    else if(suffix == "asax" || suffix == "ascx" || suffix == "asmx" ||
            suffix == "aspx" || suffix == "config" || suffix == "master" ||
            suffix == "sitemap" || suffix == "webinfo")
        set_majorMinor("ASP.Net");

    else if(suffix == "asm" || suffix == "S" || suffix == "s")
        set_majorMinor("Assembly");

    else if(suffix == "ahk")
        set_majorMinor("AutoHotkey");

    else if(suffix == "awk")
        set_majorMinor("awk");

    else if(suffix == "bash")
        set_majorMinor("Bourne Again Shell");

    else if(suffix == "sh")
        set_majorMinor("Bourne Shell");

    else if(suffix == "cs")
        set_majorMinor("C#");

    else if(suffix == "ccs")
        set_majorMinor("CCS");

    else if(suffix == "clj")
        set_majorMinor("Clojure");

    else if(suffix == "cljs")
        set_majorMinor("ClojureScript");

    else if(suffix == "CMakeLists.txt")
        set_majorMinor("CMake");

    else if(suffix == "cbl" || suffix == "CBL" || suffix == "cob" || suffix == "COB")
        set_majorMinor("COBOL");

    else if(suffix == "coffee")
        set_majorMinor("CoffeeScript");

    else if(suffix == "cfm")
        set_majorMinor("ColdFusion");

    else if(suffix == "cfc")
        set_majorMinor("ColdFusion CFScript");

    else if(suffix == "css")
        set_majorMinor("CSS");

    else if(suffix == "d")
        set_majorMinor("D");

    else if(suffix == "da")
        set_majorMinor("DAL");

    else if(suffix == "dart")
        set_majorMinor("Dart");

    else if(suffix == "bat" || suffix == "BAT")
        set_majorMinor("DOS Batch");

    else if(suffix == "dtd")
        set_majorMinor("DTD");

    else if(suffix == "erl" || suffix == "hrl")
        set_majorMinor("Erlang");

    else if(suffix == "exp")
        set_majorMinor("Expect");

    else if(suffix == "focexec")
        set_majorMinor("Focus");

    else if(suffix == "F" || suffix == "f" || suffix == "f77" || suffix == "F77" || suffix == "pfo")
        set_majorMinor("Fortran 77");

    else if(suffix == "F90" || suffix == "f90")
        set_majorMinor("Fortran 90");

    else if(suffix == "F95" || suffix == "f95")
        set_majorMinor("Fortran 95");

    else if(suffix == "go")
        set_majorMinor("Go");

    else if(suffix == "groovy")
        set_majorMinor("Groovy");

    else if(suffix == "hs" || suffix == "lhs")
        set_majorMinor("Haskell");

    else if(suffix == "idl" /*|| suffix == "pro" */)
        set_majorMinor("IDL");

    else if(suffix == "jcl")
        set_majorMinor("JCL");

    else if(suffix == "jsp")
        set_majorMinor("JSP");

    else if(suffix == "ksc")
        set_majorMinor("Kermit");

    else if(suffix == "ksh")
        set_majorMinor("Korn Shell");

    else if(suffix == "l")
        set_majorMinor("lex");

    else if(suffix == "cl")
        set_majorMinor("Lisp/OpenCL");

    else if(suffix == "oscript")
        set_majorMinor("LiveLink OScript");

    else if(suffix == "lua")
        set_majorMinor("Lua");

    else if(suffix == "ac" || suffix == "m4")
        set_majorMinor("m4");

    else if(suffix == "am" || suffix == "gnumakefile" || suffix == "Gnumakefile"
            || suffix == "Makefile" || suffix == "makefile")
        set_majorMinor("make");

//              else if(suffix == "m")
//                  set_majorMinor("MATLAB");

    else if(suffix == "i3" || suffix == "ig" || suffix == "m3" || suffix == "mg")
        set_majorMinor("Modula3");

    else if(suffix == "csproj" || suffix == "wdproj")
        set_majorMinor("MSBuild scripts");

    else if(suffix == "mps" /*suffix == "m" */)
        set_majorMinor("MUMPS");

    else if(suffix == "mxml")
        set_majorMinor("MXML");

    else if(suffix == "build")
        set_majorMinor("NAnt scripts");

    else if(suffix == "dmap")
        set_majorMinor("NASTRAN DMAP");

    else if(suffix == "m")
        set_majorMinor("Objective C");

    else if(suffix == "mm")
        set_majorMinor("Objective C++");

    else if(suffix == "ml")
        set_majorMinor("Ocaml");

    else if(suffix == "fmt")
        set_majorMinor("Oracle Forms");

    else if(suffix == "rex")
        set_majorMinor("Oracle Reports");

    else if(suffix == "dpr" || suffix == "p" || suffix == "pas" || suffix == "pp")
        set_majorMinor("Pascal");

    else if(suffix == "pcl" || suffix == "ses")
        set_majorMinor("Patran Command Language");

    else if(suffix == "inc")
        set_majorMinor("PHP/Pascal");

    else if(suffix == "qml")
        set_majorMinor("QML");

    else if(suffix == "rexx")
        set_majorMinor("Rexx");

    else if(suffix == "rhtml")
        set_majorMinor("Ruby HTML");

    else if(suffix == "scala")
        set_majorMinor("Scala");

    else if(suffix == "sed")
        set_majorMinor("sed");

    else if(suffix == "il")
        set_majorMinor("SKILL");

    else if(suffix == "ils")
        set_majorMinor("SKILL++");

    else if(suffix == "smarty" || suffix == "tpl")
        set_majorMinor("Smarty");

    else if(suffix == "sbl" || suffix == "SBL")
        set_majorMinor("Softbridge Basic");

    else if(suffix == "data.sql")
        set_majorMinor("SQL Data");

    else if(suffix == "spc.sql" || suffix == "spoc.sql" || suffix == "sproc.sql" || suffix == "udf.sql")
        set_majorMinor("SQL Stored Procedure");

    else if(suffix == "itk" || suffix == "tcl" || suffix == "tk")
        set_majorMinor("Tcl/Tk");

    else if(suffix == "def")
        set_majorMinor("Teamcenter def");

    else if(suffix == "met")
        set_majorMinor("Teamcenter met");

    else if(suffix == "mth")
        set_majorMinor("Teamcenter mth");

    else if(suffix == "vhd" || suffix == "VHD" || suffix == "VHDL" || suffix == "vhdl")
        set_majorMinor("VHDL");

    else if(suffix == "vim")
        set_majorMinor("vim script");

    else if(suffix == "bas" || suffix == "cls" || suffix == "ctl" || suffix == "dsr"
            || suffix == "frm" || suffix == "vb" || suffix == "VB" || suffix == "vba"
            || suffix == "VBA" || suffix == "vbs" || suffix == "VBS")
        set_majorMinor("Visual Basic");

    else if(suffix == "xaml")
        set_majorMinor("XAML");

    else if(suffix == "xsd" || suffix == "XSD")
        set_majorMinor("XSD");

    else if(suffix == "xsl" || suffix == "XSL" || suffix == "xslt" || suffix == "XSLT")
        set_majorMinor("XSLT");

    else if(suffix == "y")
        set_majorMinor("yacc");

    else if(suffix == "yaml" || suffix == "yml")
        set_majorMinor("YAML");

}
