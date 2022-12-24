#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFileSystemModel>
#include <QShortcut>
#include <QDir>
#include <QMessageBox>
#include <QLabel>
#include <QKeyEvent>
#include <QLineEdit>
#include "platform.h"
#include <vector>


#define GET_CSTR(qStr) (qStr.toLocal8Bit().data())


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    filesView = ui->centralwidget->findChild<QTableView*>("filesView");
    Q_ASSERT(filesView != nullptr);

    pathView = ui->centralwidget->findChild<QLabel*>("pathView");
    Q_ASSERT(pathView != nullptr);

    commandLine = ui->centralwidget->findChild<QLineEdit*>("commandLine");
    Q_ASSERT(commandLine != nullptr);
    QObject::connect(commandLine, &QLineEdit::returnPressed, this, &MainWindow::handleCommand);

    static auto const eater = new KeyPressEater(
        std::bind(&MainWindow::handleKeyPress, this, std::placeholders::_1, std::placeholders::_2)
    );
    model = new QFileSystemModel;
    model->setRootPath(QDir::currentPath());
    QObject::connect(model, &QFileSystemModel::dataChanged, this, &MainWindow::onDataChanged);
    filesView->setModel(model);
    filesView->installEventFilter(eater);

    commandMode.addCommand({CommandType::OPEN_PARENT_DIRECTORY, {EKey::H}});
    commandMode.addCommand({CommandType::OPEN_CURRENT_DIRECTORY, {EKey::L}});
    commandMode.addCommand({CommandType::SELECT_NEXT, {EKey::J}});
    commandMode.addCommand({CommandType::SELECT_PREVIOUS, {EKey::K}});
    commandMode.addCommand({CommandType::SELECT_FIRST, {EKey::G, EKey::G}});
    commandMode.addCommand({CommandType::SELECT_LAST, {EKey::SHIFT, EKey::G}});
}

MainWindow::~MainWindow()
{
    delete ui;
}

QString MainWindow::getCurrentFile() const
{
    return model->filePath(filesView->currentIndex());
}

QFileInfo MainWindow::getCurrentFileInfo() const
{
    return model->fileInfo(filesView->currentIndex());
}

QString MainWindow::getCurrentDirectory() const
{
    return model->filePath(filesView->currentIndex().parent());
}

int MainWindow::getCurrentRow() const
{
    const QModelIndex& index = filesView->currentIndex();
    return index.row();
}

void MainWindow::keyPressEvent(QKeyEvent* keyEvent)
{
    QMainWindow::keyPressEvent(keyEvent);
    handleKeyPress(this, keyEvent);
}

bool MainWindow::handleKeyPress(QObject*, QKeyEvent* keyEvent)
{
    if (keyEvent->modifiers() != Qt::NoModifier) {
        if (keyEvent->modifiers() == (keyEvent->modifiers() & Qt::ControlModifier)) {
            if (!commandMode.isLast(EKey::CONTROL))
                commandMode.addKey(EKey::CONTROL);
        }

        if (keyEvent->modifiers() == (keyEvent->modifiers() & Qt::AltModifier)) {
            if (!commandMode.isLast(EKey::META))
                commandMode.addKey(EKey::META);
        }

        if (keyEvent->modifiers() == (keyEvent->modifiers() & Qt::ShiftModifier)) {
            if (!commandMode.isLast(EKey::SHIFT))
                commandMode.addKey(EKey::SHIFT);
        }
    }

    switch (keyEvent->key()) {
    case Qt::Key_Q:
//        if (keyEvent->modifiers() && keyEvent->modifiers() == (keyEvent->modifiers() & Qt::ShiftModifier))
//            qApp->exit();
        break;
    case Qt::Key_G:
        commandMode.addKey(EKey::G);
        break;

    case Qt::Key_L:
        commandMode.addKey(EKey::L);
//        goToSelected();
        break;

    case Qt::Key_H:
        commandMode.addKey(EKey::H);
//        goToParent();
        break;

    case Qt::Key_J:
        commandMode.addKey(EKey::J);
//        goToDown();
        break;

    case Qt::Key_K:
        commandMode.addKey(EKey::K);
//        goToUp();
        break;

    case Qt::Key_Escape:
        filesView->setFocus();
        break;

    case Qt::Key_Colon:
        commandLine->setFocus();
        break;

    case Qt::Key_D:
        if (keyEvent->modifiers() && keyEvent->modifiers() == (keyEvent->modifiers() & Qt::ShiftModifier))
            deleteFile();
        break;

    default:
        return false;
    }

    const CommandMode::Status status = commandMode.handle();
    if (std::get<bool>(status)) {
        switch (std::get<CommandType>(status)) {
        case CommandType::NONE:
            break;
        case CommandType::OPEN_PARENT_DIRECTORY:
            goToParent();
            break;
        case CommandType::OPEN_CURRENT_DIRECTORY:
            goToSelected();
            break;
        case CommandType::SELECT_NEXT:
            goToDown();
            break;
        case CommandType::SELECT_PREVIOUS:
            goToUp();
            break;
        case CommandType::SELECT_FIRST:
            selectFirst();
            break;
        case CommandType::SELECT_LAST:
            goToFall();
            break;
        }
        commandMode.reset();
    }
    qDebug("Reset");
    return true;
}

void MainWindow::goToSelected()
{
    const QFileInfo& currFileInfo = getCurrentFileInfo();
    if (!currFileInfo.isDir())
        return;
    const QString& currFilePath = currFileInfo.filePath();
    const QModelIndex& newIndex = model->index(currFilePath);
    filesView->setRootIndex(newIndex);
    qDebug("%s %d", GET_CSTR(currFilePath), model->rowCount());
    filesView->selectRow(0);
    pathView->setText(currFilePath);
}

void MainWindow::goToParent()
{
    filesView->setRootIndex(filesView->rootIndex().parent());
    filesView->selectRow(0);
    pathView->setText(model->filePath(filesView->rootIndex()));
}

void MainWindow::goToDown()
{
    const int nextRow = getCurrentRow() + 1;
    filesView->selectRow(nextRow);
}

void MainWindow::goToUp()
{
    const int prevRow = getCurrentRow() - 1;
    filesView->selectRow(prevRow);
}

void MainWindow::goToFall()
{
    const int rowCount = model->rowCount(filesView->currentIndex().parent());
    qDebug("rowCount: %d", rowCount);
    filesView->selectRow(rowCount - 1);
}

void MainWindow::selectFirst()
{
    filesView->selectRow(0);
}

void MainWindow::deleteFile()
{
    const QFileInfo& info = getCurrentFileInfo();
    const auto button = QMessageBox::question(this, tr("Do you want to delete file?"), tr("Delete this file?\n") + info.fileName());
    if (button != QMessageBox::Yes)
        return;
    qDebug(GET_CSTR(info.filePath()));
    if (info.isDir()) {
        QDir dir(info.filePath());
        dir.removeRecursively();
    } else {
        QFile file(info.filePath());
        file.remove();
    }
}

void MainWindow::handleCommand()
{
    const QString& command = commandLine->text();
    if (command.isEmpty())
        return;
    const QStringList& substrings = command.split(' ');

    const QString& commandName = substrings.first().trimmed();
    if (commandName == "mkdir") {
        const QString& path = getCurrentDirectory();
        QDir dir(path);
        for (int i = 1; i < substrings.length(); ++i) {
            dir.mkdir(substrings[i]);
        }
    } else if (commandName == "open") {
        const auto& currFile = getCurrentFile();
        Platform::open(GET_CSTR(currFile));
    } else if (commandName == "touch") {
        const QString& currDir = getCurrentDirectory();
        for (int i = 1; i < substrings.length(); ++i) {
            const QString& newFilePath = currDir + QDir::separator() + substrings[i];
            QFile newFile(newFilePath);
            newFile.open(QIODevice::WriteOnly);
        }
    }

    commandLine->clear();
    filesView->setFocus();
}

void MainWindow::onDataChanged()
{
    qDebug("dataChanged");
}


KeyPressEater::KeyPressEater(Handler handler)
    : keyEventHandler(std::move(handler))
{
}

bool KeyPressEater::eventFilter(QObject* object, QEvent* event)
{
    if (event->type() != QEvent::KeyPress)
        return QObject::eventFilter(object, event);
    auto const keyEvent = static_cast<QKeyEvent*>(event);
    if (!keyEventHandler(object, keyEvent))
        return QObject::eventFilter(object, event);
    return true;
}

Mode ViMode::getCurrentMode() const
{
    return mode;
}

void ViMode::setMode(Mode newMode)
{
    mode = newMode;
}

void ViMode::reset()
{
    mode = Mode::NORMAL;
}

bool ViMode::hasSequence() const
{
    return true;
}

void CommandMode::addCommand(Command command)
{
    commands.push_back(std::move(command));
}

bool CommandMode::isLast(EKey key) const
{
    if (keySequence.empty())
        return false;
    return keySequence.back() == key;
}

void CommandMode::addKey(EKey key)
{
    keySequence.push_back(key);
}

CommandMode::Status CommandMode::handle()
{
    Status result;

    for (const Command& command : commands) {
        const int status = command.eq(keySequence);
        switch (status) {
        case -1:
            std::get<bool>(result) = true;
            break;
        case 1:
            std::get<bool>(result) = true;
            std::get<CommandType>(result) = command.getType();
            return result;
        }
    }

    std::get<CommandType>(result) = CommandType::NONE;
    return result;
}

void CommandMode::reset()
{
    keySequence.clear();
}

Command::Command(CommandType cmd, Seq seq)
    : command(cmd)
    , keySequence(std::move(seq))
{
}

int Command::eq(const Seq& rhs) const
{
    Q_ASSERT(!rhs.empty());
    if (keySequence.size() < rhs.size())
        return -1;
    if (keySequence.size() > rhs.size())
        return 0;
    const size_t len = keySequence.size();
    for (size_t i = 0; i < len; ++i) {
        if (keySequence[i] != rhs[i])
            return 0;
    }
    return 1;
}

CommandType Command::getType() const
{
    return command;
}
