#-------------------------------------------------
#
# Project created by QtCreator 2012-10-22T10:32:23
#
#-------------------------------------------------

QT       += core gui
QT       += widgets

TARGET = untitled
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
    Clock/clocklabel.cpp \
    Counter/countingsourcelineswidget.cpp \
    Buffer/texteditor.cpp \
    Editor/mytextedit.cpp \
    Dired/dired.cpp \
    Highlighter/highlighter.cpp \
    Minibuffer/minibuffer.cpp \
    Shell/Shell.cpp \

HEADERS  += mainwindow.h \
    Clock/clocklabel.h \
    Counter/countingsourcelineswidget.h \
    Buffer/texteditor.h \
    Editor/mytextedit.h \
    Dired/dired.h \
    Highlighter/highlighter.h \
    Minibuffer/minibuffer.h \
    enums.h \
    Shell/Shell.h \

FORMS    +=

RESOURCES += \
    MyResource.qrc \

OTHER_FILES += \
    docs/Actions.txt \
    docs/Colors.txt \
    docs/Bugs.txt \
    docs/Things_to_finish.txt \
    Shell/ascii2html/test.out \
    Shell/ascii2html/makefile \
    Shell/ascii2html/input.col
