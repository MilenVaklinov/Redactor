#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include <QSyntaxHighlighter>

class Highlighter : public QSyntaxHighlighter
{

public:
    Highlighter(QTextDocument *parent = 0);
    void open_file(const QString& file_name);

protected:
    void highlightBlock(const QString &text);

private:
    void create_rule();

    QStringList keywords;
    QString string_line;
    QString headers;
    QString comments;

    struct HighlightingRule
    {
        QRegExp pattern;
        QTextCharFormat format;
    };

    QVector<HighlightingRule> highlightingRules;
    QRegExp commentStartExpression;
    QRegExp commentEndExpression;
    QTextCharFormat keyword_format;
    QTextCharFormat class_format;
    QTextCharFormat single_line_comment_format;
    QTextCharFormat quotation_format;
    QTextCharFormat function_format;
    QTextCharFormat include_format;
    QTextCharFormat headers_format;
};

#endif // HIGHLIGHTER_H
