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
    normalMode.addCommand({ENormalOperation::RENAME, {EKey::C, EKey::W}});
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
    if (keyEvent->modifiers() != Qt::NoModifier) {
        if (keyEvent->modifiers() == (keyEvent->modifiers() & Qt::ControlModifier)) {
            if (!normalMode.isLast(EKey::CONTROL))
                normalMode.addKey(EKey::CONTROL);
        }

        if (keyEvent->modifiers() == (keyEvent->modifiers() & Qt::AltModifier)) {
            if (!normalMode.isLast(EKey::META))
                normalMode.addKey(EKey::META);
        }

        if (keyEvent->modifiers() == (keyEvent->modifiers() & Qt::ShiftModifier)) {
            if (!normalMode.isLast(EKey::SHIFT))
                normalMode.addKey(EKey::SHIFT);
        }
    }

    switch (keyEvent->key()) {
    case Qt::Key_C:
        normalMode.addKey(EKey::C);
        break;

    case Qt::Key_D:
        normalMode.addKey(EKey::D);
        break;

    case Qt::Key_G:
        normalMode.addKey(EKey::G);
        break;

    case Qt::Key_L:
        normalMode.addKey(EKey::L);
        break;

    case Qt::Key_H:
        normalMode.addKey(EKey::H);
        break;

    case Qt::Key_J:
        normalMode.addKey(EKey::J);
        break;

    case Qt::Key_K:
        normalMode.addKey(EKey::K);
        break;

    case Qt::Key_W:
        normalMode.addKey(EKey::W);
        break;

    case Qt::Key_Escape:
        normalMode.reset();
        filesView->setFocus();
        return true;

    case Qt::Key_Colon:
        normalMode.addKey(EKey::COLON);
        break;

    case Qt::Key_Slash:
        commandLine->setFocus();
        mode = Mode::SEARCH;
        return true;

    default:
        return false;
    }

    const NormalMode::Status status = normalMode.handle();
    if (std::get<bool>(status)) {
        switch (std::get<ENormalOperation>(status)) {
        case ENormalOperation::NONE:
            break;
        case ENormalOperation::COMMAND_MODE:
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
        case ENormalOperation::RENAME:
            mode = Mode::RENAME;
            commandLine->setFocus();
            commandLine->setText(model->fileName(getCurrentIndex()));
            break;
        }
        normalMode.reset();
    }
    return true;
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

    ui->statusbar->showMessage(tr("rc: %1").arg(model->rowCount(newIndex)));
}

void MainWindow::goToParent()
{
    const QModelIndex& newRoot = filesView->rootIndex().parent();
    filesView->setRootIndex(newRoot);
    filesView->selectRow(0);
    pathView->setText(model->filePath(newRoot));

    ui->statusbar->showMessage(tr("rc: %1").arg(model->rowCount(newRoot)));
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

void MainWindow::handleCommand()
{
    switch (mode) {
    case Mode::SEARCH:
        getView()->keyboardSearch(commandLine->text());
        switchToNormalMode();
        return;
    case Mode::RENAME:
        const QFileInfo& info = model->fileInfo(getCurrentIndex());
        if (const QString& newName = commandLine->text();
                !newName.isEmpty())
            QFile(info.filePath()).rename(info.path() + QDir::separator() + newName);
        switchToNormalMode();
        return;
    }

    const QString& operation = commandLine->text();
    if (operation.isEmpty())
        return;
    const QStringList& substrings = operation.split(' ');

    const QString& commandName = substrings.first().trimmed();
    if (commandName == "mkdir") {
        const QString& path = getCurrentDirectory();
        for (int i = 1; i < substrings.length(); ++i) {
            model->mkdir(filesView->rootIndex(), substrings[i]);
        }
    } else if (commandName == "open") {
        Platform::open(getCurrentFile().toStdWString().c_str());
    } else if (commandName == "touch") {
        const QString& currDir = getCurrentDirectory();
        for (int i = 1; i < substrings.length(); ++i) {
            const QString& newFilePath = currDir + QDir::separator() + substrings[i];
            QFile newFile(newFilePath);
            newFile.open(QIODevice::WriteOnly);
        }
    }

    switchToNormalMode();
}

void MainWindow::onRowsInserted(const QModelIndex& parent, int first, int last)
{
    qDebug("View updated");
    getView()->selectRow(0);
    ui->statusbar->showMessage(tr("rc: %1").arg(model->rowCount(parent)));
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
    mode = Mode::NORMAL;
    commandLine->clear();
    getView()->setFocus();
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
    commands.push_back(std::move(operation));
}

bool NormalMode::isLast(EKey key) const
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
    Status result;

    for (const NormalOperation& operation : commands) {
        const int status = operation.eq(keySequence);
        switch (status) {
        case -1:
            std::get<bool>(result) = true;
            break;
        case 1:
            std::get<bool>(result) = true;
            std::get<ENormalOperation>(result) = operation.getType();
            return result;
        }
    }

    std::get<ENormalOperation>(result) = ENormalOperation::NONE;
    return result;
}

void NormalMode::reset()
{
    keySequence.clear();
}

NormalOperation::NormalOperation(ENormalOperation cmd, Seq seq)
    : operation(cmd)
    , keySequence(std::move(seq))
{
}

int NormalOperation::eq(const Seq& rhs) const
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

ENormalOperation NormalOperation::getType() const
{
    return operation;
}
