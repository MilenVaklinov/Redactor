#include "mytextedit.h"
#include "enums.h"

#include <QApplication>
#include <QDebug>
#include <QKeyEvent>
#include <QShortcut>
#include <QScrollBar>
#include <QPainter>
#include <QTextBlock>
#include <QTimer>
#include <QTextDocumentFragment>

MyTextEdit::MyTextEdit()
{
    setCursorWidth(0);
    setAttribute(Qt::WA_DeleteOnClose);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setWordWrapMode(QTextOption::WrapAnywhere);

    init_variables();
}

//==========================================================================================
// Initialize all the variables.
//==========================================================================================

void MyTextEdit::init_variables()
{
    // Key variables
    alt = false;
    alt_o = false;
    ctrl = false;
    ctrl_u = false;
    ctrl_x = false;
    ctrl_x_4 = false;
    shift = false;

    // Int variables
    position_mark_set = 0;
    found_results_count = 0;

    // Bool variables
    is_first_time = true;
    is_first_time_regexp = true;

    last_was_regular_search = false;
    last_was_regexp_search = false;

    is_mark_active = false;
    is_mark_activated = false;

    should_override = false;
    should_override_selection = false;

    is_waiting_for_input = false;
    is_minibuffer_busy = false;
    are_found_results_visible = false;

    // String variables
    text = "";

    // Reset/Empty/Clear the list from old data
    if (!marked_positions.isEmpty()) {

        QList<int>::iterator it = marked_positions.begin();

        while (it != marked_positions.end())
            it = marked_positions.erase(it);
    }
}

MyTextEdit::~MyTextEdit()
{

}

//==========================================================================================
// We overrided this function to paint the text cursor of the document.
//==========================================================================================

void MyTextEdit::paintEvent(QPaintEvent *e)
{
    /* should_override is a variable which checks whether we should override the painted area.
     * By override we mean that we should first to draw the content (text, selection, etc.) and
     * after that to draw our cursor. This is especially important because otherwise the selection
     * hides our cursor. */

    QTextCursor cursor = textCursor();

    if (should_override) {

        QPainter painter(viewport());
        painter.setPen(Qt::NoPen);
        QColor cursor_color = QColor(Qt::darkGray).lighter(120);
        painter.setBrush(cursor_color);

        QRect rect = cursorRect(cursor);

        QSize cursor_size = sizeHint();
        rect.setSize(cursor_size);

        painter.setCompositionMode(QPainter::CompositionMode_Multiply);
        painter.drawRect(rect);
        viewport()->update();

        should_override = false;
        return;

    } else if (should_override_selection) {

        QPainter painter(viewport());
        painter.setPen(Qt::black);

        QRect rect = cursorRect(cursor);

        QSize cursor_size = sizeHint();
        rect.setSize(cursor_size);

        painter.drawRect(rect);

        should_override_selection = false;
        return;

    } else {
        if (hasFocus()) {

            // If the document has the keyboard focus and the minibuffer is not busy
            // we set the modeline color to "has focus".
            if (!is_minibuffer_busy)
                emit modeline_focus(true);

            should_override = true;
            QPlainTextEdit::paintEvent(e);
            paintEvent(e);
            return;

        } else {
            // We enter in this case if we do not have the focus on the current document.

            // If the document doesn't have the keyboard focus and the minibuffer is not busy
            // we set the modeline color to "has focus".
            if (!is_minibuffer_busy)
                emit modeline_focus(false);

            should_override_selection = true;
            QPlainTextEdit::paintEvent(e);
            paintEvent(e);
            return;
        }
    }
}

//==========================================================================================
// Returns the size of the current character.
//==========================================================================================

QSize MyTextEdit::sizeHint()
{
//    QTextCursor cursor = textCursor();
//    QString my_character;
//    bool move = cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);

//    if (move) {
//        my_character = cursor.selectedText();
//        qDebug() << cursor.selectedText();
//        cursor.clearSelection();
//        cursor.movePosition(QTextCursor::Left);
//        setTextCursor(cursor);
//    } else
//        my_character = " ";

    return QSize(fontMetrics().width(' '), fontMetrics().height());
}

//==========================================================================================
// This function catches all mouse events from the left button (click) and return the mark
// false so, we stops the selection
//==========================================================================================

void MyTextEdit::mousePressEvent(QMouseEvent *event)
{
    if (objectName() == "*Shell*") { /* ignore */ }
    else if (is_minibuffer_busy) {
        emit text_for_mainwindow("", "set_focus_while_in_minibuffer");
        clearFocus();
        event->ignore();
        return;
    }

    if (is_mark_activated)
        is_mark_activated = false;

    QPlainTextEdit::mousePressEvent(event);
}

//==========================================================================================
// This event is from shell widget and we should handle it here.
//==========================================================================================

void MyTextEdit::event_from_shell(QKeyEvent *event)
{
    QPlainTextEdit::keyPressEvent(event);
}

//==========================================================================================
// Here we cathes all the pressed keys.
//==========================================================================================

void MyTextEdit::keyPressEvent(QKeyEvent *event)
{
    QTextCursor cursor = textCursor();
    int current_line = cursor.blockNumber() + 1;
    bool up = true;
    bool down = false;

    Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();
    alt = modifiers.testFlag(Qt::AltModifier);
    ctrl = modifiers.testFlag(Qt::ControlModifier);
    shift = modifiers.testFlag(Qt::ShiftModifier);

    if (is_waiting_for_input) {
        QString string_character = event->text();

        if (!string_character.isEmpty()) {
            QChar character = string_character.at(0);

            if (!character.isLetterOrNumber())
                return;
            else {
                zap_to_char(character);
                return;
            }
        } else
            is_waiting_for_input = false;
    }

    if (are_found_results_visible) {
        document()->undo();
        are_found_results_visible = false;
    }

    switch(event->key()) {

    case Qt::Key_X:
        if (!ctrl_x && ctrl) {
            ctrl_x = true;
            ctrl = false;
        } else if (ctrl_x && ctrl) {
               exchange_point_and_mark();
               ctrl_x = false;
               ctrl = false;
        } else if (objectName() == "*Buffer List*")
            emit pushed_x();
        else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_0:
        if (ctrl_x && !ctrl_x_4) {
            emit pushedCtrlx0();
            ctrl_x = false;
        } else if (ctrl_x_4 && !ctrl_x) {
            emit pushedCtrlx40();
            ctrl_x_4 = false;
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_1:
        if (objectName() == "*Buffer List*")
            emit pushed_1(current_line);
        else if (ctrl_x) {
            emit pushedCtrlx1();
            ctrl_x = false;
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_2:
        if (objectName() == "*Buffer List*")
            emit pushed_2(current_line);
        else if (ctrl_x) {
            emit pushedCtrlx2();
            ctrl_x = false;
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_3:
        if (ctrl_x) {
            emit pushedCtrlx3();
            ctrl_x = false;
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_4:
        if (ctrl_x) {
                ctrl_x_4 = true;
                ctrl_x = false;
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_A:
        if (ctrl && !ctrl_x) {
            move_start_line();
            ctrl = false;
        } else if (alt) {
            move_start_word();
            alt = false;
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_B:
        if (ctrl && !ctrl_x && !ctrl_x_4) {
            move_backward_character();
            ctrl = false;
        } else if (ctrl && ctrl_x && !ctrl_x_4) {
            emit pushed_ctrl_x_ctrl_b();
            ctrl = false;
            ctrl_x = false;
        } else if (ctrl_x_4) {
            emit pushedCtrlx4b();
            ctrl_x_4 = false;
        } else if (!ctrl && ctrl_x) {
            emit pushedCtrlxb();
            ctrl_x = false;
        } else if (alt) {
            move_backward_word();
            alt = false;
        } else if (alt_o) {
            facemenu_set_bold();
            alt_o = false;
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_C:
        if (ctrl && ctrl_x) {
            emit pushed_ctrl_x_ctrl_c();
            ctrl_x = false;
            ctrl = false;
        } else if (alt) {
            capitalize_word();
            alt = false;
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_D:
        if (ctrl && !ctrl_x) {

            ctrl = false;

            if (objectName() == "*Buffer List*")
                emit pushed_ctrl_d(current_line, up);
            else
                delete_forward_char();

        } else if (alt) {
            forward_kill_word();
            alt = false;
        } else if (alt_o) {
            facemenu_set_default();
            alt_o = false;
        } else if (objectName() == "*Buffer List*") {
            emit pushed_d(current_line, down);
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_E:
        if (ctrl && !ctrl_x) {
            move_end_line();
            ctrl = false;
        } else if (alt) {
            move_end_word();
            alt = false;
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_F:
        /*if (objectName() == "*Buffer List*") {
            emit pushed_f(current_line); }
        else*/ if (ctrl_x_4) {
            emit pushedCtrlx4f();
            ctrl_x_4 = false;
        } else if (ctrl && !ctrl_x) {
            move_forward_character();
            ctrl = false;
        } else if (ctrl_x) {
            if (ctrl) {
                emit pushedCtrlxCtrlf();
                ctrl_x = false;
                ctrl = false;
            } else
                QPlainTextEdit::keyPressEvent(event);
        } else if (alt) {
            move_forward_word();
            alt = false;
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

//    case Qt::Key_G:
//        if (objectName() == "*Buffer List*")
//            emit pushed_g();
//        else {
//            is_mark_activated = false;
//            cursor.clearSelection();
//            setTextCursor(cursor);
//            QPlainTextEdit::keyPressEvent(event);
//        }
//        break;

    case Qt::Key_H:
        if (ctrl_x) {
            mark_whole_buffer();
            ctrl_x = false;
        } else if (alt) {
            mark_paragraph();
            alt = false;
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_I:
        if (ctrl_x) {
            if (isReadOnly()) {
                emit text_for_mainwindow("", "set_read_only_message");
                ctrl_x = false;
                return;
            } else {
                emit pushedCtrlxi();
                ctrl_x = false;
            }
        } else if (alt_o) {
            facemenu_set_italic();
            alt_o = false;
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_J:
        is_mark_activated = false;
        cursor.clearSelection();
        setTextCursor(cursor);
        QPlainTextEdit::keyPressEvent(event);
        break;

    case Qt::Key_K:
        if(ctrl_x) {
            emit pushedCtrlxk();
            ctrl_x = false;
        } else if (ctrl && !ctrl_x) {
            kill_line();
            ctrl = false;
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_L:
        if (alt_o) {
            facemenu_set_bold_italic();
            alt_o = false;
        } else if (ctrl) {
            recenter_top_bottom();
            ctrl = false;
        } else if (alt) {
            downcase_word();
            alt = false;
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_M:
        if (objectName() == "*Buffer List*")
            emit pushed_m(current_line);
        else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_N:
        if (ctrl && !ctrl_x) {
            move_next_line();
            ctrl = false;
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_O:
        if (objectName() == "*Buffer List*")
            emit pushed_o(current_line);
        else if (ctrl) {
            if (ctrl_x) {
                delete_blank_lines();
                ctrl_x = false;
                ctrl = false;
            } else {
                open_line();
                ctrl = false;
            }
        } else if (ctrl_x) {
            emit pushedCtrlxo();
            ctrl_x = false;
        } else if (alt) {
            alt_o = true;
            alt = false;
        }else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_P:
        if (ctrl && !ctrl_x) {
            move_previous_line();
            ctrl = false;
        } else if (ctrl && ctrl_x) {
            mark_page();
            ctrl = false;
            ctrl_x = false;
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_Q:
        if (objectName() == "*Buffer List*")
            emit pushed_q();
        else if (ctrl_x && ctrl) {
                emit pushedCtrlxCtrlq();
                ctrl = false;
                ctrl_x = false;
        } else if (!ctrl_x && ctrl) {
            quoted_insert();
            ctrl = false;
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_R:
        if (objectName() == "*Buffer List*")
            emit pushed_r(current_line);
        else if(ctrl && ctrl_x) {
            emit pushedCtrlxCtrlr();
            ctrl_x = false;
            ctrl = false;
        } else if (!ctrl && ctrl_x) {
            redo_command();
            ctrl_x = false;
        } else if (!alt && ctrl) {
            search_backward();
            ctrl = false;
        } else if (alt && ctrl) {
            search_backward_regexp();
            alt = false;
            ctrl = false;
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_S:
        if (objectName() == "*Buffer List*")
            emit pushed_s(current_line);
        else if (ctrl && ctrl_x) {
                emit pushedCtrlxCtrls();
                ctrl = false;
                ctrl_x = false;
        } else if (!ctrl && ctrl_x) {
                emit pushedCtrlxs();
                ctrl_x = false;
        } else if (ctrl && !alt) {
            search_forward();
            ctrl = false;
        } else if (ctrl && alt) {
            search_forward_regexp();
            ctrl = false;
            alt = false;
        } else if (!ctrl && alt) {
            show_founded_results();
            alt = false;
        } else if (alt_o) {
            facemenu_set_strikeout();
            alt_o = false;
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_T:
        if (!ctrl_x && ctrl) {
            transpose_chars();
            ctrl = false;
        } else if(ctrl_x && ctrl) {
            transpose_lines();
            ctrl_x = false;
            ctrl = false;
        } else if (alt) {
            transpose_words();
            alt = false;
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_U:
        if (objectName() == "*Buffer List*")
            emit pushed_u(current_line, down);
        else if (ctrl) {
            ctrl_u = true;
            ctrl = false;
        } else if (ctrl_x) {
            undo_command();
            ctrl_x = false;
        } else if (alt_o) {
            facemenu_set_underline();
            alt_o = false;
        } else if (alt) {
            upcase_word();
            alt = false;
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_V:
        if (objectName() == "*Buffer List*")
            emit pushed_v(current_line);
        else if (ctrl && !ctrl_x) {
            ctrl = false;
            scroll_down_command();
        } else if (alt) {
            alt = false;
            scroll_up_command();
        } else if (ctrl && ctrl_x) {
            emit pushedCtrlxCtrlv();
            ctrl = false;
            ctrl_x = false;
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_W:
        if (ctrl) {
            if (ctrl_x) {
                emit pushedCtrlxCtrlw();
                ctrl_x = false;
            } else {
                kill_region();
                ctrl = false;
            }
        } else if (alt) {
            kill_ring_save();
            alt = false;
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_Y:
        if (ctrl && !ctrl_x) {
            yank();
            ctrl = false;
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_Z:
        if (ctrl && !ctrl_x) {
            emit pushedCtrlz();
            ctrl = false;
        } else if (alt) {
            is_waiting_for_input = true;
            emit text_for_mainwindow("", "set_zap_text");
            alt = false;
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_AsciiTilde:
        if (objectName() == "*Buffer List*")
            emit pushed_unmodified(current_line);
        else if (alt && !ctrl_u) {
            emit pushedAltTilda();
            alt = false;
        } else if (alt && ctrl_u) {
            emit pushedCtrluAltTilda();
            ctrl_u = false;
            alt = false;
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_AsciiCircum:
        if (ctrl_x) {
            emit pushedCtrlxCircum();
            ctrl_x = false;
        } else if (alt) {
            delete_indentation();
            alt = false;
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_BraceRight:
        if (ctrl_x) {
            emit pushedCtrlxBraceRight();
            ctrl_x = false;
        } else if (alt) {
            move_next_block();
            alt = false;
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_BraceLeft:
        if (ctrl_x) {
            emit pushedCtrlxBraceLeft();
            ctrl_x = false;
        } else if (alt) {
            move_previous_block();
            alt = false;
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_Backspace:
        if (objectName() == "*Shell*") {
            emit shell_handle_left(event);
        } else if (shift && ctrl) {
                kill_whole_line();
                ctrl = false;
                shift = false;
        } else if (alt) {
            backward_kill_word();
            alt = false;
        } else if (ctrl_x) {
            backward_kill_sentence();
            ctrl_x = false;
        } else
            delete_backward_char();
        break;

    case Qt::Key_Backslash:
        if (alt) {
            delete_horizontal_space();
            alt = false;
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_Home:
        if (objectName() == "*Shell*") {
            emit shell_handle_home();
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_Delete:
        if (objectName() == "*Buffer List*")
            emit pushed_del(current_line, up);
        else if (alt) {
            alt = false;
            kill_word();
        } else if (ctrl_x) {
            ctrl_x = false;
            backward_kill_sentence();
        } else {
            delete_current_char();
        }
        break;

    case Qt::Key_Space:
        if (ctrl && !ctrl_u) {
            ctrl = false;
            set_mark_command();
        } else if (ctrl && ctrl_u) {
            ctrl = false;
            ctrl_u = false;
            move_to_remembered_position();
        } else if (alt) {
            alt = false;
            delete_horizontal_space_leave_one_space();
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_Slash:
        if (ctrl) {
            undo_command();
            ctrl = false;
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_Underscore:
        if (ctrl) {
            undo_command();
            ctrl = false;
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_Greater:
        if (alt) {
            move_end_document();
            alt = false;
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_Less:
        if (alt) {
            move_start_document();
            alt = false;
        } else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_Return:
        if (objectName() == "*Shell*") {

            emit shell_handle_enter();

        } else if (objectName() == "*Dired*") {

            emit pushed_enter(current_line);

        } else if (objectName() == "*Buffer List*") {

            emit pushed_enter(current_line);

        } /*else if (isReadOnly()) {

            qDebug() << "AA";
            focusNextPrevChild(true);

            is_mark_activated = false;

            if (cursor.hasSelection()) {
                cursor.clearSelection();
                setTextCursor(cursor);
            }

            QPlainTextEdit::keyPressEvent(event);

        } */else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_Tab:
        if (is_mark_activated) {
            is_mark_activated = false;
            cursor.setPosition(cursor.selectionStart());
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        } else
            QPlainTextEdit::keyPressEvent(event);
        break;

    case Qt::Key_At:
        if (ctrl && !ctrl_x) {
            set_mark_command();
            ctrl = false;
        } else if (alt) {
            mark_word();
            alt = false;
        }else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_Percent:
        if (objectName() == "*Buffer List*")
            emit pushed_percent(current_line);
        else {
            is_mark_activated = false;
            cursor.clearSelection();
            setTextCursor(cursor);
            QPlainTextEdit::keyPressEvent(event);
        }
        break;

    case Qt::Key_Left:
        if (objectName() == "*Shell*") {
            emit shell_handle_left(event);
        } else if (is_mark_activated && !ctrl_x) {
            moveCursor(QTextCursor::Left, QTextCursor::KeepAnchor);
        } else if (ctrl_x) {
            emit pushed_ctrl_x_left();
            ctrl_x = false;
        } else
            QPlainTextEdit::keyPressEvent(event);
        break;

    case Qt::Key_Right:
        if (is_mark_activated && !ctrl_x) {
            moveCursor(QTextCursor::Right, QTextCursor::KeepAnchor);
        } else if (ctrl_x) {
            emit pushed_ctrl_x_right();
            ctrl_x = false;
        } else
            QPlainTextEdit::keyPressEvent(event);
        break;

    case Qt::Key_Up:
        if (objectName() == "*Shell*") {
            emit shell_handle_history_up();
        } else if (is_mark_activated) {
            moveCursor(QTextCursor::Up, QTextCursor::KeepAnchor);
        } else
            QPlainTextEdit::keyPressEvent(event);
        break;

    case Qt::Key_Down:
        if (objectName() == "*Shell*") {
            emit shell_handle_history_down();
        } else if (is_mark_activated) {
            moveCursor(QTextCursor::Down, QTextCursor::KeepAnchor);
        } else
            QPlainTextEdit::keyPressEvent(event);
        break;

    default:
        // fsdfsd
        QPlainTextEdit::keyPressEvent(event);
    }
}

//==========================================================================================
//
//==========================================================================================

void MyTextEdit::insert_colors()
{
    colors.insert("AliceBlue", "#F0F8FF");
    colors.insert("AntiqueWhite", "#FAEBD7");
    colors.insert("Aqua", "#00FFFF");
    colors.insert("Aquamarine", "#7FFFD4");
    colors.insert("Azure", "#F0FFFF");
    colors.insert("Beige", "#F5F5DC");
    colors.insert("Bisque", "#FFE4C4");
    colors.insert("Black", "#000000");
    colors.insert("BlanchedAlmond", "#FFEBCD");
    colors.insert("Blue", "#0000FF");
    colors.insert("BlueViolet", "#8A2BE2");
    colors.insert("Brown", "#A52A2A");
    colors.insert("BurlyWood", "#DEB887");
    colors.insert("CadetBlue", "#5F9EA0");
    colors.insert("Chartreuse", "#7FFF00");
    colors.insert("Chocolate", "#D2691E");
    colors.insert("Coral", "#FF7F50");
    colors.insert("CornflowerBlue", "#6495ED");
    colors.insert("Cornsilk", "#FFF8DC");
    colors.insert("Crimson", "#DC143C");
    colors.insert("Cyan", "#00FFFF");
    colors.insert("DarkBlue", "#00008B");
    colors.insert("DarkCyan", "#008B8B");
    colors.insert("DarkGoldenRod", "#B8860B");
    colors.insert("DarkGray", "#A9A9A9");
    colors.insert("DarkGrey", "#A9A9A9");
    colors.insert("DarkGreen", "#006400");
    colors.insert("DarkKhaki", "#BDB76B");
    colors.insert("DarkMagenta", "#8B008B");
    colors.insert("DarkOliveGreen", "#556B2F");
    colors.insert("Darkorange", "#FF8C00");
    colors.insert("DarkOrchid", "#9932CC");
    colors.insert("DarkRed", "#8B0000");
    colors.insert("DarkSalmon", "#E9967A");
    colors.insert("DarkSeaGreen", "#8FBC8F");
    colors.insert("DarkSlateBlue", "#483D8B");
    colors.insert("DarkSlateGray", "#2F4F4F");
    colors.insert("DarkSlateGrey", "#2F4F4F");
    colors.insert("DarkTurquoise", "#00CED1");
    colors.insert("DarkViolet", "#9400D3");
    colors.insert("DeepPink", "#FF1493");
    colors.insert("DeepSkyBlue", "#00BFFF");
    colors.insert("DimGray", "#696969");
    colors.insert("DimGrey", "#696969");
    colors.insert("DodgerBlue", "#1E90FF");
    colors.insert("FireBrick", "#B22222");
    colors.insert("FloralWhite", "#FFFAF0");
    colors.insert("ForestGreen", "#228B22");
    colors.insert("Fuchsia", "#FF00FF");
    colors.insert("Gainsboro", "#DCDCDC");
    colors.insert("GhostWhite", "#F8F8FF");
    colors.insert("Gold", "#FFD700");
    colors.insert("GoldenRod", "#DAA520");
    colors.insert("Gray", "#808080");
    colors.insert("Grey", "#808080");
    colors.insert("Green", "#008000");
    colors.insert("GreenYellow", "#ADFF2F");
    colors.insert("HoneyDew", "#F0FFF0");
    colors.insert("HotPink", "#FF69B4");
    colors.insert("IndianRed", "#CD5C5C");
    colors.insert("Indigo", "#4B0082");
    colors.insert("Ivory", "#FFFFF0");
    colors.insert("Khaki", "#F0E68C");
    colors.insert("Lavender", "#E6E6FA");
    colors.insert("LavenderBlush", "#FFF0F5");
    colors.insert("LawnGreen", "#7CFC00");
    colors.insert("LemonChiffon", "#FFFACD");
    colors.insert("LightBlue", "#ADD8E6");
    colors.insert("LightCoral", "#F08080");
    colors.insert("LightCyan", "#E0FFFF");
    colors.insert("LightGoldenRodYellow", "#FAFAD2");
    colors.insert("LightGray", "#D3D3D3");
    colors.insert("LightGrey", "#D3D3D3");
    colors.insert("LightGreen", "#90EE90");
    colors.insert("LightPink", "#FFB6C1");
    colors.insert("LightSalmon", "#FFA07A");
    colors.insert("LightSeaGreen", "#20B2AA");
    colors.insert("LightSkyBlue", "#87CEFA");
    colors.insert("LightSlateGray", "#778899");
    colors.insert("LightSlateGrey", "#778899");
    colors.insert("LightSteelBlue", "#B0C4DE");
    colors.insert("LightYellow", "#FFFFE0");
    colors.insert("Lime", "#00FF00");
    colors.insert("LimeGreen", "#32CD32");
    colors.insert("Linen", "#FAF0E6");
    colors.insert("Magenta", "#FF00FF");
    colors.insert("Maroon", "#800000");
    colors.insert("MediumAquaMarine", "#66CDAA");
    colors.insert("MediumBlue", "#0000CD");
    colors.insert("MediumOrchid", "#BA55D3");
    colors.insert("MediumPurple", "#9370DB");
    colors.insert("MediumSeaGreen", "#3CB371");
    colors.insert("MediumSlateBlue", "#7B68EE");
    colors.insert("MediumSpringGreen", "#00FA9A");
    colors.insert("MediumTurquoise", "#48D1CC");
    colors.insert("MediumVioletRed", "#C71585");
    colors.insert("MidnightBlue", "#191970");
    colors.insert("MintCream", "#F5FFFA");
    colors.insert("MistyRose", "#FFE4E1");
    colors.insert("Moccasin", "#FFE4B5");
    colors.insert("NavajoWhite", "#FFDEAD");
    colors.insert("Navy", "#000080");
    colors.insert("OldLace", "#FDF5E6");
    colors.insert("Olive", "#808000");
    colors.insert("OliveDrab", "#6B8E23");
    colors.insert("Orange", "#FFA500");
    colors.insert("OrangeRed", "#FF4500");
    colors.insert("Orchid", "#DA70D6");
    colors.insert("PaleGoldenRod", "#EEE8AA");
    colors.insert("PaleGreen", "#98FB98");
    colors.insert("PaleTurquoise", "#AFEEEE");
    colors.insert("PaleVioletRed", "#DB7093");
    colors.insert("PapayaWhip", "#FFEFD5");
    colors.insert("PeachPuff", "#FFDAB9");
    colors.insert("Peru", "#CD853F");
    colors.insert("Pink", "#FFC0CB");
    colors.insert("Plum", "#DDA0DD");
    colors.insert("PowderBlue", "#B0E0E6");
    colors.insert("Purple", "#800080");
    colors.insert("Red", "#FF0000");
    colors.insert("RosyBrown", "#BC8F8F");
    colors.insert("RoyalBlue", "#4169E1");
    colors.insert("SaddleBrown", "#8B4513");
    colors.insert("Salmon", "#FA8072");
    colors.insert("SandyBrown", "#F4A460");
    colors.insert("SeaGreen", "#2E8B57");
    colors.insert("SeaShell", "#FFF5EE");
    colors.insert("Sienna", "#A0522D");
    colors.insert("Silver", "#C0C0C0");
    colors.insert("SkyBlue", "#87CEEB");
    colors.insert("SlateBlue", "#6A5ACD");
    colors.insert("SlateGray", "#708090");
    colors.insert("SlateGrey", "#708090");
    colors.insert("Snow", "#FFFAFA");
    colors.insert("SpringGreen", "#00FF7F");
    colors.insert("SteelBlue", "#4682B4");
    colors.insert("Tan", "#D2B48C");
    colors.insert("Teal", "#008080");
    colors.insert("Thistle", "#D8BFD8");
    colors.insert("Tomato", "#FF6347");
    colors.insert("Turquoise", "#40E0D0");
    colors.insert("Violet", "#EE82EE");
    colors.insert("Wheat", "#F5DEB3");
    colors.insert("White", "#FFFFFF");
    colors.insert("WhiteSmoke", "#F5F5F5");
    colors.insert("Yellow", "#FFFF00");
    colors.insert("YellowGreen", "#9ACD32");
}

//==========================================================================================
//
//==========================================================================================

void MyTextEdit::init_key_press_event_variables()
{
    alt = false;
    ctrl = false;
    shift = false;

    ctrl_u = false;
    ctrl_x = false;
    ctrl_x_4 = false;
}

//==========================================================================================
//
//==========================================================================================

void MyTextEdit::text_from_mainwindow(const QString &given_text, const QString &command)
{
    if (command == "set_minibuffer") {
        set_minibuffer(given_text);
    } else if (hasFocus()) {

        if (command == "reset_variables") {
            reset_variables();
        } else if (command == "face") {
            facemenu_set_face(given_text);
        } else if (command == "search_forward") {
            search_forward();
        } else if (command == "search_forward_first_time") {
            string_for_searching = given_text;
            search_forward_first_time();
        } else if (command == "search_backward") {
            search_backward();
        } else if (command == "search_backward_first_time") {
            string_for_searching = given_text;
            search_backward_first_time();
        } else if (command == "search_forward_regexp") {
            search_forward_regexp();
        } else if (command == "search_forward_regexp_first_time") {
            string_for_searching = given_text;
            search_forward_regexp_first_time();
        } else if (command == "search_backward_regexp") {
            search_backward_regexp();
        } else if (command == "search_backward_regexp_first_time") {
            string_for_searching = given_text;
            search_backward_regexp_first_time();
        } else
            return;

    } else
        return;
}

//==========================================================================================
//
//==========================================================================================

void MyTextEdit::set_minibuffer(const QString &text)
{
    if (text == "is_busy")
        is_minibuffer_busy = true;
    else
        is_minibuffer_busy = false;
}

//==========================================================================================
//
//==========================================================================================

void MyTextEdit::reset_variables()
{
    //position_mark_set = 0;
    string_for_searching = "";

    is_mark_activated = false;
    is_first_time = true;
    is_first_time_regexp = true;

    QTextCursor cursor = textCursor();

    if (cursor.hasSelection())
        cursor.clearSelection();

    setTextCursor(cursor);
    setFocus();
}

//==========================================================================================
//
//
//                             |Basic| 7. Basic Editing Commands
//
//
//==========================================================================================

/*
    - 7.1 Inserting Text: [Not done]
    - 7.2 Moving Point: Implemented HERE
    - 7.2.1 Movein Point in Documentation
    - 7.3 Erasing: Implemented HERE
    - 7.4 Basic Undo: Implemented HERE
    - 7.5 Files: Implemented in mainwindow.cpp and in Buffer/texteditor.cpp
    - 7.6 Help: [Not done]
    - 7.7 Blank Lines: Implemented HERE
    - 7.8 Continuation Lines: [Not done]
    - 7.9 Position Info: [Not done]
    - 7.10 Arguments: [Not done]
    - 7.11 Repeating: [Not done]
*/

//==========================================================================================
// 7.2 Moving Point
//==========================================================================================

void MyTextEdit::move_forward_character()
{
    if (is_mark_activated)
        moveCursor(QTextCursor::Right, QTextCursor::KeepAnchor);
    else
        moveCursor(QTextCursor::Right);

}

//==========================================================================================

void MyTextEdit::move_backward_character()
{
    if (is_mark_activated)
        moveCursor(QTextCursor::Left, QTextCursor::KeepAnchor);
    else
        moveCursor(QTextCursor::Left);
}

//==========================================================================================

void MyTextEdit::move_start_word()
{
    if (is_mark_activated)
        moveCursor(QTextCursor::StartOfWord, QTextCursor::KeepAnchor);
    else
        moveCursor(QTextCursor::StartOfWord);
}

//==========================================================================================

void MyTextEdit::move_end_word()
{
    if (is_mark_activated)
        moveCursor(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    else
        moveCursor(QTextCursor::EndOfWord);
}

//==========================================================================================

void MyTextEdit::move_forward_word()
{
    if (is_mark_activated)
        moveCursor(QTextCursor::NextWord, QTextCursor::KeepAnchor);
    else
        moveCursor(QTextCursor::NextWord);
}

//==========================================================================================

void MyTextEdit::move_backward_word()
{
    if (is_mark_activated)
        moveCursor(QTextCursor::PreviousWord, QTextCursor::KeepAnchor);
    else
        moveCursor(QTextCursor::PreviousWord);
}

//==========================================================================================

void MyTextEdit::move_previous_line()
{
    if (is_mark_activated)
        moveCursor(QTextCursor::Up, QTextCursor::KeepAnchor);
    else
        moveCursor(QTextCursor::Up);
}

//==========================================================================================

void MyTextEdit::move_next_line()
{
    if (is_mark_activated)
        moveCursor(QTextCursor::Down, QTextCursor::KeepAnchor);
    else
        moveCursor(QTextCursor::Down);
}

//==========================================================================================

void MyTextEdit::move_start_line()
{
    if (is_mark_activated)
        moveCursor(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
    else
        moveCursor(QTextCursor::StartOfLine);
}

//==========================================================================================

void MyTextEdit::move_end_line()
{
    if (is_mark_activated)
        moveCursor(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
    else
        moveCursor(QTextCursor::EndOfLine);
}

//==========================================================================================

void MyTextEdit::move_next_block()
{
    if (is_mark_activated)
        moveCursor(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
    else
        moveCursor(QTextCursor::NextBlock);
}

//==========================================================================================

void MyTextEdit::move_previous_block()
{
    if (is_mark_activated)
        moveCursor(QTextCursor::PreviousBlock, QTextCursor::KeepAnchor);
    else
        moveCursor(QTextCursor::PreviousBlock);
}

//==========================================================================================

void MyTextEdit::move_start_document()
{
    if (is_mark_activated)
        moveCursor(QTextCursor::Start, QTextCursor::KeepAnchor);
    else
        moveCursor(QTextCursor::Start);
}

//==========================================================================================

void MyTextEdit::move_end_document()
{
    if (is_mark_activated)
        moveCursor(QTextCursor::End, QTextCursor::KeepAnchor);
    else
        moveCursor(QTextCursor::End);
}

//==========================================================================================

void MyTextEdit::scroll_up_command()
{
    QScrollBar *vertical_bar = verticalScrollBar();
    QTextCursor cursor = textCursor();

    int minimum = vertical_bar->minimum();
    int maximum = vertical_bar->maximum();

    // If the minimum and maximum are zero, then we don't have anything to scroll.
    if ( (minimum == 0) && (maximum == 0) )
        return;

    bool moved = false;
    qreal lastY = cursorRect(cursor).top();
    qreal distance = 0;

        // move using movePosition to keep the cursor's x

        do {

            qreal y = cursorRect(cursor).top();
            distance += qAbs(y - lastY);
            lastY = y;

            moved = cursor.movePosition(QTextCursor::Up, QTextCursor::MoveAnchor);

        } while (moved && distance < viewport()->height() );

        if (moved) {
                cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor);
                cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
                vertical_bar->triggerAction(QAbstractSlider::SliderPageStepSub);
        }

    setTextCursor(cursor);
}

//==========================================================================================

void MyTextEdit::scroll_down_command()
{
    QScrollBar *vertical_bar = verticalScrollBar();
    QTextCursor cursor = textCursor();

    int minimum = vertical_bar->minimum();
    int maximum = vertical_bar->maximum();

    if ( (minimum == 0) && (maximum == 0) )
        return;

    bool moved = false;
    qreal lastY = cursorRect(cursor).top();
    qreal distance = 0;

        // move using movePosition to keep the cursor's x

        do {

            qreal y = cursorRect(cursor).top();
            distance += qAbs(y - lastY);
            lastY = y;

            moved = cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor);

        } while (moved && distance < viewport()->height() );

        if (moved) {
                cursor.movePosition(QTextCursor::Up, QTextCursor::MoveAnchor, 2);
                vertical_bar->triggerAction(QAbstractSlider::SliderPageStepAdd);
        }

        setTextCursor(cursor);

}

//==========================================================================================
// C-l ............... Scroll the selected window so the current line is the center-most text line;
// on subsequent consecutive invocations, make the current line the top line, the bottom line, and so
// on in cyclic order.
//==========================================================================================

void MyTextEdit::recenter_top_bottom()
{
    QScrollBar *vertical_bar = verticalScrollBar();
    QTextCursor cursor = textCursor();

    if (vertical_bar->minimum() == vertical_bar->maximum())
        return;

    vertical_bar->setSliderPosition(cursor.position());
    ensureCursorVisible();
}

//==========================================================================================
// 7.4 Undoing Changes / Redo Changes
//==========================================================================================

//==========================================================================================
// C-/, C-x u, C-_ .................. These tree commands all undo changes.
//==========================================================================================

void MyTextEdit::undo_command()
{
    if (!isReadOnly()) {
        if (document()->isUndoAvailable()) {
            emit text_for_mainwindow("Undo!", "text_from_editor");
            undo();
        } else {
            emit text_for_mainwindow("No \"Undo\" available.", "text_from_editor");
        }
    }  else {
        emit text_for_mainwindow("", "set_read_only_message");
        return;
    }
}

//==========================================================================================
// C-x r .................. This command "redo" changes.
//==========================================================================================

void MyTextEdit::redo_command()
{
    if (!isReadOnly()) {
        if (document()->isRedoAvailable()) {
            emit text_for_mainwindow("Redo!", "text_from_editor");
            redo();
        } else {
            emit text_for_mainwindow("No \"Redo\" available.", "text_from_editor");
        }
    }  else {
        emit text_for_mainwindow("", "set_read_only_message");
        return;
    }
}

//==========================================================================================
// 7.7 Blank Lines
//==========================================================================================

//==========================================================================================
// C-o ..................... Insert a blank line after the cursor.
//==========================================================================================

void MyTextEdit::open_line()
{
    if (!isReadOnly()) {
        QTextCursor cursor = textCursor();

        if (cursor.hasSelection() && is_mark_activated) {
            cursor.clearSelection();
            is_mark_activated = false;
        }

        int start_position = cursor.position();

        cursor.movePosition(QTextCursor::EndOfLine);
        cursor.insertText("\n");
        cursor.setPosition(start_position);

        setTextCursor(cursor);
    } else {
        emit text_for_mainwindow("", "set_read_only_message");
        return;
    }
}

//==========================================================================================
//
//
//                               |Mark| 11. The Mark and the Region
//
//
//==========================================================================================

//==========================================================================================
// 11.1 Setting the Mark
//==========================================================================================

//==========================================================================================
// C-<SPC> ................Set the mark at point, and activate it.
// C-@ ................. The same.
//==========================================================================================

void MyTextEdit::set_mark_command()
{
    QTextCursor cursor = textCursor();

    if (!is_mark_active) {

        // If this is the first time we set the mark in the current buffer, we enter here.
        // Every buffers set the variable "is_mark_active" to true when it is created.

        if (cursor.hasSelection())
            cursor.clearSelection();

        position_mark_set = cursor.position();

        setTextCursor(cursor);

        is_mark_active = true;
        is_mark_activated = true;

        emit mark_text(sQ::Set);

    } else {

        if (is_mark_activated) {

            // If this is NOT the first time we set the mark in the current buffer, we enter here.

            if (cursor.hasSelection()) {

                // If there is selection, clear the selection, and set the new position,
                // where the mark was activated again.

                cursor.clearSelection();
                position_mark_set = cursor.position();
                setTextCursor(cursor);

                emit mark_text(sQ::Set);

            } else {

                // Otherwise, deactivate the mark and remember this position.

                int position = cursor.position();
                marked_positions << position;
                is_mark_activated = false;

                emit mark_text(sQ::Deactivated);
            }

        } else {

            // If the "is_makr_activated" is false, then check whether we have mark set here previously.

            int position = cursor.position();

            if (position_mark_set == position) {

                // If the position_mark_set == position then we have a mark here which is deactivated, then
                // activated it again.

                is_mark_activated = true;
                cursor.clearSelection();
                position_mark_set = cursor.position();
                setTextCursor(cursor);

                emit mark_text(sQ::Activated);

            } else {

                // Otherwise, set new mark and activated it.

                if (cursor.hasSelection())
                    cursor.clearSelection();

                position_mark_set = cursor.position();

                setTextCursor(cursor);

                is_mark_activated = true;

                emit mark_text(sQ::Set);
            }
        }
    }
}

//==========================================================================================
// C-x C-x ................... Set the mark at point, and activate it; then move point where
// the mark used to be.
//==========================================================================================

void MyTextEdit::exchange_point_and_mark()
{
    QTextCursor cursor = textCursor();

    if (!is_mark_activated) {
        is_mark_activated = true;
        position_mark_set = cursor.position();
    } else {
        int old_position = cursor.position();
        int new_position = position_mark_set;

        position_mark_set = old_position;
        cursor.clearSelection();
        cursor.setPosition(new_position, QTextCursor::KeepAnchor);

        setTextCursor(cursor);
    }
}

//==========================================================================================
// 11.2 Commands to Mark Textual Objects
//==========================================================================================

//==========================================================================================
// M-@ .................. Set mark after end of next word (mark-word). This does not move point.
//==========================================================================================

void MyTextEdit::mark_word()
{
    QString text = toPlainText();
    if (text.isEmpty())
        return;
    else {

        if (is_mark_activated)
            is_mark_activated = false;

        QTextCursor cursor = textCursor();
        int start_position = cursor.position();

        /* Firsr, we check whether in the text there is a character. This is necessary because
         * if the text is not empty and there is no character in it then we should select until
         * the next tab or space. But if we have character we should select everything to this
         * character. */

        bool contains_character = false;

        for (int i = 0; i != text.size(); ++i) {
            if (!text.at(i).isSpace()) {
                contains_character = true;
                break;
            }
        }

        if (contains_character) {

            cursor.select(QTextCursor::WordUnderCursor);
            QString word = cursor.selectedText();
            if (cursor.hasSelection())
                cursor.clearSelection();

            while (word.isEmpty() && word != "\t" && word != " " && word != "\n") {
                cursor.movePosition(QTextCursor::WordRight);
                cursor.select(QTextCursor::WordUnderCursor);
                word = cursor.selectedText();
            }
        } else {

            if (cursor.hasSelection())
                cursor.clearSelection();

            cursor.movePosition(QTextCursor::WordRight);
        }

        cursor.movePosition(QTextCursor::EndOfWord);

        is_mark_activated = true;
        position_mark_set = cursor.position();

        cursor.setPosition(start_position, QTextCursor::KeepAnchor);
        setTextCursor(cursor);

        emit mark_text(sQ::Set);
    }
}

//==========================================================================================
// M-h ....................... Move point to the beginning of the current paragraph,
// and set mark at the end (mark-paragraph).
//==========================================================================================

void MyTextEdit::mark_paragraph()
{
    qDebug() << "As well";
}

//==========================================================================================
// C-x C-p ...................Move point to the beginning of the current page, and set mark
// at the end (mark-page).
//==========================================================================================

void MyTextEdit::mark_page()
{
    qDebug() << "Works";
}

//==========================================================================================
// C-x h ................... Move point to the beginning of the buffer, and set mark at the end.
//==========================================================================================

void MyTextEdit::mark_whole_buffer()
{
    QString text = toPlainText();
    if (text.isEmpty())
        return;
    else {

        if (is_mark_activated)
            is_mark_activated = false;

        QTextCursor cursor = textCursor();

        cursor.movePosition(QTextCursor::End);

        is_mark_activated = true;
        position_mark_set = cursor.position();

        cursor.movePosition(QTextCursor::Start, QTextCursor::KeepAnchor);
        setTextCursor(cursor);

        emit mark_text(sQ::Set);
    }
}

//==========================================================================================
// 11.3 Operating on the Region
//==========================================================================================

// Nothing to be done here

//==========================================================================================
// 11.4 The Mark Ring
//==========================================================================================

//==========================================================================================
// C-u C-<SPC> .................. Move point to where the mark was, and restore the mark from
// the ring of former marks.
//==========================================================================================

void MyTextEdit::move_to_remembered_position()
{
    QTextCursor cursor = textCursor();

    if (is_mark_activated)
        is_mark_activated = false;

    if (!marked_positions.isEmpty()) {
        int position = marked_positions.last();
        if (position < (toPlainText().size() - 1)) {
            cursor.setPosition(position);
            setTextCursor(cursor);
            int last_position = position;
            marked_positions.removeLast();
            marked_positions.prepend(last_position);
        } else
            return;
    }
}

//==========================================================================================
//
//
//                             |Killing| 12. Killing and Moving Text
//
//
//==========================================================================================

//==========================================================================================
// 12.1 Deletion and Killing
//==========================================================================================

//==========================================================================================
// <Backspace> ..................... Delete the previous character.
//==========================================================================================

void MyTextEdit::delete_backward_char()
{
    if (!isReadOnly()) {

        QTextCursor cursor = textCursor();

        if (cursor.hasSelection()) {
            cursor.removeSelectedText();
        } else {
            cursor.movePosition(QTextCursor::Left);
            cursor.deleteChar();
        }

        is_mark_activated = false;
        setTextCursor(cursor);

    } else {
        emit text_for_mainwindow("", "set_read_only_message");
        return;
    }
}

//==========================================================================================
// <DEL> ................. Delete the current character (delete-char).
//==========================================================================================

void MyTextEdit::delete_current_char()
{
    if (!isReadOnly()) {

        QTextCursor cursor = textCursor();

        if (cursor.hasSelection())
            cursor.removeSelectedText();
        else
            cursor.deleteChar();

        is_mark_activated = false;
        setTextCursor(cursor);

    } else {
        emit text_for_mainwindow("", "set_read_only_message");
        return;
    }
}

//==========================================================================================
// C-d ................... Delete the next character.
//==========================================================================================

void MyTextEdit::delete_forward_char()
{
    if (!isReadOnly()) {

        QTextCursor cursor = textCursor();

        if (cursor.hasSelection())
            cursor.clearSelection();

        cursor.movePosition(QTextCursor::Right);
        cursor.deleteChar();

        is_mark_activated = false;
        setTextCursor(cursor);

    } else {
        emit text_for_mainwindow("", "set_read_only_message");
        return;
    }
}

//==========================================================================================
// M-\ ..................... Delete spaces and tabs around point.
//==========================================================================================

void MyTextEdit::delete_horizontal_space()
{
    if (!isReadOnly()) {
        // How many spaces and tabs we should remove from the line
        int count = 0;

        // We take the cursor and its position in text
        QTextCursor cursor = textCursor();
        int char_position = cursor.position();
        int temp_pos = char_position;

        //=========== This part deletes all spaces right from the current position =============

        // This is the text on which we will work
        QString current_text = toPlainText();

        for (int i = char_position; i != current_text.size(); ++i) {
            if (current_text.at(i) == ' ' || current_text.at(i) == '\t')
                count++;
            else
                break;
        }

        // If we have more than 0 spaces and tabs, delete them
        if (count) {
            for (int i = 0; i != count; ++i)
                cursor.deleteChar();

            // Update the new position of the cursor
            setTextCursor(cursor);
        }

        //=========== This part deletes all spaces left from the current position =============

        count = 0;

        for (int i = temp_pos; i != 0; --i) {
            if (current_text.at(i-1) == QChar(' ') || current_text.at(i) == QChar('\t'))
                count++;
            else
                break;
        }

        // If we have more than 0 spaces and tabs, delete them
        if (count) {
            cursor.setPosition(temp_pos);
            for (int i = 0; i != count; ++i)
                cursor.deletePreviousChar();

            // Update the new position of the cursor
            setTextCursor(cursor);
        }
    } else {
        emit text_for_mainwindow("", "set_read_only_message");
        return;
    }
}

//==========================================================================================
// M-<SPC> ................ Delete spaces and tabs around point, leaving one space.
//==========================================================================================

void MyTextEdit::delete_horizontal_space_leave_one_space()
{
    if (!isReadOnly()) {
        /* Here we use exactly the same implementation as the 'M-/' one
     * with the exception of that we insert one space. */


        // How many spaces and tabs we should remove from the line
        int count = 0;

        // We take the cursor and its position in text
        QTextCursor cursor = textCursor();
        int char_position = cursor.position();
        int temp_pos = char_position;

        //=========== This part deletes all spaces right from the current position =============

        // This is the text on which we will work
        QString current_text = toPlainText();

        for (int i = char_position; i != current_text.size(); ++i) {
            if (current_text.at(i) == ' ' || current_text.at(i) == '\t')
                count++;
            else
                break;
        }

        // If we have more than 0 spaces and tabs, delete them
        if (count) {
            for (int i = 0; i != count; ++i)
                cursor.deleteChar();

            // Update the new position of the cursor
            setTextCursor(cursor);
        }

        //=========== This part deletes all spaces left from the current position =============

        count = 0;

        for (int i = temp_pos; i != 0; --i) {
            if (current_text.at(i-1) == QChar(' ') || current_text.at(i) == QChar('\t'))
                count++;
            else
                break;
        }

        // If we have more than 0 spaces and tabs, delete them
        if (count) {
            cursor.setPosition(temp_pos);
            for (int i = 0; i != count; ++i)
                cursor.deletePreviousChar();

            // Update the new position of the cursor
            cursor.insertText(" ");
            setTextCursor(cursor);
        }
    } else {
        emit text_for_mainwindow("", "set_read_only_message");
        return;
    }
}

//==========================================================================================
// C-x C-o ................. Delete blank lines around the current line.
//==========================================================================================

void MyTextEdit::delete_blank_lines()
{
    if (!isReadOnly()) {

        bool found_not_empty_line;
        QTextCursor cursor = textCursor();
        int start_position = cursor.position();

        QString current_line = cursor.block().text();

        if (current_line.isEmpty()) {

            // We start searching for a line with text, bottom from
            // the current line.

            found_not_empty_line = false;

            while (!found_not_empty_line && !cursor.atEnd()) {
                cursor.movePosition(QTextCursor::NextBlock);
                current_line = cursor.block().text();
                if (!current_line.isEmpty())
                    found_not_empty_line = true;
            }

            cursor.movePosition(QTextCursor::StartOfLine);
            int end_selection = cursor.position();


            // We start searching for a line with text, above from
            // the start position line.

            found_not_empty_line = false;
            cursor.setPosition(start_position);

            while (!found_not_empty_line && !cursor.atStart()) {
                cursor.movePosition(QTextCursor::PreviousBlock);
                current_line = cursor.block().text();
                if (!current_line.isEmpty())
                    found_not_empty_line = true;
            }

            if (!found_not_empty_line && cursor.atStart()) {
                cursor.setPosition(start_position);
                return;
            }

            cursor.movePosition(QTextCursor::EndOfLine);
            cursor.setPosition(end_selection, QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
            cursor.insertText("\n\n");
            cursor.movePosition(QTextCursor::Up);
            setTextCursor(cursor);

        } else {

            found_not_empty_line = false;
            cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::MoveAnchor);

            while (!found_not_empty_line && !cursor.atEnd()) {
                cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
                current_line = cursor.block().text();
                if (!current_line.isEmpty())
                    found_not_empty_line = true;
            }

            cursor.removeSelectedText();
            cursor.insertText("\n");
            cursor.setPosition(start_position);
            setTextCursor(cursor);
        }
    } else {
        emit text_for_mainwindow("", "set_read_only_message");
        return;
    }
}

//==========================================================================================
// M-^ ....................... Join two lines by deleting the intervening newline, along with
//                                                              any indentation following it.
//==========================================================================================

void MyTextEdit::delete_indentation()
{
    if (!isReadOnly()) {

        QTextCursor cursor = textCursor();
        QString current_line = cursor.block().text();
        cursor.movePosition(QTextCursor::StartOfLine);

        if (current_line.isEmpty()) {
            cursor.deletePreviousChar();
            setTextCursor(cursor);
        } else {
            for (int i = 0; i != current_line.size(); ++i) {
                if (current_line.at(i) == QChar(' ') || current_line.at(i) == QChar('\t'))
                    cursor.deleteChar();
                else
                    break;
            }

            // With these tree lines of code we check whether the line above the current
            // is empty or not and after that returning the cursor to the previous position
            cursor.movePosition(QTextCursor::Up);
            current_line = cursor.block().text();
            cursor.movePosition(QTextCursor::Down);

            // With this line we move to the line above the current and placing the cursor at the end of the line
            cursor.deletePreviousChar();

            if (!current_line.isEmpty()) {
                for (int i = current_line.size() - 1; i != 0; --i) {
                    if (current_line.at(i) == QChar(' ') || current_line.at(i) == QChar('\t'))
                        cursor.deletePreviousChar();
                    else
                        break;
                }
                cursor.insertText(" ");
                setTextCursor(cursor);

            } else {
                cursor.movePosition(QTextCursor::StartOfLine);
                setTextCursor(cursor);
            }
        }
    } else {
        emit text_for_mainwindow("", "set_read_only_message");
        return;
    }
}

//==========================================================================================
// C-k ................... Kill rest of line or one or more lines.
//==========================================================================================

void MyTextEdit::kill_line()
{
    if (!isReadOnly()) {
        QTextCursor cursor = textCursor();
        QString current_line = cursor.block().text();

        if (cursor.atEnd()) {
            emit text_for_mainwindow("", "set_end_of_buffer_text");
        } else if (current_line.isEmpty()) {
            cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor);
            cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
            cursor.movePosition(QTextCursor::StartOfLine);
            setTextCursor(cursor);
            return;
        } else {
            bool contains_any_characters = false;

            for (int i = 0; i != current_line.size(); ++i) {
                if (current_line.at(i) != QChar(' ') && current_line.at(i) != QChar('\t')) {
                    contains_any_characters = true;
                    break;
                }
            }

            if (contains_any_characters) {
                // We take the current text from the document and started to delete
                // every character from the cursor's position to the end of the current line
                QString text = toPlainText();
                for (int i = cursor.position(); i != text.size(); ++i) {
                    if (text.at(i) == '\n')
                        return;
                    cursor.deleteChar();
                }
            } else {
                cursor.movePosition(QTextCursor::StartOfLine);
                cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor);
                cursor.removeSelectedText();
                cursor.movePosition(QTextCursor::StartOfLine);
                setTextCursor(cursor);
            }
        }
    } else {
        emit text_for_mainwindow("", "set_read_only_message");
        return;
    }
}

//==========================================================================================
// C-S-backspace .................. Kill an entire line at once.
//==========================================================================================

void MyTextEdit::kill_whole_line()
{
    if (!isReadOnly()) {

        QTextCursor cursor = textCursor();
        int start_position = cursor.position();
        QString line_for_deletion = cursor.block().text();

        if (cursor.atEnd() && line_for_deletion.isEmpty()) {
            emit text_for_mainwindow("", "set_end_of_buffer_text");
        } else {
            cursor.movePosition(QTextCursor::Down);
            QString current_line = cursor.block().text();

            if (current_line == line_for_deletion) {
                cursor.movePosition(QTextCursor::StartOfLine);
                for (int i = 0; i != line_for_deletion.size(); ++i) {
                    cursor.deleteChar();
                }
                setTextCursor(cursor);
            } else {
                cursor.setPosition(start_position);
                cursor.movePosition(QTextCursor::StartOfLine);
                cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor);
                cursor.removeSelectedText();
                cursor.movePosition(QTextCursor::StartOfLine);
                setTextCursor(cursor);
            }
        }
    } else {
        emit text_for_mainwindow("", "set_read_only_message");
        return;
    }
}

//==========================================================================================
// C-w ................. Kill the region.
//==========================================================================================

void MyTextEdit::kill_region()
{
    if (!isReadOnly()) {

        if (is_mark_activated)
            cut();

        is_mark_activated = false;
    } else {
        emit text_for_mainwindow("", "set_read_only_message");
        return;
    }
}

//==========================================================================================
// M-w ................. Copy the region into the kill ring (kill-ring-save).
//==========================================================================================

void MyTextEdit::kill_ring_save()
{
    if (is_mark_activated)
        copy();

    is_mark_activated = false;
    QTextCursor cursor = textCursor();
    cursor.clearSelection();
    setTextCursor(cursor);
}

//==========================================================================================
// M-<backspace> ............... Kill one word backwards.
//==========================================================================================

void MyTextEdit::backward_kill_word()
{
    if (!isReadOnly()) {

        QTextCursor cursor = textCursor();
        cursor.clearSelection();

        cursor.movePosition(QTextCursor::StartOfWord);
        cursor.movePosition(QTextCursor::PreviousWord);
        cursor.select(QTextCursor::WordUnderCursor);
        cursor.removeSelectedText();

        setTextCursor(cursor);
    } else {
        emit text_for_mainwindow("", "set_read_only_message");
        return;
    }
}

//==========================================================================================
// M-d .................. Kill the next word.
//==========================================================================================

void MyTextEdit::forward_kill_word()
{
    if (!isReadOnly()) {

        QTextCursor cursor = textCursor();
        cursor.clearSelection();

        cursor.movePosition(QTextCursor::NextWord);
        cursor.select(QTextCursor::WordUnderCursor);
        cursor.removeSelectedText();

        setTextCursor(cursor);
    } else {
        emit text_for_mainwindow("", "set_read_only_message");
        return;
    }
}

//==========================================================================================
// M-<DEL> ................... Kill the current word.
//==========================================================================================

void MyTextEdit::kill_word()
{
    if (!isReadOnly()) {

        QTextCursor cursor = textCursor();
        cursor.clearSelection();

        cursor.select(QTextCursor::WordUnderCursor);
        cursor.removeSelectedText();
        cursor.movePosition(QTextCursor::NextWord);

        setTextCursor(cursor);
    } else {
        emit text_for_mainwindow("", "set_read_only_message");
        return;
    }
}

//==========================================================================================
// C-x <DEL> .................... Kill back to beginning of sentence.
//==========================================================================================

void MyTextEdit::backward_kill_sentence()
{
    if (!isReadOnly()) {

        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        setTextCursor(cursor);
    } else {
        emit text_for_mainwindow("", "set_read_only_message");
        return;
    }
}

//==========================================================================================
// M-z char ................. Kill through the next occurrence of char.
//==========================================================================================

void MyTextEdit::zap_to_char(const QChar &zap_character)
{
    is_waiting_for_input = false;

    if (!isReadOnly()) {

        QTextCursor cursor = textCursor();
        int start_position = cursor.position();
        cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);

        // Here we take the text which starts from cursor's current position
        // to the zap_character(the character until which we will delete text)
        QString text = cursor.selectedText();
        cursor.clearSelection();

        if (!text.contains(zap_character, Qt::CaseSensitive))
            return;

        // Here we take the whole text from the document and in this way
        // we can use the start_position for iteration
        text = toPlainText();
        cursor.setPosition(start_position);

        for (int i = start_position; i != text.size(); ++i) {
            if (text.at(i) != zap_character)
                cursor.deleteChar();
            else {
                cursor.deleteChar();
                break;
            }
        }

        emit zap_to_char_finish_signal();

    } else {
        emit zap_to_char_finish_signal();
        emit text_for_mainwindow("", "set_read_only_message");
        return;
    }
}

//==========================================================================================
// 12.2 Yanking
//==========================================================================================

//==========================================================================================
// C-y ................. Yank the last kill into the buffer, at point.
//==========================================================================================

void MyTextEdit::yank()
{
    if (!isReadOnly())
        paste();
    else {
        QString text = "Buffer is read-only";
        emit text_for_mainwindow(text, "set_read_only_message");
        return;
    }
}

//==========================================================================================
// 12.3 “Cut and Paste” Operations on Graphical Displays
//==========================================================================================

// Nothing to do here

//==========================================================================================
// 12.4 Accumulating Text
//==========================================================================================

//===========================================================================================

// 13. |Registers| ............. [Note Done]

// 14. |Display| ............... [Note Done]

//==========================================================================================
//
//
//                              |Search| 15. Searching and Replacement
//
//
//==========================================================================================

//==========================================================================================
// 15.1 Incremental Search
//==========================================================================================

//==========================================================================================
// M-x search-forward .................... Incremental search forward.
//==========================================================================================

void MyTextEdit::search_forward()
{
    if (!is_first_time) {

        QTextDocument *current_document = document();
        QTextCursor cursor = textCursor();
        QTextCursor old_cursor = cursor;

        cursor = current_document->find(string_for_searching, cursor);

        int start_pos = cursor.selectionStart();

        if (!cursor.isNull() && found_results_count > 1) {
            is_mark_activated = true;
            position_mark_set = start_pos;
            setTextCursor(cursor);
        } else {
            QString tool_tip = "No more results. Type C-s to start a new search.";
            emit text_for_mainwindow(tool_tip, "text_from_editor");

            is_first_time = true;
            is_mark_activated = false;
            position_mark_set = 0;

            string_for_searching = "";

            old_cursor.clearSelection();
            old_cursor.movePosition(QTextCursor::StartOfWord);
            setTextCursor(old_cursor);

            return;
        }
    } else
        emit text_for_mainwindow("", "set_search_forward_mode");
}

void MyTextEdit::search_forward_first_time()
{
    last_was_regular_search = true;

    if (last_was_regexp_search)
        last_was_regexp_search = false;

    QTextDocument *current_document = document();
    QTextCursor cursor = textCursor();

    bool found = false;

    QTextCursor highlight_cursor(current_document);

    while (!highlight_cursor.isNull() && !highlight_cursor.atEnd()) {
        highlight_cursor = current_document->find(string_for_searching, highlight_cursor);
        if (!highlight_cursor.isNull()) {
            ++found_results_count;
            found = true;
        }
    }

    if (found == false) {
        QString result = "Search failed: \"" + string_for_searching + "\"";
        emit text_for_mainwindow(result, "text_from_editor");
    } else {

        cursor = current_document->find(string_for_searching, cursor);

        if (!cursor.isNull()) {
            is_first_time = false;
            is_mark_activated = true;
            position_mark_set = cursor.selectionStart();
            setTextCursor(cursor);
        }
    }
}

//==========================================================================================
// M-x search-backward .................... Incremental search backward.
//==========================================================================================

void MyTextEdit::search_backward()
{
    if (!is_first_time) {

        QTextDocument *current_document = document();
        QTextCursor cursor = textCursor();
        QTextCursor old_cursor = cursor;

        cursor = current_document->find(string_for_searching, cursor, QTextDocument::FindBackward);

        int start_pos = cursor.selectionStart();

        if (!cursor.isNull() && found_results_count > 1) {
            is_mark_activated = true;
            position_mark_set = start_pos;
            setTextCursor(cursor);
        } else {
            QString tool_tip = "No more results. Type C-r to start a new search.";
            emit text_for_mainwindow(tool_tip, "text_from_editor");

            is_first_time = true;
            is_mark_activated = false;
            position_mark_set = 0;

            string_for_searching = "";

            old_cursor.clearSelection();
            old_cursor.movePosition(QTextCursor::StartOfWord);
            setTextCursor(old_cursor);

            return;
        }
    } else
        emit text_for_mainwindow("", "set_search_backward_mode");
}

void MyTextEdit::search_backward_first_time()
{
    last_was_regular_search = true;

    if (last_was_regexp_search)
        last_was_regexp_search = false;

    QTextDocument *current_document = document();
    QTextCursor cursor = textCursor();

    bool found = false;

    QTextCursor highlight_cursor(current_document);

    while (!highlight_cursor.isNull() && !highlight_cursor.atEnd()) {
        highlight_cursor = current_document->find(string_for_searching, highlight_cursor);
        if (!highlight_cursor.isNull()) {
            ++found_results_count;
            found = true;
        }
    }

    if (found == false) {
        QString result = "Search failed: \"" + string_for_searching + "\"";
        emit text_for_mainwindow(result, "text_from_editor");
    } else {

        cursor = current_document->find(string_for_searching, cursor, QTextDocument::FindBackward);

        if (!cursor.isNull()) {
            is_first_time = false;
            is_mark_activated = true;
            position_mark_set = cursor.selectionStart();
            setTextCursor(cursor);
        }
    }
}

//==========================================================================================
// C-M-s .................... Begin incremental regexp search.
//==========================================================================================

void MyTextEdit::search_forward_regexp()
{
    if (!is_first_time_regexp) {

        QTextDocument *current_document = document();
        QTextCursor cursor = textCursor();
        QTextCursor old_cursor = cursor;

        QRegExp rx(string_for_searching);

        cursor = current_document->find(rx, cursor);

        int start_pos = cursor.selectionStart();

        if (!cursor.isNull()) {
            is_mark_activated = true;
            position_mark_set = start_pos;
            setTextCursor(cursor);
        } else {
            QString tool_tip = "No more results. Type C-M-s to start a new search.";
            emit text_for_mainwindow(tool_tip, "text_from_editor");

            is_first_time_regexp = true;
            is_mark_activated = false;
            position_mark_set = 0;

            old_cursor.clearSelection();
            old_cursor.movePosition(QTextCursor::StartOfWord);
            setTextCursor(old_cursor);

            return;
        }
    } else
        emit text_for_mainwindow("", "set_search_forward_regexp_mode");
}

void MyTextEdit::search_forward_regexp_first_time()
{
    last_was_regexp_search = true;

    if (last_was_regular_search)
        last_was_regular_search = false;

    QTextDocument *current_document = document();
    QTextCursor cursor = textCursor();

    QRegExp rx(string_for_searching);

    bool found = false;

    QTextCursor highlight_cursor(current_document);

    while (!highlight_cursor.isNull() && !highlight_cursor.atEnd()) {
        highlight_cursor = current_document->find(rx, highlight_cursor);
        if (!highlight_cursor.isNull()) {
            ++found_results_count;
            found = true;
        }
    }

    if (found == false) {
        QString result = "Search failed: \"" + string_for_searching + "\"";
        emit text_for_mainwindow(result, "text_from_editor");
    } else {

        cursor = current_document->find(rx, cursor);

        if (!cursor.isNull()) {
            is_first_time_regexp = false;
            is_mark_activated = true;
            position_mark_set = cursor.selectionStart();
            setTextCursor(cursor);
        }
    }
}

//==========================================================================================
// C-M-r .................. Begin reverse incremental regexp search
//==========================================================================================

void MyTextEdit::search_backward_regexp()
{
    if (!is_first_time_regexp) {

        QTextDocument *current_document = document();
        QTextCursor cursor = textCursor();
        QTextCursor old_cursor = cursor;

        QRegExp rx(string_for_searching);

        cursor = current_document->find(rx, cursor, QTextDocument::FindBackward);

        int start_pos = cursor.selectionStart();

        if (!cursor.isNull()) {
            is_mark_activated = true;
            position_mark_set = start_pos;
            setTextCursor(cursor);
        } else {
            QString tool_tip = "No more results. Type C-M-r to start a new search.";
            emit text_for_mainwindow(tool_tip, "text_from_editor");

            is_first_time_regexp = true;
            is_mark_activated = false;
            position_mark_set = 0;

            old_cursor.clearSelection();
            old_cursor.movePosition(QTextCursor::StartOfWord);
            setTextCursor(old_cursor);

            return;
        }
    } else
        emit text_for_mainwindow("", "set_search_backward_regexp_mode");
}

void MyTextEdit::search_backward_regexp_first_time()
{
    last_was_regexp_search = true;

    if (last_was_regular_search)
        last_was_regular_search = false;

    QTextDocument *current_document = document();
    QTextCursor cursor = textCursor();

    QRegExp rx(string_for_searching);

    bool found = false;

    QTextCursor highlight_cursor(current_document);

    while (!highlight_cursor.isNull() && !highlight_cursor.atEnd()) {
        highlight_cursor = current_document->find(rx, highlight_cursor);
        if (!highlight_cursor.isNull()) {
            ++found_results_count;
            found = true;
        }
    }

    if (found == false) {
        QString result = "Search failed: \"" + string_for_searching + "\"";
        emit text_for_mainwindow(result, "text_from_editor");
    } else {

        cursor = current_document->find(rx, cursor, QTextDocument::FindBackward);

        if (!cursor.isNull()) {
            is_first_time_regexp = false;
            is_mark_activated = true;
            position_mark_set = cursor.selectionStart();
            setTextCursor(cursor);
        }
    }
}

//==========================================================================================
// M-s ....................... Show all the results that the search has founded.
//==========================================================================================

void MyTextEdit::show_founded_results()
{
    QTextDocument *current_document = document();

    if (string_for_searching.isEmpty() || (found_results_count <= 1))
        return;

    bool found = false;

    QTextCursor highlight_cursor(current_document);
    QTextCursor cursor(current_document);

    cursor.beginEditBlock();

    QTextCharFormat plainFormat(highlight_cursor.charFormat());
    QTextCharFormat colorFormat = plainFormat;

    QColor highlight_cursor_color;
    highlight_cursor_color.setNamedColor("#B4EEB4");
    colorFormat.setBackground(highlight_cursor_color);

    if (last_was_regular_search) {
        while (!highlight_cursor.isNull() && !highlight_cursor.atEnd()) {
            highlight_cursor = current_document->find(string_for_searching, highlight_cursor);
            if (!highlight_cursor.isNull()) {
                ++found_results_count;
                found = true;
                highlight_cursor.mergeCharFormat(colorFormat);
            }
        }
    } else if (last_was_regexp_search) {
        QRegExp rx(string_for_searching);
        while (!highlight_cursor.isNull() && !highlight_cursor.atEnd()) {
            highlight_cursor = current_document->find(rx, highlight_cursor);
            if (!highlight_cursor.isNull()) {
                ++found_results_count;
                found = true;
                highlight_cursor.mergeCharFormat(colorFormat);
            }
        }
    } else {
        return;
    }

    cursor.endEditBlock();

    if (found == false) {
        QString result = "There aren't any results to show.";
        emit text_for_mainwindow(result, "text_from_editor");
        return;
    } else
        are_found_results_visible = true;
}


//==========================================================================================
//
//
//                             16. |Fixit|
//
//
//==========================================================================================

//==========================================================================================
// 16.2 Transposing Text
//==========================================================================================

//==========================================================================================
// C-t .................. Transpose two characters.
//==========================================================================================

void MyTextEdit::transpose_chars()
{
    QTextCursor cursor = textCursor();
    int current_position = cursor.position();
    QString text = toPlainText();

    if (!text.isEmpty() && !isReadOnly()) {

        QString current_line = cursor.block().text();

        if (!current_line.isEmpty()) {
            // steps is a variable which count how many times we should move the cursor to the
            // first character.

            int steps = 0;
            for(int i = current_position; text.at(i) != QChar('\n'); ++i) {
                if (text.at(i) == QChar(' ') || text.at(i) == QChar('\t'))
                    ++steps;
                else
                    break;
            }

            if (steps != 0)
                cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, steps);

            QChar current_char;
            if (!cursor.atEnd() && !cursor.atStart()) {
                current_position = cursor.position();
                if(current_position < text.size())
                    current_char = text.at(current_position);
            } else if (cursor.atStart()) {
                emit text_for_mainwindow("", "set_start_of_buffer_text");
                return;
            } else {
                emit text_for_mainwindow("", "set_end_of_buffer_text");
                return;
            }

            QChar previous_char;
            if (!cursor.atEnd() && !cursor.atStart()) {
                current_position = cursor.position() - 1;
                if(current_position < text.size())
                    previous_char = text.at(current_position);
            } else if (cursor.atStart()) {
                emit text_for_mainwindow("", "set_start_of_buffer_text");
                return;
            } else {
                emit text_for_mainwindow("", "set_end_of_buffer_text");
                return;
            }

            cursor.deleteChar();
            cursor.insertText(previous_char);
            cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 2);
            cursor.deleteChar();
            cursor.insertText(current_char);

            setTextCursor(cursor);

        } else
            return;
    } else {
        emit text_for_mainwindow("", "set_read_only_message");
        return;
    }
}

//==========================================================================================
// M-t .................. Transpose two words.
//==========================================================================================

void MyTextEdit::transpose_words()
{
    if (!isReadOnly()) {

        QTextCursor cursor = textCursor();
        int current_position = cursor.position();
        QString text = toPlainText();

        if (!text.isEmpty()) {

            QString current_line = cursor.block().text();

            if (!current_line.isEmpty()) {
                // steps is a variable which count how many times we should move the cursor to the
                // first character.

                int steps = 0;
                for(int i = current_position; text.at(i) != QChar('\n'); ++i) {
                    if (text.at(i) == QChar(' ') || text.at(i) == QChar('\t'))
                        ++steps;
                    else
                        break;
                }

                if (steps != 0)
                    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, steps);

                int current_word_position = cursor.position();

                cursor.select(QTextCursor::WordUnderCursor);
                QString current_word = cursor.selectedText();

                cursor.clearSelection();

                bool is_move = cursor.movePosition(QTextCursor::NextWord);
                cursor.select(QTextCursor::WordUnderCursor);
                QString next_word = cursor.selectedText();

                if (next_word.isEmpty() || next_word == " " || next_word == "\t" || !is_move)
                    emit text_for_mainwindow("", "set_fixit_text");

                cursor.removeSelectedText();
                cursor.insertText(current_word);

                cursor.setPosition(current_word_position);
                cursor.select(QTextCursor::WordUnderCursor);
                cursor.removeSelectedText();
                cursor.insertText(next_word);

                cursor.movePosition(QTextCursor::NextWord);
                cursor.movePosition(QTextCursor::EndOfWord);
                cursor.movePosition(QTextCursor::Right);
                setTextCursor(cursor);

            } else {
               emit text_for_mainwindow("", "set_fixit_text");
               return;
            }
        } else {
            emit text_for_mainwindow("", "set_fixit_text");
            return;
        }
    } else {
        emit text_for_mainwindow("", "set_read_only_message");
        return;
    }
}
//==========================================================================================
// C-x C-t .................. Transpose two lines.
//==========================================================================================

void MyTextEdit::transpose_lines()
{
    if (!isReadOnly()) {

        QTextCursor cursor = textCursor();
        QString text = toPlainText();

        if (!text.isEmpty()) {

            QString current_line = cursor.block().text();

            bool is_move = cursor.movePosition(QTextCursor::Down);
            if (is_move) {
                QString next_line = cursor.block().text();

                cursor.select(QTextCursor::LineUnderCursor);
                cursor.removeSelectedText();
                cursor.insertText(current_line);

                cursor.movePosition(QTextCursor::Up);
                cursor.select(QTextCursor::LineUnderCursor);
                cursor.removeSelectedText();
                cursor.insertText(next_line);

                setTextCursor(cursor);
            } else {
                emit text_for_mainwindow("", "set_fixit_text");
                return;
            }
        } else {
            emit text_for_mainwindow("", "set_fixit_text");
            return;
        }
    } else {
        emit text_for_mainwindow("", "set_read_only_message");
        return;
    }
}

//==========================================================================================
//
//
//                              25. |Text|
//
//
//==========================================================================================

//==========================================================================================
// 25.13.4 Faces in Enriched Text - We will implement only these function from |Text|
//==========================================================================================

//==========================================================================================
// M-o d .................... Remove all face properties.
//==========================================================================================

void MyTextEdit::facemenu_set_default()
{
    if (!isReadOnly()) {

        QTextCursor cursor = textCursor();

        if (is_mark_activated && cursor.hasSelection()) {

            QTextCharFormat format(cursor.charFormat());
            format.setFontWeight(QFont::Normal);
            format.setFontUnderline(false);
            format.setFontItalic(false);
            format.setFontStrikeOut(false);
            cursor.mergeCharFormat(format);

            cursor.clearSelection();
            setTextCursor(cursor);

            is_mark_activated = false;
            mark_text(sQ::Deactivated);

        } else
            return;
    } else {
        emit text_for_mainwindow("", "set_read_only_message");
        return;
    }
}

//==========================================================================================
// M-o b .................... Apply the bold face.
//==========================================================================================

void MyTextEdit::facemenu_set_bold()
{
    if (!isReadOnly()) {

        QTextCursor cursor = textCursor();

        if (is_mark_activated && cursor.hasSelection() && (cursor.charFormat().fontWeight() != QFont::Bold)) {

            QTextCharFormat format(cursor.charFormat());
            format.setFontWeight(QFont::Bold);
            cursor.mergeCharFormat(format);

            cursor.clearSelection();
            setTextCursor(cursor);

            is_mark_activated = false;
            mark_text(sQ::Deactivated);

        } else
            return;
    } else {
        emit text_for_mainwindow("", "set_read_only_message");
        return;
    }
}

//==========================================================================================
// M-o i .................... Apply the italic face.
//==========================================================================================

void MyTextEdit::facemenu_set_italic()
{
    if (!isReadOnly()) {

        QTextCursor cursor = textCursor();

        if (is_mark_activated && cursor.hasSelection() && !cursor.charFormat().fontItalic()) {

            QTextCharFormat format(cursor.charFormat());
            format.setFontItalic(true);
            cursor.mergeCharFormat(format);

            cursor.clearSelection();
            setTextCursor(cursor);

            is_mark_activated = false;
            mark_text(sQ::Deactivated);

        } else
            return;
    } else {
        emit text_for_mainwindow("", "set_read_only_message");
        return;
    }
}

//==========================================================================================
// M-o l .................... Apply the bold-italic face.
//==========================================================================================

void MyTextEdit::facemenu_set_bold_italic()
{
    if (!isReadOnly()) {

        QTextCursor cursor = textCursor();

        if (is_mark_activated && cursor.hasSelection()) {

            QTextCharFormat format(cursor.charFormat());
            format.setFontWeight(QFont::Bold);
            format.setFontItalic(true);
            cursor.mergeCharFormat(format);

            cursor.clearSelection();
            setTextCursor(cursor);

            is_mark_activated = false;
            mark_text(sQ::Deactivated);

        } else
            return;
    } else {
        emit text_for_mainwindow("", "set_read_only_message");
        return;
    }
}

//==========================================================================================
// M-o u .................... Apply the underline face.
//==========================================================================================

void MyTextEdit::facemenu_set_underline()
{
    if (!isReadOnly()) {

        QTextCursor cursor = textCursor();

        if (is_mark_activated && cursor.hasSelection() && !cursor.charFormat().fontUnderline()) {

            QTextCharFormat format(cursor.charFormat());
            format.setFontUnderline(true);
            cursor.mergeCharFormat(format);

            cursor.clearSelection();
            setTextCursor(cursor);

            is_mark_activated = false;
            mark_text(sQ::Deactivated);

        } else
            return;
    } else {
        emit text_for_mainwindow("", "set_read_only_message");
        return;
    }
}

//==========================================================================================
// M-o s .................... Apply the strikeout face.
//==========================================================================================

void MyTextEdit::facemenu_set_strikeout()
{
    if (!isReadOnly()) {

        QTextCursor cursor = textCursor();

        if (is_mark_activated && cursor.hasSelection() && !cursor.charFormat().fontStrikeOut()) {

            QTextCharFormat format(cursor.charFormat());
            format.setFontStrikeOut(true);
            cursor.mergeCharFormat(format);

            cursor.clearSelection();
            setTextCursor(cursor);

            is_mark_activated = false;

        } else
            return;
    } else {
        emit text_for_mainwindow("", "set_read_only_message");
        return;
    }
}

//==========================================================================================
// M-x face ................... Apply the given face.
//==========================================================================================

void MyTextEdit::facemenu_set_face(const QString &given_text)
{
    if (!isReadOnly()) {

        QTextCursor cursor = textCursor();

        if (is_mark_activated && cursor.hasSelection()) {

            QColor foreground_color;
            QColor background_color;

            QString first_color;
            QString second_color;

            if (colors.isEmpty())
                insert_colors();

            QString color_combination = given_text;

            if (color_combination.contains("-")) {
                QStringList two_colors = color_combination.split("-");
                if (two_colors.count() == 2) {

                    first_color = two_colors.at(0);
                    second_color = two_colors.at(1);

                    if (colors.contains(first_color) && colors.contains(second_color)) {
                        first_color = colors.value(first_color);
                        second_color = colors.value(second_color);
                    }
                } else
                    return;
            } else
                return;

            QTextCharFormat format(cursor.charFormat());
            format.setFontWeight(QFont::Normal);
            format.setFontUnderline(false);
            format.setFontItalic(false);
            format.setFontStrikeOut(false);

            foreground_color.setNamedColor(first_color);
            format.setForeground(foreground_color);

            background_color.setNamedColor(second_color);
            format.setBackground(background_color);
            cursor.mergeCharFormat(format);

            cursor.clearSelection();
            setTextCursor(cursor);

            is_mark_activated = false;
            set_string("");

        } else
            return;
    } else {
        emit text_for_mainwindow("", "set_read_only_message");
        return;
    }
}

//==========================================================================================
// 25.6 Case Conversion Commands
//==========================================================================================

//==========================================================================================
// M-l .............. Convert following word to lower case.
//==========================================================================================

void MyTextEdit::downcase_word()
{
    if (!isReadOnly()) {

        QTextCursor cursor = textCursor();
        int current_position = cursor.position();
        QString text = toPlainText();

        if (!text.isEmpty()) {

            QString current_line = cursor.block().text();
            bool contains_any_characters = false;

            // Here we check whether the current line is empty or not.
            if (!current_line.isEmpty()) {
                for (int i = current_position; text.at(i) != QChar('\n'); ++i) {
                    if (text.at(i) != QChar(' ') && text.at(i) != QChar('\t')) {
                        contains_any_characters = true;
                        break;
                    }
                }
            }

            // If the current is empty, find a line which has any characters.
            while (!contains_any_characters && !cursor.atEnd()) {
                cursor.movePosition(QTextCursor::Down);
                cursor.movePosition(QTextCursor::StartOfLine);
                current_line = cursor.block().text();

                if (!current_line.isEmpty()) {
                    for (int i = 0; i != current_line.size(); ++i) {
                        if (current_line.at(i) != QChar(' ') && current_line.at(i) != QChar('\t')) {
                            contains_any_characters = true;
                            break;
                        }
                    }
                }
            }

            // We take the current position again because we need the steps from the current
            // position to the first character.
            current_position = cursor.position();

            // steps is a variable which count how many times we should move the cursor to the
            // first character.
            int steps = 0;
            for(int i = current_position; text.at(i) != QChar('\n'); ++i) {
                if (text.at(i) == QChar(' ') || text.at(i) == QChar('\t'))
                    ++steps;
                else
                    break;
            }

            if (steps != 0)
                cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, steps);

            // After that, we select the first word which we have found and
            // put all the characters to LowerCase
            cursor.select(QTextCursor::WordUnderCursor);

            QTextCharFormat format(cursor.charFormat());
            format.setFontCapitalization(QFont::AllLowercase);
            cursor.mergeCharFormat(format);

            cursor.clearSelection();
            setTextCursor(cursor);

        } else
            return;
    } else {
        emit text_for_mainwindow("", "set_read_only_message");
        return;
    }
}

//==========================================================================================
// M-u .............. Convert following word to upper case.
//==========================================================================================

void MyTextEdit::upcase_word()
{
    if (!isReadOnly()) {

        QTextCursor cursor = textCursor();
        int current_position = cursor.position();
        QString text = toPlainText();

        if (!text.isEmpty()) {

            QString current_line = cursor.block().text();
            bool contains_any_characters = false;

            // Here we check whether the current line is empty or not.
            if (!current_line.isEmpty()) {
                for (int i = current_position; text.at(i) != QChar('\n'); ++i) {
                    if (text.at(i) != QChar(' ') && text.at(i) != QChar('\t')) {
                        contains_any_characters = true;
                        break;
                    }
                }
            }

            // If the current is empty, find a line which has any characters.
            while (!contains_any_characters && !cursor.atEnd()) {
                cursor.movePosition(QTextCursor::Down);
                cursor.movePosition(QTextCursor::StartOfLine);
                current_line = cursor.block().text();

                if (!current_line.isEmpty()) {
                    for (int i = 0; i != current_line.size(); ++i) {
                        if (current_line.at(i) != QChar(' ') && current_line.at(i) != QChar('\t')) {
                            contains_any_characters = true;
                            break;
                        }
                    }
                }
            }

            // We take the current position again because we need the steps from the current
            // position to the first character.
            current_position = cursor.position();

            // steps is a variable which count how many times we should move the cursor to the
            // first character.
            int steps = 0;
            for(int i = current_position; text.at(i) != QChar('\n'); ++i) {
                if (text.at(i) == QChar(' ') || text.at(i) == QChar('\t'))
                    ++steps;
                else
                    break;
            }

            if (steps != 0)
                cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, steps);

            // After that, we select the first word which we have found and
            // put all the characters to UpperCase
            cursor.select(QTextCursor::WordUnderCursor);

            QTextCharFormat format(cursor.charFormat());
            format.setFontCapitalization(QFont::AllUppercase);
            cursor.mergeCharFormat(format);

            cursor.clearSelection();
            setTextCursor(cursor);

        } else
            return;
    } else {
        emit text_for_mainwindow("", "set_read_only_message");
        return;
    }
}

//==========================================================================================
// M-c .............. Capitalize the following word.
//==========================================================================================

void MyTextEdit::capitalize_word()
{
    if (!isReadOnly()) {

        QTextCursor cursor = textCursor();
        int current_position = cursor.position();
        QString text = toPlainText();

        if (!text.isEmpty()) {

            QString current_line = cursor.block().text();
            bool contains_any_characters = false;

            // Here we check whether the current line is empty or not.
            if (!current_line.isEmpty()) {
                for (int i = current_position; text.at(i) != QChar('\n'); ++i) {
                    if (text.at(i) != QChar(' ') && text.at(i) != QChar('\t')) {
                        contains_any_characters = true;
                        break;
                    }
                }
            }

            // If the current is empty, find a line which has any characters.
            while (!contains_any_characters && !cursor.atEnd()) {
                cursor.movePosition(QTextCursor::Down);
                cursor.movePosition(QTextCursor::StartOfLine);
                current_line = cursor.block().text();

                if (!current_line.isEmpty()) {
                    for (int i = 0; i != current_line.size(); ++i) {
                        if (current_line.at(i) != QChar(' ') && current_line.at(i) != QChar('\t')) {
                            contains_any_characters = true;
                            break;
                        }
                    }
                }
            }

            // We take the current position again because we need the steps from the current
            // position to the first character.
            current_position = cursor.position();

            // steps is a variable which count how many times we should move the cursor to the
            // first character.
            int steps = 0;
            for(int i = current_position; text.at(i) != QChar('\n'); ++i) {
                if (text.at(i) == QChar(' ') || text.at(i) == QChar('\t'))
                    ++steps;
                else
                    break;
            }

            if (steps != 0)
                cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, steps);

            // After that, we select the first word which we have found and
            // put all the characters to Capitalize
            cursor.select(QTextCursor::WordUnderCursor);

            QTextCharFormat format(cursor.charFormat());
            format.setFontCapitalization(QFont::Capitalize);
            cursor.mergeCharFormat(format);

            cursor.clearSelection();
            setTextCursor(cursor);

        } else
            return;
    } else {
        emit text_for_mainwindow("", "set_read_only_message");
        return;
    }
}

//==========================================================================================
// C-q .................. Quoted insert around the selected text.
//==========================================================================================

void MyTextEdit::quoted_insert()
{
    if (!isReadOnly()) {
        QTextCursor cursor = textCursor();

        if (!cursor.hasSelection())
            return;

        int start = cursor.selectionStart();
        int end = cursor.selectionEnd();

        if (cursor.hasSelection())
            cursor.clearSelection();

        cursor.setPosition(start);
        cursor.insertText("\"");

        cursor.setPosition(end+1);
        cursor.insertText("\"");

        setTextCursor(cursor);
    } else {
        emit text_for_mainwindow("", "set_read_only_message");
        return;
    }
}
