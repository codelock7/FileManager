#include "commandcompletion.h"


CommandCompletion::CommandCompletion(const CommandMaster& commandMaster)
    : commandContainer(&commandMaster)
    , activated(false)
{}

bool CommandCompletion::isActivated() const
{
    return activated;
}

void CommandCompletion::activate(QString line)
{
    Q_ASSERT(!isActivated());

    if (!isOneWordLine(line))
        return;
    activated = true;
    initialString = std::move(line);
    commandIter = commandContainer->cbegin();
}

QString CommandCompletion::getNextSuggestion()
{
    Q_ASSERT(isActivated());

    for (; commandIter != commandContainer->cend(); ++commandIter) {
        const auto& [name, func] = *commandIter;
        if (name.startsWith(initialString)) {
            ++commandIter;
            return name;
        }
    }
    commandIter = commandContainer->cbegin();
    return initialString;
}

void CommandCompletion::deactivate()
{
    Q_ASSERT(isActivated());

    activated = false;
    initialString.clear();
}

bool CommandCompletion::isOneWordLine(const QString& line) const
{
    if (line.isEmpty())
        return true;
    auto charIter = std::find(line.rbegin(), line.rend(), QChar(' '));
    return charIter == line.rend();
}
