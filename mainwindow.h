#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "enums.h"

#include <QMainWindow>
#include <QDir>
#include <QUrl>

//==========================================================================================
// Futher declarations
//==========================================================================================
 class QAction;
 class CountingSourceLinesWidget;
 class Dired;
 class QLabel;
 class QMenu;
 class QMdiArea;
 class QMdiSubWindow;
 class Minibuffer;
 class TextEditor;
 class QShortcut;
 class QStatusBar;
 class QSplitter;

 class MainWindow : public QMainWindow
 {
     Q_OBJECT

 public:
     MainWindow();
     ~MainWindow();

 signals:
     void new_text_for_messages(const QString& given_text);
     void text_for_editor(const QString &text, const QString &command);
     void ready_to_exit();

 protected:
     virtual void closeEvent(QCloseEvent *event);

 private slots:

     //==========================================================================================
     // Open, Save, Save As... a file from Toolbar or File menu and some actions
     //==========================================================================================
     void new_buffer();
     void open();
     void save();
     void saveAs();
     void cut();
     void copy();
     void paste();
     void about();
     void contact();
     void update_menus();

     //==========================================================================================
     // Visiting Files
     //==========================================================================================
     void set_find_file_mode();
     void set_find_file_read_only_mode();
     void set_find_alternate_file_mode();
     void set_find_file_other_window_mode();

     //==========================================================================================
     // Saving Files
     //==========================================================================================
     void save_buffer();
     void save_some_buffers();
     void set_write_file_mode();

     //==========================================================================================
     // A change in a file
     //==========================================================================================
     void not_modified();
     void set_modified();
     void is_writable(const QString& path);

     //==========================================================================================
     // Buffers
     //==========================================================================================
     void set_switch_to_buffer_mode();
     void set_switch_to_buffer_other_window_mode();
     void toggle_read_only();
     void kill_buffer();

     // *Buffer List*
     void list_buffers();
     //void revert_buffer();
     void kill_all_marked_buffers();
     void buffer_list_is_read_only();
     void quit_buffer_list();
     void show_buffer_alone(int current_line);
     void show_buffer_from_list_buffers(int current_line);
     //void show_buffer_instead_buffer_list(int current_line);
     void show_buffer_other_window_buffer_list(int current_line);
     void show_buffers_other_windows_v(int current_line);
     void mark_buffer(int current_line);
     void mark_buffer_for_killing(int current_line, bool direction);
     void mark_buffer_for_saving(int current_line);
     void undo_marked_buffer(int current_line, bool direction);
     void toggle_read_only_buffer_list(int current_line);
     void set_modified_buffer_list(int current_line);
     void set_up_two_buffers_in_buffer_list(int current_line);
     void set_rename_mode_from_buffer_list(int line_number);


     //==========================================================================================
     // Windows
     //==========================================================================================
     void split_window_below();
     void split_window_right();
     void other_window();
     void next_buffer();
     void previous_buffer();
     void delete_window();
     void delete_other_windows();
     void kill_buffer_and_window();
     void enlarge_window();
     void enlarge_window_horizontally();
     void shrink_window_horizontally();

     //==========================================================================================
     // Minibuffer
     //==========================================================================================
     void update_minibuffer();
     void event_from_minibuffer();
     void focus_on_minibuffer();
     void escape_minibuffer();
     void set_old_text();

     //==========================================================================================
     // |Basic|
     //==========================================================================================
     void set_mark_text(sQ::Mark mark);
     void set_undo_text();

     //==========================================================================================
     // |Killing|
     //==========================================================================================
     void set_insert_buffer_mode();
     void set_insert_file_mode();

     //==========================================================================================
     // |Search|
     //==========================================================================================


     //==========================================================================================
     // Dired
     //==========================================================================================
     void set_dired_mode();
     void show_next_folder(QString given_folder);
     void show_next_file(const QString &path);

     //==========================================================================================
     // Others
     //==========================================================================================
     void zap_to_char_finish_slot();
     void text_from_editor(const QString &given_text, const QString &command);
     void minimize_redactor();
     void set_counter_mode(); // *Counter*

 private:
     TextEditor *create_text_editor();
     TextEditor *current_buffer();
     QSplitter *create_splitter();
     QSplitter* current_splitter();

     QMdiArea *mdiArea;
     Minibuffer *minibuffer;
     QLabel *mode_minibuffer;

     private:


     //==========================================================================================
     // Minibuffer
     //==========================================================================================
     void set_given_text(const QString &given_text);
     void commandNotFound();
     void set_read_only_message();
     void set_focus_while_in_minibuffer();
     bool is_minibuffer_busy;
     bool is_focus_on_minibuffer;
     QString text_of_minibuffer_mode;
     QString text_of_minibuffer;

     //==========================================================================================
     // Killing
     //==========================================================================================
     void append_to_buffer();
     void set_append_to_buffer_mode();
     void prepend_to_buffer();
     void set_prepend_to_buffer_mode();
     void copy_to_buffer();
     void set_copy_to_buffer_mode();
     void insert_to_buffer();
     void set_insert_to_buffer_mode();
     void insert_buffer();
     void append_to_file();
     void set_append_to_file_mode();
     void insert_file();
     void set_start_of_buffer_text();
     void set_end_of_buffer_text();
     void set_zap_text();
     void add_help_window(sQ::HelpWindow window);

     //==========================================================================================
     // Search
     //==========================================================================================
     void set_search_forward_mode();
     void set_search_backward_mode();
     void set_search_forward_regexp_mode();
     void set_search_backward_regexp_mode();
     void goto_line(const QString &number_line);
     void set_goto_line_mode();
     bool is_in_loop_exec;

     //==========================================================================================
     // Fixit and Text
     //==========================================================================================
     void set_fixit_text();
     void set_face_mode(int step);

     //==========================================================================================
     // Windows
     //==========================================================================================
     QSplitter *my_splitter;
     TextEditor* return_current_buffer(QSplitter *given_splitter);

     bool check_this_splitter(QSplitter *given_splitter);
     void hide_one_level_up(QSplitter *given_splitter);
     void delete_buffer_completely(QSplitter *given_splitter);
     void find_the_right_splitter(QSplitter *given_splitter);
     void find_the_right_splitter_second(QSplitter *given_splitter, int count);
     void find_the_first_empty_splitter(QSplitter *given_splitter);
     void find_the_right_splitter_horizontal(QSplitter *given_splitter);
     void find_the_right_splitter_second_horizontal(QSplitter *given_splitter, int count);
     void find_the_first_empty_splitter_horizontal(QSplitter *given_splitter);
     void set_kill_window_mode();
     void save_buffers_before_closing_window();

     //==========================================================================================
     // Buffers
     //==========================================================================================
     void switch_to_buffer(const QString& name);
     void switch_to_buffer_other_window(const QString &name);
     void set_rename_buffer_mode();
     void rename_buffer(const QString& name);

     // *Buffer List*
     int current_line;

     QList<TextEditor *> all_buffers();
     QList<TextEditor *> list_of_activated_buffers;
     QList<TextEditor *> list_of_buffers_for_killing;
     QList<TextEditor *> list_of_buffers_for_saving;
     QList<TextEditor *> list_of_marked_buffers;
     QList<QString>  list_of_strings_for_every_buffer;

     QWidget* create_buffer_list();

     void save_buffer_before_killing(TextEditor *editor);
     void set_check_before_killing_mode();
     void rename_the_buffer_from_buffer_list();

     //==========================================================================================
     // Saving Files
     //==========================================================================================
     bool is_last_buffer;
     bool in_loop_exec;

     TextEditor *buffer_to_receive_focus;
     TextEditor *buffer_for_deletion;

     void set_ask_to_save_buffer_mode();
     void save_buffer_to_disk(const QString &choice);
     void set_wrote_buffer_to_disk_mode(TextEditor *buffer);
     void set_save_buffer_mode();
     void set_save_some_buffers_mode();
     void save_buffers_from_list(const QString &exit_result);
     void set_exit_mode();
     void write_file(const QString &path);

     //==========================================================================================
     // Visiting Files
     //==========================================================================================
     void find_file(const QString &path);
     void find_file_read_only(const QString &path);
     void find_alternate_file(const QString &path);
     void find_file_other_window(const QString &path);
     void set_visited_file_name_mode();
     void set_visited_file_name(const QString& name);

     //==========================================================================================
     // Exiting redactor
     //==========================================================================================
     void save_exit_from_minibuffer(TextEditor *editor);
     bool confirm_exit_redactor(TextEditor *editor);
     void set_modified_buffers_exist_mode();
     void checkExitAnyway(const QString& result);

     //==========================================================================================
     // Creating all separate parts of MainWindow
     //==========================================================================================
     void create_mdiArea();
     void create_actions();
     void create_shortcuts();
     void create_menus();
     void create_toolbars();
     void create_statusbar();
     void create_start_page();
     void create_counter(const QString &folderName = QDir::currentPath());
     void create_dired(QString folderName = QDir::currentPath());
     void create_shell();

     void init_variables();
     void read_settings();
     void write_settings();

     //==========================================================================================
     // Tutorial
     //==========================================================================================
     void show_redactor_tutorial();
     void show_unix_tutorial();

     //==========================================================================================
     // Menus and Toolbars
     //==========================================================================================
     QMenu *fileMenu;
     QMenu *editMenu;
     QMenu *viewMenu;
     QMenu *settingsMenu;
     QMenu *helpMenu;
     QToolBar *fileToolBar;
     QToolBar *editToolBar;

     //==========================================================================================
     // Actions from the File menu and Toolbar
     //==========================================================================================
     QAction *newAct;
     QAction *openAct;
     QAction *saveAct;
     QAction *saveAsAct;
     QAction *exitAct;
     QAction *cutAct;
     QAction *copyAct;
     QAction *pasteAct;
     QAction *preferencesAction;
     QAction *aboutAct;
     QAction *contactAct;
     QAction *aboutQtAct;
 };

 #endif
