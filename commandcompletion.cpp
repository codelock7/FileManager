#include "commandcompletion.h"
#include <QLineEdit>
#include <QEvent>
#include <QKeyEvent>


CommandCompletion::CommandCompletion(const CommandMaster& commandMaster, QObject *parent)
    : QObject(parent)
    , commandMaster(&commandMaster)
    , lineEdit(nullptr)
    , activated(false)
{}

bool CommandCompletion::isBinded() const
{
    return lineEdit != nullptr;
}

void CommandCompletion::bindWidget(QLineEdit& newLineEdit)
{
    Q_ASSERT(!isBinded());

    lineEdit = &newLineEdit;
    QObject::connect(lineEdit, &QLineEdit::textEdited, this, &CommandCompletion::onTextEdit);
}

void CommandCompletion::unbindWidget()
{
    Q_ASSERT(isBinded());

    QObject::disconnect(lineEdit, &QLineEdit::textEdited, this, &CommandCompletion::onTextEdit);
    lineEdit = nullptr;
}

bool CommandCompletion::isActivated() const
{
    return activated;
}

void CommandCompletion::activate()
{
    Q_ASSERT(!isActivated());

    activated = true;
    QString line = lineEdit->text();
    if (!isOneWordLine(line))
        return;
    initialString = std::move(line);
    currentCommand = commandMaster->cbegin();
    lineEdit->installEventFilter(this);
}

void CommandCompletion::updateSuggestion()
{
    Q_ASSERT(isActivated());

    if (resetIfEndReached())
        return;
    for (; currentCommand != commandMaster->cend(); ++currentCommand) {
        const auto& [name, func] = *currentCommand;
        if (name.startsWith(initialString)) {
            lineEdit->setText(name);
            ++currentCommand;
            return;
        }
    }
    resetIfEndReached();
}

void CommandCompletion::deactivate()
{
    Q_ASSERT(isActivated());

    activated = false;
    lineEdit->removeEventFilter(this);
    initialString.clear();
}

bool CommandCompletion::eventFilter(QObject* object, QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        auto const keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Tab) {
            updateSuggestion();
            return true;
        }
    }
    return QObject::eventFilter(object, event);
}

void CommandCompletion::onTextEdit()
{
    if (isActivated())
        deactivate();
}

bool CommandCompletion::isOneWordLine(const QString& line) const
{
    if (line.isEmpty())
        return true;
    auto charIter = std::find(line.rbegin(), line.rend(), QChar(' '));
    return charIter == line.rend();
}

bool CommandCompletion::resetIfEndReached()
{
    if (currentCommand != commandMaster->cend())
        return false;
    currentCommand = commandMaster->cbegin();
    lineEdit->setText(initialString);
    return true;
}
