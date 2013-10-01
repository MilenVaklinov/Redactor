#include "mainwindow.h"

#include "Buffer/texteditor.h"
#include "Counter/countingsourcelineswidget.h"
#include "Dired/dired.h"
#include "Editor/mytextedit.h"
#include "enums.h"
#include "Shell/Shell.h"
#include "Minibuffer/minibuffer.h"

#include <QAction>
#include <QApplication>
#include <QAbstractTextDocumentLayout>
#include <QCloseEvent>
#include <QCompleter>
#include <QDate>
#include <QDebug>
#include <QMenuBar>
#include <QMessageBox>
#include <QMenu>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QScrollBar>
#include <QShortcut>
#include <QStatusBar>
#include <QSettings>
#include <QSplitter>
#include <QTemporaryFile>
#include <QToolButton>
#include <QTextCursor>
#include <QTextBlock>
#include <QToolBar>
#include <QTimer>
#include <QTabBar>

#include <QLabel>
#include <QFileDialog>
#include <QPainter>
#include <QUrl>
#include <QVBoxLayout>


MainWindow::MainWindow()
{
    init_variables();

    create_mdiArea();
    create_statusbar();
    create_actions();
    create_shortcuts();
    create_menus();
    create_toolbars();

    read_settings();
    update_menus();

    setWindowTitle(tr("Redactor"));

    create_start_page();
}

MainWindow::~MainWindow()
{

}

//==========================================================================================

void MainWindow::init_variables()
{
    // Int variables
    current_line = 0;

    // Pointer variables
    buffer_for_deletion = 0;
    buffer_to_receive_focus = 0;

    // Bool variables
    is_minibuffer_busy = false;
    is_focus_on_minibuffer = false;
    is_last_buffer = false;

    // String variables
    text_of_minibuffer = "";
    text_of_minibuffer_mode = "";
}

//==========================================================================================
// MdiArea
//==========================================================================================

void MainWindow::create_mdiArea()
{
    mdiArea = new QMdiArea;
    mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mdiArea->setActivationOrder(QMdiArea::ActivationHistoryOrder);

    setCentralWidget(mdiArea);
}

//==========================================================================================
// M-x new .................. Create new empty text file named document[Number].txt
//==========================================================================================

void MainWindow::new_buffer()
{   
    QSplitter *splitter = create_splitter();

    QSplitter *new_splitter = new QSplitter;
    new_splitter->setChildrenCollapsible(false);
    new_splitter->setObjectName("Main Second Splitter");

    QWidget *container = new QWidget;
    QVBoxLayout *container_layout = new QVBoxLayout;
    container_layout->addWidget(new_splitter);
    container_layout->setContentsMargins(0, 0, 0, 0);
    container_layout->setSpacing(0);
    container->setLayout(container_layout);

    TextEditor *editor = create_text_editor();
    new_splitter->addWidget(editor);

    editor->newFile();
    editor->createModeLine();
    editor->change_the_highlighter();

    splitter->addWidget(container);

    editor->textEdit->setFocus();

    update_menus();

    emit new_text_for_messages(tr("Created a new buffer `M-x new' ....... [successful]"));
}

//==========================================================================================
// Create a pointer to new MdiChild object
//==========================================================================================

TextEditor *MainWindow::create_text_editor()
{
    TextEditor *editor = new TextEditor;
    editor->setWindowFlags(Qt::CustomizeWindowHint);

    // |Basic|
    connect(editor->textEdit, SIGNAL(mark_text(sQ::Mark)), this, SLOT(set_mark_text(sQ::Mark)));

    // Buffers    
    connect(editor->textEdit, SIGNAL(pushed_ctrl_x_ctrl_b()), this, SLOT(list_buffers()));
    connect(editor->textEdit, SIGNAL(pushedCtrlxb()), this, SLOT(set_switch_to_buffer_mode()));
    connect(editor->textEdit, SIGNAL(pushedCtrlxCtrlq()), this, SLOT(toggle_read_only()));
    connect(editor->textEdit, SIGNAL(pushedCtrlx4b()), this, SLOT(set_switch_to_buffer_other_window_mode()));
    connect(editor->textEdit, SIGNAL(pushedCtrlxk()), this, SLOT(kill_buffer()));

    // Windows
    connect(editor->textEdit, SIGNAL(pushedCtrlxo()), this, SLOT(other_window()));
    connect(editor->textEdit, SIGNAL(pushedCtrlx0()), this, SLOT(delete_window()));
    connect(editor->textEdit, SIGNAL(pushedCtrlx1()), this, SLOT(delete_other_windows()));
    connect(editor->textEdit, SIGNAL(pushedCtrlx2()), this, SLOT(split_window_below()));
    connect(editor->textEdit, SIGNAL(pushedCtrlx3()), this, SLOT(split_window_right()));
    connect(editor->textEdit, SIGNAL(pushedCtrlx40()), this, SLOT(kill_buffer_and_window()));
    connect(editor->textEdit, SIGNAL(pushedCtrlxBraceRight()), this, SLOT(enlarge_window_horizontally()));
    connect(editor->textEdit, SIGNAL(pushedCtrlxBraceLeft()), this, SLOT(shrink_window_horizontally()));
    connect(editor->textEdit, SIGNAL(pushedCtrlxCircum()), this, SLOT(enlarge_window()));
    connect(editor->textEdit, SIGNAL(pushed_ctrl_x_right()), this, SLOT(next_buffer()));
    connect(editor->textEdit, SIGNAL(pushed_ctrl_x_left()), this, SLOT(previous_buffer()));

    // Saving Files
    connect(editor->textEdit, SIGNAL(pushedCtrlxCtrls()), this, SLOT(save_buffer()));
    connect(editor->textEdit, SIGNAL(pushedCtrlxs()), this, SLOT(save_some_buffers()));
    connect(editor->textEdit, SIGNAL(pushedCtrlxCtrlw()), this, SLOT(set_write_file_mode()));
    connect(editor->textEdit, SIGNAL(pushedCtrlxi()), this, SLOT(set_insert_file_mode()));

    // Visiting Files
    connect(editor->textEdit, SIGNAL(pushedCtrlxCtrlf()), this, SLOT(set_find_file_mode()));
    connect(editor->textEdit, SIGNAL(pushedCtrlxCtrlr()), this, SLOT(set_find_file_read_only_mode()));
    connect(editor->textEdit, SIGNAL(pushedCtrlxCtrlv()), this, SLOT(set_find_alternate_file_mode()));
    connect(editor->textEdit, SIGNAL(pushedCtrlx4f()), this, SLOT(set_find_file_other_window_mode()));

    // A change in a buffer
    connect(editor->textEdit, SIGNAL(pushedAltTilda()), this, SLOT(not_modified()));
    connect(editor->textEdit, SIGNAL(pushedCtrluAltTilda()), this, SLOT(set_modified()));
    connect(editor->textEdit->document(), SIGNAL(contentsChanged()), this, SLOT(update_menus()));
    connect(editor->textEdit, SIGNAL(copyAvailable(bool)),  cutAct, SLOT(setEnabled(bool)));
    connect(editor->textEdit, SIGNAL(copyAvailable(bool)), copyAct, SLOT(setEnabled(bool)));

    // Exit and Minimize
    connect(editor->textEdit, SIGNAL(pushed_ctrl_x_ctrl_c()), qApp, SLOT(closeAllWindows()));
    connect(editor->textEdit, SIGNAL(pushedCtrlz()), this, SLOT(minimize_redactor()));

    // Communication between Editor/mytextedit.cpp and mainwindow.cpp
    connect(this, SIGNAL(text_for_editor(QString,QString)), editor->textEdit, SLOT(text_from_mainwindow(QString, QString)));
    connect(editor->textEdit, SIGNAL(text_for_mainwindow(QString,QString)), this, SLOT(text_from_editor(QString,QString)));

    // Others
    connect(editor->textEdit, SIGNAL(zap_to_char_finish_signal()), this, SLOT(zap_to_char_finish_slot()));

    return editor;
}

//==========================================================================================
//
//==========================================================================================

void MainWindow::text_from_editor(const QString & given_text, const QString &command)
{
    if (command == "set_focus_while_in_minibuffer") {
        set_focus_while_in_minibuffer();
    } else if (command == "set_start_of_buffer_text") {
        set_start_of_buffer_text();
    } else if (command == "set_end_of_buffer_text") {
        set_end_of_buffer_text();
    } else if (command == "set_zap_text") {
        set_zap_text();
    } else if (command == "set_fixit_text") {
        set_fixit_text();
    } else if (command == "set_read_only_message") {
        set_read_only_message();
    } else if (command == "text_from_editor") {

        mode_minibuffer->setText(given_text);
        mode_minibuffer->setStyleSheet("color:black;");

    } else if (command == "set_search_forward_mode") {
        set_search_forward_mode();
    } else if (command == "set_search_backward_mode") {
        set_search_backward_mode();
    } else if (command == "set_search_forward_regexp_mode") {
        set_search_forward_regexp_mode();
    } else if (command == "set_search_backward_regexp_mode") {
        set_search_backward_regexp_mode();
    } else if (command == "set_goto_line_mode") {
        set_goto_line_mode();
    } else
        return;
}

//==========================================================================================
// Return the current text editor if there is one
//==========================================================================================

TextEditor *MainWindow::current_buffer()
{
    if (mdiArea->currentSubWindow())
        mdiArea->currentSubWindow()->setFocus();

    QSplitter *splitter = current_splitter();
    if (splitter) {
        QList<TextEditor *> all = splitter->findChildren<TextEditor *>();
        for (int i = 0; i != all.count(); ++i) {
            if (all.at(i)->textEdit->hasFocus())
                return all.at(i);
        }
    }

    return 0;
}

//==========================================================================================
// Create and return a splitter object
//==========================================================================================

QSplitter* MainWindow::create_splitter()
{
    QSplitter *splitter = new QSplitter;

    mdiArea->addSubWindow(splitter, Qt::FramelessWindowHint | Qt::CustomizeWindowHint);
    splitter->setChildrenCollapsible(false);

    return splitter;
}

//==========================================================================================
// Return the current splitter if there is one
//==========================================================================================

QSplitter* MainWindow::current_splitter()
{
    QMdiSubWindow *subWindow = mdiArea->currentSubWindow();
    if (subWindow) {
        QSplitter *splitter = qobject_cast<QSplitter *>(subWindow->widget());
        if (splitter) {
            return splitter;
        }
    }

    return 0;
}

//==========================================================================================
//
//
//                        Graphical User Interface of Redactor
//
//
//==========================================================================================

//==========================================================================================
// Open a given file
//==========================================================================================

void MainWindow::open()
{
    QString fileName = QFileDialog::getOpenFileName(this);

    if (!fileName.isEmpty()) {

        QList<TextEditor *> buffers = all_buffers();

        for (int i = 0; i != buffers.size(); ++i) {
            if (buffers.at(i)->currentFilePath() == fileName) {
                buffers.at(i)->textEdit->setFocus();
                return;
            }
        }

        QSplitter *splitter = current_splitter();

        QList<int> new_sizes;
        QList<int> splitter_sizes = splitter->sizes();
        int half_of_the_current_size = splitter_sizes.at(0) / 2;
        new_sizes << half_of_the_current_size << half_of_the_current_size;

        QSplitter *new_splitter = new QSplitter;
        new_splitter->setChildrenCollapsible(false);
        new_splitter->setObjectName("Main Second Splitter");

        QWidget *container = new QWidget;
        QVBoxLayout *container_layout = new QVBoxLayout;
        container_layout->addWidget(new_splitter);
        container_layout->setContentsMargins(0, 0, 0, 0);
        container_layout->setSpacing(0);
        container->setLayout(container_layout);

        TextEditor *editor = create_text_editor();
        new_splitter->addWidget(editor);

        editor->loadFile(fileName);
        editor->createModeLine();
        editor->change_the_highlighter();

        splitter->addWidget(container);
        splitter->setSizes(new_sizes);

        editor->textEdit->setFocus();

        update_menus();

    }
}

//==========================================================================================

void MainWindow::save()
{
    if (current_buffer() && current_buffer()->save())
        return;
}

//==========================================================================================

void MainWindow::saveAs()
{
    if (current_buffer() && current_buffer()->saveAs())
        return;
}

//==========================================================================================

void MainWindow::cut()
{
    if (current_buffer())
        current_buffer()->textEdit->cut();
}

//==========================================================================================

void MainWindow::copy()
{
    if (current_buffer())
        current_buffer()->textEdit->copy();
}

//==========================================================================================

void MainWindow::paste()
{
    if (current_buffer())
        current_buffer()->textEdit->paste();
}

//==========================================================================================

void MainWindow::about()
{
   QMessageBox::about(this, tr("About sQuezzle"),
            tr("The <b>sQuezzle</b> editor tries to implement the basic "
               "functionality of emacs using using C++ and Qt."));
}

//==========================================================================================

void MainWindow::contact()
{
    QMessageBox::about(this, tr("Contact"),
           tr("<b>Author:</b> <i>Milen Vaklinov</i> <b>Country:</b> <i>Bulgaria</i> "
              "<b>City:</b> <i>Plovdiv</i> \n"
              "<b>Url:</b> <i>www.sQuezzle.org</i> \n"));
}

//==========================================================================================
//  Update all GUI menus and buttons
//==========================================================================================

void MainWindow::update_menus()
{
    TextEditor *buffer = current_buffer();

    if (!buffer) {

        saveAct->setEnabled(false);
        saveAsAct->setEnabled(false);
        pasteAct->setEnabled(false);
        cutAct->setEnabled(false);
        copyAct->setEnabled(false);

        return;
    }

    bool has_modified_buffer = buffer->textEdit->document()->isModified();
    bool has_editable_buffer = !buffer->textEdit->isReadOnly();

    saveAct->setEnabled(has_modified_buffer);
    saveAsAct->setEnabled(buffer);
    pasteAct->setEnabled(has_editable_buffer);

    bool has_selection = (buffer->textEdit->textCursor().hasSelection());

    cutAct->setEnabled(has_selection && has_editable_buffer);
    copyAct->setEnabled(has_selection && has_editable_buffer);
}

//==========================================================================================
// Create all actions
//==========================================================================================

void MainWindow::create_actions()
{
    // M-x new ................. Create new empty text file
    newAct = new QAction(QIcon(":/img/icons/new.png"), tr("New"), this);
    connect(newAct, SIGNAL(triggered()), this, SLOT(new_buffer()));

    //==========================================================================================
    // Gui Version -> Save, Save As... Open... and Quit actions from Toolbar
    //==========================================================================================

    openAct = new QAction(QIcon(":/img/icons/open.png"), tr("Open..."), this);
    connect(openAct, SIGNAL(triggered()), this, SLOT(open()));

    saveAct = new QAction(QIcon(":/img/icons/save.png"), tr("Save"), this);
    connect(saveAct, SIGNAL(triggered()), this, SLOT(save()));

    saveAsAct = new QAction(QIcon(":/img/icons/saveAs.png"), tr("Save As..."), this);
    connect(saveAsAct, SIGNAL(triggered()), this, SLOT(saveAs()));

    exitAct = new QAction(QIcon(":/img/icons/exit.png"), tr("Quit"), this);
    connect(exitAct, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));

    //==========================================================================================
    // Cut, Copy and Paste
    //==========================================================================================

    cutAct = new QAction(QIcon(":/img/icons/cut.png"), tr("Cut"), this);
    connect(cutAct, SIGNAL(triggered()), this, SLOT(cut()));

    copyAct = new QAction(QIcon(":/img/icons/copy.png"), tr("Copy"), this);
    connect(copyAct, SIGNAL(triggered()), this, SLOT(copy()));

    pasteAct = new QAction(QIcon(":/img/icons/paste.png"), tr("Paste"), this);
    connect(pasteAct, SIGNAL(triggered()), this, SLOT(paste()));

    //==========================================================================================
    // About Redactor, About Qt, Contact Information
    //==========================================================================================

    aboutAct = new QAction(QIcon(":/img/icons/info.png"), tr("&About"), this);
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

    contactAct = new QAction(QIcon(":/img/icons/contact.png"), tr("Contact"), this);
    contactAct->setToolTip(tr("Contact"));
    connect(contactAct, SIGNAL(triggered()), this, SLOT(contact()));

    aboutQtAct = new QAction(tr("About &Qt"), this);
    connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
}

//==========================================================================================
// Create the Shortcuts
//==========================================================================================

void MainWindow::create_shortcuts()
{
    // Go to Esc mode
    QShortcut *escapeAct = new QShortcut(QKeySequence(Qt::Key_Escape, Qt::Key_Escape, Qt::Key_Escape), this);
    connect(escapeAct, SIGNAL(activated()), this, SLOT(escape_minibuffer()));

    // Go to M-x mode
    QShortcut *focus_minibuffer_shortcut = new QShortcut(tr("Alt+x"), this);
    connect(focus_minibuffer_shortcut, SIGNAL(activated()), this, SLOT(focus_on_minibuffer()));

    // C-g ..................... Discard a command that you do not want to
    QShortcut *clear_statusbar = new QShortcut(tr("Ctrl+g"), this);
    connect(clear_statusbar, SIGNAL(activated()), this, SLOT(escape_minibuffer()));
}

//==========================================================================================
// Create the Menus
//==========================================================================================

void MainWindow::create_menus()
{
    fileMenu = menuBar()->addMenu(tr("File"));
    fileMenu->addAction(newAct);
    fileMenu->addAction(openAct);
    fileMenu->addAction(saveAct);
    fileMenu->addAction(saveAsAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    editMenu = menuBar()->addMenu(tr("Edit"));
    editMenu->addAction(cutAct);
    editMenu->addAction(copyAct);
    editMenu->addAction(pasteAct);

    viewMenu = menuBar()->addMenu(tr("View"));

    settingsMenu = menuBar()->addMenu(tr("Settings"));

    menuBar()->addSeparator();

    helpMenu = menuBar()->addMenu(tr("Help"));
    helpMenu->addAction(aboutAct);
    helpMenu->addAction(contactAct);
    helpMenu->addAction(aboutQtAct);
}

//==========================================================================================
// Create the Toolbars
//==========================================================================================

void MainWindow::create_toolbars()
{
    fileToolBar = addToolBar(tr("File ToolBar"));
    fileToolBar->setObjectName("fileToolBar");

    fileToolBar->addAction(newAct);
    fileToolBar->addAction(openAct);
    fileToolBar->addAction(saveAct);
    viewMenu->addAction(fileToolBar->toggleViewAction());


    editToolBar = addToolBar(tr("Edit ToolBar"));
    editToolBar->setObjectName("editToolBar");
    editToolBar->addAction(cutAct);
    editToolBar->addAction(copyAct);
    editToolBar->addAction(pasteAct);
    viewMenu->addAction(editToolBar->toggleViewAction());
}

//==========================================================================================
// Create the StatusBar
//==========================================================================================

void MainWindow::create_statusbar()
{
    QFont font("DejaVu Sans Mono", 10);

    mode_minibuffer = new QLabel;
    mode_minibuffer->setFont(font);

    minibuffer = new Minibuffer;

    connect(minibuffer, SIGNAL(returnPressed()), this, SLOT(event_from_minibuffer()));

    statusBar()->setStyleSheet("background-color: #FFFFFF;");
    statusBar()->addPermanentWidget(mode_minibuffer);
    statusBar()->addPermanentWidget(minibuffer, 1);

    update_minibuffer();
}

//==========================================================================================
//
//
//                              Command Mode
//
//
//==========================================================================================

//==========================================================================================
// Focus on minibuffer every time we press M-x
//==========================================================================================

void MainWindow::focus_on_minibuffer()
{
    mode_minibuffer->clear();
    minibuffer->clear();

    if (is_minibuffer_busy || is_focus_on_minibuffer) {

        text_of_minibuffer_mode = mode_minibuffer->text();
        text_of_minibuffer = minibuffer->text();

        minibuffer->setReadOnly(true);

        mode_minibuffer->setText(tr("Command attempted to use minibuffer while in minibuffer."));
        mode_minibuffer->setStyleSheet("color:black;");

        QTimer::singleShot(1500, mode_minibuffer, SLOT(clear()));
        QTimer::singleShot(1501, this, SLOT(set_old_text()));

        return;

    } else {

        is_focus_on_minibuffer = true;
        emit text_for_editor("is_busy", "set_minibuffer");

        mode_minibuffer->setText("M-x");
        mode_minibuffer->setStyleSheet("color: blue;");

        minibuffer->setReadOnly(false);
        minibuffer->setFocus();
    }
}

//==========================================================================================
// Escape from minibiffer focus by typing Esc-Esc-Esc
//==========================================================================================

void MainWindow::escape_minibuffer()
{
    if (in_loop_exec) {
        minibuffer->set_the_string("escape");
        in_loop_exec = false;
        emit ready_to_exit();
    }

    if (minibuffer->hasFocus() || is_minibuffer_busy) {

        is_minibuffer_busy = false;
        is_focus_on_minibuffer = false;
        emit text_for_editor("not_busy", "set_minibuffer");

        minibuffer->clear();
        mode_minibuffer->clear();

        mode_minibuffer->setText(tr("Quit"));
        mode_minibuffer->setStyleSheet("color: black;");

        minibuffer->setReadOnly(true);

        if(mdiArea->activeSubWindow())
            mdiArea->activeSubWindow()->setFocus();

        return;

    } else
        emit text_for_editor("", "reset_variables");
}

//==========================================================================================
//
//==========================================================================================

void MainWindow::set_focus_while_in_minibuffer()
{
    text_of_minibuffer_mode = mode_minibuffer->text();
    text_of_minibuffer = minibuffer->text();

    mode_minibuffer->clear();
    minibuffer->clear();

    minibuffer->setReadOnly(true);

    mode_minibuffer->setText(tr("You are in minibuffer mode."));
    mode_minibuffer->setStyleSheet("color:black;");

    QTimer::singleShot(1500, mode_minibuffer, SLOT(clear()));
    QTimer::singleShot(1501, this, SLOT(set_old_text()));
}

//==========================================================================================
//
//==========================================================================================

void MainWindow::set_old_text()
{
    mode_minibuffer->setText(text_of_minibuffer_mode);
    mode_minibuffer->setStyleSheet("color:blue;");

    if (!text_of_minibuffer.isEmpty())
        minibuffer->setText(text_of_minibuffer);

    minibuffer->setReadOnly(false);
    minibuffer->setFocus();
}

//==========================================================================================
//
//==========================================================================================

void MainWindow::commandNotFound()
{
    minibuffer->setText("Command not found!");
    QTimer::singleShot(1000, minibuffer, SLOT(clear()));
}

//==========================================================================================
//
//==========================================================================================

void MainWindow::set_read_only_message()
{
    mode_minibuffer->clear();
    minibuffer->clear();

    TextEditor *buffer = current_buffer();

    QString text = "Buffer is read only: #<buffer " + buffer->currentFile() + ">";
    mode_minibuffer->setText(text);
    mode_minibuffer->setStyleSheet("color:black;");
}

//==========================================================================================
// Update the status bar by cleaning the content
//==========================================================================================

void MainWindow::update_minibuffer()
{
    mode_minibuffer->clear();
    minibuffer->clear();

    minibuffer->setReadOnly(true);
    minibuffer->clearFocus();

    if(mdiArea->activeSubWindow())
        mdiArea->activeSubWindow()->setFocus();
}

//==========================================================================================
// Check whether the file is write protected and update the minibuffer
//==========================================================================================

void MainWindow::is_writable(const QString& path)
{
    if(!QFileInfo(path).isWritable()) {
        mode_minibuffer->setText(tr("Note: file is write protected"));
        mode_minibuffer->setStyleSheet("color:black");

        if(mdiArea->activeSubWindow())
            mdiArea->activeSubWindow()->setFocus();

        TextEditor *editor = current_buffer();
        QString currentMode = editor->chFr->text();

        if(editor) {
            editor->textEdit->setReadOnly(true);
            editor->textEdit->setTextInteractionFlags(editor->textEdit->textInteractionFlags() | Qt::TextSelectableByKeyboard);

            if(editor->textEdit->document()->isModified())
                editor->chFr->setText(currentMode.replace(0, 1, "%"));
            else
                editor->chFr->setText(currentMode.replace(0, 2, "%%"));
        } else
            return;
    }
}

//==========================================================================================
// "M-x" commands from the minibuffer
//==========================================================================================

void MainWindow::event_from_minibuffer()
{
    QString command_string = minibuffer->text().trimmed();
    QStringList listCommand = command_string.split(" ", QString::SkipEmptyParts);

    if(listCommand.isEmpty()) {
        minibuffer->setText(tr("No command name given"));
        QTimer::singleShot(1000, minibuffer, SLOT(clear()));
    }

    else if(mode_minibuffer->text() == "Find file:") {
        update_minibuffer();
        if(!listCommand.at(0).isEmpty()) {
            is_minibuffer_busy = false;
            emit text_for_editor("not_busy", "set_minibuffer");
            find_file(listCommand.at(0));
            return;
        }
        else {
            find_file("");
            return;
        }
    }

    else if(mode_minibuffer->text() == "Find file read only:") {
        update_minibuffer();
        if(!listCommand.at(0).isEmpty()) {
            is_minibuffer_busy = false;
            emit text_for_editor("not_busy", "set_minibuffer");
            find_file_read_only(listCommand.at(0));
            return;
        }
        else {
            find_file_read_only("");
            return;
        }
    }

    else if(mode_minibuffer->text() == "Find alternate file:") {
        update_minibuffer();
        if(!listCommand.at(0).isEmpty()) {
            is_minibuffer_busy = false;
            emit text_for_editor("not_busy", "set_minibuffer");
            find_alternate_file(listCommand.at(0));
            return;
        }
    }

    else if(mode_minibuffer->text() == "Find file in other window:") {
        update_minibuffer();
        if(!listCommand.at(0).isEmpty()) {
            is_minibuffer_busy = false;
            emit text_for_editor("not_busy", "set_minibuffer");
            find_file_other_window(listCommand.at(0));
            return;
        }
    }

    else if(mode_minibuffer->text() == "Write file:") {
        if(!listCommand.at(0).isEmpty()) {
            is_minibuffer_busy = false;
            emit text_for_editor("not_busy", "set_minibuffer");
            write_file(listCommand.at(0));
            return;
        }
    }

    else if (mode_minibuffer->text().startsWith("[Closing window...] Save file ")) {

        update_minibuffer();

        if (!listCommand.at(0).isEmpty()) {

            is_minibuffer_busy = false;
            emit text_for_editor("not_busy", "set_minibuffer");

            QRegExp rx("y|n|\\!|\\.");

            if (rx.exactMatch(listCommand.at(0))) {
                emit ready_to_exit();
                in_loop_exec = false;
                minibuffer->set_the_string(listCommand.at(0));
            } else if (listCommand.at(0) == "help") {

                add_help_window(sQ::ClosingWindow);

                emit ready_to_exit();
                in_loop_exec = false;
                set_kill_window_mode();

            } else {
                emit ready_to_exit();
                in_loop_exec = false;
                set_kill_window_mode();
            }
        }
    }

    else if (mode_minibuffer->text().endsWith("? (y, n, !, ., help)")) {
        update_minibuffer();

        if (!listCommand.at(0).isEmpty()) {
            is_minibuffer_busy = false;
            emit text_for_editor("not_busy", "set_minibuffer");

            QRegExp rx("y|n|\\!|\\.|help");

            if (rx.exactMatch(listCommand.at(0)))
                save_buffers_from_list(listCommand.at(0));
            else
                set_save_some_buffers_mode();
        }
    }

    else if (mode_minibuffer->text().endsWith("? (y, n, !, ., q, help)")) {
        update_minibuffer();

        if (!listCommand.at(0).isEmpty()) {

            is_minibuffer_busy = false;
            emit text_for_editor("not_busy", "set_minibuffer");

            QRegExp rx("y|n|\\!|\\.|q");

            if (rx.exactMatch(listCommand.at(0))) {
                emit ready_to_exit();
                in_loop_exec = false;
                minibuffer->set_the_string(listCommand.at(0));
            } else if (listCommand.at(0) == "help") {
                emit ready_to_exit();
                in_loop_exec = false;
                minibuffer->set_the_string(listCommand.at(0));
            } else {
                emit ready_to_exit();
                in_loop_exec = false;
                set_exit_mode();
            }
        }
    }

    else if (mode_minibuffer->text().contains(" is modified: save it first? (yes or no)")) {
        update_minibuffer();
        if (!listCommand.at(0).isEmpty()) {

            is_minibuffer_busy = false;
            emit text_for_editor("not_busy", "set_minibuffer");

            QRegExp rx ("yes|no");

            if (rx.exactMatch(listCommand.at(0)))
                save_buffer_to_disk(listCommand.at(0));
            else
                set_ask_to_save_buffer_mode();
        }
    }

    else if (mode_minibuffer->text().contains("Modified buffers exist; exit anyway? (yes or no)")) {
        update_minibuffer();
        if (!listCommand.at(0).isEmpty()) {

            is_minibuffer_busy = false;
            emit text_for_editor("not_busy", "set_minibuffer");

            QRegExp rx("yes|no");

            if (rx.exactMatch(listCommand.at(0))) {
                emit ready_to_exit();
                in_loop_exec = false;
                minibuffer->set_the_string(listCommand.at(0));
            } else {
                emit ready_to_exit();
                in_loop_exec = false;
                set_modified_buffers_exist_mode();
            }
        }
    }

    else if (mode_minibuffer->text().contains("Modified buffers exist; kill anyway? (yes or no)")) {

        update_minibuffer();

        if (!listCommand.at(0).isEmpty()) {

            is_minibuffer_busy = false;
            emit text_for_editor("not_busy", "set_minibuffer");

            QRegExp rx("yes|no");

            if (rx.exactMatch(listCommand.at(0))) {
                emit ready_to_exit();
                in_loop_exec = false;
                minibuffer->set_the_string(listCommand.at(0));
            } else {
                emit ready_to_exit();
                in_loop_exec = false;
                set_check_before_killing_mode();
            }
        }
    }

    else if (listCommand.at(0).toLower() == "set-visited-file-name") {
        update_minibuffer();
        set_visited_file_name_mode();
    }

    else if (mode_minibuffer->text().contains("Set visited file name:")) {
        update_minibuffer();
        if (!listCommand.at(0).isEmpty()) {

            is_minibuffer_busy = false;
            emit text_for_editor("not_busy", "set_minibuffer");

            set_visited_file_name(listCommand.at(0));
            return;
        }
    }

    else if (listCommand.at(0).toLower() == "rename-buffer") {
        update_minibuffer();
        set_rename_buffer_mode();
        return;
    }

    else if (mode_minibuffer->text().contains("Rename buffer (to new name):")) {
        update_minibuffer();
        if (!listCommand.at(0).isEmpty())
            rename_buffer(listCommand.at(0));
    }

    // This is the same as the above part but from *Buffer List*
    else if (mode_minibuffer->text().contains("Rename buffer:")) {
        update_minibuffer();
        if (!listCommand.at(0).isEmpty()) {

            is_minibuffer_busy = false;
            emit text_for_editor("not_busy", "set_minibuffer");

            minibuffer->set_the_string(listCommand.at(0));

            rename_the_buffer_from_buffer_list();
        }
    }

    else if (mode_minibuffer->text().contains("Switch to buffer:")) {
        update_minibuffer();
        if (!listCommand.at(0).isEmpty()) {

            is_minibuffer_busy = false;
            emit text_for_editor("not_busy", "set_minibuffer");

            switch_to_buffer(command_string);
        }
    }

    else if (mode_minibuffer->text().contains("Switch to buffer in other window:")) {
        update_minibuffer();
        if (!listCommand.at(0).isEmpty()) {

            is_minibuffer_busy = false;
            emit text_for_editor("not_busy", "set_minibuffer");

            switch_to_buffer_other_window(listCommand.at(0));
        }
    }

    else if(listCommand.at(0).toLower() == "append-to-buffer") {
        update_minibuffer();
        set_append_to_buffer_mode();
        return;
    }

    else if (mode_minibuffer->text().contains("Append to buffer:")) {
        update_minibuffer();

        if (!listCommand.at(0).isEmpty()) {

            is_minibuffer_busy = false;
            emit text_for_editor("not_busy", "set_minibuffer");

            minibuffer->set_the_string(listCommand.at(0));
            append_to_buffer();
        }
    }

    else if(listCommand.at(0).toLower() == "append-to-file") {
        update_minibuffer();
        set_append_to_file_mode();
        return;
    }

    else if (mode_minibuffer->text().contains("Append to file:")) {
        update_minibuffer();
        if (!listCommand.at(0).isEmpty()) {

            is_minibuffer_busy = false;
            emit text_for_editor("not_busy", "set_minibuffer");

            minibuffer->set_the_string(listCommand.at(0));
            append_to_file();
        }
    }


    else if(listCommand.at(0).toLower() == "prepend-to-buffer") {
        update_minibuffer();
        set_prepend_to_buffer_mode();
        return;
    }

    else if (mode_minibuffer->text().contains("Prepend to buffer:")) {
        update_minibuffer();
        if (!listCommand.at(0).isEmpty()) {

            is_minibuffer_busy = false;
            emit text_for_editor("not_busy", "set_minibuffer");

            minibuffer->set_the_string(listCommand.at(0));
            prepend_to_buffer();
        }
    }

    else if(listCommand.at(0).toLower() == "copy-to-buffer") {
        update_minibuffer();
        set_copy_to_buffer_mode();
        return;
    }

    else if (mode_minibuffer->text().contains("Copy to buffer:")) {
        update_minibuffer();
        if (!listCommand.at(0).isEmpty()) {

            is_minibuffer_busy = false;
            emit text_for_editor("not_busy", "set_minibuffer");

            minibuffer->set_the_string(listCommand.at(0));
            copy_to_buffer();
        }
    }

    else if(listCommand.at(0).toLower() == "insert-buffer") {
        update_minibuffer();
        set_insert_buffer_mode();
        return;
    }

    else if (mode_minibuffer->text().contains("Insert buffer:")) {
        update_minibuffer();
        if (!listCommand.at(0).isEmpty()) {

            is_minibuffer_busy = false;
            emit text_for_editor("not_busy", "set_minibuffer");

            minibuffer->set_the_string(listCommand.at(0));
            insert_buffer();
        }
    }

    else if(listCommand.at(0).toLower() == "insert-to-buffer") {
        update_minibuffer();
        set_insert_to_buffer_mode();
        return;
    }

    else if (mode_minibuffer->text().contains("Insert to buffer:")) {
        update_minibuffer();
        if (!listCommand.at(0).isEmpty()) {

            is_minibuffer_busy = false;
            emit text_for_editor("not_busy", "set_minibuffer");

            minibuffer->set_the_string(listCommand.at(0));
            insert_to_buffer();
        }
    }

    else if (listCommand.at(0).toLower() == "insert-file") {
        update_minibuffer();
        set_insert_file_mode();
        return;
    }

    else if (mode_minibuffer->text() == "Insert file:") {
        update_minibuffer();
        if (!listCommand.at(0).isEmpty()) {

            is_minibuffer_busy = false;
            emit text_for_editor("not_busy", "set_minibuffer");

            minibuffer->set_the_string(listCommand.at(0));
            insert_file();
        }
    }

    else if(listCommand.at(0).toLower() == "search-forward") {
        update_minibuffer();
        emit text_for_editor("", "search_forward");
        return;
    }

    else if (mode_minibuffer->text().contains("Search forward:")) {
        update_minibuffer();

        if (!listCommand.isEmpty()) {

            is_minibuffer_busy = false;
            emit text_for_editor("not_busy", "set_minibuffer");

            emit text_for_editor(command_string, "search_forward_first_time");
        }
    }

    else if(listCommand.at(0).toLower() == "search-backward") {
        update_minibuffer();
        emit text_for_editor("", "search_backward");
        return;
    }

    else if (mode_minibuffer->text().contains("Search backward:")) {   
        update_minibuffer();

        if (!listCommand.isEmpty()) {

            is_minibuffer_busy = false;
            emit text_for_editor("not_busy", "set_minibuffer");

            emit text_for_editor(command_string, "search_backward_first_time");
        }
    }

    else if(listCommand.at(0).toLower() == "search-forward-regexp") {
        update_minibuffer();
        emit text_for_editor("", "search_forward_regexp");
        return;
    }

    else if (mode_minibuffer->text().contains("RE search:")) {
        update_minibuffer();

        if (!listCommand.isEmpty()) {

            is_minibuffer_busy = false;
            emit text_for_editor("not_busy", "set_minibuffer");

            emit text_for_editor(command_string, "search_forward_regexp_first_time");
        }
    }

    else if(listCommand.at(0).toLower() == "search-backward-regexp") {
        update_minibuffer();
        emit text_for_editor("", "search_backward_regexp");
        return;
    }

    else if (mode_minibuffer->text().contains("RE search backward:")) {
        update_minibuffer();

        if (!listCommand.isEmpty()) {

            is_minibuffer_busy = false;
            emit text_for_editor("not_busy", "set_minibuffer");

            emit text_for_editor(command_string, "search_backward_regexp_first_time");
        }
    }

    else if(listCommand.at(0).toLower() == "goto-line") {
        update_minibuffer();
        set_goto_line_mode();
        return;
    }

    else if (mode_minibuffer->text().contains("Goto line:")) {
        update_minibuffer();

        if (!listCommand.at(0).isEmpty()) {

            is_minibuffer_busy = false;
            emit text_for_editor("not_busy", "set_minibuffer");

            goto_line(listCommand.at(0));
        }
    }

    // =====================================================================
    // |Text|

    else if(listCommand.at(0).toLower() == "face") {
        update_minibuffer();
        set_face_mode(0);
        return;
    }

    else if (mode_minibuffer->text().contains("Foreground color:")) {
        update_minibuffer();
        if (!listCommand.isEmpty())
            minibuffer->set_the_string(listCommand.at(0));

        is_minibuffer_busy = false;
        emit text_for_editor("not_busy", "set_minibuffer");

        set_face_mode(1);
    }

    else if (mode_minibuffer->text().contains("Background color:")) {
        update_minibuffer();
        if (!listCommand.isEmpty()) {

            is_minibuffer_busy = false;
            emit text_for_editor("not_busy", "set_minibuffer");

            QString color_choices = minibuffer->get_the_string() + "-" + listCommand.at(0);
            emit text_for_editor(color_choices, "face");
        }
    }

    // Others

    else if(listCommand.at(0).toLower() == "buffer-menu") {
        update_minibuffer();
        list_buffers();
        if (!mdiArea->subWindowList().isEmpty()) {
            mode_minibuffer->setText(tr("Commands: d, s, x, u; f, o, 1, 2, m, v; ~, %; q to quit; ? for help."));
            mode_minibuffer->setStyleSheet("color:black;");
        } else {
            mode_minibuffer->setText(tr("The *Buffer List* is empty."));
            mode_minibuffer->setStyleSheet("color:black;");
        }
        return;
    }

    else if(listCommand.at(0).toLower() == "new") {

        update_minibuffer();

        is_minibuffer_busy = false;
        is_focus_on_minibuffer = false;
        emit text_for_editor("not_busy", "set_minibuffer");

        new_buffer();
        return;
    }

    else if(listCommand.at(0).toLower() == "clock-on") {
        update_minibuffer();

        is_minibuffer_busy = false;
        is_focus_on_minibuffer = false;
        emit text_for_editor("not_busy", "set_minibuffer");

        TextEditor *buffer = current_buffer();
        buffer->create_clock();

        return;
    }

    else if(listCommand.at(0).toLower() == "clock-off") {
        update_minibuffer();

        is_minibuffer_busy = false;
        is_focus_on_minibuffer = false;
        emit text_for_editor("not_busy", "set_minibuffer");

        TextEditor *buffer = current_buffer();
        buffer->delete_clock();

        return;
    }

    else if(listCommand.at(0).toLower() == "dired") {
        update_minibuffer();
        set_dired_mode();
        return;
    }

    else if(mode_minibuffer->text() == "Dired (directory):") {
        update_minibuffer();

        is_minibuffer_busy = false;
        is_focus_on_minibuffer = false;
        emit text_for_editor("not_busy", "set_minibuffer");

        if(!listCommand.at(0).isEmpty()) {
            create_dired(listCommand.at(0));
            return;
        }
    }

    else if(listCommand.at(0).toLower() == "counter") {
        update_minibuffer();
        set_counter_mode();
        return;
    }

    else if(mode_minibuffer->text() == "Counter (directory):") {
        update_minibuffer();
        if(!listCommand.at(0).isEmpty()) {

            is_minibuffer_busy = false;
            is_focus_on_minibuffer = false;
            emit text_for_editor("not_busy", "set_minibuffer");

            create_counter(listCommand.at(0));
            return;
        }
    }

    else if(listCommand.at(0).toLower() == "shell") {
        update_minibuffer();

        is_minibuffer_busy = false;
        is_focus_on_minibuffer = false;
        emit text_for_editor("not_busy", "set_minibuffer");

        create_shell();
        return;
    }

    else if(listCommand.at(0).toLower() == "redactor-tutorial") {
        update_minibuffer();

        is_minibuffer_busy = false;
        is_focus_on_minibuffer = false;
        emit text_for_editor("not_busy", "set_minibuffer");

        show_redactor_tutorial();
        return;
    }

    else if(listCommand.at(0).toLower() == "unix-tutorial") {
        update_minibuffer();

        is_minibuffer_busy = false;
        is_focus_on_minibuffer = false;
        emit text_for_editor("not_busy", "set_minibuffer");

        show_unix_tutorial();
        return;
    }

    else
        commandNotFound();
}

//==========================================================================================
//
//
//                          Windows
//
//
//==========================================================================================

//==========================================================================================
// C-x 0 ...................... Delete the selected window. The last character in this key
//                                                                       sequence is a zero.
//==========================================================================================

void MainWindow::delete_window()
{
    // We can delete buffers only from the current splitter/window.

    QSplitter *splitter = current_splitter();
    if (splitter) {

        // A list with all buffers
        QList<TextEditor *> all = splitter->findChildren<TextEditor *>();

        // We loop over all the buffers and append the visible ones in the all_visible_buffers list.
        QList<TextEditor *> visible_buffers;
        for (int x = 0; x != all.count(); ++x) {
            if (all.at(x)->isVisible()) {
                visible_buffers.append(all.at(x));
            }
        }

        // If we have only one visible buffer, Don't close it! ... [*]
        if (visible_buffers.count() > 1) {

            for (int i = 0 ; i != visible_buffers.count(); ++i) {

                if (!(visible_buffers.at(i) == current_buffer())) { }
                else {

                    // This is the splitter of the current buffer
                    QSplitter *current_splitter =
                            qobject_cast<QSplitter *>(visible_buffers.at(i)->parentWidget());

                    // This is the parent splitter of the current buffer's splitter
                    QSplitter *current_splitter_parrent_splitter =
                            qobject_cast<QSplitter *>((current_splitter->parentWidget())->parentWidget());

                    // Here we count all the visible buffers in the parent splitter of current splitter
                    int count = 0;
                    QList<TextEditor *> buffers_from_parent_splitter = current_splitter_parrent_splitter->findChildren<TextEditor *>();
                    for (int y = 0; y != buffers_from_parent_splitter.count(); ++y) {
                        if (buffers_from_parent_splitter.at(y)->isVisible())
                            count++;
                    }

                    // If the parent splitter has only one buffer in it, we enter here... [*]
                    if ( count == 1 ) {

                        // First we hide the current splitter
                        (current_splitter->parentWidget())->hide();

                        // After that, we check the parrent splitter of this splitter
                        // whether it is empty and if so, hide it too, and so on the loop
                        // continues until the parrent splitter is not empty
                        hide_one_level_up(current_splitter_parrent_splitter);

                        // We decrement i here, so we can set the focus to the next visible buffer
                        --i;

                        // If i >= 0, we move the focus to the next buffer
                        if (i >= 0) {
                            visible_buffers.at(i)->textEdit->setFocus();
                            return;
                        }

                        // If i < 0, we move the focus to the first buffer
                        if (i < 0) {
                            visible_buffers.at(visible_buffers.count()-1)->textEdit->setFocus();
                            return;
                        }
                    } else {
                        // [*] ... otherwise hide only the  current splitter
                        (current_splitter->parentWidget())->hide();
                        --i;

                        if (i >= 0) {
                            visible_buffers.at(i)->textEdit->setFocus();
                            return;
                        }

                        if (i < 0) {
                            visible_buffers.at(visible_buffers.count()-1)->textEdit->setFocus();
                            return;
                        }
                    }
                }
            }
        } else {
                // [*] ... otherwise show the following message
                minibuffer->clear();
                mode_minibuffer->clear();
                mode_minibuffer->setText(tr("Attempt to delete minibuffer or sole ordinary window"));
                mode_minibuffer->setStyleSheet("color: black;");
                return;
            }
        }
    }

//==========================================================================================
// Here we check whether the parent splitter of a given splitter is empty and if so hide it
// and using recursion check one more level up until the splitter is not empty
//==========================================================================================

void MainWindow::hide_one_level_up(QSplitter *given_splitter)
{
    if (given_splitter) {

        int count = 0;
        QList<TextEditor *> buffers = given_splitter->findChildren<TextEditor *>();
        for (int x = 0; x != buffers.count(); ++x) {
            if (buffers.at(x)->isVisible())
                count++;
        }

        if (count == 0){
            QSplitter *parent_splitter = qobject_cast<QSplitter *>((given_splitter->parentWidget())->parentWidget());
            QMdiSubWindow* subWindow = qobject_cast<QMdiSubWindow *>(given_splitter->parentWidget());

            // If we have subWindow then we have empty QMdiSubWindow, so we must remove it
            // otherwise we should hide the widget.
            if (subWindow) {
                mdiArea->removeSubWindow(subWindow);
                return;
            } else {

                given_splitter->parentWidget()->hide();

                if (parent_splitter)
                    hide_one_level_up(parent_splitter);
                else
                    return;
            }
        } else
            return;
    } else
        return;
}

//==========================================================================================
// Here we delete the buffer's splitter and its widget
//==========================================================================================

void MainWindow::delete_buffer_completely(QSplitter *given_splitter)
{
    if (given_splitter) {

        QWidget* widget_of_splitter = qobject_cast<QWidget *>(given_splitter->parentWidget());
        QSplitter* parent_splitter = qobject_cast<QSplitter *>(widget_of_splitter->parentWidget());

        delete given_splitter;

        if (widget_of_splitter)
            delete widget_of_splitter;

        if (parent_splitter)
            hide_one_level_up(parent_splitter);
        else
            return;
    } else
        return;
}

//==========================================================================================
// C-x 1 ................ Delete all windows in the selected frame except the selected window.
//==========================================================================================

void MainWindow::delete_other_windows()
{
    // We can hide buffers only from the current splitter/window
    QSplitter *splitter = current_splitter();
    TextEditor *active_buf = 0;
    if (splitter) {

        QList<TextEditor *> buffers = splitter->findChildren<TextEditor *>();

        if(!buffers.isEmpty()) {
            for (int x = 0; x != buffers.count(); ++x) {
                if (buffers.at(x) == current_buffer()) {
                    active_buf = buffers.at(x);
                    active_buf->textEdit->setFocus();
                }
                else {

                    QWidget *buffer_widget = qobject_cast<QWidget *>(buffers.at(x)->parentWidget()->parentWidget());
                    QSplitter *parent_splitter = qobject_cast<QSplitter *>(buffer_widget->parentWidget());

                    buffer_widget->hide();

                    hide_one_level_up(parent_splitter);
                }
            }

            active_buf->textEdit->setFocus();

        } else
            return;
    }
}

//==========================================================================================
// C-x 2 ................. Split the selected window into two windows, one above the other.
//==========================================================================================

void MainWindow::split_window_below()
{
    QSplitter *splitter = current_splitter();
    if (splitter) {

        QList<QSplitter *> all_splitters = splitter->findChildren<QSplitter *>();
        for (int x = 0; x != all_splitters.count(); ++x) {
            if ( check_this_splitter(all_splitters.at(x)) ) {

                // Every time when we split window, we first insert new splitter on the place of
                // the current text editor and after that we add this buffer to the new splitter.
                // In this way we can split this buffer without changing the whole structure
                TextEditor* buffer = return_current_buffer(all_splitters.at(x));

                // Here we take the current splitter size and divede it by two
                // so we are able to have the same height for the both parts of the splitter
                QList<int> new_sizes;
                QList<int> splitter_sizes = all_splitters.at(x)->sizes();
                int half_of_the_current_size = splitter_sizes.at(0) / 2;
                new_sizes << half_of_the_current_size << half_of_the_current_size;

                if (buffer) {
                    QSplitter *first_splitter = new QSplitter;
                    first_splitter->setChildrenCollapsible(false);

                    QWidget *first_container = new QWidget;
                    QVBoxLayout *first_container_layout = new QVBoxLayout;

                    first_container_layout->addWidget(first_splitter);
                    first_container_layout->setContentsMargins(0, 0, 0, 0);
                    first_container_layout->setSpacing(0);
                    first_container->setLayout(first_container_layout);

                    first_splitter->addWidget(buffer);
                    all_splitters.at(x)->addWidget(first_container);
                }

                all_splitters.at(x)->setOrientation(Qt::Vertical);

                QSplitter *second_splitter = new QSplitter;
                second_splitter->setChildrenCollapsible(false);

                QVBoxLayout *second_container_layout = new QVBoxLayout;
                second_container_layout->addWidget(second_splitter);
                second_container_layout->setContentsMargins(0, 0, 0, 0);
                second_container_layout->setSpacing(0);

                QWidget *second_container = new QWidget;
                second_container->setLayout(second_container_layout);

                TextEditor *editor = create_text_editor();
                second_splitter->addWidget(editor);

                editor->newFile();
                editor->createModeLine();
                editor->change_the_highlighter();

                all_splitters.at(x)->addWidget(second_container);

                // Here we set the new size manually
                all_splitters.at(x)->setSizes(new_sizes);

                editor->textEdit->setFocus();

                update_menus();

                return;
            }
        }
    }
}

//==========================================================================================
// C-x 3 .............. Split the selected window into two windows, positioned side by side.
//==========================================================================================

void MainWindow::split_window_right() {

    QSplitter *splitter = current_splitter();
    if (splitter) {

        QList<QSplitter *> all_splitters = splitter->findChildren<QSplitter *>();
        for (int x = 0; x != all_splitters.count(); ++x) {
            if ( check_this_splitter(all_splitters.at(x)) ) {

                TextEditor *buffer = return_current_buffer(all_splitters.at(x));

                // Here we take the current splitter size and divede it by two
                // so we are able to have the same height for the both parts of the splitter
                QList<int> new_sizes;
                QList<int> splitter_sizes = all_splitters.at(x)->sizes();
                int half_of_the_current_size = splitter_sizes.at(0) / 2;
                new_sizes << half_of_the_current_size << half_of_the_current_size;

                if (buffer) {
                    QSplitter *insert_splitter = new QSplitter;
                    insert_splitter->setChildrenCollapsible(false);

                    QWidget *new_container = new QWidget;
                    QVBoxLayout *new_container_layout = new QVBoxLayout;

                    new_container_layout->addWidget(insert_splitter);
                    new_container_layout->setContentsMargins(0, 0, 0, 0);
                    new_container_layout->setSpacing(0);
                    new_container->setLayout(new_container_layout);

                    insert_splitter->addWidget(buffer);
                    all_splitters.at(x)->addWidget(new_container);
                }

                all_splitters.at(x)->setOrientation(Qt::Horizontal);

                QSplitter *new_splitter = new QSplitter;
                new_splitter->setChildrenCollapsible(false);

                QWidget *container = new QWidget;
                QVBoxLayout *container_layout = new QVBoxLayout;

                container_layout->addWidget(new_splitter);
                container_layout->setContentsMargins(0, 0, 0, 0);
                container_layout->setSpacing(0);
                container->setLayout(container_layout);

                TextEditor *editor = create_text_editor();
                new_splitter->addWidget(editor);

                editor->newFile();
                editor->createModeLine();
                editor->change_the_highlighter();

                all_splitters.at(x)->addWidget(container);
                all_splitters.at(x)->setSizes(new_sizes);

                editor->textEdit->setFocus();

                update_menus();

                return;
            }
        }
    }
}

//==========================================================================================
// Help functions for C-x 2 and C-x 3 commands
//==========================================================================================

bool MainWindow::check_this_splitter(QSplitter *given_splitter)
{
    if (given_splitter) {
        QList<TextEditor *> buffers = given_splitter->findChildren<TextEditor *>();
        for (int x = 0; x != buffers.count(); ++x) {
            if (buffers.at(x)->textEdit->hasFocus()) {
                if (buffers.at(x)->parentWidget() == given_splitter) {
                    return true;
                }
            }
        }
    }

    return false;
}

TextEditor* MainWindow::return_current_buffer(QSplitter *given_splitter)
{
    if (given_splitter) {
        QList<TextEditor *> buffers = given_splitter->findChildren<TextEditor *>();
        for (int x = 0; x != buffers.count(); ++x) {
            if (buffers.at(x)->parentWidget() == given_splitter) {
                return buffers.at(x);
            }
        }
    }

    return 0;
}

//==========================================================================================
// C-x o ..................... Select another window.
//==========================================================================================

void MainWindow::other_window() {

    if(mdiArea->activeSubWindow())
        mdiArea->activeSubWindow()->setFocus();

    // We can move only through buffers from the current splitter
    QSplitter *splitter = current_splitter();
    if (splitter) {
        QList<TextEditor *> all = splitter->findChildren<TextEditor *>();

        // We loop over all the buffers and append the visible ones in the visible_buffers list.
        QList<TextEditor *> visible_buffers;
        for (int x = 0; x != all.count(); ++x) {
            if (all.at(x)->isVisible()) {
                visible_buffers.append(all.at(x));
            }
        }

        if (visible_buffers.count() > 1) {
            // Counting starts from visible_buffers - 1 [from the end of the list all]
            for (int i = 0; i != visible_buffers.count(); ++i) {
                // If the buffer is not the current one, keep going, else go to the next buffer
                if(!(visible_buffers.at(i) == current_buffer())) {  }
                else {
                    --i;
                    // If the buffer is not the last one, set focus, else start from the beginning
                    if (i >= 0) {
                        visible_buffers.at(i)->textEdit->setFocus();
                        return;
                    } else {
                        if(visible_buffers.at(visible_buffers.count() - 1)) {
                            visible_buffers.at(visible_buffers.count() - 1)->textEdit->setFocus();
                            return;
                        }
                    }
                }
            }
        }
    }
}

//==========================================================================================
// C-x <Right> ................... Select next buffer.
//==========================================================================================

void MainWindow::next_buffer()
{
    TextEditor *current_buf = current_buffer();
    QWidget *current_buf_widget = qobject_cast<QWidget *>(current_buf->parentWidget()->parentWidget());

    // This is the current_buf_widget's parent splitter
    QSplitter *parent_splitter = qobject_cast<QSplitter *>(current_buf_widget->parentWidget());
    QList<int> splitter_sizes = parent_splitter->sizes();

    // Here we take all the buffers which are created until now
    QList<TextEditor *> all_buffs = all_buffers();
    int buffers_count = all_buffs.count();

    // Here we will find all the buffers in the current splitter and take the visible buffers only
    QList<TextEditor *> splitter_buffers = current_splitter()->findChildren<TextEditor *>();
    QList<TextEditor *> visible_buffers;
    int splitter_buffers_count = splitter_buffers.count();
    for (int i = 0; i != splitter_buffers_count; ++i) {
        if (splitter_buffers.at(i)->isVisible())
            visible_buffers << splitter_buffers.at(i);
    }

    for (int i = 0; i != buffers_count; ++i) {

        if (all_buffs.at(i) == current_buf) {

            for (int x = i+1; x != buffers_count; ++x) {

                if (x != buffers_count && !visible_buffers.contains(all_buffs.at(x)) && (all_buffs.at(x) != current_buf)) {

                    QWidget *widget = qobject_cast<QWidget *>(all_buffs.at(x)->parentWidget()->parentWidget());
                    QSplitter *widget_splitter = qobject_cast<QSplitter *>(widget->parentWidget());

                    parent_splitter->insertWidget(parent_splitter->indexOf(current_buf_widget), widget);

                    if (widget->isHidden())
                        widget->show();

                    current_buf_widget->hide();
                    hide_one_level_up(widget_splitter);

                    int index_of_zero = splitter_sizes.indexOf(0);
                    if (index_of_zero)
                        splitter_sizes.removeAt(index_of_zero);

                    if (parent_splitter->sizes().count() != splitter_sizes.count()) {
                        int index_of_hidden_widget = parent_splitter->indexOf(current_buf_widget);
                        splitter_sizes.insert(index_of_hidden_widget, 0);
                    }

                    parent_splitter->setSizes(splitter_sizes);

                    all_buffs.at(x)->textEdit->setFocus();

                    return;
                }
            }

            // This code is the same as the previous one, but we enter here
            // if there are no other buffers and we start from the first one in the list
            for (int x = 0; x != buffers_count; ++x) {

                if (x != buffers_count && !visible_buffers.contains(all_buffs.at(x)) && (all_buffs.at(x) != current_buf)) {

                    QWidget *widget = qobject_cast<QWidget *>(all_buffs.at(x)->parentWidget()->parentWidget());
                    QSplitter *widget_splitter = qobject_cast<QSplitter *>(widget->parentWidget());

                    parent_splitter->insertWidget(parent_splitter->indexOf(current_buf_widget), widget);

                    if (widget->isHidden())
                        widget->show();

                    current_buf_widget->hide();
                    hide_one_level_up(widget_splitter);

                    int index_of_zero = splitter_sizes.indexOf(0);
                    if (index_of_zero)
                        splitter_sizes.removeAt(index_of_zero);

                    if (parent_splitter->sizes().count() != splitter_sizes.count()) {
                        int index_of_hidden_widget = parent_splitter->indexOf(current_buf_widget);
                        splitter_sizes.insert(index_of_hidden_widget, 0);
                    }

                    parent_splitter->setSizes(splitter_sizes);

                    all_buffs.at(x)->textEdit->setFocus();

                    return;
                }
            }
        }
    }
}

//==========================================================================================
// C-x <Left> ................... Select next buffer.
//==========================================================================================

void MainWindow::previous_buffer()
{

}

//==========================================================================================
// C-x 4 0 ................. Delete the selected window and kill the buffer that was showing
//                                 in it. The last character in this key sequence is a zero.
//==========================================================================================

void MainWindow::kill_buffer_and_window()
{
    if (!mdiArea->subWindowList().isEmpty()) {

        QList<QMdiSubWindow *> list_of_all_windows = mdiArea->subWindowList();

        if (list_of_all_windows.count() > 1) {

            QMdiSubWindow *current_window = mdiArea->currentSubWindow();

            for (int x = 0; x != list_of_all_windows.count(); ++x) {

                if (list_of_all_windows.at(x) == current_window) {

                    save_buffers_before_closing_window();

                    mdiArea->activatePreviousSubWindow();
                    mdiArea->removeSubWindow(current_window);

                }
            }
        } else {
            mode_minibuffer->setText(tr("Attempt to delete the last window"));
            mode_minibuffer->setStyleSheet("color: black;");
            return;
        }
    } else
        return;
}

// Help function for C-x 4 0 command
void MainWindow::save_buffers_before_closing_window()
{
    bool stopAsking = false;

    QSplitter *splitter = qobject_cast<QSplitter *>(mdiArea->activeSubWindow()->widget());
    QList<TextEditor *> editors = splitter->findChildren<TextEditor *>();

    for(int y = 0; y != editors.count(); ++y) {

        if (confirm_exit_redactor(editors.at(y))) {

            if(!stopAsking) {

                buffer_for_deletion = editors.at(y);
                set_kill_window_mode();
                buffer_for_deletion = 0;

                if (minibuffer->get_the_string() == "y") {
                    save_exit_from_minibuffer(editors.at(y));
                } else if (minibuffer->get_the_string() == "n") { /* do nothing */
                } else if (minibuffer->get_the_string() == "!") {
                    save_exit_from_minibuffer(editors.at(y));
                    stopAsking = true;
                } else if (minibuffer->get_the_string() == ".") {
                    TextEditor *editor = current_buffer();
                    save_exit_from_minibuffer(editor);
                    return;
                }
            } else {
                save_exit_from_minibuffer(editors.at(y));
            }
        }
    }
}

void MainWindow::add_help_window(sQ::HelpWindow window)
{
    QSplitter *active_splitter = current_splitter();

    QList<TextEditor *> buffers = active_splitter->findChildren<TextEditor *>();

    for (int i = 0; i != buffers.count(); ++i) {
        if (buffers.at(i)->objectName() == "*Help*")
            return;
    }

    QSplitter *new_splitter = new QSplitter;
    new_splitter->setChildrenCollapsible(false);
    new_splitter->setObjectName("Main Second Splitter");

    QWidget *container = new QWidget;
    QVBoxLayout *container_layout = new QVBoxLayout;
    container_layout->addWidget(new_splitter);
    container_layout->setContentsMargins(0, 0, 0, 0);
    container_layout->setSpacing(0);
    container->setLayout(container_layout);

    TextEditor *buffer = create_text_editor();
    buffer->setObjectName("*Help*");

    new_splitter->addWidget(buffer);

    buffer->createModeLine();
    buffer->set_current_file_name("*Help*");

    active_splitter->setOrientation(Qt::Vertical);
    active_splitter->addWidget(container);

    buffer->textEdit->document()->setModified(false);
    buffer->textEdit->setReadOnly(true);
    buffer->textEdit->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);

    QString current_mode = buffer->chFr->text();
    buffer->chFr->setText(current_mode.replace(0, 2, "%%"));

    emit new_text_for_messages(tr("In *Help* mode... [called with command C-x 4 0]"));

    QString text = "";

    if (window == sQ::ExitEditor) {

        text = "Type `y' to save the current buffer;"
            "\n`n' to skip the current buffer;"
            "\n`q' to give up on the save (skip all remaining buffers);"
            "\n`!' to save all remaining buffers;"
            "\n`.' (period) to save the current buffer and exit.";      
    } else {

        text = "Type `y' to save the current buffer;"
            "\n`n' to skip the current buffer;"
            "\n`q' to give up on the save (skip all remaining buffers, if any);"
            "\n`!' to save all remaining buffers (if any);"
            "\n`.' (period) to save the current buffer and close the window.";
    }

    buffer->textEdit->setPlainText(text);
}

void MainWindow::set_kill_window_mode()
{
    is_minibuffer_busy = true;
    is_focus_on_minibuffer = false;
    emit text_for_editor("is_busy", "set_minibuffer");

    minibuffer->clear();
    mode_minibuffer->clear();

    mode_minibuffer->setText("[Closing window...] Save file "
          + buffer_for_deletion->currentFile() + "? (y, n, !, ., q, help)");
    mode_minibuffer->setStyleSheet("color: blue;");

    minibuffer->setReadOnly(false);
    minibuffer->setFocus();

    {
        in_loop_exec = true;
        QEventLoop loop;
        loop.connect(this, SIGNAL(ready_to_exit()), &loop, SLOT(quit()));
        loop.exec();
    }
}

//==========================================================================================
// C-x ^ ...................... Make selected window taller.
//==========================================================================================

void MainWindow::enlarge_window()
{
    if (mdiArea->activeSubWindow()) {
        mdiArea->activeSubWindow()->setFocus();

        TextEditor *the_current_buffer = current_buffer();
        QSplitter *parent_splitter = qobject_cast<QSplitter *>(the_current_buffer->parentWidget()->parentWidget()->parentWidget());

        if (parent_splitter) {
            QList<TextEditor *> buffers_in_it = parent_splitter->findChildren<TextEditor *>();
            QList<TextEditor *> visible_buffers;
            for (int i = 0; i != buffers_in_it.count(); ++i) {
                if (buffers_in_it.at(i)->isVisible())
                    visible_buffers.append(buffers_in_it.at(i));
            }

            if (visible_buffers.count() > 1) {

                if (parent_splitter->orientation() == Qt::Vertical) {
                    qDebug() << "I";
                    if (the_current_buffer == visible_buffers.at(0)) {
                        // If the current_buffer is the first in the list
                        QList<int> current_sizes = parent_splitter->sizes();
                        // adjust sizes individually here, e.g.
                        current_sizes[0] = current_sizes[0] - 10;
                        current_sizes[1] = current_sizes[1] + 10;
                        parent_splitter->setSizes(current_sizes);
                    } else {
                        QList<int> current_sizes = parent_splitter->sizes();
                        // adjust sizes individually here, e.g.
                        current_sizes[0] = current_sizes[0] + 10;
                        current_sizes[1] = current_sizes[1] - 10;
                        parent_splitter->setSizes(current_sizes);
                    }

                } else {
                    // Here starts the case if (orientation == Qt::Horizontal)

                    find_the_first_empty_splitter(parent_splitter);

                    if (my_splitter) {

                        QList<TextEditor *> my_splitter_visible_buffers;
                        QList<TextEditor *> my_splitter_buffers = my_splitter->findChildren<TextEditor *>();
                        for (int i = 0; i != my_splitter_buffers.count(); ++i) {
                            if (my_splitter_buffers.at(i)->isVisible())
                                my_splitter_visible_buffers.append(my_splitter_buffers.at(i));
                        }

                        QWidget *first_widget = qobject_cast<QWidget *>(my_splitter->widget(0));
                        QSplitter *first_splitter = first_widget->findChild<QSplitter *>();
                        QWidget *second_widget = qobject_cast<QWidget *>(my_splitter->widget(1));
                        QSplitter *second_splitter = second_widget->findChild<QSplitter *>();

                        if (first_splitter && second_splitter) {
                            // We test here whether on the first and second postions parent splitter we have Splitter
                            // and if so, we check in which of these two splitters is the current buffer. If it is in the
                            // first splitter then go UP +10 otherwise go DOWN -10
                            // III Variant
                            // ---*--- For illustration see (Paper 4.4.1 / 2.3.2)
                            QList<TextEditor *> first_splitter_buffers = first_splitter->findChildren<TextEditor *>();
                            if (first_splitter_buffers.contains(the_current_buffer)) {
                                qDebug() << "III - 1";
                                QList<int> current_sizes = my_splitter->sizes();
                                // adjust sizes individually here, e.g.
                                current_sizes[0] = current_sizes[0] + 10;
                                current_sizes[1] = current_sizes[1] - 10;
                                my_splitter->setSizes(current_sizes);

                                my_splitter = 0;
                                return;
                            } else {
                                qDebug() << "III - 2";
                                QList<int> current_sizes = my_splitter->sizes();
                                // adjust sizes individually here, e.g.
                                current_sizes[0] = current_sizes[0] - 10;
                                current_sizes[1] = current_sizes[1] + 10;
                                my_splitter->setSizes(current_sizes);

                                my_splitter = 0;
                            }
                        }
                    }
                }

            } else {
                // Here starts the case if (visible_buffers.count() < 1)
                qDebug() << "IV";
                find_the_right_splitter(parent_splitter);
                if (my_splitter) {
                    qDebug() << "bb-bob" << my_splitter << my_splitter->sizes();
                    QList<int> current_sizes = my_splitter->sizes();
                    // adjust sizes individually here, e.g.
                    current_sizes[0] = current_sizes[0] - 10;
                    current_sizes[1] = current_sizes[1] + 10;
                    my_splitter->setSizes(current_sizes);

                    my_splitter = 0;
                    return;
                }
            }



        } // Here ends the if (buffer_splitter)
    }
}

void MainWindow::find_the_right_splitter(QSplitter *given_splitter)
{
    if (given_splitter) {
        QSplitter *parent_splitter = qobject_cast<QSplitter *>(given_splitter->parentWidget()->parentWidget());
        if (parent_splitter) {

            QList<TextEditor *> all_buffers = parent_splitter->findChildren<TextEditor *>();
            QList<TextEditor *> visible_buffers;
            for (int i = 0; i != all_buffers.count(); ++i) {
                if (all_buffers.at(i)->isVisible())
                    visible_buffers.append(all_buffers.at(i));
            }

            if (visible_buffers.count() > 1 && parent_splitter->orientation() == Qt::Vertical) {
                my_splitter = parent_splitter;
                return;
            } else
                find_the_right_splitter(parent_splitter);
        } else
            return;
    }
}

void MainWindow::find_the_right_splitter_second(QSplitter *given_splitter, int count)
{
    if (given_splitter) {
        QSplitter *parent_splitter = qobject_cast<QSplitter *>(given_splitter->parentWidget()->parentWidget());
        if (parent_splitter) {
            QList<TextEditor *> all_buffers = parent_splitter->findChildren<TextEditor *>();
            QList<TextEditor *> visible_buffers;
            for (int i = 0; i != all_buffers.count(); ++i) {
                if (all_buffers.at(i)->isVisible())
                    visible_buffers.append(all_buffers.at(i));
            }

            if (visible_buffers.count() > count && parent_splitter->orientation() == Qt::Vertical) {
                my_splitter = parent_splitter;
            } else
                find_the_right_splitter_second(parent_splitter, count);
        } else
            return;
    }
}

//==========================================================================================
// This function helps us to find the first splitter which doesn't have its own TextEditors
// and in this way we know which is the most outer splitter
//==========================================================================================

void MainWindow::find_the_first_empty_splitter(QSplitter *given_splitter)
{
    if (given_splitter) {
        QSplitter *parent_splitter = qobject_cast<QSplitter *>(given_splitter->parentWidget()->parentWidget());
        if (parent_splitter) {
            QList<TextEditor *> all_buffers = parent_splitter->findChildren<TextEditor *>();
            // We need only the visible buffers
            QList<TextEditor *> visible_buffers;
            for (int i = 0; i != all_buffers.count(); ++i) {
                if (all_buffers.at(i)->isVisible() && all_buffers.at(i)->objectName() != "*Buffer List*")
                    visible_buffers.append(all_buffers.at(i));
            }

            if (visible_buffers.count() > 2) {
                if (parent_splitter->orientation() == Qt::Vertical) {
                    my_splitter = parent_splitter;
                    return;
                } else {
                    QSplitter *parent_splitter_up = qobject_cast<QSplitter *>(parent_splitter->parentWidget()->parentWidget());
                    QList<TextEditor *> buffers_one_level_up = parent_splitter_up->findChildren<TextEditor *>();
                    QList<TextEditor *> visible_buffers_up;
                    for (int i = 0; i != buffers_one_level_up.count(); ++i) {
                        if (buffers_one_level_up.at(i)->isVisible())
                            visible_buffers_up.append(all_buffers.at(i));
                    }
                    if (visible_buffers_up.count() == visible_buffers.count()) {

                        find_the_right_splitter_second(parent_splitter, visible_buffers.count());
                    } else {

                        find_the_first_empty_splitter(parent_splitter);
                    }
                }
            } else {

                find_the_right_splitter_second(parent_splitter, visible_buffers.count());
            }
        }
    }
}

//==========================================================================================
// C-x } ...................... Make selected window wider.
//==========================================================================================

void MainWindow::enlarge_window_horizontally()
{
    if (!mdiArea->subWindowList().isEmpty()) {
        mdiArea->activeSubWindow()->setFocus();

        TextEditor *the_current_buffer = current_buffer();
        QSplitter *parent_splitter = qobject_cast<QSplitter *>(the_current_buffer->parentWidget()->parentWidget()->parentWidget());

        if (parent_splitter) {
            QList<TextEditor *> buffers_in_it = parent_splitter->findChildren<TextEditor *>();
            QList<TextEditor *> visible_buffers;
            for (int i = 0; i != buffers_in_it.count(); ++i) {
                if (buffers_in_it.at(i)->isVisible())
                    visible_buffers.append(buffers_in_it.at(i));
            }

            if (visible_buffers.count() > 1) {

                if (parent_splitter->orientation() == Qt::Horizontal) {

                    if (the_current_buffer == visible_buffers.at(0)) {
                        // If the current_buffer is the first in the list
                        QList<int> current_sizes = parent_splitter->sizes();
                        // adjust sizes individually here, e.g.
                        current_sizes[0] = current_sizes[0] - 10;
                        current_sizes[1] = current_sizes[1] + 10;
                        parent_splitter->setSizes(current_sizes);
                    } else {
                        QList<int> current_sizes = parent_splitter->sizes();
                        // adjust sizes individually here, e.g.
                        current_sizes[0] = current_sizes[0] + 10;
                        current_sizes[1] = current_sizes[1] - 10;
                        parent_splitter->setSizes(current_sizes);
                    }

                } else {
                    // Here starts the case if (orientation == Qt::Vertical)

                    find_the_first_empty_splitter_horizontal(parent_splitter);

                    if (my_splitter) {

                        QList<TextEditor *> my_splitter_visible_buffers;
                        QList<TextEditor *> my_splitter_buffers = my_splitter->findChildren<TextEditor *>();
                        for (int i = 0; i != my_splitter_buffers.count(); ++i) {
                            if (my_splitter_buffers.at(i)->isVisible())
                                my_splitter_visible_buffers.append(my_splitter_buffers.at(i));
                        }

                        QWidget *first_widget = qobject_cast<QWidget *>(my_splitter->widget(0));
                        QSplitter *first_splitter = first_widget->findChild<QSplitter *>();
                        QWidget *second_widget = qobject_cast<QWidget *>(my_splitter->widget(1));
                        QSplitter *second_splitter = second_widget->findChild<QSplitter *>();

                        if (first_splitter && second_splitter) {
                            QList<TextEditor *> first_splitter_buffers = first_splitter->findChildren<TextEditor *>();
                            if (first_splitter_buffers.contains(the_current_buffer)) {

                                QList<int> current_sizes = my_splitter->sizes();
                                // adjust sizes individually here, e.g.
                                current_sizes[0] = current_sizes[0] + 10;
                                current_sizes[1] = current_sizes[1] - 10;
                                my_splitter->setSizes(current_sizes);

                                my_splitter = 0;
                                return;
                            } else {

                                QList<int> current_sizes = my_splitter->sizes();
                                // adjust sizes individually here, e.g.
                                current_sizes[0] = current_sizes[0] - 10;
                                current_sizes[1] = current_sizes[1] + 10;
                                my_splitter->setSizes(current_sizes);

                                my_splitter = 0;
                            }
                        }
                    }
                } // Ends II
            }  else {
                // Here starts the case if (visible_buffers.count() < 1)

                find_the_right_splitter_horizontal(parent_splitter);
                if (my_splitter) {

                    QList<int> current_sizes = my_splitter->sizes();
                    // adjust sizes individually here, e.g.
                    current_sizes[0] = current_sizes[0] - 10;
                    current_sizes[1] = current_sizes[1] + 10;
                    my_splitter->setSizes(current_sizes);

                    my_splitter = 0;
                    return;
                }
            }
        } // Ends if(parent_splitter)
    }
}

void MainWindow::find_the_right_splitter_horizontal(QSplitter *given_splitter)
{
    if (given_splitter) {
        QSplitter *parent_splitter = qobject_cast<QSplitter *>(given_splitter->parentWidget()->parentWidget());
        if (parent_splitter) {

            QList<TextEditor *> all_buffers = parent_splitter->findChildren<TextEditor *>();
            QList<TextEditor *> visible_buffers;
            for (int i = 0; i != all_buffers.count(); ++i) {
                if (all_buffers.at(i)->isVisible())
                    visible_buffers.append(all_buffers.at(i));
            }

            if (visible_buffers.count() > 1 && parent_splitter->orientation() == Qt::Horizontal) {
                my_splitter = parent_splitter;
                return;
            } else
                find_the_right_splitter_horizontal(parent_splitter);
        } else
            return;
    }
}

void MainWindow::find_the_right_splitter_second_horizontal(QSplitter *given_splitter, int count)
{
    if (given_splitter) {
        QSplitter *parent_splitter = qobject_cast<QSplitter *>(given_splitter->parentWidget()->parentWidget());
        if (parent_splitter) {
            QList<TextEditor *> all_buffers = parent_splitter->findChildren<TextEditor *>();
            QList<TextEditor *> visible_buffers;
            for (int i = 0; i != all_buffers.count(); ++i) {
                if (all_buffers.at(i)->isVisible())
                    visible_buffers.append(all_buffers.at(i));
            }

            if (visible_buffers.count() > count && parent_splitter->orientation() == Qt::Horizontal) {
                my_splitter = parent_splitter;
            } else
                find_the_right_splitter_second_horizontal(parent_splitter, count);
        } else
            return;
    }
}

//==========================================================================================
// This function helps us to find the first splitter which doesn't have its own TextEditors
// and in this way we know which is the most outer splitter
//==========================================================================================

void MainWindow::find_the_first_empty_splitter_horizontal(QSplitter *given_splitter)
{
    if (given_splitter) {
        QSplitter *parent_splitter = qobject_cast<QSplitter *>(given_splitter->parentWidget()->parentWidget());
        if (parent_splitter) {
            QList<TextEditor *> all_buffers = parent_splitter->findChildren<TextEditor *>();
            // We need only the visible buffers
            QList<TextEditor *> visible_buffers;
            for (int i = 0; i != all_buffers.count(); ++i) {
                if (all_buffers.at(i)->isVisible() && all_buffers.at(i)->objectName() != "*Buffer List*")
                    visible_buffers.append(all_buffers.at(i));
            }

            if (visible_buffers.count() > 2) {
                if (parent_splitter->orientation() == Qt::Horizontal) {

                    my_splitter = parent_splitter;
                    return;
                } else {
                    QSplitter *parent_splitter_up = qobject_cast<QSplitter *>(parent_splitter->parentWidget()->parentWidget());
                    QList<TextEditor *> buffers_one_level_up = parent_splitter_up->findChildren<TextEditor *>();
                    QList<TextEditor *> visible_buffers_up;
                    for (int i = 0; i != buffers_one_level_up.count(); ++i) {
                        if (buffers_one_level_up.at(i)->isVisible())
                            visible_buffers_up.append(all_buffers.at(i));
                    }
                    if (visible_buffers_up.count() == visible_buffers.count()) {

                        find_the_right_splitter_second_horizontal(parent_splitter, visible_buffers.count());
                    } else {

                        find_the_first_empty_splitter_horizontal(parent_splitter);
                    }
                }
            } else {

                find_the_right_splitter_second_horizontal(parent_splitter, visible_buffers.count());
            }
        }
    }
}

//==========================================================================================
// C-x { ...................... Make selected window narrower.
//==========================================================================================

void MainWindow::shrink_window_horizontally()
{
    if (!mdiArea->subWindowList().isEmpty()) {
        mdiArea->activeSubWindow()->setFocus();

        TextEditor *the_current_buffer = current_buffer();
        QSplitter *parent_splitter = qobject_cast<QSplitter *>(the_current_buffer->parentWidget()->parentWidget()->parentWidget());

        if (parent_splitter) {
            QList<TextEditor *> buffers_in_it = parent_splitter->findChildren<TextEditor *>();
            QList<TextEditor *> visible_buffers;
            for (int i = 0; i != buffers_in_it.count(); ++i) {
                if (buffers_in_it.at(i)->isVisible())
                    visible_buffers.append(buffers_in_it.at(i));
            }

            if (visible_buffers.count() > 1) {

                if (parent_splitter->orientation() == Qt::Horizontal) {

                    if (the_current_buffer == visible_buffers.at(0)) {
                        // If the current_buffer is the first in the list
                        QList<int> current_sizes = parent_splitter->sizes();
                        // adjust sizes individually here, e.g.
                        current_sizes[0] = current_sizes[0] + 10;
                        current_sizes[1] = current_sizes[1] - 10;
                        parent_splitter->setSizes(current_sizes);
                    } else {
                        QList<int> current_sizes = parent_splitter->sizes();
                        // adjust sizes individually here, e.g.
                        current_sizes[0] = current_sizes[0] - 10;
                        current_sizes[1] = current_sizes[1] + 10;
                        parent_splitter->setSizes(current_sizes);
                    }

                } else {
                    // Here starts the case if (orientation == Qt::Vertical)

                    find_the_first_empty_splitter_horizontal(parent_splitter);

                    if (my_splitter) {

                        QList<TextEditor *> my_splitter_visible_buffers;
                        QList<TextEditor *> my_splitter_buffers = my_splitter->findChildren<TextEditor *>();
                        for (int i = 0; i != my_splitter_buffers.count(); ++i) {
                            if (my_splitter_buffers.at(i)->isVisible())
                                my_splitter_visible_buffers.append(my_splitter_buffers.at(i));
                        }

                        QWidget *first_widget = qobject_cast<QWidget *>(my_splitter->widget(0));
                        QSplitter *first_splitter = first_widget->findChild<QSplitter *>();
                        QWidget *second_widget = qobject_cast<QWidget *>(my_splitter->widget(1));
                        QSplitter *second_splitter = second_widget->findChild<QSplitter *>();

                        if (first_splitter && second_splitter) {
                            QList<TextEditor *> first_splitter_buffers = first_splitter->findChildren<TextEditor *>();
                            if (first_splitter_buffers.contains(the_current_buffer)) {

                                QList<int> current_sizes = my_splitter->sizes();
                                // adjust sizes individually here, e.g.
                                current_sizes[0] = current_sizes[0] - 10;
                                current_sizes[1] = current_sizes[1] + 10;
                                my_splitter->setSizes(current_sizes);

                                my_splitter = 0;
                                return;
                            } else {

                                QList<int> current_sizes = my_splitter->sizes();
                                // adjust sizes individually here, e.g.
                                current_sizes[0] = current_sizes[0] + 10;
                                current_sizes[1] = current_sizes[1] - 10;
                                my_splitter->setSizes(current_sizes);

                                my_splitter = 0;
                            }
                        }
                    }

                } // Ends II
            }  else {
                // Here starts the case if (visible_buffers.count() < 1)

                find_the_right_splitter_horizontal(parent_splitter);
                if (my_splitter) {

                    QList<int> current_sizes = my_splitter->sizes();
                    // adjust sizes individually here, e.g.
                    current_sizes[0] = current_sizes[0] + 10;
                    current_sizes[1] = current_sizes[1] - 10;
                    my_splitter->setSizes(current_sizes);

                    my_splitter = 0;
                    return;
                }
            }

        } // Ends if(parent_splitter)
    }
}

//==========================================================================================
//
//
//                          Buffers
//
//
//==========================================================================================

//==========================================================================================
// C-x b BUFFER <RET> ................ Select or create a buffer named BUFFER.
//==========================================================================================

void MainWindow::set_switch_to_buffer_mode()
{
    is_minibuffer_busy = true;
    is_focus_on_minibuffer = false;
    emit text_for_editor("is_busy", "set_minibuffer");

    minibuffer->clear();
    mode_minibuffer->clear();

    mode_minibuffer->setText(tr("Switch to buffer:"));
    mode_minibuffer->setStyleSheet("color: blue;");

    minibuffer->setReadOnly(false);
    minibuffer->setFocus();
}

void MainWindow::switch_to_buffer(const QString &name)
{
    QList<TextEditor*> buffers = all_buffers();

    if(!buffers.isEmpty()) {
        for (int x = 0; x != buffers.count(); ++x) {
            if (buffers.at(x)->currentFile() == name) {

                TextEditor *active_buffer = current_buffer();
                if (active_buffer) {

                    QWidget *widget_active_buffer =
                            qobject_cast<QWidget *>(active_buffer->parentWidget()->parentWidget());
                    QSplitter *parent_splitter = qobject_cast<QSplitter *>(widget_active_buffer->parentWidget());

                    QWidget *widget_buffer = qobject_cast<QWidget *>(buffers.at(x)->parentWidget()->parentWidget());
                    if (widget_buffer->isHidden())
                        widget_buffer->show();

                    widget_active_buffer->hide();
                    hide_one_level_up(parent_splitter);

                    buffers.at(x)->textEdit->setFocus();
                    return;

                } else
                    return;
            }
        }

        minibuffer->clear();
        mode_minibuffer->clear();

        mode_minibuffer->setText(tr("This buffer doens't exist"));
        mode_minibuffer->setStyleSheet("color:black;");

    } else {
        minibuffer->clear();
        mode_minibuffer->clear();

        mode_minibuffer->setText(tr("There aren't any buffers"));
        mode_minibuffer->setStyleSheet("color:black;");
    }
}

//==========================================================================================
// C-x 4 b buffer ..................... Similar, but select buffer in another window.
//==========================================================================================

void MainWindow::set_switch_to_buffer_other_window_mode()
{
    is_minibuffer_busy = true;
    is_focus_on_minibuffer = false;
    emit text_for_editor("is_busy", "set_minibuffer");

    minibuffer->clear();
    mode_minibuffer->clear();

    mode_minibuffer->setText(tr("Switch to buffer in other window:"));
    mode_minibuffer->setStyleSheet("color: blue;");

    minibuffer->setReadOnly(false);
    minibuffer->setFocus();
}

void MainWindow::switch_to_buffer_other_window(const QString &name)
{
    QList<TextEditor*> buffers = all_buffers();

    if(!buffers.isEmpty()) {

        for (int x = 0; x != buffers.count(); ++x) {
            if (buffers.at(x)->currentFile() == name) {

                TextEditor *buffer_for_switching = buffers.at(x);
                QWidget *buffer_widget = qobject_cast<QWidget *>(buffer_for_switching->parentWidget()->parentWidget());
                QSplitter *parent_splitter = qobject_cast<QSplitter *>(buffer_widget->parentWidget());

                QSplitter *splitter = create_splitter();
                splitter->addWidget(buffer_widget);

                if (buffer_widget->isHidden())
                    buffer_widget->show();

                hide_one_level_up(parent_splitter);

                buffer_for_switching->textEdit->setFocus();

                return;
            }
        }

        minibuffer->clear();
        mode_minibuffer->clear();

        mode_minibuffer->setText(tr("This buffer doens't exist."));
        mode_minibuffer->setStyleSheet("color:black;");
    } else {
        minibuffer->clear();
        mode_minibuffer->clear();

        mode_minibuffer->setText(tr("There aren't any buffers."));
        mode_minibuffer->setStyleSheet("color:black;");
    }
}

//==========================================================================================
// C-x k BUFNAME ................. Kill buffer BUFNAME.
//==========================================================================================

void MainWindow::kill_buffer()
{
    QList<QMdiSubWindow *> windows = mdiArea->subWindowList();
    bool is_last_window = false;

    if (!windows.isEmpty()) {

        if (windows.count() == 1)
            is_last_window = true;
        else
            is_last_window = false;

    } else {
        minibuffer->clear();
        mode_minibuffer->clear();

        mode_minibuffer->setText(tr("There aren't any windows opened"));
        mode_minibuffer->setStyleSheet("color: black;");
        return;
    }

    TextEditor *active_buffer = current_buffer();

    if (!active_buffer)
        return;

    QSplitter *splitter = current_splitter();

    if (splitter) {

        // A list with all buffers in this window
        QList<TextEditor *> all = splitter->findChildren<TextEditor *>();

        // We loop over all the buffers and append the visible ones in the all_visible_buffers list.
        QList<TextEditor *> visible_buffers;
        for (int x = 0; x != all.count(); ++x) {
            if (all.at(x)->isVisible()) {
                visible_buffers.append(all.at(x));
            }
        }

        if (visible_buffers.count() > 1) {

            for (int i = 0 ; i != visible_buffers.count(); ++i) {

                if (!(visible_buffers.at(i) == active_buffer)) { } // Do nothing, Go Next

                else {

                    if (active_buffer->textEdit->document()->isModified()) {

                        buffer_for_deletion = active_buffer;

                        --i;

                        if (i >= 0)
                            buffer_to_receive_focus = visible_buffers.at(i);
                        else
                            buffer_to_receive_focus = visible_buffers.at(visible_buffers.count()-1);

                        set_ask_to_save_buffer_mode();

                        return;
                    }

                    // This is the splitter of the current buffer
                    QSplitter *current_splitter =
                            qobject_cast<QSplitter *>(active_buffer->parentWidget());

                    delete active_buffer;
                    delete_buffer_completely(current_splitter);

                    // We decrement i here, so we can set the focus to the next visible buffer
                    --i;

                    // If i >= 0, we move the focus to the next buffer
                    if (i >= 0) {
                        visible_buffers.at(i)->textEdit->setFocus();
                        return;
                    }

                    // If i < 0, we move the focus to the first buffer
                    if (i < 0) {
                        visible_buffers.at(visible_buffers.count()-1)->textEdit->setFocus();
                        return;
                    }
                }
            } // Here ends for ()
        } else {

            if (is_last_window) {
                minibuffer->clear();
                mode_minibuffer->clear();

                mode_minibuffer->setText(tr("Attempt to delete the last opened window"));
                mode_minibuffer->setStyleSheet("color: black;");
                return;
            } else {

                is_last_buffer = true;
                buffer_for_deletion = active_buffer;

                if (active_buffer->textEdit->document()->isModified()) {
                    set_ask_to_save_buffer_mode();
                    return;
                }
                else {

                    QMdiSubWindow *current_subWindow = mdiArea->activeSubWindow();

                    delete active_buffer;

                    mdiArea->activateNextSubWindow();                    
                    mdiArea->removeSubWindow(current_subWindow);
                }
            }
        }

    } else
        return; // Here ends if (splitter)
}

//==========================================================================================
//
//
//                          *Buffer List*
//
//
//==========================================================================================

//==========================================================================================
// Return a list of all buffers.
//==========================================================================================

QList<TextEditor*> MainWindow::all_buffers()
{
    QList<TextEditor*> list_of_buffers;

    if(!mdiArea->subWindowList().isEmpty()) {
        QList<QMdiSubWindow *> windows = mdiArea->subWindowList();
        for (int x = 0; x != windows.count(); ++x) {
            QSplitter *splitter = qobject_cast<QSplitter *>(windows.at(x)->widget());
            QList<TextEditor *> buffers = splitter->findChildren<TextEditor *>();
            for(int y = 0; y != buffers.count(); ++y) {
                list_of_buffers.append(buffers.at(y));
            }
        }
    }

    return list_of_buffers;
}

//==========================================================================================
// C-x C-b ...................... List the existing buffers.
//==========================================================================================

void MainWindow::list_buffers() {

    QSplitter *splitter = current_splitter();
    if(splitter) {

        //-------------------------------------------------------------------------------
        // Here we take all the buffers, because we need to check whether we have an
        // opened *Buffer List* somewhere.
        QList<TextEditor*> buffers = all_buffers();
        int all_buffers_count = buffers.count();

        // Also we need the visible buffers to check whether we have an opened *Buffer List*
        // in the current visible window.
        QList<TextEditor *> visible_buffers;
        QList<TextEditor *> splitter_buffers = splitter->findChildren<TextEditor *>();
        int splitter_buffers_count = splitter_buffers.count();

        for (int i = 0; i != splitter_buffers_count; ++i) {
            if (splitter_buffers.at(i)->isVisible())
                visible_buffers << splitter_buffers.at(i);
        }

        int visible_buffers_count = visible_buffers.count();

        //-------------------------------------------------------------------------------

        if(all_buffers_count > 0) {

            for(int y = 0; y != all_buffers_count; ++y) {

                if(buffers.at(y)->objectName() == "*Buffer List*") {

                    //==========================================================================================
                    // If we have an opened *Buffer List*, we enter here refresh it with 3 cases.
                    // 1. If we have only one visible buffer.
                    // 2. If we have more than one visible buffer and we have a following buffer.
                    // 3. If we have more than one visible buffer and we don't have a following buffer - we take the previsous one.
                    // What does it following and previous? - Every time when we call the command C-x C-b we shoud open the
                    // *Buffer List* in the next buffer, not in the current buffer from which we call the command.
                    // But if we do not have a following buffer, we open it in the previous one from the current.
                    //==========================================================================================

                    TextEditor *buffer_list = buffers.at(y);
                    QWidget *widget_buffer = qobject_cast<QWidget *>(buffer_list->parentWidget()->parentWidget());

                    if (widget_buffer->isHidden())
                        widget_buffer->setVisible(true);

                    QSplitter *splitter_of_widget = qobject_cast<QSplitter *>(widget_buffer->parentWidget());

                    if ( visible_buffers.contains(buffer_list) ) {

                        int buf_index = splitter_of_widget->indexOf(widget_buffer);
                        QList<int> splitter_sizes = splitter_of_widget->sizes();

                        QWidget *new_buffer_list = create_buffer_list();
                        if (new_buffer_list) {

                            splitter_of_widget->insertWidget(buf_index, new_buffer_list);

                            delete widget_buffer;

                            // To be able to set the focus to the newly created *Buffer List*
                            // we should find it in the new_buffer_list widget and cast it to the TextEditor
                            TextEditor *buffer_list_text_editor = new_buffer_list->findChild<TextEditor *>("*Buffer List*");
                            buffer_list_text_editor->textEdit->setFocus();

                            splitter_of_widget->setSizes(splitter_sizes);

                            return;
                        }
                    } else {

                        QSplitter *buffer_list_splitter = qobject_cast<QSplitter *>(buffer_list->parentWidget());
                        delete buffer_list;
                        delete_buffer_completely(buffer_list_splitter);

                        QWidget *new_buffer_list = create_buffer_list();

                        if (visible_buffers_count == 1) {

                            //==========================================================================================
                            // If we have only one visible buffer, we split the current splitter in half.
                            //==========================================================================================

                            QList<int> new_sizes;
                            QList<int> splitter_sizes = splitter->sizes();
                            int half_of_the_current_size = splitter_sizes.at(0) / 2;
                            new_sizes << half_of_the_current_size << half_of_the_current_size;

                            splitter->setOrientation(Qt::Vertical);
                            splitter->addWidget(new_buffer_list);

                            TextEditor *buffer_list_text_editor = new_buffer_list->findChild<TextEditor *>("*Buffer List*");
                            buffer_list_text_editor->textEdit->setFocus();

                            // Here we set the new size manually
                            splitter->setSizes(new_sizes);

                            return;

                        } else {

                            TextEditor *active_buffer = current_buffer();

                            for (int i = 0; i != visible_buffers_count; ++i) {

                                if (visible_buffers.at(i) == active_buffer) {

                                    if ( ++i != visible_buffers_count) {

                                        //==========================================================================================
                                        // If we have a next buffer in the visible_buffers, we enter here.
                                        //==========================================================================================

                                        QWidget *widget = qobject_cast<QWidget *>(visible_buffers.at(i)->parentWidget()->parentWidget());
                                        QSplitter *widget_splitter = qobject_cast<QSplitter *>(widget->parentWidget());
                                        QList<int> splitter_sizes = widget_splitter->sizes();

                                        widget_splitter->insertWidget(widget_splitter->indexOf(widget), new_buffer_list);

                                        widget->hide();

                                        // This is necessary if we want the same size of the widget
                                        QSizePolicy widget_policy = widget->sizePolicy();
                                        widget_policy.setHorizontalStretch(1);
                                        widget_policy.setVerticalStretch(1);
                                        widget->setSizePolicy(widget_policy);

                                        widget_splitter->setSizes(splitter_sizes);

                                        TextEditor *buffer_list_text_editor = new_buffer_list->findChild<TextEditor *>("*Buffer List*");
                                        buffer_list_text_editor->textEdit->setFocus();

                                        return;

                                    } else {

                                        //==========================================================================================
                                        // We enter here if we don't have a next buffer in the visible_buffers list,
                                        // so we take the buffer before the active/current buffer.
                                        //==========================================================================================

                                        --(--i);

                                        QWidget *widget = qobject_cast<QWidget *>(visible_buffers.at(i)->parentWidget()->parentWidget());
                                        QSplitter *widget_splitter = qobject_cast<QSplitter *>(widget->parentWidget());
                                        QList<int> splitter_sizes = widget_splitter->sizes();

                                        widget_splitter->insertWidget(widget_splitter->indexOf(widget), new_buffer_list);

                                        widget->hide();

                                        // This is necessary if we want the same size of the widget
                                        QSizePolicy widget_policy = widget->sizePolicy();
                                        widget_policy.setHorizontalStretch(1);
                                        widget_policy.setVerticalStretch(1);
                                        widget->setSizePolicy(widget_policy);
                                        widget_splitter->setSizes(splitter_sizes);

                                        TextEditor *buffer_list_text_editor = new_buffer_list->findChild<TextEditor *>("*Buffer List*");
                                        buffer_list_text_editor->textEdit->setFocus();

                                        return;

                                    }
                                }
                            }
                        }
                    }
                }
            }

            //==========================================================================================
            // If we don't have an opened *Buffer List*, we enter here and create new one with 3 cases.
            // 1. If we have only one visible buffer.
            // 2. If we have more than one visible buffer and we have a following buffer.
            // 3. If we have more than one visible buffer and we don't have a following buffer - we take the previsous one.
            //==========================================================================================

            QWidget *new_buffer_list = create_buffer_list();
            if (new_buffer_list) {

                if (visible_buffers_count == 1) {

                    // If we have only one visible buffer, we split the current splitter in a half.

                    QList<int> new_sizes;
                    QList<int> splitter_sizes = splitter->sizes();
                    int half_of_the_current_size = splitter_sizes.at(0) / 2;
                    new_sizes << half_of_the_current_size << half_of_the_current_size;

                    splitter->setOrientation(Qt::Vertical);
                    splitter->addWidget(new_buffer_list);

                    TextEditor *buffer_list_text_editor = new_buffer_list->findChild<TextEditor *>("*Buffer List*");
                    buffer_list_text_editor->textEdit->setFocus();

                    splitter->setSizes(new_sizes);

                } else {
                    TextEditor *active_buffer = current_buffer();

                    for (int i = 0; i != visible_buffers_count; ++i) {

                        if (visible_buffers.at(i) == active_buffer) {

                            if ( ++i != visible_buffers_count) {

                                // If we have a next buffer, we enter here.

                                QWidget *widget = qobject_cast<QWidget *>(visible_buffers.at(i)->parentWidget()->parentWidget());
                                QSplitter *widget_splitter = qobject_cast<QSplitter *>(widget->parentWidget());
                                QList<int> splitter_sizes = widget_splitter->sizes();

                                widget_splitter->insertWidget(widget_splitter->indexOf(widget), new_buffer_list);

                                widget->hide();

                                // This is necessary if we want the same size of the widget.
                                QSizePolicy widget_policy = widget->sizePolicy();
                                widget_policy.setHorizontalStretch(1);
                                widget_policy.setVerticalStretch(1);
                                widget->setSizePolicy(widget_policy);

                                widget_splitter->setSizes(splitter_sizes);

                                TextEditor *buffer_list_text_editor = new_buffer_list->findChild<TextEditor *>("*Buffer List*");
                                buffer_list_text_editor->textEdit->setFocus();

                                return;

                            } else {

                                // We enter here if we do not have a next buffer in the visible_ones list,
                                // so we take the buffer before the active/current buffer.

                                --(--i);

                                QWidget *widget = qobject_cast<QWidget *>(visible_buffers.at(i)->parentWidget()->parentWidget());
                                QSplitter *widget_splitter = qobject_cast<QSplitter *>(widget->parentWidget());
                                QList<int> splitter_sizes = widget_splitter->sizes();

                                widget_splitter->insertWidget(widget_splitter->indexOf(widget), new_buffer_list);

                                widget->hide();

                                QSizePolicy widget_policy = widget->sizePolicy();
                                widget_policy.setHorizontalStretch(1);
                                widget_policy.setVerticalStretch(1);
                                widget->setSizePolicy(widget_policy);
                                widget_splitter->setSizes(splitter_sizes);

                                TextEditor *buffer_list_text_editor = new_buffer_list->findChild<TextEditor *>("*Buffer List*");
                                buffer_list_text_editor->textEdit->setFocus();

                                return;

                            }
                        }
                    }
                }
            }
        }
    }
}

QWidget* MainWindow::create_buffer_list()
{
    if(!mdiArea->subWindowList().isEmpty()) {
        QSplitter *splitter = current_splitter();
        if(splitter) {

            QSplitter *second_splitter = new QSplitter;
            second_splitter->setObjectName("2 Second Splitter");
            second_splitter->setChildrenCollapsible(false);
            QWidget *second_container = new QWidget;
            QVBoxLayout *second_container_layout = new QVBoxLayout;

            second_container_layout->addWidget(second_splitter);
            second_container_layout->setContentsMargins(0, 0, 0, 0);
            second_container_layout->setSpacing(0);
            second_container->setLayout(second_container_layout);

            TextEditor *editor = create_text_editor();
            editor->setObjectName("*Buffer List*");
            editor->textEdit->setObjectName("*Buffer List*");
            second_splitter->addWidget(editor);

            QFont font("DejaVu Sans Mono", 10);
            editor->textEdit->setFont(font);

            editor->textEdit->setWordWrapMode(QTextOption::NoWrap);

            editor->createModeLine();
            editor->set_current_file_name(tr("*Buffer List*"));
            editor->textEdit->insertPlainText("CRM Buffer" + QString(40, ' ') + "Size" + QString(4, ' ') + "Mode" + QString(20, ' ') + "File\n");
            editor->textEdit->insertPlainText("--- ------" + QString(40, ' ') + "----" + QString(4, ' ') + "----" + QString(20, ' ') + "----\n");


            // Here we delete all the items in the list before populating it with new data
            if (!list_of_strings_for_every_buffer.isEmpty()) {
                QList<QString>::iterator it = list_of_strings_for_every_buffer.begin();
                while (it != list_of_strings_for_every_buffer.end())
                    it = list_of_strings_for_every_buffer.erase(it);
            }

            list_of_strings_for_every_buffer.append("CRM Buffer" + QString(40, ' ') + "Size" + QString(4, ' ') + "Mode" + QString(20, ' ') + "File");
            list_of_strings_for_every_buffer.append("--- ------" + QString(40, ' ') + "----" + QString(4, ' ') + "----" + QString(20, ' ') + "----");


            // Here we empty the list with activated buffers, if it is not empty
            // before populating it with the new ones
            if (!list_of_activated_buffers.isEmpty()) {
                QList<TextEditor *>::iterator it = list_of_activated_buffers.begin();
                while (it != list_of_activated_buffers.end())
                    it = list_of_activated_buffers.erase(it);
            }

            QList<TextEditor *> buffers = all_buffers();

            if(!buffers.isEmpty()) {
                for(int y = 0; y != buffers.count(); ++y) {
                    if(buffers.at(y)->objectName() == "*Buffer List*" ||
                       buffers.at(y)->objectName() == "*Home Page*") { }
                    else {
                        // If the buffer is not the *Buffer List* append it here and in this way
                        // we have the list of all of the activated buffers
                        list_of_activated_buffers.append(buffers.at(y));

                        QString current;
                        if(buffers.at(y) == current_buffer())
                            current = ".";
                        else
                            current = " ";

                        QString read_only;
                        if(buffers.at(y)->textEdit->isReadOnly())
                            read_only = "%";
                        else
                            read_only = " ";

                        QString modified;
                        if(buffers.at(y)->textEdit->document()->isModified())
                            modified = "*";
                        else
                            modified = " ";

                        QFileInfo info(buffers.at(y)->currentFilePath());

                        QString buffer_size = QVariant(info.size()).toString();

                        QString mode = buffers.at(y)->get_majorMinor();

                        editor->textEdit->insertPlainText(current + read_only + modified + " "
                                                 + buffers.at(y)->currentFile() + QString((46 - (QString(buffers.at(y)->currentFile()).size())), ' ')
                                                 + buffer_size + QString((8 - QString(buffer_size).size()), ' ')
                                                 + mode + QString((24 - (QString(mode).size())), ' ')
                                                          + buffers.at(y)->currentFilePath() + "\n");

                        list_of_strings_for_every_buffer.append(current + read_only + modified + " "
                                                                  + buffers.at(y)->currentFile() + QString((46 - (QString(buffers.at(y)->currentFile()).size())), ' ')
                                                                  + buffer_size + QString((8 - QString(buffer_size).size()), ' ')
                                                                  + mode + QString((24 - (QString(mode).size())), ' ')
                                                                  + buffers.at(y)->currentFilePath());

                    }
                }
            }

            connect(editor->textEdit, SIGNAL(pushed_x()), this, SLOT(kill_all_marked_buffers()));
            connect(editor->textEdit, SIGNAL(pushed_q()), this, SLOT(quit_buffer_list()));
            //connect(editor->textEdit, SIGNAL(pushed_g()), this, SLOT(revert_buffer()));
            connect(editor, SIGNAL(pushed_read_only()), this, SLOT(buffer_list_is_read_only()));
            connect(editor->textEdit, SIGNAL(pushed_enter(int)), this, SLOT(show_buffer_from_list_buffers(int)));
            connect(editor->textEdit, SIGNAL(pushed_d(int, bool)), this, SLOT(mark_buffer_for_killing(int, bool)));
            connect(editor->textEdit, SIGNAL(pushed_u(int, bool)), this, SLOT(undo_marked_buffer(int, bool)));
            connect(editor->textEdit, SIGNAL(pushed_s(int)), this, SLOT(mark_buffer_for_saving(int)));
            //connect(editor->textEdit, SIGNAL(pushed_f(int)), this, SLOT(show_buffer_instead_buffer_list(int)));
            connect(editor->textEdit, SIGNAL(pushed_o(int)), this, SLOT(show_buffer_other_window_buffer_list(int)));
            connect(editor->textEdit, SIGNAL(pushed_m(int)), this, SLOT(mark_buffer(int)));
            connect(editor->textEdit, SIGNAL(pushed_v(int)), this, SLOT(show_buffers_other_windows_v(int)));
            connect(editor->textEdit, SIGNAL(pushed_r(int)), this, SLOT(set_rename_mode_from_buffer_list(int)));
            connect(editor->textEdit, SIGNAL(pushed_1(int)), this, SLOT(show_buffer_alone(int)));
            connect(editor->textEdit, SIGNAL(pushed_2(int)), this, SLOT(set_up_two_buffers_in_buffer_list(int)));
            connect(editor->textEdit, SIGNAL(pushed_del(int,bool)), this, SLOT(undo_marked_buffer(int,bool)));
            connect(editor->textEdit, SIGNAL(pushed_ctrl_d(int,bool)), this, SLOT(mark_buffer_for_killing(int,bool)));
            connect(editor->textEdit, SIGNAL(pushed_percent(int)), this, SLOT(toggle_read_only_buffer_list(int)));
            connect(editor->textEdit, SIGNAL(pushed_unmodified(int)), this, SLOT(set_modified_buffer_list(int)));

            editor->textEdit->document()->setModified(false);
            editor->textEdit->setReadOnly(true);
            editor->textEdit->moveCursor(QTextCursor::Start);
            editor->textEdit->moveCursor(QTextCursor::Down);
            editor->textEdit->moveCursor(QTextCursor::Down);
            editor->textEdit->moveCursor(QTextCursor::StartOfLine);
            editor->textEdit->setTextInteractionFlags(editor->textEdit->textInteractionFlags()
                                                      | Qt::TextSelectableByKeyboard);
            QString current_mode = editor->chFr->text();
            editor->chFr->setText(current_mode.replace(0, 2, "%%"));

            return second_container;
        }
    }

    return 0;
}

//==========================================================================================
// Show buffer from list_buffers when pressing "Return"
//==========================================================================================

void MainWindow::show_buffer_from_list_buffers(int current_line)
{
    if (current_line > 2) {

        QSplitter *splitter = current_splitter();
        QList<TextEditor *> buffers = splitter->findChildren<TextEditor *>();

        TextEditor *buffer = list_of_activated_buffers.at(current_line - 3);
        QWidget *widget_of_buffer = qobject_cast<QWidget *>(buffer->parentWidget()->parentWidget());
        QSplitter *splitter_of_widget = qobject_cast<QSplitter *>(widget_of_buffer->parentWidget());

        // If the buffer is visible, we just set the focus on it
        if (buffers.contains(buffer) && buffer->isVisible()) {
            buffer->textEdit->setFocus();
            return;
        } else {
            // If the buffer which we are looking for is not visible, we show it
            // in the place of *Buffer List*
            for (int x = 0; x != buffers.count(); ++x) {

                if (buffers.at(x)->objectName() == "*Buffer List*") {

                    QWidget *buffer_list_widget = qobject_cast<QWidget *>(buffers.at(x)->parentWidget()->parentWidget());
                    QSplitter *buffer_list_splitter = qobject_cast<QSplitter *>(buffer_list_widget->parentWidget());
                    QList<int> splitter_sizes = buffer_list_splitter->sizes();

                    if (widget_of_buffer->isHidden())
                        widget_of_buffer->setVisible(true);

                    buffer_list_splitter->insertWidget(buffer_list_splitter->indexOf(buffer_list_widget), widget_of_buffer);

                    buffer_list_widget->hide();
                    hide_one_level_up(splitter_of_widget);

                    // This is necessary if we want the same size of the widget.
                    QSizePolicy widget_policy = widget_of_buffer->sizePolicy();
                    widget_policy.setHorizontalStretch(1);
                    widget_policy.setVerticalStretch(1);
                    widget_of_buffer->setSizePolicy(widget_policy);

                    buffer_list_splitter->setSizes(splitter_sizes);

                    buffer->textEdit->setFocus();

                    return;

                }
            }
        }
    } else {
        minibuffer->clear();
        mode_minibuffer->clear();

        mode_minibuffer->setText(tr("There is no buffer on the current line."));
        mode_minibuffer->setStyleSheet("color: black;");
    }
}

//==========================================================================================
// Set "D" infront of buffer name, meaning that it is mark for killing and go down by one line
//==========================================================================================

void MainWindow::mark_buffer_for_killing(int current_line, bool direction)
{
    if (current_line > 2) {
        QSplitter *splitter = current_splitter();
        QList<TextEditor *> buffers = splitter->findChildren<TextEditor *>();
        for (int x = 0; x != buffers.count(); ++x) {
            if (buffers.at(x)->objectName() == "*Buffer List*") {

                QString current_string = list_of_strings_for_every_buffer.at(current_line - 1);

                if (current_string.at(0) == 'D') {
                    mode_minibuffer->setText(tr("The current buffer is already marked for deleting."));
                    mode_minibuffer->setStyleSheet("color:black;");
                    return;
                } else {

                    list_of_buffers_for_killing << list_of_activated_buffers.at(current_line - 3);

                    buffers.at(x)->textEdit->setReadOnly(false);
                    current_string.replace(0, 1, "D");
                    list_of_strings_for_every_buffer.replace((current_line - 1), current_string);

                    buffers.at(x)->textEdit->clear();

                    for (int y = 0; y != list_of_strings_for_every_buffer.count(); ++y)
                        buffers.at(x)->textEdit->appendPlainText(list_of_strings_for_every_buffer.at(y));

                    buffers.at(x)->textEdit->document()->setModified(false);
                    buffers.at(x)->textEdit->setReadOnly(true);

                    buffers.at(x)->textEdit->moveCursor(QTextCursor::Start);

                    if (direction) {
                        for (int z = 0; z != (current_line - 2); ++z)
                            buffers.at(x)->textEdit->moveCursor(QTextCursor::Down);
                    } else {
                        for (int z = 0; z != current_line; ++z)
                            buffers.at(x)->textEdit->moveCursor(QTextCursor::Down);
                    }

                    buffers.at(x)->textEdit->moveCursor(QTextCursor::StartOfLine);
                    buffers.at(x)->textEdit->setTextInteractionFlags(buffers.at(x)->textEdit->textInteractionFlags()
                                                                     | Qt::TextSelectableByKeyboard);
                    QString current_mode = buffers.at(x)->chFr->text();
                    buffers.at(x)->chFr->setText(current_mode.replace(0, 2, "%%"));
                }
            }
        }
    } else {
        minibuffer->clear();
        mode_minibuffer->clear();

        mode_minibuffer->setText(tr("There is no buffer on the current line."));
        mode_minibuffer->setStyleSheet("color: black;");
    }
}

//==========================================================================================
// Undo setting "D", "S" or ">" infront of buffer name, meaning that it is not marked for
// killing, saving or displaying anymore
//==========================================================================================

void MainWindow::undo_marked_buffer(int current_line, bool direction)
{
    if (current_line > 2) {
        QSplitter *splitter = current_splitter();
        QList<TextEditor *> buffers = splitter->findChildren<TextEditor *>();
        for (int x = 0; x != buffers.count(); ++x) {
            if (buffers.at(x)->objectName() == "*Buffer List*") {

                QString current_string = list_of_strings_for_every_buffer.at(current_line - 1);

                if ((current_string.at(0) != 'D') && (current_string.at(0) != '>')
                        && (current_string.at(2) != 'S')) {
                    mode_minibuffer->setText(tr("There is nothing to undo from marking."));
                    mode_minibuffer->setStyleSheet("color:black;");
                    return;
                } else {

                    // Here we remove the buffer from the list_of_buffers_for_killing
                    QList<TextEditor *>::iterator it = list_of_buffers_for_killing.begin();
                    while (it != list_of_buffers_for_killing.end()) {
                        if (*it == list_of_activated_buffers.at(current_line - 3))
                            it = list_of_buffers_for_killing.erase(it);
                        else
                            ++it;
                    }

                    buffers.at(x)->textEdit->setReadOnly(false);

                    if (current_string.at(0) == 'D') {
                        current_string.replace(0, 1, " ");

                        if (current_string.at(2) == 'S') {
                            current_string.replace(2, 1, " ");

                            // Here we remove the buffer from the list_of_buffers_for_saving
                            QList<TextEditor *>::iterator it = list_of_buffers_for_saving.begin();
                            while (it != list_of_buffers_for_saving.end()) {
                                if (*it == list_of_activated_buffers.at(current_line - 3))
                                    it = list_of_buffers_for_saving.erase(it);
                                else
                                    ++it;
                            }
                        }
                    } else if (current_string.at(2) == 'S') {
                        current_string.replace(2, 1, " ");

                        // Here we remove the buffer from the list_of_buffers_for_saving
                        QList<TextEditor *>::iterator it = list_of_buffers_for_saving.begin();
                        while (it != list_of_buffers_for_saving.end()) {
                            if (*it == list_of_activated_buffers.at(current_line - 3))
                                it = list_of_buffers_for_saving.erase(it);
                            else
                                ++it;
                        }
                    } else {
                        current_string.replace(0, 1, " ");

                        // Here we remove the buffer from the list_of_marked_buffers
                        QList<TextEditor *>::iterator it = list_of_marked_buffers.begin();
                        while (it != list_of_marked_buffers.end()) {
                            if (*it == list_of_activated_buffers.at(current_line - 3))
                                it = list_of_marked_buffers.erase(it);
                            else
                                ++it;
                        }
                    }

                    // Here we replace the whole line with the new current string
                    list_of_strings_for_every_buffer.replace((current_line - 1), current_string);

                    // We clear the whole text in the *Buffer List* and update it with the new one
                    buffers.at(x)->textEdit->clear();
                    for (int y = 0; y != list_of_strings_for_every_buffer.count(); ++y)
                        buffers.at(x)->textEdit->appendPlainText(list_of_strings_for_every_buffer.at(y));

                    // Move the cursor to the next position
                    buffers.at(x)->textEdit->document()->setModified(false);
                    buffers.at(x)->textEdit->setReadOnly(true);
                    buffers.at(x)->textEdit->moveCursor(QTextCursor::Start);

                    if (direction) {
                        for (int z = 0; z != (current_line - 2); ++z)
                            buffers.at(x)->textEdit->moveCursor(QTextCursor::Down);
                    } else {
                        for (int z = 0; z != current_line; ++z)
                            buffers.at(x)->textEdit->moveCursor(QTextCursor::Down);
                    }

                    buffers.at(x)->textEdit->moveCursor(QTextCursor::StartOfLine);
                    buffers.at(x)->textEdit->setTextInteractionFlags(buffers.at(x)->textEdit->textInteractionFlags()
                                                                     | Qt::TextSelectableByKeyboard);
                    QString current_mode = buffers.at(x)->chFr->text();
                    buffers.at(x)->chFr->setText(current_mode.replace(0, 2, "%%"));
                }
            }
        }
    } else {
        minibuffer->clear();
        mode_minibuffer->clear();

        mode_minibuffer->setText(tr("There is no buffer on the current line."));
        mode_minibuffer->setStyleSheet("color: black;");
    }
}

//==========================================================================================
// Set "S" infront of buffer name, meaning that it is mark for saving
//==========================================================================================

void MainWindow::mark_buffer_for_saving(int current_line)
{
    if (current_line > 2) {
        QSplitter *splitter = current_splitter();
        QList<TextEditor *> buffers = splitter->findChildren<TextEditor *>();
        for (int x = 0; x != buffers.count(); ++x) {
            if (buffers.at(x)->objectName() == "*Buffer List*") {

                QString current_string = list_of_strings_for_every_buffer.at(current_line - 1);

                if (current_string.at(2) == 'S') {
                    mode_minibuffer->setText(tr("The current buffer is already marked for saving."));
                    mode_minibuffer->setStyleSheet("color:black;");
                    return;
                } else {

                    list_of_buffers_for_saving << list_of_activated_buffers.at(current_line - 3);

                    buffers.at(x)->textEdit->setReadOnly(false);
                    current_string.replace(2, 1, "S");
                    list_of_strings_for_every_buffer.replace((current_line - 1), current_string);
                    buffers.at(x)->textEdit->clear();

                    for (int y = 0; y != list_of_strings_for_every_buffer.count(); ++y)
                        buffers.at(x)->textEdit->appendPlainText(list_of_strings_for_every_buffer.at(y));

                    buffers.at(x)->textEdit->document()->setModified(false);
                    buffers.at(x)->textEdit->setReadOnly(true);

                    buffers.at(x)->textEdit->moveCursor(QTextCursor::Start);

                    for (int z = 0; z != current_line; ++z)
                        buffers.at(x)->textEdit->moveCursor(QTextCursor::Down);

                    buffers.at(x)->textEdit->moveCursor(QTextCursor::StartOfLine);
                    buffers.at(x)->textEdit->setTextInteractionFlags(buffers.at(x)->textEdit->textInteractionFlags()
                                                                     | Qt::TextSelectableByKeyboard);
                    QString current_mode = buffers.at(x)->chFr->text();
                    buffers.at(x)->chFr->setText(current_mode.replace(0, 2, "%%"));
                }
            }
        }
    } else {
        minibuffer->clear();
        mode_minibuffer->clear();

        mode_minibuffer->setText(tr("There is no buffer on the current line."));
        mode_minibuffer->setStyleSheet("color: black;");
    }
}

//==========================================================================================
// "m" Mark this line's buffer to be displayed in another window if you exit with the v command.
// The request shows as a ‘>’ at the beginning of the line.
// (A single buffer may not have both a delete request and a display request.)
// If we have only two buffers marked and we press "2" insight *Buffer List*, we open these
// two buffers with a Horizontal Splitter
//==========================================================================================

void MainWindow::mark_buffer(int current_line)
{
    if (current_line > 2) {
        QSplitter *splitter = current_splitter();
        QList<TextEditor *> buffers = splitter->findChildren<TextEditor *>();
        for (int x = 0; x != buffers.count(); ++x) {
            if (buffers.at(x)->objectName() == "*Buffer List*") {

                QString current_string = list_of_strings_for_every_buffer.at(current_line - 1);

                if (current_string.at(0) == 'D') {
                    mode_minibuffer->setText(tr("A single buffer may not have both a delete request and a display request."));
                    mode_minibuffer->setStyleSheet("color:black;");
                    return;
                } else if (current_string.at(0) == '>') {
                    mode_minibuffer->setText(tr("The current buffer is already marked."));
                    mode_minibuffer->setStyleSheet("color:black;");
                    return;
                } else if (current_string.at(2) == 'S') {
                    mode_minibuffer->setText(tr("A single buffer may not have both a save request and a display request. Type ? for help. "));
                    mode_minibuffer->setStyleSheet("color:black;");
                    return;
                } else {

                    list_of_marked_buffers << list_of_activated_buffers.at(current_line - 3);

                    buffers.at(x)->textEdit->setReadOnly(false);
                    current_string.replace(0, 1, ">");
                    list_of_strings_for_every_buffer.replace((current_line - 1), current_string);
                    buffers.at(x)->textEdit->clear();

                    for (int y = 0; y != list_of_strings_for_every_buffer.count(); ++y)
                        buffers.at(x)->textEdit->appendPlainText(list_of_strings_for_every_buffer.at(y));

                    buffers.at(x)->textEdit->document()->setModified(false);
                    buffers.at(x)->textEdit->setReadOnly(true);

                    buffers.at(x)->textEdit->moveCursor(QTextCursor::Start);

                    for (int z = 0; z != current_line; ++z)
                        buffers.at(x)->textEdit->moveCursor(QTextCursor::Down);

                    buffers.at(x)->textEdit->moveCursor(QTextCursor::StartOfLine);
                    buffers.at(x)->textEdit->setTextInteractionFlags(buffers.at(x)->textEdit->textInteractionFlags()
                                                                     | Qt::TextSelectableByKeyboard);
                    QString current_mode = buffers.at(x)->chFr->text();
                    buffers.at(x)->chFr->setText(current_mode.replace(0, 2, "%%"));
                }
            }
        }
    } else {
        minibuffer->clear();
        mode_minibuffer->clear();

        mode_minibuffer->setText(tr("There is no buffer on the current line."));
        mode_minibuffer->setStyleSheet("color: black;");
    }
}

//==========================================================================================
// "x" Execute deleting for all buffers marked with "D" in the *Buffer List*
//==========================================================================================

void MainWindow::kill_all_marked_buffers()
{

    // We use this variable to stop asking whether to kill or not the buffer
    bool stop_asking = false;

    // We use this variable to stop killing the buffers and continue to work with the editor
    bool leave = false;

    if (!list_of_buffers_for_killing.isEmpty()) {
        for (int x = 0; x != list_of_buffers_for_killing.count(); ++x) {
            if (!leave) {
                if (!stop_asking) {
                    if (list_of_buffers_for_killing.at(x)->textEdit->document()->isModified()) {

                        if (list_of_buffers_for_saving.contains(list_of_buffers_for_killing.at(x))) {

                            // We take the current buffer for deleting ... [*]
                            TextEditor* current_buffer_for_deleting = list_of_buffers_for_killing.at(x);
                            QSplitter *buffer_splitter = qobject_cast<QSplitter *>(current_buffer_for_deleting->parentWidget());
                            delete current_buffer_for_deleting;
                            delete_buffer_completely(buffer_splitter);

                            // [*]... and remove it from the list_of_activated_buffers
                            QList<TextEditor *>::iterator iter = list_of_activated_buffers.begin();
                            while (iter != list_of_activated_buffers.end()) {
                                if (*iter == current_buffer_for_deleting) {
                                    iter = list_of_activated_buffers.erase(iter);
                                } else
                                    ++iter;
                            }
                        } else {

                            set_check_before_killing_mode();

                            if (minibuffer->get_the_string() == "yes") {

                                stop_asking = true;

                                // We take the current buffer for deleting ... [*]
                                TextEditor* current_buffer_for_deleting = list_of_buffers_for_killing.at(x);
                                QSplitter *buffer_splitter = qobject_cast<QSplitter *>(current_buffer_for_deleting->parentWidget());
                                delete current_buffer_for_deleting;
                                delete_buffer_completely(buffer_splitter);

                                // [*]... and remove it from the list_of_activated_buffers
                                QList<TextEditor *>::iterator iter = list_of_activated_buffers.begin();
                                while (iter != list_of_activated_buffers.end()) {
                                    if (*iter == current_buffer_for_deleting) {
                                        iter = list_of_activated_buffers.erase(iter);
                                    } else
                                        ++iter;
                                }

                            } else {
                                stop_asking = true;
                                leave = true;
                            }
                        }
                    } else {
                        // Here we check whether a buffer which is not modified was marked with "S"
                        if (list_of_buffers_for_saving.contains(list_of_buffers_for_killing.at(x))) {
                            mode_minibuffer->setText("(No changes need to be saved)");
                            mode_minibuffer->setStyleSheet("color:black;");
                        }

                        // We take the current buffer for deleting ... [*]
                        TextEditor* current_buffer_for_deleting = list_of_buffers_for_killing.at(x);
                        QSplitter *buffer_splitter = qobject_cast<QSplitter *>(current_buffer_for_deleting->parentWidget());
                        delete current_buffer_for_deleting;
                        delete_buffer_completely(buffer_splitter);

                        QList<TextEditor *>::iterator iter = list_of_activated_buffers.begin();
                        while (iter != list_of_activated_buffers.end()) {
                            if (*iter == current_buffer_for_deleting) {
                                iter = list_of_activated_buffers.erase(iter);
                            } else
                                ++iter;
                        }
                    }
                } else {
                    // We take the current buffer for deleting ... [*]
                    TextEditor* current_buffer_for_deleting = list_of_buffers_for_killing.at(x);
                    QSplitter *buffer_splitter = qobject_cast<QSplitter *>(current_buffer_for_deleting->parentWidget());
                    delete current_buffer_for_deleting;
                    delete_buffer_completely(buffer_splitter);

                    QList<TextEditor *>::iterator iter = list_of_activated_buffers.begin();
                    while (iter != list_of_activated_buffers.end()) {
                        if (*iter == current_buffer_for_deleting) {
                            iter = list_of_activated_buffers.erase(iter);
                        } else
                            ++iter;
                    }

                }
            } else {               
                // We put the focus on the first buffer of the list, so we will have active buffer
                // when update the *Buffer List*
                if (!list_of_activated_buffers.isEmpty()) {
                    for (int c = 0; c != list_of_activated_buffers.count(); c++)
                        if (list_of_activated_buffers.at(c)->objectName() != "*Buffer List*") {
                            list_of_activated_buffers.at(c)->textEdit->setFocus();
                            break;
                        }
                }

                // Update the *Buffer List*
                // And before leaving this function, delete the old data from the list "list_of_buffers_for_killing"
                // so it is ready for the next time when we call "x" */

                list_buffers();

                if (!list_of_buffers_for_killing.isEmpty()) {
                    QList<TextEditor *>::iterator it = list_of_buffers_for_killing.begin();
                    while (it != list_of_buffers_for_killing.end())
                        it = list_of_buffers_for_killing.erase(it);
                }

                return;
            }
        }

        // We put the focus on the first buffer of the list, so we will have active buffer
        if (!list_of_activated_buffers.isEmpty()) {
            for (int c = 0; c != list_of_activated_buffers.count(); c++)
                if (list_of_activated_buffers.at(c)->objectName() != "*Buffer List*") {
                    list_of_activated_buffers.at(c)->textEdit->setFocus();
                    break;
                }
        }

        // Here we update the *Buffer List*
        list_buffers();

        // Delete all of the old data from this list, so it will be ready for next call
        if (!list_of_buffers_for_killing.isEmpty()) {
            QList<TextEditor *>::iterator it = list_of_buffers_for_killing.begin();
            while (it != list_of_buffers_for_killing.end())
                it = list_of_buffers_for_killing.erase(it);
        }

        // Delete all of the old data from this list, so it will be ready for next call
        if (!list_of_buffers_for_saving.isEmpty()) {
            QList<TextEditor *>::iterator it = list_of_buffers_for_saving.begin();
            while (it != list_of_buffers_for_saving.end())
                it = list_of_buffers_for_saving.erase(it);
        }

    } else if (list_of_buffers_for_killing.isEmpty() && !list_of_buffers_for_saving.isEmpty()) {
        // Here starts the case in which there aren't any marked buffers for killing but there are
        // buffers for saving
        for (int j = 0; j != list_of_buffers_for_saving.count(); ++j) {
            if (list_of_buffers_for_saving.at(j)->textEdit->document()->isModified()) {
                save_buffer_before_killing(list_of_buffers_for_saving.at(j));
            } else {
                mode_minibuffer->setText("(No changes need to be saved)");
                mode_minibuffer->setStyleSheet("color:black;");
            }
        }

        // We put the focus on the first buffer of the list, so we will have active buffer
        if (!list_of_activated_buffers.isEmpty()) {
            for (int c = 0; c != list_of_activated_buffers.count(); c++)
                if (list_of_activated_buffers.at(c)->objectName() != "*Buffer List*") {
                    list_of_activated_buffers.at(c)->textEdit->setFocus();
                    break;
                }
        }

        list_buffers();

        QList<TextEditor *>::iterator it = list_of_buffers_for_saving.begin();
        while (it != list_of_buffers_for_saving.end()) {
            it = list_of_buffers_for_saving.erase(it);
        }

    } else {
        mode_minibuffer->setText("There aren't any marked buffers for deleting or saving.");
        mode_minibuffer->setStyleSheet("color:black;");
    }
}

void MainWindow::set_check_before_killing_mode()
{   
    is_minibuffer_busy = true;
    is_focus_on_minibuffer = false;
    emit text_for_editor("is_busy", "set_minibuffer");

    minibuffer->clear();
    mode_minibuffer->clear();

    mode_minibuffer->setText("Modified buffers exist; kill anyway? (yes or no)");
    mode_minibuffer->setStyleSheet("color:blue;");

    minibuffer->setReadOnly(false);
    minibuffer->setFocus();

    {
        in_loop_exec = true;
        QEventLoop loop;
        loop.connect(this, SIGNAL(ready_to_exit()), &loop, SLOT(quit()));
        loop.exec();
    }
}

void MainWindow::save_buffer_before_killing(TextEditor *editor)
{
    if(editor) {
        QString errorMsg;
        QFileInfo fi(editor->currentFilePath());

        if(fi.isDir())
            errorMsg = editor->saveFileNonGui(editor->currentFilePath() + "/" + editor->currentFile());
        else
            errorMsg = editor->saveFileNonGui(editor->currentFilePath());

        if(!errorMsg.isEmpty())
        {
            mode_minibuffer->setText(errorMsg);
            mode_minibuffer->setStyleSheet("color: red;");
            return;
        }

        set_wrote_buffer_to_disk_mode(editor);
    }
}

//==========================================================================================
// "1" ... When we are in a *Buffer List* and press "1" on a line with buffer in it, show
// this buffer allone without any other buffers or splitters
//==========================================================================================

void MainWindow::show_buffer_alone(int current_line)
{
    if (current_line > 2) {
        show_buffer_from_list_buffers(current_line);
        delete_other_windows();
    } else {
        minibuffer->clear();
        mode_minibuffer->clear();

        mode_minibuffer->setText(tr("There is no buffer on the current line."));
        mode_minibuffer->setStyleSheet("color: black;");
    }
}

//==========================================================================================
// Show the message in the minibuffer that the *Buffer List* is read only
// when pressing a key different from the commands we have
//==========================================================================================

void MainWindow::buffer_list_is_read_only()
{
    minibuffer->clear();
    mode_minibuffer->clear();

    mode_minibuffer->setText(tr("Buffer is read-only: #<*Buffer List*>"));
    mode_minibuffer->setStyleSheet("color: black;");
}

//==========================================================================================
// "%" ... Toggle the buffer's read-only flag. The command % does this immediately when you
// type it.
//==========================================================================================

void MainWindow::toggle_read_only_buffer_list(int current_line)
{
    if (current_line > 2) {
        TextEditor *editor = list_of_activated_buffers.at(current_line - 3);

        QString current_mode = editor->chFr->text();

        if(editor->textEdit->isReadOnly()) {
            editor->textEdit->setReadOnly(false);

            if(editor->textEdit->document()->isModified())
                editor->chFr->setText(current_mode.replace(0, 1, "*"));
            else
                editor->chFr->setText(current_mode.replace(0, 2, "--"));


            QSplitter *splitter = current_splitter();
            QList<TextEditor *> buffers = splitter->findChildren<TextEditor *>();
            for (int x = 0; x != buffers.count(); ++x) {
                if (buffers.at(x)->objectName() == "*Buffer List*") {

                    QString current_string = list_of_strings_for_every_buffer.at(current_line - 1);

                    buffers.at(x)->textEdit->setReadOnly(false);
                    current_string.replace(1, 1, " ");
                    list_of_strings_for_every_buffer.replace((current_line - 1), current_string);
                    buffers.at(x)->textEdit->clear();

                    for (int y = 0; y != list_of_strings_for_every_buffer.count(); ++y)
                        buffers.at(x)->textEdit->appendPlainText(list_of_strings_for_every_buffer.at(y));

                    buffers.at(x)->textEdit->document()->setModified(false);
                    buffers.at(x)->textEdit->setReadOnly(true);

                    buffers.at(x)->textEdit->moveCursor(QTextCursor::Start);

                    for (int z = 0; z != current_line; ++z)
                        buffers.at(x)->textEdit->moveCursor(QTextCursor::Down);

                    buffers.at(x)->textEdit->moveCursor(QTextCursor::StartOfLine);
                    buffers.at(x)->textEdit->setTextInteractionFlags(buffers.at(x)->textEdit->textInteractionFlags()
                                                                     | Qt::TextSelectableByKeyboard);
                    QString current_mode = buffers.at(x)->chFr->text();
                    buffers.at(x)->chFr->setText(current_mode.replace(0, 2, "%%"));
                }
            }

        } else {
            editor->textEdit->setReadOnly(true);
            editor->textEdit->setTextInteractionFlags(editor->textEdit->textInteractionFlags() | Qt::TextSelectableByKeyboard);

            if(editor->textEdit->document()->isModified())
                editor->chFr->setText(current_mode.replace(0, 1, "%"));
            else
                editor->chFr->setText(current_mode.replace(0, 2, "%%"));

            QSplitter *splitter = current_splitter();
            QList<TextEditor *> buffers = splitter->findChildren<TextEditor *>();
            for (int x = 0; x != buffers.count(); ++x) {
                if (buffers.at(x)->objectName() == "*Buffer List*") {

                    QString current_string = list_of_strings_for_every_buffer.at(current_line - 1);

                    buffers.at(x)->textEdit->setReadOnly(false);
                    current_string.replace(1, 1, "%");
                    list_of_strings_for_every_buffer.replace((current_line - 1), current_string);
                    buffers.at(x)->textEdit->clear();

                    for (int y = 0; y != list_of_strings_for_every_buffer.count(); ++y)
                        buffers.at(x)->textEdit->appendPlainText(list_of_strings_for_every_buffer.at(y));

                    buffers.at(x)->textEdit->document()->setModified(false);
                    buffers.at(x)->textEdit->setReadOnly(true);

                    buffers.at(x)->textEdit->moveCursor(QTextCursor::Start);

                    for (int z = 0; z != current_line; ++z)
                        buffers.at(x)->textEdit->moveCursor(QTextCursor::Down);

                    buffers.at(x)->textEdit->moveCursor(QTextCursor::StartOfLine);
                    buffers.at(x)->textEdit->setTextInteractionFlags(buffers.at(x)->textEdit->textInteractionFlags()
                                                                     | Qt::TextSelectableByKeyboard);
                    QString current_mode = buffers.at(x)->chFr->text();
                    buffers.at(x)->chFr->setText(current_mode.replace(0, 2, "%%"));
                }
            }
        }
    } else {
        minibuffer->clear();
        mode_minibuffer->clear();

        mode_minibuffer->setText(tr("There is no buffer on the current line."));
        mode_minibuffer->setStyleSheet("color: black;");
    }
}

//==========================================================================================
// "~" ... Mark the buffer “unmodified”. The command ~ does this immediately when you type it.
//==========================================================================================

void MainWindow::set_modified_buffer_list(int current_line)
{
    if (current_line > 2) {
        TextEditor *editor = list_of_activated_buffers.at(current_line - 3);

        if (editor->textEdit->document()->isModified()) {
            editor->textEdit->document()->setModified(false);

            minibuffer->clear();
            mode_minibuffer->clear();

            mode_minibuffer->setText(tr("Modification-flag cleared"));
            mode_minibuffer->setStyleSheet("color:black;");

            QSplitter *splitter = current_splitter();
            QList<TextEditor *> buffers = splitter->findChildren<TextEditor *>();
            for (int x = 0; x != buffers.count(); ++x) {
                if (buffers.at(x)->objectName() == "*Buffer List*") {

                    QString current_string = list_of_strings_for_every_buffer.at(current_line - 1);

                    buffers.at(x)->textEdit->setReadOnly(false);
                    current_string.replace(2, 1, " ");
                    list_of_strings_for_every_buffer.replace((current_line - 1), current_string);
                    buffers.at(x)->textEdit->clear();

                    for (int y = 0; y != list_of_strings_for_every_buffer.count(); ++y)
                        buffers.at(x)->textEdit->appendPlainText(list_of_strings_for_every_buffer.at(y));

                    buffers.at(x)->textEdit->document()->setModified(false);
                    buffers.at(x)->textEdit->setReadOnly(true);

                    buffers.at(x)->textEdit->moveCursor(QTextCursor::Start);

                    for (int z = 0; z != current_line; ++z)
                        buffers.at(x)->textEdit->moveCursor(QTextCursor::Down);

                    buffers.at(x)->textEdit->moveCursor(QTextCursor::StartOfLine);
                    buffers.at(x)->textEdit->setTextInteractionFlags(buffers.at(x)->textEdit->textInteractionFlags()
                                                                     | Qt::TextSelectableByKeyboard);
                    QString current_mode = buffers.at(x)->chFr->text();
                    buffers.at(x)->chFr->setText(current_mode.replace(0, 2, "%%"));
                }
            }
        } else {
            editor->textEdit->document()->setModified(true);
            mode_minibuffer->setText(tr("Modification-flag set"));
            mode_minibuffer->setStyleSheet("color:black;");

            QSplitter *splitter = current_splitter();
            QList<TextEditor *> buffers = splitter->findChildren<TextEditor *>();
            for (int x = 0; x != buffers.count(); ++x) {
                if (buffers.at(x)->objectName() == "*Buffer List*") {

                    QString current_string = list_of_strings_for_every_buffer.at(current_line - 1);

                    buffers.at(x)->textEdit->setReadOnly(false);
                    current_string.replace(2, 1, "*");
                    list_of_strings_for_every_buffer.replace((current_line - 1), current_string);
                    buffers.at(x)->textEdit->clear();

                    for (int y = 0; y != list_of_strings_for_every_buffer.count(); ++y)
                        buffers.at(x)->textEdit->appendPlainText(list_of_strings_for_every_buffer.at(y));

                    buffers.at(x)->textEdit->document()->setModified(false);
                    buffers.at(x)->textEdit->setReadOnly(true);

                    buffers.at(x)->textEdit->moveCursor(QTextCursor::Start);

                    for (int z = 0; z != current_line; ++z)
                        buffers.at(x)->textEdit->moveCursor(QTextCursor::Down);

                    buffers.at(x)->textEdit->moveCursor(QTextCursor::StartOfLine);
                    buffers.at(x)->textEdit->setTextInteractionFlags(buffers.at(x)->textEdit->textInteractionFlags()
                                                                     | Qt::TextSelectableByKeyboard);
                    QString current_mode = buffers.at(x)->chFr->text();
                    buffers.at(x)->chFr->setText(current_mode.replace(0, 2, "%%"));
                }
            }
        }
    } else {
        minibuffer->clear();
        mode_minibuffer->clear();

        mode_minibuffer->setText(tr("There is no buffer on the current line."));
        mode_minibuffer->setStyleSheet("color: black;");
    }
}

//==========================================================================================
// "q" ... Quit the buffer menu—immediately display the most recent formerly visible buffer
// int its place.
//==========================================================================================

void MainWindow::quit_buffer_list()
{
    delete_window();
}

//===========================================================================================
// "f" ... Immediately select this line's buffer in place of the *Buffer List* buffer.
//===========================================================================================

//void MainWindow::show_buffer_instead_buffer_list(int current_line)
//{
//    qDebug() << "Here";
//    if (current_line > 2) {
//        QSplitter *splitter = current_splitter();
//        QList<TextEditor *> all_buffers = splitter->findChildren<TextEditor *>();

//        for (int i = 0; i != all_buffers.count(); ++i) {
//            if (all_buffers.at(i)->objectName() == "*Buffer List*") {
//                int buffer_index = splitter->indexOf((all_buffers.at(i)->parentWidget())->parentWidget());

//                qDebug() << buffer_index;

//                if ( buffer_index >= 0) {

//                    TextEditor* buffer_for_showing = list_of_activated_buffers.at(current_line - 3);
//                    QSplitter *buffer_splitter_for_showing = qobject_cast<QSplitter *>(buffer_for_showing->parentWidget());
//                    QWidget* widget_of_splitter = qobject_cast<QWidget *>(buffer_splitter_for_showing->parentWidget());

//                    TextEditor* buffer_list = all_buffers.at(i);
//                    QSplitter *buffer_list_splitter = qobject_cast<QSplitter *>(buffer_list->parentWidget());

//                    delete buffer_list;
//                    delete_buffer_completely(buffer_list_splitter);

//                    splitter->insertWidget(buffer_index, widget_of_splitter);
//                    buffer_for_showing->textEdit->setFocus();
//                }
//            }
//        }
//    } else {
//        minibuffer->clear();
//        mode_minibuffer->clear();

//        mode_minibuffer->setText(tr("There is no buffer on the current line."));
//        mode_minibuffer->setStyleSheet("color: black;");
//    }
//}

//==========================================================================================
// "o" ... Immediately select this line's buffer in another window as if by C-x 4 b,
// leaving *Buffer List* visible.
//==========================================================================================

void MainWindow::show_buffer_other_window_buffer_list(int current_line)
{
    if (current_line > 2) {
        if (!mdiArea->subWindowList().isEmpty()) {
            // Here we start looking for *Buffer List*
            QList<TextEditor*> buffers = all_buffers();
            if(!buffers.isEmpty()) {
                for(int x = 0; x != buffers.count(); ++x) {
                    if(buffers.at(x)->objectName() == "*Buffer List*") {

                        TextEditor *buffer_list = buffers.at(x);
                        QWidget *widget_buffer_list = qobject_cast<QWidget *>(buffer_list->parentWidget()->parentWidget());

                        TextEditor *buffer_for_showing = list_of_activated_buffers.at(current_line - 3);
                        QWidget *widget_of_the_buffer = qobject_cast<QWidget *>(buffer_for_showing->parentWidget()->parentWidget());

                        // When we found the *Buffer List* we need, we create the new window
                        // which will show the buffer chosen from the list with the *Buffer List*
                        // on the bottom

                        QSplitter *splitter = create_splitter();
                        splitter->addWidget(widget_of_the_buffer);
                        splitter->setOrientation(Qt::Vertical);
                        splitter->addWidget(widget_buffer_list);
                        buffer_for_showing->textEdit->setFocus();
                    }
                }
            }
        }
    } else {
        minibuffer->clear();
        mode_minibuffer->clear();

        mode_minibuffer->setText(tr("There is no buffer on the current line."));
        mode_minibuffer->setStyleSheet("color: black;");
    }
}

//==========================================================================================
// "g" ... If you have created, deleted or renamed buffers, the way to update *Buffer List*
// to show what you have done is to type "g" from insight *Buffer List*.
//==========================================================================================

//void MainWindow::revert_buffer()
//{
//    // We put the focus on the first buffer of the list, so we will have active buffer
//    // when update the *Buffer List*
//    if (!list_of_activated_buffers.isEmpty()) {
//        for (int x = 0; x != list_of_activated_buffers.count(); x++)
//            if (list_of_activated_buffers.at(x)->objectName() != "*Buffer List*") {
//                list_of_activated_buffers.at(x)->textEdit->setFocus();
//                break;
//            }
//    }

//    list_buffers();
//}

//==========================================================================================
// "2" ... Immediately set up two windows, with this line's buffer selected in one, and the
// previously current buffer (aside from the buffer *Buffer List*) displayed in the other.
//==========================================================================================

void MainWindow::set_up_two_buffers_in_buffer_list(int current_line)
{
    if (current_line > 2) {

        if (list_of_marked_buffers.count() == 2) {

            // This is the first buffer marked
            TextEditor *first_buffer = list_of_marked_buffers.at(0);
            QSplitter *first_buffer_splitter = qobject_cast<QSplitter *>(first_buffer->parentWidget());
            QWidget *first_buffer_widget = qobject_cast<QWidget *>(first_buffer_splitter->parentWidget());
            QSplitter *parent_splitter_first = qobject_cast<QSplitter *>(first_buffer_widget->parentWidget());

            // This is the second buffer marked
            TextEditor *second_buffer = list_of_marked_buffers.at(1);
            QSplitter *second_buffer_splitter = qobject_cast<QSplitter *>(second_buffer->parentWidget());
            QWidget *second_buffer_widget = qobject_cast<QWidget *>(second_buffer_splitter->parentWidget());
            QSplitter *parent_splitter_second = qobject_cast<QSplitter *>(second_buffer_widget->parentWidget());

            // We add the first one to the Main Splitter
            QSplitter *splitter = create_splitter();

            splitter->addWidget(first_buffer_widget);
            hide_one_level_up(parent_splitter_first);

            splitter->addWidget(second_buffer_widget);
            hide_one_level_up(parent_splitter_second);

            if (!first_buffer_widget->isVisible())
                first_buffer_widget->show();

            if (!second_buffer_widget->isVisible())
                second_buffer_widget->show();

            second_buffer->textEdit->setFocus();

            // Here we delete the old data before to using it next time
            if (!list_of_marked_buffers.isEmpty()) {
                QList<TextEditor *>::iterator it = list_of_marked_buffers.begin();
                while (it != list_of_marked_buffers.end())
                    it = list_of_marked_buffers.erase(it);
            }

        } else if (list_of_marked_buffers.isEmpty()) {
            minibuffer->clear();
            mode_minibuffer->clear();

            mode_minibuffer->setText(tr("There aren't any marked buffers."));
            mode_minibuffer->setStyleSheet("color: black;");
        } else if (list_of_marked_buffers.count() == 1) {
            minibuffer->clear();
            mode_minibuffer->clear();

            mode_minibuffer->setText(tr("There is only one marked buffer. For this command you need two buffers."));
            mode_minibuffer->setStyleSheet("color: black;");
        } else {
            minibuffer->clear();
            mode_minibuffer->clear();

            mode_minibuffer->setText(tr("There are more than two marked buffers. For this command you need exactly two buffers."));
            mode_minibuffer->setStyleSheet("color: black;");
        }
    } else {
        minibuffer->clear();
        mode_minibuffer->clear();

        mode_minibuffer->setText(tr("There is no buffer on the current line."));
        mode_minibuffer->setStyleSheet("color: black;");
    }
}

//==========================================================================================
// "v" ... Immediately select this line's buffer, and also display in other windows any
// buffers previously marked with the "m" command. If you have not marked any buffers, this
// command is equivalent to 1
//==========================================================================================

void MainWindow::show_buffers_other_windows_v(int current_line)
{
    if (current_line > 2) {

        if (list_of_marked_buffers.isEmpty()) {
            TextEditor *buffer = list_of_activated_buffers.at(current_line - 3);
            buffer->textEdit->setFocus();
            delete_other_windows();
        } else {

            for (int x = 0; x != list_of_marked_buffers.count(); ++x) {

                TextEditor *buffer = list_of_marked_buffers.at(x);
                QWidget *buffer_widget = qobject_cast<QWidget *>(buffer->parentWidget()->parentWidget());
                QSplitter *parent_splitter = qobject_cast<QSplitter *>(buffer_widget->parentWidget());

                QSplitter *splitter = create_splitter();
                splitter->addWidget(buffer_widget);

                hide_one_level_up(parent_splitter);

                if (!buffer_widget->isVisible())
                    buffer_widget->show();

                buffer->textEdit->setFocus();

            }
        }

        // Here we delete the old data before using it next time
        if (!list_of_marked_buffers.isEmpty()) {
            QList<TextEditor *>::iterator it = list_of_marked_buffers.begin();
            while (it != list_of_marked_buffers.end())
                it = list_of_marked_buffers.erase(it);
        }
    } else {
        minibuffer->clear();
        mode_minibuffer->clear();

        mode_minibuffer->setText(tr("There is no buffer on the current line."));
        mode_minibuffer->setStyleSheet("color: black;");
    }
}

//==========================================================================================
// "r" ... Rename the buffer from *Buffer List* when pressing "r"
//==========================================================================================

void MainWindow::set_rename_mode_from_buffer_list(int line_number)
{
    if (line_number > 2) {

        is_minibuffer_busy = true;
        is_focus_on_minibuffer = false;
        emit text_for_editor("is_busy", "set_minibuffer");

        minibuffer->clear();
        mode_minibuffer->clear();

        mode_minibuffer->setText(tr("Rename buffer:"));
        mode_minibuffer->setStyleSheet("color: blue;");

        minibuffer->setReadOnly(false);
        minibuffer->setFocus();

        current_line = line_number;

    } else {
        minibuffer->clear();
        mode_minibuffer->clear();

        mode_minibuffer->setText(tr("There is no buffer on the current line."));
        mode_minibuffer->setStyleSheet("color: black;");
    }
}

void MainWindow::rename_the_buffer_from_buffer_list()
{
    QString new_name = minibuffer->get_the_string();

    TextEditor *buffer = list_of_activated_buffers.at(current_line - 3);

    buffer->set_current_file_name(new_name);

    // After the using of the variable current_line we init it to zero again
    current_line = 0;

    list_buffers();
}

//==========================================================================================
// C-x C-q ................ Toggle read-only mode
//==========================================================================================

void MainWindow::toggle_read_only()
{
    TextEditor *editor = current_buffer();

    if(editor) {

        QString current_mode = editor->chFr->text();

        if(editor->textEdit->isReadOnly()) {
            editor->textEdit->setReadOnly(false);

            if(editor->textEdit->document()->isModified())
                editor->chFr->setText(current_mode.replace(0, 1, "*"));
            else
                editor->chFr->setText(current_mode.replace(0, 2, "--"));
        } else {
            editor->textEdit->setReadOnly(true);
            editor->textEdit->setTextInteractionFlags(editor->textEdit->textInteractionFlags() | Qt::TextSelectableByKeyboard);

            if(editor->textEdit->document()->isModified())
                editor->chFr->setText(current_mode.replace(0, 1, "%"));
            else
                editor->chFr->setText(current_mode.replace(0, 2, "%%"));
        }
    }
}

//==========================================================================================
// M-x rename-buffer <RET> name <RET> ................. Change the name of the current buffer.
//==========================================================================================

void MainWindow::set_rename_buffer_mode()
{
    mode_minibuffer->setText(tr("Rename buffer (to new name):"));
    mode_minibuffer->setStyleSheet("color: blue;");
    minibuffer->setReadOnly(false);
    minibuffer->setFocus();
}

void MainWindow::rename_buffer(const QString &name)
{
    if(mdiArea->activeSubWindow())
        mdiArea->activeSubWindow()->setFocus();

    TextEditor *editor = current_buffer();
    if(editor) {
        editor->set_current_file_name(name);
    }
}

//==========================================================================================
//
//
//                          Exiting Redactor
//
//
//==========================================================================================

//==========================================================================================
// This method is call when the user tries to quit from Redactor
//==========================================================================================

void MainWindow::closeEvent(QCloseEvent *event)
{
    QList<QMdiSubWindow *> windows = mdiArea->subWindowList();

    for (int i = 0; i != windows.count(); ++i) {

        QMdiSubWindow *window = mdiArea->activeSubWindow();
        QSplitter *splitter = qobject_cast<QSplitter *>(window->widget());
        QList<TextEditor *> editors = splitter->findChildren<TextEditor *>();

        bool stop_asking = false;

        for(int i = 0; i != editors.size(); ++i) {

            if (editors.at(i)->objectName() == "*Buffer List*" ||
                    editors.at(i)->objectName() == "*Messages*" ||
                    editors.at(i)->objectName() == "*Home Page*" ||
                    editors.at(i)->objectName() == "*Dired*" ||
                    editors.at(i)->objectName() == "Counter" ||
                    editors.at(i)->objectName() == "*Shell*") { /* do nothing */ }

            else {

                if (!stop_asking) {

                    if (confirm_exit_redactor(editors.at(i))) {

                        buffer_for_deletion = editors.at(i);
                        set_exit_mode();
                        buffer_for_deletion = 0;

                        if (minibuffer->get_the_string() == "escape") {
                            event->ignore();
                            buffer_for_deletion = 0;
                            return;
                        } else if (minibuffer->get_the_string() == "y") {
                            save_exit_from_minibuffer(editors.at(i));
                        } else if (minibuffer->get_the_string() == "n") {
                            /* do nothing */
                        } else if (minibuffer->get_the_string() == "q") {

                            set_modified_buffers_exist_mode();

                            if(minibuffer->get_the_string() == "yes" ) {
                                write_settings();
                                event->accept();
                                return;
                            } else {
                                event->ignore();
                                return;
                            }

                        } else if (minibuffer->get_the_string() == "!") {
                            save_exit_from_minibuffer(editors.at(i));
                            stop_asking = true;

                        } else if (minibuffer->get_the_string() == ".") {
                            TextEditor *editor = current_buffer();
                            save_exit_from_minibuffer(editor);
                            write_settings();
                            event->accept();
                            return;

                        } else if (minibuffer->get_the_string() == "help") {

                            add_help_window(sQ::ExitEditor);

                            event->ignore();

                            closeEvent(event);

                            return;
                        }

                        else {
                            event->ignore();
                            return;
                        }

                    }

                } else {
                    save_exit_from_minibuffer(editors.at(i));
                }
            }
        }

        mdiArea->closeActiveSubWindow();

    }

    if (mdiArea->currentSubWindow()) {
        event->ignore();
    } else {
        write_settings();
        event->accept();
    }
}

//==========================================================================================
// C-x C-c ............... Exit from Redactor.
//==========================================================================================

bool MainWindow::confirm_exit_redactor(TextEditor *editor)
{
    if(editor->textEdit->document()->isModified())
        return true;
    else
        return false;
}

void MainWindow::set_exit_mode()
{
    is_minibuffer_busy = true;
    is_focus_on_minibuffer = false;
    emit text_for_editor("is_busy", "set_minibuffer");

    minibuffer->clear();
    mode_minibuffer->clear();

    mode_minibuffer->setText("Save file " + buffer_for_deletion->currentFile() + "? (y, n, !, ., q, help)");
    mode_minibuffer->setStyleSheet("color: blue;");

    minibuffer->setReadOnly(false);
    minibuffer->setFocus();

    {
        in_loop_exec = true;
        QEventLoop loop;
        connect(this, SIGNAL(ready_to_exit()), &loop, SLOT(quit()));
        loop.exec();
    }
}

void MainWindow::save_exit_from_minibuffer(TextEditor *editor)
{
    if(editor) {
        QString errorMsg;
        QFileInfo fi(editor->currentFilePath());

        if(fi.isDir())
            errorMsg = editor->saveFileNonGui(editor->currentFilePath() + "/" + editor->currentFile());
        else
            errorMsg = editor->saveFileNonGui(editor->currentFilePath());

        if(!errorMsg.isEmpty())
        {
            mode_minibuffer->setText(errorMsg);
            mode_minibuffer->setStyleSheet("color: red;");
            return;
        }
    } else
        return;
}

//==========================================================================================
// C-z .................. Minimize the selected frame.
//==========================================================================================

void MainWindow::minimize_redactor()
{
    setWindowState(Qt::WindowMinimized);
}

//==========================================================================================
// Exit when pressing 'q'
//==========================================================================================

void MainWindow::set_modified_buffers_exist_mode()
{

    is_minibuffer_busy = true;
    is_focus_on_minibuffer = false;
    emit text_for_editor("is_busy", "set_minibuffer");

    minibuffer->clear();
    mode_minibuffer->clear();

    mode_minibuffer->setText("Modified buffers exist; exit anyway? (yes or no)");
    mode_minibuffer->setStyleSheet("color:blue;");

    minibuffer->setReadOnly(false);
    minibuffer->setFocus();

    {
        in_loop_exec = true;
        QEventLoop loop;
        loop.connect(this, SIGNAL(ready_to_exit()), &loop, SLOT(quit()));
        loop.exec();
    }
}

//==========================================================================================
//
//
//                          Visiting Files
//
//
//==========================================================================================

//==========================================================================================
// C-x C-f ................ Reading a file from minibuffer.
//==========================================================================================

void MainWindow::set_find_file_mode()
{

    is_minibuffer_busy = true;
    is_focus_on_minibuffer = false;
    emit text_for_editor("is_busy", "set_minibuffer");

    minibuffer->clear();
    mode_minibuffer->clear();

    mode_minibuffer->setText("Find file:");
    mode_minibuffer->setStyleSheet("color: blue;");

    minibuffer->setText(QDir::homePath() + "/");
    minibuffer->setReadOnly(false);
    minibuffer->setFocus();
}

void MainWindow::find_file(const QString &path)
{
    QFile file(path);

    if (QFileInfo(path).isDir()) {
        create_dired(path);
        return;
    } else {
        if (file.exists()) {
            if (mdiArea->subWindowList().isEmpty()) {

                if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    mode_minibuffer->setText(tr("Cannot read file %1: \n%2.").arg(path).arg(file.errorString()));
                    mode_minibuffer->setStyleSheet("color: red;");
                    return;
                }

                QSplitter *splitter = create_splitter();
                TextEditor *editor = create_text_editor();
                QSplitter *new_splitter = new QSplitter;
                new_splitter->setChildrenCollapsible(false);

                QWidget *container = new QWidget;
                QVBoxLayout *container_layout = new QVBoxLayout;
                container_layout->addWidget(new_splitter);
                container_layout->setContentsMargins(0, 0, 0, 0);
                container_layout->setSpacing(0);
                container->setLayout(container_layout);

                new_splitter->addWidget(editor);
                splitter->addWidget(container);

                QTextStream in(&file);
                QApplication::setOverrideCursor(Qt::WaitCursor);
                editor->textEdit->setPlainText(in.readAll());
                QApplication::restoreOverrideCursor();
                file.close();

                editor->setTitleFromMainWindow(path);
                editor->createModeLine();
                editor->change_the_highlighter();
                editor->textEdit->setFocus();

                is_writable(path);
                update_menus();

                return;
            } else {
                QSplitter *splitter = current_splitter();
                if (splitter) {
                    QList<QSplitter *> all_splitters = splitter->findChildren<QSplitter *>();
                    for (int x = 0; x != all_splitters.count(); ++x) {
                        if ( check_this_splitter(all_splitters.at(x)) ) {
                            TextEditor* buffer = return_current_buffer(all_splitters.at(x));
                            if (buffer) {

                                if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                                    mode_minibuffer->setText(tr("Cannot read file %1: \n%2.").arg(path).arg(file.errorString()));
                                    mode_minibuffer->setStyleSheet("color: red;");
                                    return;
                                }

                                QSplitter *parent_splitter = qobject_cast<QSplitter *>(((all_splitters.at(x)->parentWidget())->parentWidget()));

                                // Here we take the parent splitter' sizes of widgets, so we can
                                // set the same sizes when we alternate the buffer
                                QList<int> splitter_sizes = parent_splitter->sizes();

                                int buffer_index = parent_splitter->indexOf((buffer->parentWidget())->parentWidget());
                                (buffer->parentWidget())->parentWidget()->hide();

                                QSplitter *new_splitter = new QSplitter;
                                new_splitter->setChildrenCollapsible(false);
                                QWidget *new_container = new QWidget;
                                QVBoxLayout *new_container_layout = new QVBoxLayout;

                                new_container_layout->addWidget(new_splitter);
                                new_container_layout->setContentsMargins(0, 0, 0, 0);
                                new_container_layout->setSpacing(0);
                                new_container->setLayout(new_container_layout);

                                TextEditor *editor = create_text_editor();
                                new_splitter->addWidget(editor);

                                parent_splitter->insertWidget(buffer_index, new_container);
                                parent_splitter->setSizes(splitter_sizes);

                                QTextStream in(&file);
                                QApplication::setOverrideCursor(Qt::WaitCursor);
                                editor->textEdit->setPlainText(in.readAll());
                                QApplication::restoreOverrideCursor();
                                file.close();

                                editor->setTitleFromMainWindow(path);
                                editor->createModeLine();
                                editor->change_the_highlighter();
                                editor->textEdit->setFocus();

                                is_writable(path);
                                update_menus();

                                return;
                            }
                        }
                    }
                }
            }
        } else {
            if (mdiArea->subWindowList().isEmpty()) {
                QSplitter *splitter = create_splitter();
                TextEditor *editor = create_text_editor();
                QSplitter *new_splitter = new QSplitter;
                new_splitter->setChildrenCollapsible(false);

                QWidget *container = new QWidget;
                QVBoxLayout *container_layout = new QVBoxLayout;
                container_layout->addWidget(new_splitter);
                container_layout->setContentsMargins(0, 0, 0, 0);
                container_layout->setSpacing(0);
                container->setLayout(container_layout);

                new_splitter->addWidget(editor);
                splitter->addWidget(container);

                editor->setTitleFromMainWindow(path);
                editor->createModeLine();
                editor->change_the_highlighter();
                editor->textEdit->setFocus();

                mode_minibuffer->setText(tr("(New file)"));
                mode_minibuffer->setStyleSheet("color:black;");

                update_menus();

                return;
            } else {
                QSplitter *splitter = current_splitter();
                if (splitter) {
                    QList<QSplitter *> all_splitters = splitter->findChildren<QSplitter *>();
                    for (int x = 0; x != all_splitters.count(); ++x) {
                        if ( check_this_splitter(all_splitters.at(x)) ) {
                            TextEditor* buffer = return_current_buffer(all_splitters.at(x));
                            if (buffer) {

                                QSplitter *parent_splitter = qobject_cast<QSplitter *>(((all_splitters.at(x)->parentWidget())->parentWidget()));

                                // Here we take the parent splitter' sizes of widgets, so we can
                                // set the same sizes when we alternate the buffer
                                QList<int> splitter_sizes = parent_splitter->sizes();

                                int buffer_index = parent_splitter->indexOf((buffer->parentWidget())->parentWidget());
                                (buffer->parentWidget())->parentWidget()->hide();

                                QSplitter *new_splitter = new QSplitter;
                                new_splitter->setChildrenCollapsible(false);
                                QWidget *new_container = new QWidget;
                                QVBoxLayout *new_container_layout = new QVBoxLayout;

                                new_container_layout->addWidget(new_splitter);
                                new_container_layout->setContentsMargins(0, 0, 0, 0);
                                new_container_layout->setSpacing(0);
                                new_container->setLayout(new_container_layout);

                                TextEditor *editor = create_text_editor();
                                new_splitter->addWidget(editor);

                                parent_splitter->insertWidget(buffer_index, new_container);
                                parent_splitter->setSizes(splitter_sizes);

                                editor->setTitleFromMainWindow(path);
                                editor->createModeLine();
                                editor->change_the_highlighter();
                                editor->textEdit->setFocus();

                                mode_minibuffer->setText(tr("(New file)"));
                                mode_minibuffer->setStyleSheet("color:black;");

                                update_menus();

                                return;
                            }
                        }
                    }
                }
            }
        }
    }
}

//==========================================================================================
// C-x C-r ................ Visit a file for viewing, without allowing changes to it
//==========================================================================================

void MainWindow::set_find_file_read_only_mode()
{
    is_minibuffer_busy = true;
    is_focus_on_minibuffer = false;
    emit text_for_editor("is_busy", "set_minibuffer");

    minibuffer->clear();
    mode_minibuffer->clear();

    mode_minibuffer->setText(tr("Find file read only:"));
    mode_minibuffer->setStyleSheet("color:blue;");

    minibuffer->setText(QDir::homePath() + "/");
    minibuffer->setReadOnly(false);
    minibuffer->setFocus();
}

void MainWindow::find_file_read_only(const QString &path)
{
    QFile file(path);

    if (QFileInfo(path).isDir()) {
        create_dired(path);
        return;
    } else {

        if (file.exists()) {
            // If there is no window, enter here
            if (mdiArea->subWindowList().isEmpty()) {

                if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    mode_minibuffer->setText(tr("Cannot read file %1: \n%2.").arg(path).arg(file.errorString()));
                    mode_minibuffer->setStyleSheet("color: red;");
                    return;
                }

                QSplitter *splitter = create_splitter();
                TextEditor *editor = create_text_editor();
                QSplitter *new_splitter = new QSplitter;
                new_splitter->setChildrenCollapsible(false);

                QWidget *container = new QWidget;
                QVBoxLayout *container_layout = new QVBoxLayout;
                container_layout->addWidget(new_splitter);
                container_layout->setContentsMargins(0, 0, 0, 0);
                container_layout->setSpacing(0);
                container->setLayout(container_layout);

                new_splitter->addWidget(editor);
                splitter->addWidget(container);

                QTextStream in(&file);
                QApplication::setOverrideCursor(Qt::WaitCursor);
                editor->textEdit->setPlainText(in.readAll());
                QApplication::restoreOverrideCursor();
                file.close();

                editor->setTitleFromMainWindow(path);
                editor->createModeLine();
                editor->change_the_highlighter();
                editor->textEdit->setFocus();

                // Here we set the file for read-only
                editor->textEdit->document()->setModified(false);
                editor->textEdit->setReadOnly(true);
                editor->textEdit->setTextInteractionFlags(editor->textEdit->textInteractionFlags()
                                                          | Qt::TextSelectableByKeyboard);
                QString current_mode = editor->chFr->text();
                editor->chFr->setText(current_mode.replace(0, 2, "%%"));

                is_writable(path);
                update_menus();

                return;

            // Here starts the case where there is atleast one window
            } else {
                QSplitter *splitter = current_splitter();
                if (splitter) {
                    QList<QSplitter *> all_splitters = splitter->findChildren<QSplitter *>();
                    for (int x = 0; x != all_splitters.count(); ++x) {
                        if ( check_this_splitter(all_splitters.at(x)) ) {
                            TextEditor* buffer = return_current_buffer(all_splitters.at(x));
                            if (buffer) {

                                if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                                    mode_minibuffer->setText(tr("Cannot read file %1: \n%2.").arg(path).arg(file.errorString()));
                                    mode_minibuffer->setStyleSheet("color: red;");
                                    return;
                                }

                                QSplitter *parent_splitter = qobject_cast<QSplitter *>(((all_splitters.at(x)->parentWidget())->parentWidget()));
                                int buffer_index = parent_splitter->indexOf((buffer->parentWidget())->parentWidget());

                                // Here we take the parent splitter' sizes of widgets, so we can
                                // set the same sizes when we alternate the buffer
                                QList<int> splitter_sizes = parent_splitter->sizes();

                                (buffer->parentWidget())->parentWidget()->hide();

                                QSplitter *new_splitter = new QSplitter;
                                new_splitter->setChildrenCollapsible(false);
                                QWidget *new_container = new QWidget;
                                QVBoxLayout *new_container_layout = new QVBoxLayout;

                                new_container_layout->addWidget(new_splitter);
                                new_container_layout->setContentsMargins(0, 0, 0, 0);
                                new_container_layout->setSpacing(0);
                                new_container->setLayout(new_container_layout);

                                TextEditor *editor = create_text_editor();
                                new_splitter->addWidget(editor);

                                parent_splitter->insertWidget(buffer_index, new_container);
                                parent_splitter->setSizes(splitter_sizes);

                                QTextStream in(&file);
                                QApplication::setOverrideCursor(Qt::WaitCursor);
                                editor->textEdit->setPlainText(in.readAll());
                                QApplication::restoreOverrideCursor();
                                file.close();

                                editor->setTitleFromMainWindow(path);
                                editor->createModeLine();
                                editor->change_the_highlighter();
                                editor->textEdit->setFocus();

                                // Here we set the file for read-only
                                editor->textEdit->document()->setModified(false);
                                editor->textEdit->setReadOnly(true);
                                editor->textEdit->setTextInteractionFlags(editor->textEdit->textInteractionFlags()
                                                                          | Qt::TextSelectableByKeyboard);
                                QString current_mode = editor->chFr->text();
                                editor->chFr->setText(current_mode.replace(0, 2, "%%"));

                                is_writable(path);
                                update_menus();

                                return;
                            }
                        }
                    }
                }
            }
        // Here starts the case where the buffer doesn't exist
        } else {
            minibuffer->clear();
            mode_minibuffer->clear();

            mode_minibuffer->setText(path + "does not exist");
            mode_minibuffer->setStyleSheet("color:black;");
        }
    }
}

//==========================================================================================
// C-x C-v ................ Visit a different file instead of the one visited last.
//==========================================================================================

void MainWindow::set_find_alternate_file_mode()
{
    is_minibuffer_busy = true;
    is_focus_on_minibuffer = false;
    emit text_for_editor("is_busy", "set_minibuffer");

    minibuffer->clear();
    mode_minibuffer->clear();

    mode_minibuffer->setText("Find alternate file:");
    mode_minibuffer->setStyleSheet("color: blue;");

    minibuffer->setText(QDir::homePath() + "/");
    minibuffer->setReadOnly(false);
    minibuffer->setFocus();
}

void MainWindow::find_alternate_file(const QString &path)
{
    if (!mdiArea->subWindowList().isEmpty()) {

        QFile file(path);

        if (QFileInfo(path).isDir()) {
            create_dired(path);
            return;

        // Here starts the case if the path is not a dir
        } else {

            if (file.exists()) {
                QSplitter *splitter = current_splitter();
                if (splitter) {
                    QList<QSplitter *> all_splitters = splitter->findChildren<QSplitter *>();
                    for (int x = 0; x != all_splitters.count(); ++x) {
                        if ( check_this_splitter(all_splitters.at(x)) ) {
                            TextEditor* buffer = return_current_buffer(all_splitters.at(x));
                            if (buffer) {

                                if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                                    mode_minibuffer->setText(tr("Cannot read file %1: \n%2.").arg(path).arg(file.errorString()));
                                    mode_minibuffer->setStyleSheet("color: red;");
                                    return;
                                }

                                QSplitter *parent_splitter = qobject_cast<QSplitter *>(((all_splitters.at(x)->parentWidget())->parentWidget()));
                                int buffer_index = parent_splitter->indexOf((buffer->parentWidget())->parentWidget());

                                // Here we take the parent splitter' sizes of widgets, so we can
                                // set the same sizes when we alternate the buffer
                                QList<int> splitter_sizes = parent_splitter->sizes();

                                QWidget *widget_for_deleting = (buffer->parentWidget())->parentWidget();

                                if (buffer->textEdit->document()->isModified()) {
                                    buffer_for_deletion = buffer;
                                    set_ask_to_save_buffer_mode();
                                }

                                delete widget_for_deleting;

                                QSplitter *new_splitter = new QSplitter;
                                new_splitter->setChildrenCollapsible(false);
                                QWidget *new_container = new QWidget;
                                QVBoxLayout *new_container_layout = new QVBoxLayout;

                                new_container_layout->addWidget(new_splitter);
                                new_container_layout->setContentsMargins(0, 0, 0, 0);
                                new_container_layout->setSpacing(0);
                                new_container->setLayout(new_container_layout);

                                TextEditor *editor = create_text_editor();
                                new_splitter->addWidget(editor);

                                parent_splitter->insertWidget(buffer_index, new_container);
                                parent_splitter->setSizes(splitter_sizes);

                                QTextStream in(&file);
                                QApplication::setOverrideCursor(Qt::WaitCursor);
                                editor->textEdit->setPlainText(in.readAll());
                                QApplication::restoreOverrideCursor();
                                file.close();

                                editor->setTitleFromMainWindow(path);
                                editor->createModeLine();
                                editor->change_the_highlighter();
                                editor->textEdit->setFocus();

                                is_writable(path);
                                update_menus();

                                return;
                            }
                        }
                    }
                }

            // Here starts the case if there is a window but the buffer doesn't exist
            } else {
                QSplitter *splitter = current_splitter();
                if (splitter) {
                    QList<QSplitter *> all_splitters = splitter->findChildren<QSplitter *>();
                    for (int x = 0; x != all_splitters.count(); ++x) {
                        if ( check_this_splitter(all_splitters.at(x)) ) {
                            TextEditor* buffer = return_current_buffer(all_splitters.at(x));
                            if (buffer) {

                                QSplitter *parent_splitter = qobject_cast<QSplitter *>(((all_splitters.at(x)->parentWidget())->parentWidget()));
                                int buffer_index = parent_splitter->indexOf((buffer->parentWidget())->parentWidget());

                                // Here we take the parent splitter' sizes of widgets, so we can
                                // set the same sizes when we alternate the buffer
                                QList<int> splitter_sizes = parent_splitter->sizes();

                                QWidget *widget_for_deleting = (buffer->parentWidget())->parentWidget();
                                delete widget_for_deleting;

                                QSplitter *new_splitter = new QSplitter;
                                new_splitter->setChildrenCollapsible(false);
                                QWidget *new_container = new QWidget;
                                QVBoxLayout *new_container_layout = new QVBoxLayout;

                                new_container_layout->addWidget(new_splitter);
                                new_container_layout->setContentsMargins(0, 0, 0, 0);
                                new_container_layout->setSpacing(0);
                                new_container->setLayout(new_container_layout);

                                TextEditor *editor = create_text_editor();
                                new_splitter->addWidget(editor);

                                parent_splitter->insertWidget(buffer_index, new_container);
                                parent_splitter->setSizes(splitter_sizes);

                                editor->setTitleFromMainWindow(path);
                                editor->createModeLine();
                                editor->change_the_highlighter();
                                editor->textEdit->setFocus();

                                mode_minibuffer->setText(tr("(New file)"));
                                mode_minibuffer->setStyleSheet("color:black;");

                                update_menus();

                                return;
                            }
                        }
                    }
                }
            }
        }

    // Here starts the case if there aren't any windows and so, there isn't a buffer for alternation
    } else {
        minibuffer->clear();
        mode_minibuffer->clear();

        mode_minibuffer->setText(tr("There is no buffer for alternating."));
        mode_minibuffer->setStyleSheet("color:black;");
    }
}

//==========================================================================================
// Save the buffer before alterating
//==========================================================================================

void MainWindow::set_ask_to_save_buffer_mode()
{
    is_minibuffer_busy = true;
    is_focus_on_minibuffer = false;
    emit text_for_editor("is_busy", "set_minibuffer");

    minibuffer->clear();
    mode_minibuffer->clear();

    mode_minibuffer->setText("Buffer " + buffer_for_deletion->currentFile() + " is modified: save it first? (yes or no)");
    mode_minibuffer->setStyleSheet("color: blue;");

    minibuffer->setReadOnly(false);
    minibuffer->setFocus();
}

void MainWindow::save_buffer_to_disk(const QString &choice)
{
    TextEditor *buffer = buffer_for_deletion;

    if (choice == "yes") {
        QString errorMsg;
        QFileInfo fi(buffer->currentFilePath());

        if(fi.isDir())
            errorMsg = buffer->saveFileNonGui(buffer->currentFilePath() + "/" + buffer->currentFile());
        else
            errorMsg = buffer->saveFileNonGui(buffer->currentFilePath());

        if(!errorMsg.isEmpty())
        {
            mode_minibuffer->setText(errorMsg);
            mode_minibuffer->setStyleSheet("color: red;");
            return;
        }

        set_wrote_buffer_to_disk_mode(buffer);

    }

    if (is_last_buffer) {

        QMdiSubWindow *current_subWindow = mdiArea->activeSubWindow();

        delete buffer;

        mdiArea->activateNextSubWindow();
        mdiArea->removeSubWindow(current_subWindow);

        buffer_to_receive_focus = 0;
        buffer_for_deletion = 0;

        return;

    } else {

        // This is the splitter of the current buffer
        QSplitter *current_splitter =
                qobject_cast<QSplitter *>(buffer->parentWidget());

        delete buffer;
        delete_buffer_completely(current_splitter);

        if (buffer_to_receive_focus) {
            buffer = buffer_to_receive_focus;
            buffer->textEdit->setFocus();
            buffer_to_receive_focus = 0;
            buffer_for_deletion = 0;
        } else
            return;
    }
}

void MainWindow::set_wrote_buffer_to_disk_mode(TextEditor *buffer)
{
    minibuffer->clear();
    mode_minibuffer->clear();

    QString saveModeText = "Wrote " + buffer->currentFilePath();

    mode_minibuffer->setStyleSheet("color: black;");
    mode_minibuffer->setText(saveModeText);
}

//==========================================================================================
// C-x 4 f ................ Visit a file, in another window . Don't alter what is displayed
//                                                                   in the selected window.
//==========================================================================================

void MainWindow::set_find_file_other_window_mode()
{
    is_minibuffer_busy = true;
    is_focus_on_minibuffer = false;
    emit text_for_editor("is_busy", "set_minibuffer");

    minibuffer->clear();
    mode_minibuffer->clear();

    mode_minibuffer->setText("Find file in other window:");
    mode_minibuffer->setStyleSheet("color: blue;");

    minibuffer->setText(QDir::homePath() + "/");
    minibuffer->setReadOnly(false);
    minibuffer->setFocus();
}

void MainWindow::find_file_other_window(const QString &path)
{
    QFile file(path);

    if (QFileInfo(path).isDir()) {
        create_dired(path);
        return;
    } else {
        if (file.exists()) {

            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                mode_minibuffer->setText(tr("Cannot read file %1: \n%2.").arg(path).arg(file.errorString()));
                mode_minibuffer->setStyleSheet("color: red;");
                return;
            }

            QSplitter *splitter = create_splitter();
            QSplitter *new_splitter = new QSplitter;
            new_splitter->setChildrenCollapsible(false);
            QWidget *new_container = new QWidget;
            QVBoxLayout *new_container_layout = new QVBoxLayout;

            new_container_layout->addWidget(new_splitter);
            new_container_layout->setContentsMargins(0, 0, 0, 0);
            new_container_layout->setSpacing(0);
            new_container->setLayout(new_container_layout);

            TextEditor *editor = create_text_editor();
            new_splitter->addWidget(editor);

            splitter->addWidget(new_container);

            QTextStream in(&file);
            QApplication::setOverrideCursor(Qt::WaitCursor);
            editor->textEdit->setPlainText(in.readAll());
            QApplication::restoreOverrideCursor();
            file.close();

            editor->setTitleFromMainWindow(path);
            editor->createModeLine();
            editor->change_the_highlighter();
            editor->textEdit->setFocus();

            is_writable(path);
            update_menus();

            return;

        } else {
            QSplitter *splitter = create_splitter();
            TextEditor *editor = create_text_editor();
            QSplitter *new_splitter = new QSplitter;
            new_splitter->setChildrenCollapsible(false);

            QWidget *container = new QWidget;
            QVBoxLayout *container_layout = new QVBoxLayout;
            container_layout->addWidget(new_splitter);
            container_layout->setContentsMargins(0, 0, 0, 0);
            container_layout->setSpacing(0);
            container->setLayout(container_layout);

            new_splitter->addWidget(editor);
            splitter->addWidget(container);

            editor->setTitleFromMainWindow(path);
            editor->createModeLine();
            editor->change_the_highlighter();
            editor->textEdit->setFocus();

            mode_minibuffer->setText(tr("(New file)"));
            mode_minibuffer->setStyleSheet("color:black;");

            update_menus();

            return;
        }
    }
}

//==========================================================================================
//
//
//                          Saving Files
//
//
//==========================================================================================

//==========================================================================================
// C-x C-s ................ Save the current buffer in its visited file on disk
//==========================================================================================

void MainWindow::set_save_buffer_mode()
{
    if(mdiArea->activeSubWindow())
        mdiArea->activeSubWindow()->setFocus();

    TextEditor *editor = current_buffer();
    if(editor) {
        QString saveModeText = "Wrote " + editor->currentFilePath();
        mode_minibuffer->setStyleSheet("color: black;");
        mode_minibuffer->setText(saveModeText);
    }
}

void MainWindow::save_buffer()
{
    if(mdiArea->activeSubWindow())
        mdiArea->activeSubWindow()->setFocus();

    TextEditor *editor = current_buffer();

    if(editor && (editor->objectName() != "*Shell*")) {

        if(editor->textEdit->document()->isModified()) {
            QString errorMsg;
            QFileInfo fi(editor->currentFilePath());

            if(fi.isDir())
                errorMsg = editor->saveFileNonGui(editor->currentFilePath() + "/" + editor->currentFile());
            else
                errorMsg = editor->saveFileNonGui(editor->currentFilePath());

            if(!errorMsg.isEmpty())
            {
                mode_minibuffer->setText(errorMsg);
                mode_minibuffer->setStyleSheet("color: red;");
                return;
            }

            set_save_buffer_mode();

        } else {
            mode_minibuffer->setText("(No changes need to be saved)");
            mode_minibuffer->setStyleSheet("color:black;");
        }
    }
}

//==========================================================================================
// C-x s ................ Save any or all buffers to their files.
//==========================================================================================

void MainWindow::save_some_buffers()
{
    // Delete all of the old data from this list, so it will be ready for next call
    if (!list_of_buffers_for_saving.isEmpty()) {
        QList<TextEditor *>::iterator it = list_of_buffers_for_saving.begin();
        while (it != list_of_buffers_for_saving.end())
            it = list_of_buffers_for_saving.erase(it);
    }

    QList<QMdiSubWindow *> windowList = mdiArea->subWindowList();

    for(int x = 0; x != windowList.count(); ++x) {
        QSplitter *splitter = qobject_cast<QSplitter *>(windowList.at(x)->widget());
        QList<TextEditor *> allEditors = splitter->findChildren<TextEditor *>();

        for(int y = 0; y != allEditors.count(); ++y) {
            if (confirm_exit_redactor(allEditors.at(y)))
                list_of_buffers_for_saving.append(allEditors.at(y));
        }
    }

    if (!list_of_buffers_for_saving.isEmpty()) {
        buffer_for_deletion = list_of_buffers_for_saving.at(0);
        set_save_some_buffers_mode();
    }
}

void MainWindow::save_buffers_from_list(const QString &exit_result)
{
    if (exit_result == "y") {
        save_exit_from_minibuffer(buffer_for_deletion);
    } else if (exit_result == "n") { /* do nothing */
    } else if (exit_result == "!") {

        for (int i = 0; i != list_of_buffers_for_saving.count(); ++i)
            save_exit_from_minibuffer(list_of_buffers_for_saving.at(i));

        return;

    } else if (exit_result == ".") {
        TextEditor *editor = current_buffer();
        save_exit_from_minibuffer(editor);
        buffer_for_deletion = 0;
        return;
    } else if (exit_result == "help") {
        update_minibuffer();
        return;
    }

    list_of_buffers_for_saving.removeFirst();

    if (!list_of_buffers_for_saving.isEmpty()) {
        buffer_for_deletion = list_of_buffers_for_saving.at(0);
        set_save_some_buffers_mode();
    } else {
        buffer_for_deletion = 0;
        return;
    }
}

void MainWindow::set_save_some_buffers_mode()
{
    is_minibuffer_busy = true;
    is_focus_on_minibuffer = false;
    emit text_for_editor("is_busy", "set_minibuffer");

    minibuffer->clear();
    mode_minibuffer->clear();

    mode_minibuffer->setText("Save file "
          + buffer_for_deletion->currentFile() + "? (y, n, !, ., help)");
    mode_minibuffer->setStyleSheet("color: blue;");

    minibuffer->setReadOnly(false);
    minibuffer->setFocus();
}

//==========================================================================================
// C-x C-w ................ Save the current buffer with a specified file name.
//==========================================================================================

void MainWindow::set_write_file_mode()
{
    is_minibuffer_busy = true;
    is_focus_on_minibuffer = false;
    emit text_for_editor("is_busy", "set_minibuffer");

    minibuffer->clear();
    mode_minibuffer->clear();

    mode_minibuffer->setText("Write file:");
    mode_minibuffer->setStyleSheet("color: blue;");

    minibuffer->setText(QDir::homePath() + "/");
    minibuffer->setReadOnly(false);
    minibuffer->setFocus();
}

void MainWindow::write_file(const QString &path)
{
    if(mdiArea->activeSubWindow())
        mdiArea->activeSubWindow()->setFocus();

    if(current_buffer()) {
        QFileInfo fi(path);
        QString errorMsg;
        QString saveText;

        if(fi.isDir()) {
            errorMsg = current_buffer()->saveFileNonGui(path + "/" + current_buffer()->currentFile());
            saveText = path + "/" + current_buffer()->currentFile();
        } else {
            errorMsg = current_buffer()->saveFileNonGui(path);
            saveText = path;
        }

        if(!errorMsg.isEmpty())
        {
            mode_minibuffer->setText(errorMsg);
            mode_minibuffer->setStyleSheet("color: red;");
            return;
        }

        QString saveModeText = "Wrote " + saveText;
        mode_minibuffer->setStyleSheet("color: black;");
        mode_minibuffer->setText(saveModeText);
    }
}

//==========================================================================================
// M-~ ................ Forget that the current buffer has been changed (not-modified).
//==========================================================================================

void MainWindow::not_modified()
{
    update_minibuffer();

    TextEditor *editor = current_buffer();
    if(editor) {
        editor->textEdit->document()->setModified(false);

        mode_minibuffer->setText(tr("Modification-flag cleared"));
        mode_minibuffer->setStyleSheet("color:black;");
    }
}

//==========================================================================================
// C-u M-~ ............... Mark the current buffer as changed.
//==========================================================================================

void MainWindow::set_modified()
{
    update_minibuffer();

    TextEditor *editor = current_buffer();
    if(editor) {
        editor->textEdit->document()->setModified(true);

        mode_minibuffer->setText(tr("Modification-flag set"));
        mode_minibuffer->setStyleSheet("color:black;");
    }
}

//==========================================================================================
// 'M-x set-visited-file-name' ................ Change the file name under which the current
//                                                                     buffer will be saved.
//==========================================================================================

void MainWindow::set_visited_file_name_mode()
{
    is_minibuffer_busy = true;
    is_focus_on_minibuffer = false;
    emit text_for_editor("is_busy", "set_minibuffer");

    minibuffer->clear();
    mode_minibuffer->clear();

    mode_minibuffer->setText(tr("Set visited file name:"));
    mode_minibuffer->setStyleSheet("color:blue;");

    minibuffer->setReadOnly(false);
    minibuffer->setFocus();
}

void MainWindow::set_visited_file_name(const QString &name)
{
    if(name.isEmpty())
        return;
    else {
        TextEditor *editor = current_buffer();
        if(editor)
            editor->set_current_file_name(name);
    }
}

//==========================================================================================
// Reading and writing Redactor settings
//==========================================================================================

void MainWindow::read_settings()
{
    QSettings settings("MySettings", "app");

    settings.beginGroup("MainWindow");

    resize(settings.value("size", QSize(670, 400)).toSize());
    move(settings.value("pos", QPoint(200, 200)).toPoint());

    QByteArray windowState = settings.value("state").toByteArray();
    restoreState(windowState);

    settings.endGroup();
}

void MainWindow::write_settings()
{
    QSettings settings("MySettings", "app");

    settings.beginGroup("MainWindow");

    settings.setValue("size", size());
    settings.setValue("pos", pos());
    settings.setValue("state", saveState());

    settings.endGroup();
}

//==========================================================================================
// *Home Page*
//==========================================================================================

void MainWindow::create_start_page()
{

    QSplitter *splitter = create_splitter();
    splitter->setWindowState(Qt::WindowMaximized);

    QSplitter *new_splitter = new QSplitter;
    new_splitter->setChildrenCollapsible(false);
    new_splitter->setObjectName("Main Second Splitter");

    QWidget *container = new QWidget;
    QVBoxLayout *container_layout = new QVBoxLayout;
    container_layout->addWidget(new_splitter);
    container_layout->setContentsMargins(0, 0, 0, 0);
    container_layout->setSpacing(0);
    container->setLayout(container_layout);

    TextEditor *buffer = create_text_editor();
    buffer->setObjectName("*Home Page*");

    new_splitter->addWidget(buffer);

    buffer->createModeLine();
    buffer->change_the_highlighter();
    buffer->set_current_file_name("*Home Page*");

    splitter->addWidget(container);

    buffer->textEdit->insertPlainText("\n\n\n");
    buffer->textEdit->insertPlainText(QString(40 - (QString("Redactor Editor").size() / 2), ' ') + "Redactor Editor\n");
    buffer->textEdit->insertPlainText("\n");
    buffer->textEdit->insertPlainText(QString(40 - (QString("Linux Version 1.0").size() / 2), ' ') + "Linux Version 1.0\n");
    buffer->textEdit->insertPlainText(QString(40 - (QString("by Milen Vaklinov").size() / 2), ' ') + "by Milen Vaklinov\n");
    buffer->textEdit->insertPlainText(QString(40 - (QString("Redactor is open source and freely distributable").size() / 2), ' ') + "Redactor is open source and freely distributable\n");
    buffer->textEdit->insertPlainText("\n\n");
    buffer->textEdit->insertPlainText(QString(40 - (QString("Few Shortcuts:").size() / 2), ' ') + "Few Shortcuts:\n");
    buffer->textEdit->insertPlainText(QString(40 - (QString("C-x C-f ............ Open file").size() / 2), ' ') + "C-x h   ............ Help\n");
    buffer->textEdit->insertPlainText(QString(40 - (QString("C-x C-f ............ Open file").size() / 2), ' ') + "C-x C-c ............ Exit\n");
    buffer->textEdit->insertPlainText(QString(40 - (QString("C-x C-f ............ Open file").size() / 2), ' ') + "C-x C-f ............ Open file\n");
    buffer->textEdit->insertPlainText(QString(40 - (QString("C-x C-s ............ Save file").size() / 2), ' ') + "C-x C-s ............ Save file\n");

    //=========================================================

    buffer->textEdit->document()->setModified(false);
    buffer->textEdit->setReadOnly(true);
    buffer->textEdit->setTextInteractionFlags(Qt::TextSelectableByKeyboard);

    QString current_mode = buffer->chFr->text();
    buffer->chFr->setText(current_mode.replace(0, 2, "%%"));

    buffer->textEdit->setFocus();

    return;
}

//==========================================================================================
//  Create and show Redactor's Source Counter
//==========================================================================================

void MainWindow::set_counter_mode()
{
    is_minibuffer_busy = true;
    is_focus_on_minibuffer = false;
    emit text_for_editor("is_busy", "set_minibuffer");

    minibuffer->clear();
    mode_minibuffer->clear();

    mode_minibuffer->setText("Counter (directory):");
    mode_minibuffer->setStyleSheet("color: blue;");

    minibuffer->setText(QDir::homePath() + "/");
    minibuffer->setReadOnly(false);
    minibuffer->setFocus();
}

void MainWindow::create_counter(const QString &folderName)
{
    QFileInfo path(folderName);

    if (path.exists()) {

        TextEditor *buffer = create_text_editor();
        CountingSourceLinesWidget *sourceLinesCounter = new CountingSourceLinesWidget(folderName, buffer);

        if (sourceLinesCounter) {
            QSplitter *splitter = create_splitter();
            splitter->addWidget(sourceLinesCounter);
            buffer->setFocus();
            return;

        } else {
            minibuffer->clear();
            mode_minibuffer->setText(tr("Warning: I am sorry, but the counter cannot be open."));
            mode_minibuffer->setStyleSheet("color:#990000;");
            return;
        }
    } else {
        mode_minibuffer->setText(tr("This folder doesn't exist."));
        mode_minibuffer->setStyleSheet("color:black;");
        QTimer::singleShot(1500, this, SLOT(set_counter_mode()));
        return;
    }
}

//==========================================================================================
// Create and show Dired
//==========================================================================================

void MainWindow::set_dired_mode()
{
    mode_minibuffer->setText("Dired (directory):");
    mode_minibuffer->setStyleSheet("color: blue;");

    minibuffer->setText(QDir::homePath() + "/");
    minibuffer->setReadOnly(false);
    minibuffer->setFocus();
}

void MainWindow::create_dired(QString folderName)
{
    if (folderName.startsWith("~"))
        folderName.replace("~", QDir::homePath());

    QFileInfo path(folderName);

    if (path.exists()) {
        if (!mdiArea->subWindowList().empty()) {
            QList<TextEditor *> buffers = all_buffers();
            for (int x = 0; x != buffers.count(); ++x) {
                if (buffers.at(x)->objectName() == "*Dired*") {

                    TextEditor *dired_buffer = buffers.at(x);
                    QSplitter *dired_splitter = qobject_cast<QSplitter *>(dired_buffer->parentWidget());
                    QWidget *dired_widget = qobject_cast<QWidget *>(dired_splitter->parentWidget());

                    QSplitter *splitter = current_splitter();
                    int index_of_dired = splitter->indexOf(dired_widget);

                    if (index_of_dired >= 0) {

                        TextEditor *editor = create_text_editor();
                        editor->setObjectName("*Dired*");

                        Dired *dired = new Dired(folderName, editor);

                        connect(dired, SIGNAL(folder_ready(QString)), this, SLOT(show_next_folder(QString)));
                        connect(dired, SIGNAL(file_ready(QString)), this, SLOT(show_next_file(QString)));

                        splitter->setOrientation(Qt::Vertical);
                        splitter->insertWidget(index_of_dired, dired);

                        delete dired_buffer;
                        delete_buffer_completely(dired_splitter);

                        editor->setFocus();
                        return;

                    } else {

                        TextEditor *editor = create_text_editor();
                        editor->setObjectName("*Dired*");

                        Dired *dired = new Dired(folderName, editor);

                        connect(dired, SIGNAL(folder_ready(QString)), this, SLOT(show_next_folder(QString)));
                        connect(dired, SIGNAL(file_ready(QString)), this, SLOT(show_next_file(QString)));

                        if (dired) {
                            QSplitter *splitter = create_splitter();
                            splitter->setOrientation(Qt::Vertical);
                            splitter->addWidget(dired);

                            delete dired_buffer;
                            delete_buffer_completely(dired_splitter);

                            editor->setFocus();
                            return;

                        } else {

                            delete dired;
                            delete editor;

                            minibuffer->clear();
                            mode_minibuffer->setText(tr("Warning: The dired cannot be open."));
                            mode_minibuffer->setStyleSheet("color:#990000;");
                            return;
                        }
                    }
                }
            }

            // Here starts the case if we do not have open *Dired*
            TextEditor *editor = create_text_editor();
            editor->setObjectName("*Dired*");

            Dired *dired = new Dired(folderName, editor);

            connect(dired, SIGNAL(folder_ready(QString)), this, SLOT(show_next_folder(QString)));
            connect(dired, SIGNAL(file_ready(QString)), this, SLOT(show_next_file(QString)));

            if (dired) {
                QSplitter *splitter = create_splitter();
                splitter->setOrientation(Qt::Vertical);
                splitter->addWidget(dired);

                editor->setFocus();
                return;

            } else {

                delete dired;
                delete editor;

                minibuffer->clear();
                mode_minibuffer->setText(tr("Warning: The dired cannot be open."));
                mode_minibuffer->setStyleSheet("color:#990000;");
                return;
            }
        } else {
            // Here starts the case if we do not have any open windows
            TextEditor *editor = create_text_editor();
            editor->setObjectName("*Dired*");

            Dired *dired = new Dired(folderName, editor);

            connect(dired, SIGNAL(folder_ready(QString)), this, SLOT(show_next_folder(QString)));
            connect(dired, SIGNAL(file_ready(QString)), this, SLOT(show_next_file(QString)));

            if (dired) {
                QSplitter *splitter = create_splitter();
                splitter->addWidget(dired);

                editor->setFocus();
                return;

            } else {

                delete dired;
                delete editor;

                minibuffer->clear();
                mode_minibuffer->setText(tr("Warning: The dired cannot be open."));
                mode_minibuffer->setStyleSheet("color:#990000;");
                return;
            }
        }
    } else {
        minibuffer->clear();
        mode_minibuffer->setText(tr("This folder doesn't exist."));
        mode_minibuffer->setStyleSheet("color:black;");
        QTimer::singleShot(1500, this, SLOT(set_dired_mode()));
        return;
    }
}

void MainWindow::show_next_folder(QString given_folder)
{
    create_dired(given_folder);
}

void MainWindow::show_next_file(const QString &path)
{
    QFile file(path);

    if (path.endsWith(".pdf") || path.endsWith(".chm") || path.endsWith(".jpg") || path.endsWith(".png")) {
        minibuffer->clear();
        mode_minibuffer->setText(tr("Cannot read file %1").arg(path));
        mode_minibuffer->setStyleSheet("color: red;");
        return;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        mode_minibuffer->setText(tr("Cannot read file %1").arg(path));
        mode_minibuffer->setStyleSheet("color: red;");
        return;
    }

    QSplitter *splitter = create_splitter();

    QSplitter *new_splitter = new QSplitter;
    new_splitter->setChildrenCollapsible(false);

    QWidget *container = new QWidget;
    QVBoxLayout *container_layout = new QVBoxLayout;
    container_layout->addWidget(new_splitter);
    container_layout->setContentsMargins(0, 0, 0, 0);
    container_layout->setSpacing(0);
    container->setLayout(container_layout);

    TextEditor *buffer = create_text_editor();
    new_splitter->addWidget(buffer);

    buffer->setTitleFromMainWindow(path);
    buffer->createModeLine();
    buffer->change_the_highlighter();

    splitter->addWidget(container);

    //====================================================

    QTextStream in(&file);

#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif

    buffer->textEdit->insertPlainText(in.readAll());

    file.close();

#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif

    //===================================================

    QString current_mode = buffer->chFr->text();
    buffer->chFr->setText(current_mode.replace(0, 2, "--"));
    buffer->textEdit->document()->setModified(false);
    buffer->textEdit->setFocus();

    QTextCursor cursor = buffer->textEdit->textCursor();
    cursor.movePosition(QTextCursor::Start);
    buffer->textEdit->setTextCursor(cursor);

    QScrollBar *vScrollBar = buffer->textEdit->verticalScrollBar();
    vScrollBar->triggerAction(QScrollBar::SliderToMinimum);

    is_writable(path);
    update_menus();

    return;
}

//==========================================================================================
//
//
//                               |Mark| 11. The Mark and the Region
//
//
//==========================================================================================

// C-<SPC> ................ Set the mark.
void MainWindow::set_mark_text(sQ::Mark mark)
{
    update_minibuffer();

    switch (mark) {

    case sQ::Deactivated:
        mode_minibuffer->setText(tr("Mark deactivated"));
        mode_minibuffer->setStyleSheet("color:black;");
        break;

    case sQ::Activated:
        mode_minibuffer->setText(tr("Mark activated"));
        mode_minibuffer->setStyleSheet("color:black;");
        break;

    case sQ::Set:
        mode_minibuffer->setText(tr("Mark set"));
        mode_minibuffer->setStyleSheet("color:black;");
        break;

    case sQ::Position:
        mode_minibuffer->setText(tr("Position marked"));
        mode_minibuffer->setStyleSheet("color:black;");
        break;
    }
}

void MainWindow::set_undo_text()
{
    update_minibuffer();

    mode_minibuffer->setText(tr("Undo!"));
    mode_minibuffer->setStyleSheet("color:black;");
}

//==========================================================================================
//
//
//                              |Killing| 12. Killing and Moving Text
//
//
//==========================================================================================


//==========================================================================================
// *** 12.1 Deletion and Killing ***
//==========================================================================================

// Implemented in the file: Editor/mytextedit.cpp

//==========================================================================================
// *** 12.2 Yanking ***
//==========================================================================================

// Implemented in the file: Editor/mytextedit.cpp


//==========================================================================================
// Next functions show text which is connected with the functions from the mytextedit.cpp
// file.
//==========================================================================================

//==========================================================================================
// Set this text when trying to delete or change the end of buffer
//==========================================================================================

void MainWindow::set_end_of_buffer_text()
{
    minibuffer->clear();
    mode_minibuffer->clear();

    mode_minibuffer->setText(tr("End of buffer"));
    mode_minibuffer->setStyleSheet("color:black;");

}

//==========================================================================================
// Set this text when trying to delete or change the start of buffer
//==========================================================================================

void MainWindow::set_start_of_buffer_text()
{
    minibuffer->clear();
    mode_minibuffer->clear();

    mode_minibuffer->setText(tr("Beginning of buffer"));
    mode_minibuffer->setStyleSheet("color:black;");
}

//============================================================================================
// Set this text to minibuffer when waiting for character until which the text will be deleted
//============================================================================================

void MainWindow::set_zap_text()
{
    is_minibuffer_busy = true;
    emit text_for_editor("is_busy", "set_minibuffer");

    minibuffer->clear();
    mode_minibuffer->clear();

    mode_minibuffer->setText(tr("Zap to char:"));
    mode_minibuffer->setStyleSheet("color:blue;");

    minibuffer->setReadOnly(true);
}

//============================================================================================
// Here we set the minibuffer ready for new input.
//============================================================================================

void MainWindow::zap_to_char_finish_slot()
{
    minibuffer->clear();
    mode_minibuffer->clear();

    is_minibuffer_busy = false;
    is_focus_on_minibuffer = false;

    emit text_for_editor("not_busy", "set_minibuffer");
}

//==========================================================================================
// *** 12.4 Accumulating Text ***
//==========================================================================================

//==========================================================================================
// M-x append-to-buffer ............... Append region to the end of specified buffer.
//==========================================================================================

void MainWindow::set_append_to_buffer_mode()
{
    is_minibuffer_busy = true;
    is_focus_on_minibuffer = false;
    emit text_for_editor("is_busy", "set_minibuffer");

    mode_minibuffer->clear();
    minibuffer->clear();

    mode_minibuffer->setText("Append to buffer:");
    mode_minibuffer->setStyleSheet("color:blue;");

    minibuffer->setReadOnly(false);
    minibuffer->setFocus();
}

void MainWindow::append_to_buffer()
{
    TextEditor *buffer = current_buffer();
    QTextCursor cursor = buffer->textEdit->textCursor();

    if (cursor.hasSelection() && buffer->textEdit->is_mark_activated) {

        QString text_for_appending = cursor.selectedText();

        cursor.clearSelection();
        buffer->textEdit->setTextCursor(cursor);
        buffer->textEdit->is_mark_activated = false;

        QString given_buffer_name = minibuffer->get_the_string();

        if (given_buffer_name == "*Dired*" || given_buffer_name == "*Counter*" ||
                given_buffer_name == "*Home Page*" || given_buffer_name == "*Messages*")
            return;

        QList<TextEditor *> buffers = all_buffers();

        for (int i = 0; i != buffers.size(); ++i) {
            if (buffers.at(i)->currentFile() == given_buffer_name) {

#ifndef QT_NO_CURSOR
                QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif
                buffers.at(i)->textEdit->insertPlainText(text_for_appending);

#ifndef QT_NO_CURSOR
                QApplication::restoreOverrideCursor();
#endif

                cursor = buffers.at(i)->textEdit->textCursor();
                cursor.movePosition(QTextCursor::End);
                buffers.at(i)->textEdit->setTextCursor(cursor);

                return;
            }
        }

        // If the buffers doesn't contain the given_buffer_name, create new
        // one and append the new text

        QSplitter *splitter = create_splitter();
        TextEditor *editor = create_text_editor();
        QSplitter *new_splitter = new QSplitter;
        new_splitter->setChildrenCollapsible(false);

        QWidget *container = new QWidget;
        QVBoxLayout *container_layout = new QVBoxLayout;
        container_layout->addWidget(new_splitter);
        container_layout->setContentsMargins(0, 0, 0, 0);
        container_layout->setSpacing(0);
        container->setLayout(container_layout);

        new_splitter->addWidget(editor);
        splitter->addWidget(container);

        editor->setTitleFromMainWindow(given_buffer_name);
        editor->createModeLine();
        editor->change_the_highlighter();

#ifndef QT_NO_CURSOR
                QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif
                editor->textEdit->insertPlainText(text_for_appending);

#ifndef QT_NO_CURSOR
                QApplication::restoreOverrideCursor();
#endif

        update_menus();

        // We return the focus to the main buffer
        buffer->textEdit->setFocus();

        return;
    } else {

        update_minibuffer();

        QString result = tr("No region for appending.");
        mode_minibuffer->setText(result);
        mode_minibuffer->setStyleSheet("color:black;");
    }
}

//==========================================================================================
// M-x prepend-to-buffer ............... Prepend region to the beginning of a specified buffer.
//==========================================================================================

void MainWindow::set_prepend_to_buffer_mode()
{
    is_minibuffer_busy = true;
    is_focus_on_minibuffer = false;
    emit text_for_editor("is_busy", "set_minibuffer");

    mode_minibuffer->clear();
    minibuffer->clear();

    mode_minibuffer->setText("Prepend to buffer:");
    mode_minibuffer->setStyleSheet("color:blue;");

    minibuffer->setReadOnly(false);
    minibuffer->setFocus();
}

void MainWindow::prepend_to_buffer()
{
    TextEditor *buffer = current_buffer();
    QTextCursor cursor = buffer->textEdit->textCursor();

    if (cursor.hasSelection() && buffer->textEdit->is_mark_activated) {

        QString text_for_prepending = cursor.selectedText();

        cursor.clearSelection();
        buffer->textEdit->setTextCursor(cursor);
        buffer->textEdit->is_mark_activated = false;

        QString given_buffer_name = minibuffer->get_the_string();

        if (given_buffer_name == "*Dired*" || given_buffer_name == "*Counter*" ||
                given_buffer_name == "*Home Page*" || given_buffer_name == "*Messages*")
            return;

        QList<TextEditor *> buffers = all_buffers();

        for (int i = 0; i != buffers.size(); ++i) {
            if (buffers.at(i)->currentFile() == given_buffer_name) {

                cursor = buffers.at(i)->textEdit->textCursor();
                cursor.movePosition(QTextCursor::Start);
                buffers.at(i)->textEdit->setTextCursor(cursor);

#ifndef QT_NO_CURSOR
                QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif
                buffers.at(i)->textEdit->insertPlainText(text_for_prepending);

#ifndef QT_NO_CURSOR
                QApplication::restoreOverrideCursor();
#endif

                return;
            }
        }

        // If the buffers doesn't contain the given_buffer_name, create new
        // one and append the new text

        QSplitter *splitter = create_splitter();
        TextEditor *editor = create_text_editor();
        QSplitter *new_splitter = new QSplitter;
        new_splitter->setChildrenCollapsible(false);

        QWidget *container = new QWidget;
        QVBoxLayout *container_layout = new QVBoxLayout;
        container_layout->addWidget(new_splitter);
        container_layout->setContentsMargins(0, 0, 0, 0);
        container_layout->setSpacing(0);
        container->setLayout(container_layout);

        new_splitter->addWidget(editor);
        splitter->addWidget(container);

        editor->setTitleFromMainWindow(given_buffer_name);
        editor->createModeLine();
        editor->change_the_highlighter();

#ifndef QT_NO_CURSOR
                QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif
                editor->textEdit->insertPlainText(text_for_prepending);

#ifndef QT_NO_CURSOR
                QApplication::restoreOverrideCursor();
#endif

        update_menus();

        // We return the focus to the main buffer
        buffer->textEdit->setFocus();

        return;
    } else {

        update_minibuffer();

        QString result = tr("No region for prepending.");
        mode_minibuffer->setText(result);
        mode_minibuffer->setStyleSheet("color:black;");
    }
}

//==========================================================================================
// M-x copy-to-buffer ............... Copy region into a specified buffer, deleting that
// buffer's old contents
//==========================================================================================

void MainWindow::set_copy_to_buffer_mode()
{
    is_minibuffer_busy = true;
    is_focus_on_minibuffer = false;
    emit text_for_editor("is_busy", "set_minibuffer");

    minibuffer->clear();
    mode_minibuffer->clear();

    mode_minibuffer->setText("Copy to buffer:");
    mode_minibuffer->setStyleSheet("color:blue;");

    minibuffer->setReadOnly(false);
    minibuffer->setFocus();
}

void MainWindow::copy_to_buffer()
{
    TextEditor *buffer = current_buffer();
    QTextCursor cursor = buffer->textEdit->textCursor();

    if (cursor.hasSelection() && buffer->textEdit->is_mark_activated) {

        QString copied_text = cursor.selectedText();

        cursor.clearSelection();
        buffer->textEdit->setTextCursor(cursor);
        buffer->textEdit->is_mark_activated = false;

        QString given_buffer_name = minibuffer->get_the_string();

        if (given_buffer_name == "*Dired*" || given_buffer_name == "*Counter*" ||
                given_buffer_name == "*Home Page*" || given_buffer_name == "*Messages*")
            return;

        QList<TextEditor *> buffers = all_buffers();

        for (int i = 0; i != buffers.size(); ++i) {
            if (buffers.at(i)->currentFile() == given_buffer_name) {

                buffers.at(i)->textEdit->clear();

#ifndef QT_NO_CURSOR
                QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif
                buffers.at(i)->textEdit->insertPlainText(copied_text);

#ifndef QT_NO_CURSOR
                QApplication::restoreOverrideCursor();
#endif

                cursor = buffers.at(i)->textEdit->textCursor();
                cursor.movePosition(QTextCursor::End);
                buffers.at(i)->textEdit->setTextCursor(cursor);

                return;
            }
        }

        // If the buffers doesn't contain the given_buffer_name, create new
        // one and append the new text

        QSplitter *splitter = create_splitter();
        TextEditor *editor = create_text_editor();
        QSplitter *new_splitter = new QSplitter;
        new_splitter->setChildrenCollapsible(false);

        QWidget *container = new QWidget;
        QVBoxLayout *container_layout = new QVBoxLayout;
        container_layout->addWidget(new_splitter);
        container_layout->setContentsMargins(0, 0, 0, 0);
        container_layout->setSpacing(0);
        container->setLayout(container_layout);

        new_splitter->addWidget(editor);
        splitter->addWidget(container);

        editor->setTitleFromMainWindow(given_buffer_name);
        editor->createModeLine();
        editor->change_the_highlighter();

#ifndef QT_NO_CURSOR
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif
            editor->textEdit->insertPlainText(copied_text);

#ifndef QT_NO_CURSOR
            QApplication::restoreOverrideCursor();
#endif

        update_menus();

        // We return the focus to the main buffer
        buffer->textEdit->setFocus();

        return;
    } else {

        update_minibuffer();

        QString result = tr("No region for pasting.");
        mode_minibuffer->setText(result);
        mode_minibuffer->setStyleSheet("color:black;");
    }
}

//==========================================================================================
// M-x insert-to-buffer ............... Insert the region into a specified buffer at current
// point.
//==========================================================================================

void MainWindow::set_insert_to_buffer_mode()
{
    is_minibuffer_busy = true;
    is_focus_on_minibuffer = false;
    emit text_for_editor("is_busy", "set_minibuffer");

    minibuffer->clear();
    mode_minibuffer->clear();

    mode_minibuffer->setText("Insert to buffer:");
    mode_minibuffer->setStyleSheet("color:blue;");

    minibuffer->setReadOnly(false);
    minibuffer->setFocus();
}

void MainWindow::insert_to_buffer()
{
    TextEditor *buffer = current_buffer();
    QTextCursor cursor = buffer->textEdit->textCursor();

    if (cursor.hasSelection() && buffer->textEdit->is_mark_activated) {

        QString text_for_inserting = cursor.selectedText();

        cursor.clearSelection();
        buffer->textEdit->setTextCursor(cursor);
        buffer->textEdit->is_mark_activated = false;

        QString given_buffer_name = minibuffer->get_the_string();

        if (given_buffer_name == "*Dired*" || given_buffer_name == "*Counter*" ||
                given_buffer_name == "*Home Page*" || given_buffer_name == "*Messages*")
            return;

        QList<TextEditor *> buffers = all_buffers();

        for (int i = 0; i != buffers.size(); ++i) {
            if (buffers.at(i)->currentFile() == given_buffer_name) {

#ifndef QT_NO_CURSOR
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif
                buffers.at(i)->textEdit->insertPlainText(text_for_inserting);
#ifndef QT_NO_CURSOR
            QApplication::restoreOverrideCursor();
#endif

                return;
            }
        }

        // If the buffers doesn't contain the given_buffer_name, create new
        // one and append the new text

        QSplitter *splitter = create_splitter();
        TextEditor *editor = create_text_editor();
        QSplitter *new_splitter = new QSplitter;
        new_splitter->setChildrenCollapsible(false);

        QWidget *container = new QWidget;
        QVBoxLayout *container_layout = new QVBoxLayout;
        container_layout->addWidget(new_splitter);
        container_layout->setContentsMargins(0, 0, 0, 0);
        container_layout->setSpacing(0);
        container->setLayout(container_layout);

        new_splitter->addWidget(editor);
        splitter->addWidget(container);

        editor->setTitleFromMainWindow(given_buffer_name);
        editor->createModeLine();
        editor->change_the_highlighter();

#ifndef QT_NO_CURSOR
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif
        editor->textEdit->insertPlainText(text_for_inserting);
#ifndef QT_NO_CURSOR
            QApplication::restoreOverrideCursor();
#endif

        update_menus();

        // We return the focus to the main buffer
        buffer->textEdit->setFocus();

        return;
    } else {

        update_minibuffer();

        QString result = tr("No region for inserting.");
        mode_minibuffer->setText(result);
        mode_minibuffer->setStyleSheet("color:black;");
    }
}

//==========================================================================================
// M-x insert-buffer ............... Insert the contents of a specified buffer into the
// current buffer at point.
//==========================================================================================

void MainWindow::set_insert_buffer_mode()
{
    is_minibuffer_busy = true;
    is_focus_on_minibuffer = false;
    emit text_for_editor("is_busy", "set_minibuffer");

    minibuffer->clear();
    mode_minibuffer->clear();

    mode_minibuffer->setText(tr("Insert buffer:"));
    mode_minibuffer->setStyleSheet("color:blue;");

    minibuffer->setReadOnly(false);
    minibuffer->setFocus();
}

void MainWindow::insert_buffer()
{
    QString given_buffer_name = minibuffer->get_the_string();

    if (given_buffer_name == "*Dired*" || given_buffer_name == "*Counter*" ||
            given_buffer_name == "*Home Page*" || given_buffer_name == "*Messages*")
        return;


    QList<TextEditor *> buffers = all_buffers();

    for (int i = 0; i != buffers.size(); ++i) {
        if (buffers.at(i)->currentFile() == given_buffer_name) {
            TextEditor *buffer = current_buffer();
            QString text_for_inserting = buffers.at(i)->textEdit->toPlainText();

#ifndef QT_NO_CURSOR
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif
            buffer->textEdit->insertPlainText(text_for_inserting);
#ifndef QT_NO_CURSOR
            QApplication::restoreOverrideCursor();
#endif
            return;
        }
    }

    mode_minibuffer->setText("Insert buffer:");
    minibuffer->setText(tr("[No match]"));
    QTimer::singleShot(1000, this, SLOT(set_insert_buffer_mode()));
}

//==========================================================================================
// M-x insert-file ............... Insert contents of another buffer into the current buffer
//==========================================================================================

void MainWindow::set_insert_file_mode()
{
    is_minibuffer_busy = true;
    is_focus_on_minibuffer = false;
    emit text_for_editor("is_busy", "set_minibuffer");

    minibuffer->clear();
    mode_minibuffer->clear();

    mode_minibuffer->setText(tr("Insert file:"));
    mode_minibuffer->setStyleSheet("color: blue;");

    minibuffer->setText(QDir::homePath() + "/");
    minibuffer->setReadOnly(false);
    minibuffer->setFocus();
}

void MainWindow::insert_file()
{
    TextEditor *editor = current_buffer();
    QString path = minibuffer->get_the_string();

    if (editor) {
        QFileInfo fi(path);
        if (fi.isDir()) {
            mode_minibuffer->setText(tr("Opening input file: file is a directory, %1").arg(path));
            mode_minibuffer->setStyleSheet("color:black;");
            return;
        } else {
            QFile file(path);

            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                mode_minibuffer->setText(tr("Cannot read file %1: %2.").arg(path).arg(file.errorString()));
                mode_minibuffer->setStyleSheet("color:black;");
                return;
            }

            QTextStream in(&file);
#ifndef QT_NO_CURSOR
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif
            editor->textEdit->insertPlainText(in.readAll());
#ifndef QT_NO_CURSOR
            QApplication::restoreOverrideCursor();
#endif
        }
    }

    return;
}

//==========================================================================================
// M-x append-to-file ............... Append region to the contents of a specified file, at
// the end.
//==========================================================================================

void MainWindow::set_append_to_file_mode()
{
    is_minibuffer_busy = true;
    is_focus_on_minibuffer = false;
    emit text_for_editor("is_busy", "set_minibuffer");

    mode_minibuffer->clear();
    minibuffer->clear();

    mode_minibuffer->setText(tr("Append to file:"));
    mode_minibuffer->setStyleSheet("color: blue;");

    minibuffer->setText(QDir::homePath() + "/");
    minibuffer->setReadOnly(false);
    minibuffer->setFocus();
}

void MainWindow::append_to_file()
{
    TextEditor *buffer = current_buffer();
    QTextCursor cursor = buffer->textEdit->textCursor();

    if (cursor.hasSelection() && buffer->textEdit->is_mark_activated) {

        QString text_for_appending = cursor.selectedText();

        cursor.clearSelection();
        buffer->textEdit->setTextCursor(cursor);
        buffer->textEdit->is_mark_activated = false;

        QString path = minibuffer->get_the_string();

        QFile file(path);
        if (file.exists()) {
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
                mode_minibuffer->setText(tr("Warning:"));
                mode_minibuffer->setStyleSheet("color: #990000");

                QString error_message = tr("Cannot write to file %1").arg(path);
//                QString error_reason = tr("Reason: %1").arg(file.errorString());

                minibuffer->setText(error_message);
                minibuffer->setStyleSheet("color:black;");

                return;
            }

            QTextStream out(&file);

#ifndef QT_NO_CURSOR
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif
            out << text_for_appending;

#ifndef QT_NO_CURSOR
            QApplication::restoreOverrideCursor();
#endif
            file.close();

            QString result = tr("Added to %1").arg(path);

            mode_minibuffer->setText(result);
            mode_minibuffer->setStyleSheet("color:black");

        } else {
            mode_minibuffer->setText(tr("Warning:"));
            mode_minibuffer->setStyleSheet("color: #990000");

            QString check_message = tr("The file %1 does not exist.").arg(path);

            minibuffer->setText(check_message);
            minibuffer->setStyleSheet("color:black;");
        }

    } else {
        update_minibuffer();

        QString result = tr("No region for appending.");
        mode_minibuffer->setText(result);
        mode_minibuffer->setStyleSheet("color:black;");
    }
}

//==========================================================================================
//                              |Search| 15. Searching and Replacement
//==========================================================================================

void MainWindow::set_search_forward_mode()
{  
    is_minibuffer_busy = true;
    is_focus_on_minibuffer = false;
    emit text_for_editor("is_busy", "set_minibuffer");

    mode_minibuffer->clear();
    minibuffer->clear();

    mode_minibuffer->setText(tr("Search forward:"));
    mode_minibuffer->setStyleSheet("color: blue;");
    minibuffer->setReadOnly(false);
    minibuffer->setFocus();
}

//==========================================================================================
// M-x search-backward .................... Incremental search backward.
//==========================================================================================

void MainWindow::set_search_backward_mode()
{
    is_minibuffer_busy = true;
    is_focus_on_minibuffer = false;
    emit text_for_editor("is_busy", "set_minibuffer");

    mode_minibuffer->clear();
    minibuffer->clear();

    mode_minibuffer->setText(tr("Search backward:"));
    mode_minibuffer->setStyleSheet("color: blue;");

    minibuffer->setReadOnly(false);
    minibuffer->setFocus();
}

//==========================================================================================
// C-M-s .................... Begin incremental regexp search.
//==========================================================================================

void MainWindow::set_search_forward_regexp_mode()
{
    is_minibuffer_busy = true;
    is_focus_on_minibuffer = false;
    emit text_for_editor("is_busy", "set_minibuffer");

    mode_minibuffer->clear();
    minibuffer->clear();

    mode_minibuffer->setText(tr("RE search:"));
    mode_minibuffer->setStyleSheet("color: blue;");

    minibuffer->setReadOnly(false);
    minibuffer->setFocus();
}

//==========================================================================================
// C-M-r .................. Begin reverse incremental regexp search
//==========================================================================================

void MainWindow::set_search_backward_regexp_mode()
{
    is_minibuffer_busy = true;
    is_focus_on_minibuffer = false;
    emit text_for_editor("is_busy", "set_minibuffer");

    mode_minibuffer->clear();
    minibuffer->clear();

    mode_minibuffer->setText(tr("RE search backward:"));
    mode_minibuffer->setStyleSheet("color: blue;");

    minibuffer->setReadOnly(false);
    minibuffer->setFocus();
}

//==========================================================================================
// M-x goto-line ................ Go to a specified line number.
//==========================================================================================

void MainWindow::set_goto_line_mode()
{
    is_minibuffer_busy = true;
    is_focus_on_minibuffer = false;
    emit text_for_editor("is_busy", "set_minibuffer");

    mode_minibuffer->clear();
    minibuffer->clear();

    mode_minibuffer->setText(tr("Goto line:"));
    mode_minibuffer->setStyleSheet("color: blue;");

    minibuffer->setReadOnly(false);
    minibuffer->setFocus();
}

void MainWindow::goto_line(const QString &number_line)
{
    TextEditor *buffer = current_buffer();

    int given_line = number_line.toInt();

    if (given_line < 1 || given_line > buffer->textEdit->blockCount()) {

        minibuffer->clear();
        mode_minibuffer->clear();

        mode_minibuffer->setText(tr("The given line number is not valid."));
        mode_minibuffer->setStyleSheet("color:black;");

    } else {

        QTextCursor cursor = buffer->textEdit->textCursor();

        if (cursor.hasSelection())
            cursor.clearSelection();

        int line_number = cursor.blockNumber() + 1;

        if (given_line > line_number)
            cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, given_line - line_number);
        else if (given_line < line_number)
            cursor.movePosition(QTextCursor::Up, QTextCursor::MoveAnchor, line_number - given_line);
        else {
            // The cursor is on the same position as
            // the given_number, so do nothing and set the cursor.
        }

        buffer->textEdit->setTextCursor(cursor);

        return;
    }
}

//==========================================================================================
//
//
//                             16. |Fixit|
//
//
//==========================================================================================

void MainWindow::set_fixit_text()
{
    minibuffer->clear();
    mode_minibuffer->clear();

    mode_minibuffer->setText(tr("Don't have two things to transpose"));
    mode_minibuffer->setStyleSheet("color:black;");
}

//==========================================================================================
//
//
//                              25. |Text|
//
//
//==========================================================================================

//==========================================================================================
// 25.13.4 Faces in Enriched Text - All functions are implemented in mytextedit.cpp
// int |Text| section.
//==========================================================================================

void MainWindow::set_face_mode(int step)
{
    if ((step != 0 && step != 1) || is_minibuffer_busy)
        return;

    is_minibuffer_busy = true;
    is_focus_on_minibuffer = false;
    emit text_for_editor("is_busy", "set_minibuffer");

    if (step == 0) {
        mode_minibuffer->setText(tr("Foreground color:"));
        mode_minibuffer->setStyleSheet("color: blue;");
        minibuffer->setReadOnly(false);
        minibuffer->setFocus();
    } else {
        mode_minibuffer->setText(tr("Background color:"));
        mode_minibuffer->setStyleSheet("color: blue;");
        minibuffer->setReadOnly(false);
        minibuffer->setFocus();
    }
}

//==========================================================================================
//
//
//                                      Shell
//
//
//==========================================================================================

void MainWindow::create_shell()
{
    TextEditor *buffer = create_text_editor();
    Shell *shell = new Shell(buffer);

    if (shell) {
        QSplitter *splitter = create_splitter();
        splitter->addWidget(shell);

        buffer->setFocus();

        return;
    }
}

//==========================================================================================
//
//
//                                      Tutorials
//
//
//==========================================================================================

void MainWindow::show_redactor_tutorial()
{
    QString path = ":tuts/Documentation/redactor_tutorial_en";
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        mode_minibuffer->setText(tr("Cannot read file %1").arg(path));
        mode_minibuffer->setStyleSheet("color: #800000;");
        return;
    }

    QSplitter *splitter = create_splitter();

    QSplitter *new_splitter = new QSplitter;
    new_splitter->setChildrenCollapsible(false);

    QWidget *container = new QWidget;
    QVBoxLayout *container_layout = new QVBoxLayout;
    container_layout->addWidget(new_splitter);
    container_layout->setContentsMargins(0, 0, 0, 0);
    container_layout->setSpacing(0);
    container->setLayout(container_layout);

    TextEditor *buffer = create_text_editor();
    buffer->setObjectName("*Redactor Tutorial");
    new_splitter->addWidget(buffer);

    buffer->createModeLine();
    buffer->set_current_file_name("*Redactor Tutorial*");
    buffer->change_the_highlighter();

    splitter->addWidget(container);

    //====================================================

    QTextStream in(&file);

#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif

    buffer->textEdit->insertPlainText(in.readAll());

    file.close();

#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif

    //===================================================

    QString current_mode = buffer->chFr->text();
    buffer->chFr->setText(current_mode.replace(0, 2, "--"));
    buffer->textEdit->document()->setModified(false);
    buffer->textEdit->setFocus();

    QTextCursor cursor = buffer->textEdit->textCursor();
    cursor.movePosition(QTextCursor::Start);
    buffer->textEdit->setTextCursor(cursor);

    QScrollBar *vScrollBar = buffer->textEdit->verticalScrollBar();
    vScrollBar->triggerAction(QScrollBar::SliderToMinimum);

    is_writable(path);
    update_menus();

    return;
}

void MainWindow::show_unix_tutorial()
{
    QString path = "/home/zak/Qt/Redactor/Documentation/unixtoolbox.html";
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        mode_minibuffer->setText(tr("Cannot read file %1").arg(path));
        mode_minibuffer->setStyleSheet("color: #800000;");
        return;
    }

    QSplitter *splitter = create_splitter();

    QSplitter *new_splitter = new QSplitter;
    new_splitter->setChildrenCollapsible(false);

    QWidget *container = new QWidget;
    QVBoxLayout *container_layout = new QVBoxLayout;
    container_layout->addWidget(new_splitter);
    container_layout->setContentsMargins(0, 0, 0, 0);
    container_layout->setSpacing(0);
    container->setLayout(container_layout);

    TextEditor *buffer = create_text_editor();
    buffer->setObjectName("*Unix Tutorial");
    new_splitter->addWidget(buffer);

    buffer->createModeLine();
    buffer->set_current_file_name("*Unix Tutorial*");
    buffer->change_the_highlighter();

    splitter->addWidget(container);

    //====================================================

    QTextStream in(&file);

#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif

    buffer->textEdit->appendHtml(in.readAll());

    file.close();

#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif

    //===================================================

    buffer->textEdit->document()->setModified(false);
    buffer->textEdit->setReadOnly(true);
    buffer->textEdit->setTextInteractionFlags(buffer->textEdit->textInteractionFlags() | Qt::TextSelectableByKeyboard);

    QString current_mode = buffer->chFr->text();
    buffer->chFr->setText(current_mode.replace(0, 2, "%%"));

    buffer->textEdit->setFocus();

    QScrollBar *vScrollBar = buffer->textEdit->verticalScrollBar();
    vScrollBar->triggerAction(QScrollBar::SliderToMinimum);

    QTextCursor cursor = buffer->textEdit->textCursor();
    cursor.movePosition(QTextCursor::Start);
    buffer->textEdit->setTextCursor(cursor);


    is_writable(path);
    update_menus();

    return;
}
