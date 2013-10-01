#ifndef MYTEXTEDIT_H
#define MYTEXTEDIT_H

#include "enums.h"

#include <QPlainTextEdit>

class MyTextEdit : public QPlainTextEdit {
    Q_OBJECT

public:
   MyTextEdit();
   ~MyTextEdit();

   int position_mark_set;
   bool is_mark_active;
   bool is_mark_activated;

   void event_from_shell(QKeyEvent *event);

signals:

   void zap_to_char_finish_signal();

   void shell_handle_enter();
   void shell_handle_left(QKeyEvent *e);
   void shell_handle_history_up();
   void shell_handle_history_down();
   void shell_handle_home();

   void text_for_mainwindow(const QString &given_text, const QString &command);
   void modeline_focus(bool status);

   // Buffer Menu
   void pushed_x();
   void pushed_q();
   //void pushed_g();
   void pushed_d(int current_line, bool down);
   void pushed_u(int current_line, bool down);
   void pushed_s(int current_line);
   //void pushed_f(int current_line);
   void pushed_o(int current_line);
   void pushed_m(int current_line);
   void pushed_v(int current_line);
   void pushed_r(int current_line);
   void pushed_1(int current_line);
   void pushed_2(int current_line);
   void pushed_ctrl_d(int current_line, bool up);
   void pushed_del(int current_line, bool up);
   void pushed_percent(int current_line);
   void pushed_unmodified(int current_line);
   //------------

   void pushed_alt_s();
   void pushed_alt_v();

   void pushedCtrlx0();
   void pushedCtrlx1();
   void pushedCtrlx2();
   void pushedCtrlx3();

   void pushedCtrlx40(); // zero, not 'o'
   void pushedCtrlx4b();
   void pushedCtrlx4f();

   void pushedCtrlz();

   void pushedCtrlxi();
   void pushedCtrlxs();
   void pushedCtrlxb();
   void pushedCtrlxo(); // o, not zero
   void pushedCtrlxk();

   void pushed_ctrl_x_ctrl_b();
   void pushed_ctrl_x_ctrl_c();
   void pushed_ctrl_x_right();
   void pushed_ctrl_x_left();

   void pushedCtrlxCtrls();
   void pushedCtrlxCtrlw();
   void pushedCtrlxCtrlv();
   void pushedCtrlxCtrlf();
   void pushedCtrlxCtrlr();
   void pushedCtrlxCtrlq();

   void pushedAltTilda();
   void pushedCtrluAltTilda();
   void pushedCtrlxBraceRight();
   void pushedCtrlxBraceLeft();
   void pushedCtrlxCircum();

   void mark_text(sQ::Mark mark);
   void next_found_result();

   // Signals From Buffer/texteditor.cpp [Moved here]
   void pushed_enter(int current_line);

protected:
   virtual void keyPressEvent(QKeyEvent *event);
   virtual void mousePressEvent(QMouseEvent *event);
   virtual void paintEvent(QPaintEvent *e);
   virtual QSize sizeHint();

private slots:
   void text_from_mainwindow(const QString &given_text, const QString &command);

private:

   int found_results_count;

   bool alt;
   bool alt_o;
   bool ctrl;
   bool ctrl_u;
   bool ctrl_x;
   bool ctrl_x_4;
   bool shift;
   bool is_waiting_for_input;
   bool should_override;
   bool should_override_selection;
   bool is_minibuffer_busy;
   bool is_first_time;
   bool is_first_time_regexp;
   bool are_found_results_visible;
   bool last_was_regular_search;
   bool last_was_regexp_search;

   QString string_for_searching;
   QString text;
   QList<int> marked_positions;
   QMap<QString, QString> colors;

private:

   // Local
   void init_variables();
   void init_key_press_event_variables();
   void insert_colors();
   QString get_string() { return text; }
   void set_string(const QString &given_string) { text = given_string; }
   void reset_variables();
   void set_minibuffer(const QString &text);

   // |Basic|
   void move_forward_character();
   void move_backward_character();
   void move_end_word();
   void move_start_word();
   void move_forward_word();
   void move_backward_word();
   void move_previous_line();
   void move_next_line();
   void move_start_line();
   void move_end_line();
   void move_next_block();
   void move_previous_block();
   void move_end_document();
   void move_start_document();
   void scroll_up_command();
   void scroll_down_command();
   void recenter_top_bottom();
   void undo_command();
   void redo_command();
   void open_line();

   // |Mark|
   void set_mark_command();
   void exchange_point_and_mark();
   void mark_word();
   void mark_paragraph();
   void mark_page();
   void mark_whole_buffer();
   void move_to_remembered_position();

   // |Killing|
   void delete_backward_char();
   void delete_current_char();
   void delete_forward_char();
   void delete_horizontal_space();
   void delete_horizontal_space_leave_one_space();
   void delete_blank_lines();
   void delete_indentation();
   void kill_whole_line();
   void kill_line();
   void kill_region();
   void kill_ring_save();
   void kill_word();
   void backward_kill_word();
   void forward_kill_word();
   void backward_kill_sentence();
   void zap_to_char(const QChar &zap_character);
   void yank();

   // |Registers|

   // |Display|

   // |Search|
   void search_forward();
   void search_forward_first_time();
   void search_backward();
   void search_backward_first_time();
   void search_forward_regexp();
   void search_forward_regexp_first_time();
   void search_backward_regexp();
   void search_backward_regexp_first_time();
   void show_founded_results();
   void go_to_line();

   // |Fixit|
   void transpose_chars();
   void transpose_words();
   void transpose_lines();

   // |Text|
   void facemenu_set_default();
   void facemenu_set_bold();
   void facemenu_set_italic();
   void facemenu_set_bold_italic();
   void facemenu_set_underline();
   void facemenu_set_strikeout();
   void facemenu_set_face(const QString &given_text);
   void downcase_word();
   void upcase_word();
   void capitalize_word();
   void quoted_insert();
};

#endif // MYTEXTEDIT_H
