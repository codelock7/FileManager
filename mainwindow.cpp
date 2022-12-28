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


QString operator/(const QString& lhs, const QString& rhs)
{
    return lhs + QDir::separator() + rhs;
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QObject::connect(ui->statusbar, &QStatusBar::messageChanged, this, &MainWindow::onMessageChange);

    fileViewer = ui->centralwidget->findChild<QTableView*>("fileViewer");
    Q_ASSERT(fileViewer != nullptr);

    pathViewer = ui->centralwidget->findChild<QLabel*>("pathViewer");
    Q_ASSERT(pathViewer != nullptr);

    commandLine = ui->centralwidget->findChild<QLineEdit*>("commandLine");
    Q_ASSERT(commandLine != nullptr);
    QObject::connect(commandLine, &QLineEdit::returnPressed, this, &MainWindow::onCommandLineEnter);

    static auto const eater = new KeyPressEater(
        std::bind(&MainWindow::handleKeyPress, this, std::placeholders::_1, std::placeholders::_2)
    );
    model = new QFileSystemModel;
    model->setRootPath(QDir::currentPath());
    QObject::connect(model, &QFileSystemModel::rowsInserted, this, &MainWindow::onRowsInserted);
    fileViewer->setModel(model);
    fileViewer->installEventFilter(eater);
    fileViewer->setColumnWidth(0, 400);

    normalMode.addCommand({ENormalOperation::COMMAND_MODE, {EKey::SHIFT, EKey::COLON}});
    normalMode.addCommand({ENormalOperation::OPEN_PARENT_DIRECTORY, {EKey::H}});
    normalMode.addCommand({ENormalOperation::OPEN_CURRENT_DIRECTORY, {EKey::L}});
    normalMode.addCommand({ENormalOperation::SELECT_NEXT, {EKey::J}});
    normalMode.addCommand({ENormalOperation::SELECT_PREVIOUS, {EKey::K}});
    normalMode.addCommand({ENormalOperation::SELECT_FIRST, {EKey::G, EKey::G}});
    normalMode.addCommand({ENormalOperation::SELECT_LAST, {EKey::SHIFT, EKey::G}});
    normalMode.addCommand({ENormalOperation::DELETE_FILE, {EKey::SHIFT, EKey::D}});
    normalMode.addCommand({ENormalOperation::RENAME_FILE, {EKey::C, EKey::W}});
    normalMode.addCommand({ENormalOperation::YANK_FILE, {EKey::Y, EKey::Y}});
    normalMode.addCommand({ENormalOperation::PASTE_FILE, {EKey::P}});
    normalMode.addCommand({ENormalOperation::SEARCH_NEXT, {EKey::N}});
    normalMode.addCommand({ENormalOperation::EXIT, {EKey::CONTROL, EKey::Q}});

    normalOperations[static_cast<size_t>(ENormalOperation::COMMAND_MODE)] = &MainWindow::activateCommandMode;
    normalOperations[static_cast<size_t>(ENormalOperation::OPEN_PARENT_DIRECTORY)] = &MainWindow::openParentDirectory;
    normalOperations[static_cast<size_t>(ENormalOperation::OPEN_CURRENT_DIRECTORY)] = &MainWindow::openCurrentDirectory;
    normalOperations[static_cast<size_t>(ENormalOperation::SELECT_NEXT)] = &MainWindow::selectNext;
    normalOperations[static_cast<size_t>(ENormalOperation::SELECT_PREVIOUS)] = &MainWindow::selectPrevious;
    normalOperations[static_cast<size_t>(ENormalOperation::SELECT_FIRST)] = &MainWindow::selectFirst;
    normalOperations[static_cast<size_t>(ENormalOperation::SELECT_LAST)] = &MainWindow::selectLast;
    normalOperations[static_cast<size_t>(ENormalOperation::DELETE_FILE)] = &MainWindow::deleteFile;
    normalOperations[static_cast<size_t>(ENormalOperation::RENAME_FILE)] = &MainWindow::renameFile;
    normalOperations[static_cast<size_t>(ENormalOperation::YANK_FILE)] = &MainWindow::copyFilePath;
    normalOperations[static_cast<size_t>(ENormalOperation::PASTE_FILE)] = &MainWindow::pasteFile;
    normalOperations[static_cast<size_t>(ENormalOperation::SEARCH_NEXT)] = &MainWindow::searchNext;
    normalOperations[static_cast<size_t>(ENormalOperation::EXIT)] = &MainWindow::exit;

    commands = Commands({
        std::make_pair(QString("cd"), &MainWindow::changeDirectory),
        std::make_pair(QString("touch"), &MainWindow::createEmptyFile),
        std::make_pair(QString("open"), &MainWindow::openFile),
        std::make_pair(QString("mkdir"), &MainWindow::makeDirectory)
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

QString MainWindow::getCurrentFile() const
{
    return model->filePath(getCurrentIndex());
}

QFileInfo MainWindow::getCurrentFileInfo() const
{
    return model->fileInfo(getCurrentIndex());
}

QString MainWindow::getCurrentDirectory() const
{
    return model->filePath(fileViewer->rootIndex());
}

int MainWindow::getCurrentRow() const
{
    const auto& currIndex = getCurrentIndex();
    if (currIndex.parent() != fileViewer->rootIndex())
        return -1;
    return currIndex.row();
}

void MainWindow::keyPressEvent(QKeyEvent* keyEvent)
{
    QMainWindow::keyPressEvent(keyEvent);
    handleKeyPress(this, keyEvent);
}

bool MainWindow::handleKeyPress(QObject*, QKeyEvent* keyEvent)
{
    if (keyEvent->type() != QKeyEvent::KeyPress)
        return false;

    if (const Qt::KeyboardModifiers modifiers = keyEvent->modifiers();
            modifiers != Qt::NoModifier) {
        if (modifiers == (modifiers & Qt::ControlModifier)) {
            if (!normalMode.isLastEqual(EKey::CONTROL))
                normalMode.addKey(EKey::CONTROL);
        } else if (modifiers == (modifiers & Qt::AltModifier)) {
            if (!normalMode.isLastEqual(EKey::META))
                normalMode.addKey(EKey::META);
        }

        if (keyEvent->modifiers() == (keyEvent->modifiers() & Qt::ShiftModifier)) {
            if (!normalMode.isLastEqual(EKey::SHIFT))
                normalMode.addKey(EKey::SHIFT);
        }
    }

    switch (keyEvent->key()) {
    case Qt::Key_Escape:
        switchToNormalMode();
        return true;

    case Qt::Key_Colon:
        normalMode.addKey(EKey::COLON);
        break;

    case Qt::Key_Slash:
        activateCommandLine();
        mode = Mode::SEARCH;
        return true;

    default:
        if (keyEvent->key() >= Qt::Key_A && keyEvent->key() <= Qt::Key_Z) {
            const int offset = keyEvent->key() - Qt::Key_A;
            const auto key = static_cast<EKey>(static_cast<int>(EKey::A) + offset);
            normalMode.addKey(key);
            break;
        }
        return false;
    }

    showStatus(tr("rc: %1, ks: %2").arg(model->rowCount(getView()->rootIndex())).arg(normalMode.seq()));

    handleNormalOperation();
    return true;
}

void MainWindow::handleNormalOperation()
{
    const NormalMode::Status status = normalMode.handle();
    if (std::get<bool>(status)) {
        const auto& operation = std::get<ENormalOperation>(status);
        switch (operation) {
        case ENormalOperation::NONE:
            return;
        default:
            Q_ASSERT(operation != ENormalOperation::COUNT);
            const size_t i = static_cast<size_t>(operation);
            auto ptr = normalOperations[i];
            (this->*ptr)();
            break;
        }
    }
    normalMode.reset();
}

void MainWindow::openCurrentDirectory()
{
    const QModelIndex& newIndex = getCurrentIndex();
    if (!model->isDir(newIndex))
        return;
    fileViewer->setRootIndex(newIndex);
    const QString& newPath = model->filePath(newIndex);
    fileViewer->selectRow(0);
    pathViewer->setText(newPath);

    showStatus(tr("rc: %1").arg(model->rowCount(newIndex)));
}

void MainWindow::openParentDirectory()
{
    const QModelIndex& newRoot = fileViewer->rootIndex().parent();
    fileViewer->setRootIndex(newRoot);
    fileViewer->selectRow(0);
    pathViewer->setText(model->filePath(newRoot));

    showStatus(tr("rc: %1").arg(model->rowCount(newRoot)));
}

void MainWindow::selectNext()
{
    const int currRow = getCurrentRow();
    if (currRow == -1) {
        fileViewer->selectRow(0);
        return;
    }
    fileViewer->selectRow(currRow + 1);
}

void MainWindow::selectPrevious()
{
    const int currRow = getCurrentRow();
    if (currRow == -1) {
        fileViewer->selectRow(0);
        return;
    }
    fileViewer->selectRow(currRow - 1);
}

void MainWindow::selectLast()
{
    const int rowCount = model->rowCount(fileViewer->rootIndex());
    fileViewer->selectRow(rowCount - 1);
}

void MainWindow::selectFirst()
{
    fileViewer->selectRow(0);
}

void MainWindow::deleteFile()
{
    const QModelIndex& currIndex = getCurrentIndex();
    const QMessageBox::StandardButton button = QMessageBox::question(
        this, tr("Do you want to delete file?"),
        tr("Delete this file?\n") + model->fileName(currIndex)
    );
    if (button == QMessageBox::Yes) {
        if (model->isDir(currIndex))
            QDir(model->filePath(currIndex)).removeRecursively();
        else
            model->remove(currIndex);
    }
}

void MainWindow::onCommandLineEnter()
{
    switch (mode) {
    case Mode::COMMAND:
        handleCommand();
        break;
    case Mode::SEARCH:
        lastSearch = getCommandLineString();
        getView()->keyboardSearch(lastSearch);
        break;
    case Mode::RENAME:
        handleRename();
        break;
    case Mode::RENAME_FOR_COPY:
        handleRenameForCopy();
        break;
    case Mode::NORMAL:
        Q_ASSERT(false);
        break;
    }
    switchToNormalMode();
}

void MainWindow::onRowsInserted(const QModelIndex& parent, int first, int last)
{
    qDebug("View updated");
    getView()->selectRow(0);
    showStatus(tr("rc: %1").arg(model->rowCount(parent)));
}

void MainWindow::onMessageChange(const QString& message)
{
    if (message.isEmpty())
        showStatus(tr("rc: %1, ks: %2").arg(model->rowCount(getView()->rootIndex())).arg(normalMode.seq()));
}

QTableView *MainWindow::getView() const
{
    return fileViewer;
}

QModelIndex MainWindow::getCurrentIndex() const
{
    const QModelIndex& currIndex = fileViewer->currentIndex();
    return model->index(currIndex.row(), 0, currIndex.parent());
}

void MainWindow::switchToNormalMode()
{
    switch (mode) {
    case Mode::NORMAL:
        normalMode.reset();
        break;
    default:
        mode = Mode::NORMAL;
        commandLine->clear();
        getView()->setFocus();
        break;
    }
}

void MainWindow::showStatus(const QString& message, int seconds)
{
    Q_ASSERT(seconds >= 0);
    ui->statusbar->showMessage(message, seconds * 1000);
}

void MainWindow::handleCommand()
{
    const QString& line = getCommandLineString();
    if (line.isEmpty())
        return;
    const QStringList& keys = line.split(' ', QString::SkipEmptyParts);
    const QString& commandName = keys.first().trimmed();
    auto iter = commands.find(commandName);
    if (iter == commands.end()) {
        showStatus(tr("Unknown command"), 4);
        return;
    }
    (this->*(iter->second))(keys);
}

void MainWindow::handleRename()
{
    const QString& newName = getCommandLineString();
    if (newName.isEmpty())
        return;
    const QFileInfo& info = model->fileInfo(getCurrentIndex());
    QFile::rename(info.filePath(), info.path() / newName);
}

void MainWindow::handleRenameForCopy()
{
    const QString& destPath = getCurrentDirectory() / getCommandLineString();
    if (QFile::copy(pathCopy, destPath))
        return;
    if (!QFileInfo::exists(pathCopy))
        showStatus("The file being copied no longer exists", 4);
    else if (QFileInfo::exists(destPath))
        showStatus("The file being copied with that name already exists", 4);
    else
        showStatus("Unexpected copy error", 4);
}

QString MainWindow::getCommandLineString() const
{
    return commandLine->text();
}

void MainWindow::activateCommandLine(const QString &initialValue)
{
    if (!initialValue.isEmpty())
        commandLine->setText(initialValue);
    commandLine->setFocus();
}

void MainWindow::setRootIndex(const QModelIndex& newRootIndex)
{
    fileViewer->setRootIndex(newRootIndex);
    pathViewer->setText(model->filePath(newRootIndex));
    fileViewer->selectRow(0);
}

void MainWindow::activateCommandMode()
{
    mode = Mode::COMMAND;
    activateCommandLine();
}

void MainWindow::renameFile()
{
    mode = Mode::RENAME;
    activateCommandLine(model->fileName(getCurrentIndex()));
}

void MainWindow::copyFilePath()
{
    pathCopy = model->filePath(getCurrentIndex());
}

void MainWindow::pasteFile()
{
    if (pathCopy.isEmpty())
        return;
    const QFileInfo fileInfo(pathCopy);
    const QString& newFileName = fileInfo.fileName();
    const QString& newFilePath = getCurrentDirectory() / newFileName;
    if (QFile::copy(pathCopy, newFilePath))
        return;
    if (!fileInfo.exists()) {
        showStatus("The file being copied no longer exists", 4);
    } else if (QFileInfo::exists(newFilePath)) {
        mode = Mode::RENAME_FOR_COPY;
        activateCommandLine(newFileName);
        showStatus("Set a new name for the destination file");
    } else {
        showStatus("Enexpected copy error", 4);
    }
}

void MainWindow::searchNext()
{
    getView()->keyboardSearch(lastSearch);
}

void MainWindow::exit()
{
    qApp->exit();
}

void MainWindow::createEmptyFile(const QStringList& args)
{
    const QString& currDir = getCurrentDirectory();
    for (int i = 1; i < args.length(); ++i) {
        QFile newFile(currDir / args[i]);
        newFile.open(QIODevice::WriteOnly);
    }
}

void MainWindow::changeDirectory(const QStringList& args)
{
    if (args.size() != 2) {
        showStatus("Invalid command signature", 4);
        return;
    }
    QString newDirPath = args[1];
    QFileInfo newDirInfo(newDirPath);
    if (newDirInfo.isRelative()) {
        newDirInfo.setFile(getCurrentDirectory() / newDirPath);
        newDirPath = newDirInfo.absoluteFilePath();
    }
    if (!newDirInfo.exists()) {
        showStatus("Target directory does not exist", 4);
        return;
    }
    const QModelIndex& newRootIndex = model->index(newDirPath);
    if (!newRootIndex.isValid()) {
        showStatus("Unexpected error", 4);
        return;
    }
    setRootIndex(newRootIndex);
}

void MainWindow::openFile(const QStringList&)
{
    Platform::open(getCurrentFile().toStdWString().c_str());
}

void MainWindow::makeDirectory(const QStringList& args)
{
    for (int i = 1; i < args.length(); ++i)
        model->mkdir(fileViewer->rootIndex(), args[i]);
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


void NormalMode::addCommand(NormalOperation operation)
{
    operations.push_back(std::move(operation));
}

bool NormalMode::isLastEqual(EKey key) const
{
    if (keySequence.empty())
        return false;
    return keySequence.back() == key;
}

bool NormalMode::isEmptySequence() const
{
    return keySequence.empty();
}

void NormalMode::addKey(EKey key)
{
    keySequence.push_back(key);
}

NormalMode::Status NormalMode::handle()
{
    Status result = {false, ENormalOperation::NONE};

    using CompareResult = NormalOperation::CompareResult;
    for (const NormalOperation& operation : operations) {
        switch (operation.compareKeySequence(keySequence)) {
        case CompareResult::FALSE:
            break;
        case CompareResult::MAYBE:
            std::get<bool>(result) = true;
            break;
        case CompareResult::TRUE:
            std::get<bool>(result) = true;
            std::get<ENormalOperation>(result) = operation.getType();
            return result;
        }
    }

    return result;
}

void NormalMode::reset()
{
    keySequence.clear();
}

NormalOperation::NormalOperation(ENormalOperation operation, KeySequence keys)
    : operation(operation)
    , keys(std::move(keys))
{
}

NormalOperation::CompareResult NormalOperation::compareKeySequence(const KeySequence& seq) const
{
    Q_ASSERT(!seq.empty());
    if (seq.size() > keys.size())
        return CompareResult::FALSE;
    for (size_t i = 0; i < seq.size(); ++i) {
        if (keys[i] != seq[i])
            return CompareResult::FALSE;
    }
    if (seq.size() != keys.size())
        return CompareResult::MAYBE;
    return CompareResult::TRUE;
}

ENormalOperation NormalOperation::getType() const
{
    return operation;
}

QString toString(const std::vector<EKey>& keys)
{
    QString result;
    const int reserveSize = static_cast<int>(keys.size());
    Q_ASSERT(reserveSize == keys.size());
    result.reserve(reserveSize);

    for (EKey key : keys) {
        switch (key) {
        case EKey::CONTROL:
            result.append('C');
            break;
        case EKey::SHIFT:
            result.append('S');
            break;
        case EKey::META:
            result.append('M');
            break;
        default:
            if (key >= EKey::A && key <= EKey::Z) {
                const int offset = static_cast<int>(key) - static_cast<int>(EKey::A);
                result.append('a' + offset);
            }
            break;
        }
    }
    return result;
}
