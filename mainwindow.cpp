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

    filesView = ui->centralwidget->findChild<QTableView*>("filesView");
    Q_ASSERT(filesView != nullptr);

    pathView = ui->centralwidget->findChild<QLabel*>("pathView");
    Q_ASSERT(pathView != nullptr);

    commandLine = ui->centralwidget->findChild<QLineEdit*>("commandLine");
    Q_ASSERT(commandLine != nullptr);
    QObject::connect(commandLine, &QLineEdit::returnPressed, this, &MainWindow::onCommandLineEnter);

    static auto const eater = new KeyPressEater(
        std::bind(&MainWindow::handleKeyPress, this, std::placeholders::_1, std::placeholders::_2)
    );
    model = new QFileSystemModel;
    model->setRootPath(QDir::currentPath());
    QObject::connect(model, &QFileSystemModel::rowsInserted, this, &MainWindow::onRowsInserted);
    filesView->setModel(model);
    filesView->installEventFilter(eater);
    filesView->setColumnWidth(0, 400);

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
    return model->filePath(filesView->rootIndex());
}

int MainWindow::getCurrentRow() const
{
    const auto& currIndex = getCurrentIndex();
    if (currIndex.parent() != filesView->rootIndex())
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
    if (keyEvent->key() == 0)
        return false;

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
        commandLine->setFocus();
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
        switch (std::get<ENormalOperation>(status)) {
        case ENormalOperation::NONE:
            return;

        case ENormalOperation::COMMAND_MODE:
            mode = Mode::COMMAND;
            commandLine->setFocus();
            break;

        case ENormalOperation::OPEN_PARENT_DIRECTORY:
            goToParent();
            break;

        case ENormalOperation::OPEN_CURRENT_DIRECTORY:
            goToSelected();
            break;

        case ENormalOperation::SELECT_NEXT:
            goToDown();
            break;

        case ENormalOperation::SELECT_PREVIOUS:
            goToUp();
            break;

        case ENormalOperation::SELECT_FIRST:
            selectFirst();
            break;

        case ENormalOperation::SELECT_LAST:
            goToFall();
            break;

        case ENormalOperation::DELETE_FILE:
            deleteFile();
            break;

        case ENormalOperation::RENAME_FILE:
            mode = Mode::RENAME;
            commandLine->setFocus();
            commandLine->setText(model->fileName(getCurrentIndex()));
            break;

        case ENormalOperation::YANK_FILE:
            pathCopy = model->filePath(getCurrentIndex());
            break;

        case ENormalOperation::PASTE_FILE:
            copyFile(pathCopy);
            break;

        case ENormalOperation::SEARCH_NEXT:
            getView()->keyboardSearch(lastSearch);
            break;
        }
    }
    normalMode.reset();
}

void MainWindow::goToSelected()
{
    const QModelIndex& newIndex = getCurrentIndex();
    if (!model->isDir(newIndex))
        return;
    filesView->setRootIndex(newIndex);
    const QString& newPath = model->filePath(newIndex);
    filesView->selectRow(0);
    pathView->setText(newPath);

    showStatus(tr("rc: %1").arg(model->rowCount(newIndex)));
}

void MainWindow::goToParent()
{
    const QModelIndex& newRoot = filesView->rootIndex().parent();
    filesView->setRootIndex(newRoot);
    filesView->selectRow(0);
    pathView->setText(model->filePath(newRoot));

    showStatus(tr("rc: %1").arg(model->rowCount(newRoot)));
}

void MainWindow::goToDown()
{
    const auto currRow = getCurrentRow();
    if (currRow == -1) {
        filesView->selectRow(0);
        return;
    }
    filesView->selectRow(currRow + 1);
}

void MainWindow::goToUp()
{
    const auto currRow = getCurrentRow();
    if (currRow == -1) {
        filesView->selectRow(0);
        return;
    }
    filesView->selectRow(currRow - 1);
}

void MainWindow::goToFall()
{
    const int rowCount = model->rowCount(filesView->rootIndex());
    qDebug("rowCount: %d", rowCount);
    filesView->selectRow(rowCount - 1);
}

void MainWindow::selectFirst()
{
    filesView->selectRow(0);
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
    case Mode::COMMAND: {
        const QString& operation = commandLine->text();
        if (operation.isEmpty())
            return;
        const QStringList& substrings = operation.split(' ');

        const QString& commandName = substrings.first().trimmed();
        if (commandName == "mkdir") {
            for (int i = 1; i < substrings.length(); ++i) {
                model->mkdir(filesView->rootIndex(), substrings[i]);
            }
        } else if (commandName == "open") {
            Platform::open(getCurrentFile().toStdWString().c_str());
        } else if (commandName == "touch") {
            const QString& currDir = getCurrentDirectory();
            for (int i = 1; i < substrings.length(); ++i) {
                const QString& newFilePath = currDir / substrings[i];
                QFile newFile(newFilePath);
                newFile.open(QIODevice::WriteOnly);
            }
        }
        break;
    }
    case Mode::SEARCH: {
        lastSearch = commandLine->text();
        getView()->keyboardSearch(lastSearch);
        break;
    }
    case Mode::RENAME: {
        const QFileInfo& info = model->fileInfo(getCurrentIndex());
        if (const QString& newName = commandLine->text();
                !newName.isEmpty()) {
            QFile::rename(info.filePath(), info.path() / newName);
        }
        break;
    }
    case Mode::RENAME_FOR_COPY: {
        const QString& destPath = getCurrentDirectory() / commandLine->text();
        if (!QFile::copy(pathCopy, destPath)) {
            if (!QFileInfo::exists(pathCopy))
                showStatus("The file being copied no longer exists", 4);
            else if (QFileInfo::exists(destPath))
                showStatus("The file being copied with that name already exists", 4);
            else
                showStatus("Unexpected copy error", 4);
        }
        break;
    }
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
    return filesView;
}

QModelIndex MainWindow::getCurrentIndex() const
{
    const QModelIndex& currIndex = filesView->currentIndex();
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

void MainWindow::copyFile(const QString &filePath)
{
    if (filePath.isEmpty())
        return;
    const QFileInfo fileInfo(filePath);
    const QString& newFileName = fileInfo.fileName();
    const QString& newFilePath = getCurrentDirectory() / newFileName;
    if (QFile::copy(filePath, newFilePath))
        return;
    if (!fileInfo.exists()) {
        showStatus("The file being copied no longer exists", 4);
    } else if (QFileInfo::exists(newFilePath)) {
        mode = Mode::RENAME_FOR_COPY;
        commandLine->setFocus();
        commandLine->setText(newFileName);
        showStatus("Set a new name for the destination file");
    } else {
        showStatus("Enexpected copy error", 4);
    }
}

void MainWindow::showStatus(const QString& message, int seconds)
{
    Q_ASSERT(seconds >= 0);
    ui->statusbar->showMessage(message, seconds * 1000);
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

    using EqRes = NormalOperation::EqRes;
    for (const NormalOperation& operation : operations) {
        switch (operation.eq(keySequence)) {
        case EqRes::FALSE:
            break;
        case EqRes::MAYBE:
            std::get<bool>(result) = true;
            break;
        case EqRes::TRUE:
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

NormalOperation::NormalOperation(ENormalOperation operation, Seq keys)
    : operation(operation)
    , keys(std::move(keys))
{
}

NormalOperation::EqRes NormalOperation::eq(const Seq& seq) const
{
    Q_ASSERT(!seq.empty());
    if (seq.size() > keys.size())
        return EqRes::FALSE;
    for (size_t i = 0; i < seq.size(); ++i) {
        if (keys[i] != seq[i])
            return EqRes::FALSE;
    }
    if (seq.size() != keys.size())
        return EqRes::MAYBE;
    return EqRes::TRUE;
}

ENormalOperation NormalOperation::getType() const
{
    return operation;
}

QString toString(const std::vector<EKey>& keys)
{
    QString result;
    result.reserve(keys.size());
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
