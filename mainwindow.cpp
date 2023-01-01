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
#include "util.h"


#define GET_CSTR(qStr) (qStr.toLocal8Bit().data())


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , commandMaster(*this)
    , commandCompletion(commandMaster)
{
    setStyleSheet("background-color: rgb(0, 0, 0); color: rgb(255, 255, 255);");

    ui->setupUi(this);
    QObject::connect(ui->statusbar, &QStatusBar::messageChanged, this, &MainWindow::onMessageChange);

    fileViewer = ui->centralwidget->findChild<QTableView*>("fileViewer");
    Q_ASSERT(fileViewer != nullptr);
    fileViewer->setFocus();

    qApp->setStyleSheet(
        "QTableView {"
        "    outline: 0;"
        "    selection-color: black;"
        "    selection-background-color: yellow;"
        "}"
        "QTablView::item {"
        "    background-color: yellow;"
        "}"
        "QTableView::item:focus {"
        "    color: black;"
        "    background-color: yellow;"
        "    border: 0px;"
        "}"
    );

    pathViewer = ui->centralwidget->findChild<QLabel*>("pathViewer");
    Q_ASSERT(pathViewer != nullptr);

    commandLine = ui->centralwidget->findChild<QLineEdit*>("commandLine");
    Q_ASSERT(commandLine != nullptr);
    commandLine->installEventFilter(this);
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

    commandCompletion.bindWidget(*commandLine);
}

MainWindow::~MainWindow()
{
    delete ui;
}

QString MainWindow::getCurrentFile() const
{
    return model->filePath(getCurrentIndex());
}

QString MainWindow::getCurrentDir() const
{
    return model->filePath(fileViewer->rootIndex());
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
    const QModelIndex& currIndex = getCurrentIndex();
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

    const int key = keyEvent->key();
    switch (key) {
    case Qt::Key_Alt:
        [[fallthrough]];
    case Qt::Key_Meta:
        [[fallthrough]];
    case Qt::Key_Shift:
        [[fallthrough]];
    case Qt::Key_Control:
        [[fallthrough]];
    case Qt::Key_Tab:
        return true;
    }

    Key k;

    switch (key) {
    case Qt::Key_Escape:
        k.value = EKey::ESCAPE;
        break;

    case Qt::Key_Colon:
        k.value = EKey::COLON;
        break;

    case Qt::Key_Slash:
        k.value = EKey::SLASH;
        break;

    default:
        if (key >= Qt::Key_A && key <= Qt::Key_Z) {
            if (const Qt::KeyboardModifiers modifiers = keyEvent->modifiers();
                    modifiers != Qt::NoModifier) {
                if (modifiers == (modifiers & Qt::ControlModifier))
                    k.control = true;
                if (modifiers == (modifiers & Qt::AltModifier))
                    k.meta = true;
                if (modifiers == (modifiers & Qt::ShiftModifier))
                    k.shift = true;
            }
            const int offset = key - Qt::Key_A;
            const EKey trKey = static_cast<EKey>(static_cast<int>(EKey::A) + offset);
            k.value = trKey;
            break;
        }
        return false;
    }
    commandMaster.handleKeyPress(k);
    return true;
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

void MainWindow::selectRow(int row)
{
    fileViewer->selectRow(row);
}

int MainWindow::getRowCount() const
{
    return model->rowCount(fileViewer->rootIndex());
}

void MainWindow::onCommandLineEnter()
{
    commandMaster.handleCommandEnter(getCommandLineString());
}

void MainWindow::onRowsInserted(const QModelIndex& parent, int first, int last)
{
    qDebug("View updated");
    fileViewer->selectRow(0);
    showStatus(tr("rc: %1").arg(model->rowCount(parent)));
}

void MainWindow::onMessageChange(const QString& message)
{
//    if (message.isEmpty())
//        showStatus(tr("rc: %1, ks: %2").arg(model->rowCount(fileViewer->rootIndex())).arg(normalMode.seq()));
}

QModelIndex MainWindow::getCurrentIndex() const
{
    const QModelIndex& currIndex = fileViewer->currentIndex();
    if (currIndex.column() == 0)
        return currIndex;
    return model->index(currIndex.row(), 0, currIndex.parent());
}

void MainWindow::switchToNormalMode()
{
//    switch (mode) {
//    case Mode::NORMAL:
//        normalMode.reset();
//        break;
//    case Mode::COMMAND:
//        if (commandCompletion.isActivated())
//            commandCompletion.deactivate();

//        mode = Mode::NORMAL;
//        commandLine->clear();
//        fileViewer->setFocus();
//        break;
//    default:
//        mode = Mode::NORMAL;
//        commandLine->clear();
//        fileViewer->setFocus();
//        break;
//    }
}

void MainWindow::showStatus(const QString& message, int secTimeout)
{
    Q_ASSERT(secTimeout >= 0);
    ui->statusbar->showMessage(message, secTimeout * 1000);
}

void MainWindow::mkdir(const QString& dirName)
{
    model->mkdir(fileViewer->rootIndex(), dirName);
}

void MainWindow::setColorSchemeName(const QString& name)
{
    if (name == "light") {
        setStyleSheet("");
        qApp->setStyleSheet("");
    } else if (name == "dark") {
        setStyleSheet("background-color: rgb(0, 0, 0); color: rgb(255, 255, 255);");
        qApp->setStyleSheet(
            "QTableView {"
            "    outline: 0;"
            "    selection-color: black;"
            "    selection-background-color: yellow;"
            "}"
            "QTablView::item {"
            "    background-color: yellow;"
            "}"
            "QTableView::item:focus {"
            "    color: black;"
            "    background-color: yellow;"
            "    border: 0px;"
            "}"
        );
    }
}

bool MainWindow::changeDirectoryIfCan(const QString &dirPath)
{
    const QModelIndex& rootIndex = model->index(dirPath);
    if (!rootIndex.isValid())
        return false;
    setRootIndex(rootIndex);
    return true;
}

QString MainWindow::getCommandLineString() const
{
    return commandLine->text();
}

void MainWindow::setRootIndex(const QModelIndex& newRootIndex)
{
    fileViewer->setRootIndex(newRootIndex);
    pathViewer->setText(model->filePath(newRootIndex));
    fileViewer->selectRow(0);
}

void MainWindow::searchForward(const QString& line)
{
    fileViewer->keyboardSearch(line);
}

void MainWindow::focusToCommandLine(const QString &line)
{
    if (!line.isEmpty())
        commandLine->setText(line);
    commandLine->setFocus();
}

void MainWindow::activateFileViewer()
{
    commandLine->clear();
    fileViewer->setFocus();
}

void MainWindow::activateCommandMode()
{
    commandCompletion.activate();
    focusToCommandLine();
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
