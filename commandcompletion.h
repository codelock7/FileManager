#pragma once
#include "vimodel.h"
#include <optional>


class CommandCompletion {
public:
    using Commands = ViModel::Commands;

    explicit CommandCompletion(const ViModel&);
    void setInitialString(const QString&);
    bool isValid() const;
    bool isEmpty() const;
    QString getNext();
    void reset();

private:
    static bool isOneWordLine(const QString&);

private:
    const ViModel* viModel;
    std::optional<QString> initialString;
    Commands::const_iterator nextCommand;
    bool valid;
};
