#pragma once
#include "commandmaster.h"


class CommandCompletion {
public:
    using CommandContainer = CommandMaster;
    using CommandIterator = CommandContainer::Commands::const_iterator;

    explicit CommandCompletion(const CommandMaster&);
    bool isActivated() const;
    void activate(QString);
    void deactivate();
    QString getNextSuggestion();

private:
    bool isOneWordLine(const QString&) const;

private:
    const CommandContainer* commandContainer;
    QString initialString;
    CommandIterator commandIter;
    bool activated;
};
