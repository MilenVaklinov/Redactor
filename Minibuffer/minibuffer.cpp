#include "minibuffer.h"

#include <QCompleter>
#include <QMouseEvent>
#include <QDebug>

Minibuffer::Minibuffer(QWidget *parent) : QLineEdit(parent), custom_completer(0)
{
    setFrame(false);
    setAlignment(Qt::AlignVCenter);
    setFocusPolicy(Qt::NoFocus);

    QFont font("DejaVu Sans Mono", 10);
    setFont(font);

    setStyleSheet("color: black;");

    create_completer();
}

Minibuffer::~Minibuffer()
{

}

//==========================================================================================
//
//==========================================================================================

void Minibuffer::create_completer()
{
    QStringList wordList;

    // A B C D E F G H I J K L M N O P Q R S T U V W X Y Z

    wordList << "append-to-buffer" << "append-to-file" << "buffer-menu"
             << "counter" << "copy-to-buffer" << "clock-on" << "clock-off"
             << "dired" << "face" << "goto-line" << "insert-buffer" << "insert-to-buffer" << "insert-file"
             << "prepend-to-buffer" << "new" << "rename-buffer" << "shell" << "set-visited-file-name"
             << "search-backward" << "search-forward" << "search-backward-regexp"
             << "search-forward-regexp" << "unix-tutorial" << "redactor-tutorial"
             << "AliceBlue" << "AntiqueWhite" << "Aqua" << "Aquamarine" << "Azure"
             << "Beige" << "Bisque" << "Black" << "BlanchedAlmond" << "Blue"
             << "BlueViolet" << "Brown" << "BurlyWood" << "CadetBlue"
             << "Chartreuse" << "Chocolate" << "Coral" << "CornflowerBlue"
             << "Cornsilk" << "Crimson" << "Cyan" << "DarkBlue" << "DarkCyan"
             << "DarkGoldenRod" << "DarkGray" << "DarkGrey"
             << "DarkGreen" << "DarkKhaki" << "DarkMagenta" << "DarkOliveGreen"
             << "Darkorange" << "DarkOrchid" << "DarkRed" << "DarkSalmon"
             << "DarkSeaGreen" << "DarkSlateBlue" << "DarkSlateGray"
             << "DarkSlateGrey" << "DarkTurquoise" << "DarkViolet" << "DeepPink"
             << "DeepSkyBlue" << "DimGray" << "DimGrey" << "DodgerBlue"
             << "FireBrick" <<"FloralWhite" << "ForestGreen" << "Fuchsia" << "Gainsboro"
             << "GhostWhite" << "Gold" << "GoldenRod" << "Gray" << "Grey" << "Green"
             << "GreenYellow" << "HoneyDew" << "HotPink" << "IndianRed" << "Indigo"
             << "Ivory" << "Khaki" << "Lavender" << "LavenderBlush"
             << "LawnGreen" << "LemonChiffon" << "LightBlue" << "LightCoral"
             << "LightCyan" << "LightGoldenRodYellow" << "LightGray" << "LightGrey"
             << "LightGreen" << "LightPink" << "LightSalmon" << "LightSeaGreen"
             << "LightSkyBlue" << "LightSlateGray" << "LightSlateGrey" << "LightSteelBlue"
             << "LightYellow" << "Lime" << "LimeGreen" << "Linen" << "Magenta"
             << "Maroon" << "MediumAquaMarine" << "MediumBlue" << "MediumOrchid"
             << "MediumPurple" << "MediumSeaGreen" << "MediumSlateBlue" << "MediumSpringGreen"
             << "MediumTurquoise" << "MediumVioletRed" << "MidnightBlue"
             << "MintCream" << "MistyRose" << "Moccasin" << "NavajoWhite"
             << "Navy" << "OldLace" << "Olive" << "OliveDrab" << "Orange"
             << "OrangeRed" << "Orchid" << "PaleGoldenRod" << "PaleGreen" << "PaleTurquoise"
             << "PaleVioletRed" << "PapayaWhip" << "PeachPuff" << "Peru" << "Pink"
             << "Plum" << "PowderBlue" << "Purple" << "Red" << "RosyBrown" << "RoyalBlue"
             << "SaddleBrown" << "Salmon" << "SandyBrown" << "SeaGreen"
             << "SeaShell" << "Sienna" << "Silver" << "SkyBlue" << "SlateBlue"
             << "SlateGray" << "SlateGrey" << "Snow" << "SpringGreen" << "SteelBlue"
             << "Tan" << "Teal" << "Thistle" << "Tomato" << "Turquoise"
             << "Violet" << "Wheat" << "White" << "WhiteSmoke" << "Yellow" << "YellowGreen";

    QCompleter *completer = new QCompleter(wordList, this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setCompletionMode(QCompleter::InlineCompletion);

    set_completer(completer);
}

//==========================================================================================
//
//==========================================================================================

void Minibuffer::set_completer(QCompleter *given_completer)
{
    custom_completer = given_completer;

    if (!custom_completer)
        return;

    custom_completer = given_completer;

    custom_completer->setCompletionMode(QCompleter::InlineCompletion);
    custom_completer->setCaseSensitivity(Qt::CaseInsensitive);
    custom_completer->setWrapAround(false);

    connect(this, SIGNAL(textChanged(QString)), this, SLOT(insert_completion(QString)));

    setCompleter(custom_completer);
}

//==========================================================================================
//
//==========================================================================================

void Minibuffer::insert_completion(const QString &completion)
{
    if (custom_completer->widget() != this)
        return;

    if (completion.length() < 3)
        custom_completer->blockSignals(true);
    else
        custom_completer->blockSignals(false);
}
