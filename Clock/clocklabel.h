#ifndef CLOCKLABEL_H
#define CLOCKLABEL_H

#include <QLabel>

class ClockLabel : public QLabel
{
    Q_OBJECT
public:
    explicit ClockLabel(QWidget *parent = 0);
    
private slots:
    void updateTime();
    
};

#endif // CLOCKLABEL_H
