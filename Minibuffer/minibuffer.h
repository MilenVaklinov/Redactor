#ifndef MINIBUFFER_H
#define MINIBUFFER_H

#include <QLineEdit>

class Minibuffer : public QLineEdit
{
    Q_OBJECT

public:
    Minibuffer(QWidget *parent = 0);
    ~Minibuffer();

public:
    QString get_the_string() { return name; }
    void set_the_string(const QString &given_name) { name = given_name; }

private slots:
    void insert_completion(const QString &completion);

private:
    void create_completer();
    void set_completer(QCompleter *given_completer);

private:
    QString name;
    QCompleter *custom_completer;
};

#endif // MINIBUFFER_H
