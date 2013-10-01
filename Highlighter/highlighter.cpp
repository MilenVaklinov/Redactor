#include "highlighter.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>

Highlighter::Highlighter(QTextDocument *parent) : QSyntaxHighlighter(parent) { }

void Highlighter::open_file(const QString &file_name)
{
    QFile file(file_name);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    } else {


        QTextStream in(&file);
        QString text = in.readAll();

        QStringList lines = text.split("\n", QString::SkipEmptyParts);
        QStringList keywords_only;
        QString keywords_line;

        for (int i = 0; i != lines.size(); ++i) {
            if (lines.at(i).startsWith("keywords: ") && lines.at(i).endsWith("; ")) {

                keywords_line = lines.at(i);
                keywords_line = keywords_line.remove("keywords: ");
                keywords_line = keywords_line.remove(";");
                keywords_only = keywords_line.split(" ", QString::SkipEmptyParts);

                QString word;

                for (int i = 0; i != keywords_only.size(); ++i) {
                    word = keywords_only.at(i);
                    word = word.insert(0, "\\b");
                    word = word.insert(word.size(), "\\b");
                    keywords.append(word);
                }

            } else if (lines.at(i).startsWith("strings: ") && lines.at(i).endsWith("; ")) {

                string_line = lines.at(i);
                string_line = string_line.remove("strings: ");
                string_line = string_line.remove(";");
                string_line = string_line.insert(0, "\\");
                string_line = string_line.insert(2, ".*\\");

            } else if (lines.at(i).startsWith("headers: ") && lines.at(i).endsWith("; ")) {

                qDebug() << lines.at(i);
                headers = lines.at(i);
                headers = headers.remove("headers: ");
                headers = headers.remove("; ");
                headers = headers.insert(1, ".*");

            } else if (lines.at(i).startsWith("comments: ") && lines.at(i).endsWith(";")) {

                comments = lines.at(i);
                comments = comments.remove("comments: ");
                comments = comments.remove(";");
                comments = comments.insert(comments.size(), "[^\n]*");

            }
        }

        create_rule();
    }
}

void Highlighter::create_rule()
{
    HighlightingRule rule;
    keyword_format.setForeground(Qt::blue);

    foreach (const QString &pattern, keywords) {
        rule.pattern = QRegExp(pattern);
        rule.format = keyword_format;
        highlightingRules.append(rule);
    }

    quotation_format.setForeground(Qt::darkRed);
    rule.pattern = QRegExp(string_line);
    rule.format = quotation_format;
    highlightingRules.append(rule);

    headers_format.setForeground(Qt::darkRed);
    rule.pattern = QRegExp(headers);
    rule.format = headers_format;
    highlightingRules.append(rule);

    single_line_comment_format.setForeground(Qt::darkGreen);
    rule.pattern = QRegExp(comments);
    rule.format = single_line_comment_format;
    highlightingRules.append(rule);
}

void Highlighter::highlightBlock(const QString &text)
{
    foreach (const HighlightingRule &rule, highlightingRules) {
        QRegExp expression(rule.pattern);
        int index = expression.indexIn(text);
        while (index >= 0) {
            int length = expression.matchedLength();
            setFormat(index, length, rule.format);
            index = expression.indexIn(text, index + length);
        }
    }
}
