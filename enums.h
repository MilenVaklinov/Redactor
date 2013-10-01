#ifndef ENUMS_H
#define ENUMS_H

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

#ifndef Q_MOC_RUN

namespace

#endif

sQ {

#if defined(Q_MOC_RUN)

    Q_OBJECT

#endif

#if defined(Q_MOC_RUN)

    Q_ENUMS(Mark HelpWindow)

#endif // (defined(Q_MOC_RUN))

    enum Mark { Deactivated, Activated, Set, Position };
    enum HelpWindow { ClosingWindow , ExitEditor };
}

QT_END_NAMESPACE

#endif // ENUMS_H
