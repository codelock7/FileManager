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
    , multiRowSelector(*this)
    , viModel(*this)
    , commandSuggestor(viModel)
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
    QObject::connect(commandLine, &QLineEdit::textEdited, this, &MainWindow::onCommandEdit);

    model = new QFileSystemModel;
    model->setRootPath(QDir::currentPath());
    QObject::connect(model, &QFileSystemModel::rowsInserted, this, &MainWindow::onRowsInserted);
    fileViewer->setModel(model);
    fileViewer->installEventFilter(this);
    fileViewer->setColumnWidth(0, 400);
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
    handleKeyPress(keyEvent);
}

bool MainWindow::handleKeyPress(QKeyEvent* keyEvent)
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
    viModel.handleKeyPress(k);
    return true;
}

bool MainWindow::eventFilter(QObject* object, QEvent* event)
{
    if (object == commandLine) {
        switch (event->type()) {
        case QEvent::KeyPress:
            if (static_cast<QKeyEvent*>(event)->key() == Qt::Key_Tab) {
                if (commandSuggestor.isEmpty())
                    commandSuggestor.setInitialString(commandLine->text());
                if (commandSuggestor.isValid())
                    commandLine->setText(commandSuggestor.getNext());
                return true;
            }
            break;
        case QEvent::KeyRelease:
            if (static_cast<QKeyEvent*>(event)->key() == Qt::Key_Tab)
                return true;
            break;
        }
    } else if (object == fileViewer) {
        if (event->type() == QEvent::KeyPress) {
            if (handleKeyPress(static_cast<QKeyEvent*>(event)))
                return true;
        }
    }
    return QObject::eventFilter(object, event);
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
    if (rowSelectionStrategy)
        rowSelectionStrategy->selectRow(row);
    else
        fileViewer->selectRow(row);
}

int MainWindow::getRowCount() const
{
    return model->rowCount(fileViewer->rootIndex());
}

void MainWindow::onCommandLineEnter()
{
    viModel.handleCommandEnter(commandLine->text());
}

void MainWindow::onCommandEdit()
{
    if (!commandSuggestor.isEmpty())
        commandSuggestor.reset();
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

void MainWindow::setMultiSelectionEnabled(bool enabled)
{
    if (enabled)
        rowSelectionStrategy = multiRowSelector.init();
    else
        rowSelectionStrategy = nullptr;
}

bool MainWindow::isMultiSelectionEnabled() const
{
    return rowSelectionStrategy != nullptr;
}

bool MainWindow::showQuestion(const QString& question)
{
    return QMessageBox::question(this, "Question", question) == QMessageBox::Yes;
}

QItemSelectionModel* MainWindow::getSelectionModel()
{
    return fileViewer->selectionModel();
}

QModelIndex MainWindow::getIndexForRow(int row) const
{
    return model->index(row, 0, fileViewer->rootIndex());
}



MultiRowSelector::MultiRowSelector(IFileViewer& newOwner)
    : owner(&newOwner)
{}

void MultiRowSelector::selectRow(int row)
{
    row = std::clamp(row, 0, owner->getRowCount());

    const QModelIndex& newIndex = owner->getIndexForRow(row);

    if (row < startingRow)
        setRowSelected(row < owner->getCurrentRow(), newIndex);
    else if (row > startingRow)
        setRowSelected(row > owner->getCurrentRow(), newIndex);
    else
        deselectCurrentRow();

    owner->getSelectionModel()->setCurrentIndex(newIndex, QItemSelectionModel::NoUpdate);
}

IRowSelectionStrategy* MultiRowSelector::init()
{
    startingRow = owner->getCurrentRow();
    return this;
}

void MultiRowSelector::deselectCurrentRow()
{
    const auto flag = QItemSelectionModel::Deselect | QItemSelectionModel::Rows;
    owner->getSelectionModel()->select(owner->getCurrentIndex(), flag);
}

void MultiRowSelector::setRowSelected(bool selected, const QModelIndex& newIndex)
{
    if (selected) {
        const auto flag = QItemSelectionModel::Select | QItemSelectionModel::Rows;
        owner->getSelectionModel()->select(newIndex, flag);
    } else {
        deselectCurrentRow();
    }
}
