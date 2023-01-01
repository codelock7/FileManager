#pragma once
#include <QObject>
#include <QLineEdit>
#include "commandmaster.h"
#include <optional>


class ICommandLine {
public:
    virtual QString getValue() const = 0;
    virtual void setValue(QString) = 0;
};

class CommandCompletion : public QObject
{
    Q_OBJECT

public:
    using Commands = CommandMaster::Commands;
    using CommandIterator = Commands::const_iterator;

    explicit CommandCompletion(const CommandMaster&, QObject* parent = nullptr);
    bool isBinded() const;
    void bindWidget(QLineEdit&);
    void unbindWidget();
    bool isActivated() const;
    void activate();
    void deactivate();

protected:
    bool eventFilter(QObject*, QEvent*) override;
    void updateSuggestion();

private slots:
    void onTextEdit();

private:
    bool isOneWordLine(const QString& line) const;
    bool resetIfEndReached();

private:
    const CommandMaster* commandMaster;
    QLineEdit* lineEdit;
    QString initialString;
    CommandIterator currentCommand;
    bool activated;
};
