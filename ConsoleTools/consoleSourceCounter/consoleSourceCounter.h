#ifndef CONSOLESOURCECOUNTER_H
#define CONSOLESOURCECOUNTER_H

#include <QCoreApplication>
#include <QMap>

class QStringList;
class QTime;
class QString;


class SourceCounter
{

public:
    SourceCounter(const QString &folderName);
    ~SourceCounter();

private:
    QString name_of_the_source;
    QTime *timer;

    QMap<QString, QVector<int> > *myMap;

    qint64 allLines;
    qint64 allCodeLines;
    qint64 allEmptyLines;
    qint64 allComments;
    qint64 allFiles;

    QStringList *filtersPtr;

    void initVariables();
    void found(const QString & nameOfFolder);
    void printResult();

    void count(const QString& nameOfFile, const QString& fileType);
    void countCpp(const QString& nameOfFile, const QString& fileType);
};

#endif // CONSOLESOURCECOUNTER_H
