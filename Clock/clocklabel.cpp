#include "clocklabel.h"

#include <QFont>
#include <QString>
#include <QTimer>
#include <QDateTime>

ClockLabel::ClockLabel(QWidget *parent) :  QLabel(parent)
{

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateTime()));
    timer->start(1000);

    updateTime();
}

void ClockLabel::updateTime()
{
    QString currentDateAndTime = QDateTime::currentDateTime().toString("dd MMM yyyy, ddd hh:mm:ss");
    setText(QString(8, ' ') + currentDateAndTime);
}
