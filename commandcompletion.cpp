#include "commandcompletion.h"


CommandCompletion::CommandCompletion(const ViModel& newViModel)
    : viModel(&newViModel)
    , valid(false)
{}

void CommandCompletion::setInitialString(const QString& value)
{
    initialString = value;
    if (!isOneWordLine(*initialString))
        return;
    nextCommand = viModel->cbegin();
    valid = true;
}

bool CommandCompletion::isValid() const
{
    return valid;
}

bool CommandCompletion::isEmpty() const
{
    return !initialString.has_value();
}

QString CommandCompletion::getNext()
{
    for (; nextCommand != viModel->cend(); ++nextCommand) {
        const auto& [name, func] = *nextCommand;
        if (name.startsWith(*initialString)) {
            ++nextCommand;
            return name;
        }
    }
    nextCommand = viModel->cbegin();
    return *initialString;
}

void CommandCompletion::reset()
{
    valid = false;
    if (initialString.has_value())
        initialString.reset();
}

bool CommandCompletion::isOneWordLine(const QString& line)
{
    if (line.isEmpty())
        return true;
    auto charIter = std::find(line.rbegin(), line.rend(), QChar(' '));
    return charIter == line.rend();
}
