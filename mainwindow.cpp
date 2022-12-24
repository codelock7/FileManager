#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFileSystemModel>
#include <QShortcut>
#include <QDir>
#include <QMessageBox>
#include <QLabel>
#include <QKeyEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    filesView = ui->centralwidget->findChild<QTableView*>("filesView");
    Q_ASSERT(filesView != nullptr);

    pathView = ui->centralwidget->findChild<QLabel*>("pathView");
    Q_ASSERT(pathView != nullptr);

    static auto const eater = new KeyPressEater(
        std::bind(&MainWindow::handleKeyPress, this, std::placeholders::_1, std::placeholders::_2)
    );
    model = new QFileSystemModel;
    model->setRootPath(QDir::currentPath());
    filesView->setModel(model);
    filesView->installEventFilter(eater);
}

MainWindow::~MainWindow()
{
    delete ui;
}

QString MainWindow::getSelectedPath() const
{
    const QModelIndex& index = filesView->currentIndex();
    const QVariant& path = index.data(QFileSystemModel::FilePathRole);
    return path.toString();
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
    switch (keyEvent->key()) {
    case Qt::Key_Q:
        qDebug("key q");
        break;

    case Qt::Key_L:
        goToSelected();
        break;

    case Qt::Key_H:
        goToParent();
        break;

    case Qt::Key_J:
        goToDown();
        break;

    case Qt::Key_K:
        goToUp();
        break;

    default:
        return false;
    }
    return true;
}

void MainWindow::goToSelected()
{
    const QModelIndex& currentIndex = filesView->currentIndex();
    const QFileInfo& newPathInfo = model->fileInfo(currentIndex);
    if (!newPathInfo.isDir())
        return;
    const QModelIndex& newIndex = model->index(newPathInfo.filePath());
    filesView->setRootIndex(newIndex);
    qDebug("%s %d", newPathInfo.path().toLocal8Bit().data(), model->rowCount());
    filesView->selectRow(0);
    pathView->setText(newPathInfo.filePath());
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

void MainWindow::onDirectoryLoaded()
{
    filesView->selectRow(0);
}

void MainWindow::deleteFile()
{
    const auto& currIndex = filesView->currentIndex();
    const auto& info = model->fileInfo(currIndex);
    const auto button = QMessageBox::question(this, tr("Do you want to delete file?"), tr("Delete this file?\n") + info.fileName());
    if (button == QMessageBox::Yes) {
        info.dir();

        qDebug(info.filePath().toLocal8Bit().data());
    }
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
