#include "util.h"
#include <QDir>


QString operator/(const QString& lhs, const QString& rhs)
{
    QString result;
    constexpr int separatorSize = 1;
    result.reserve(lhs.size() + separatorSize + rhs.size());
    result += lhs;
    result += QDir::separator();
    result += rhs;
    return result;
}
