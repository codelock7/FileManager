#include "util.h"
#include <QDir>


QString operator/(const QString& lhs, const QString& rhs)
{
    return lhs + QDir::separator() + rhs;
}


