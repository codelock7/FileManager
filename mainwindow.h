#pragma once

#include <QMainWindow>
#include <QFileInfo>
#include <functional>
#include <array>
#include <map>
#include "commandmaster.h"
#include "commandcompletion.h"
#include "searchcontroller.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


class QTableView;
class QShortcut;
class QFileSystemModel;
class QLabel;
class QLineEdit;


class KeyPressEater : public QObject {
public:
    using Handler = std::function<bool(QObject*, QKeyEvent*)>;
    explicit KeyPressEater(Handler);

protected:
    bool eventFilter(QObject*, QEvent*) override;

private:
    Handler keyEventHandler;
};


class MainWindow : public QMainWindow, ICommandMasterOwner
{
    Q_OBJECT

public:
    using OperationFunction = void(MainWindow::*)();
    using NormalOperations = std::array<OperationFunction, static_cast<size_t>(ENormalOperation::COUNT)>;

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    QString getCurrentFile() const override;
    QString getCurrentDir() const override;
    QFileInfo getCurrentFileInfo() const;
    QString getCurrentDirectory() const override;
    void keyPressEvent(QKeyEvent*) override;
    bool handleKeyPress(QObject*, QKeyEvent*);

private slots:
    void openCurrentDirectory() override;
    void openParentDirectory() override;
    void selectRow(int) override;
    int getRowCount() const override;
    int getCurrentRow() const override;
    void onCommandLineEnter();
    void onRowsInserted(const QModelIndex& parent, int first, int last);
    void onMessageChange(const QString&);

private:
    void completeCommand();
    bool resetCompletionIfEndReached();
    QModelIndex getCurrentIndex() const;
    void switchToNormalMode();
    void showStatus(const QString&, int secTimeout = 0) override;
    void mkdir(const QString& dirName) override;
    void setColorSchemeName(const QString&) override;
    bool changeDirectoryIfCan(const QString& dirPath) override;
    QString getCommandLineString() const;
    void setRootIndex(const QModelIndex&);
    void searchForward(const QString&) override;

    void focusToCommandLine(const QString& line = {}) override;
    void activateFileViewer() override;

    void activateCommandMode();

private:
    Ui::MainWindow *ui;
    QTableView* fileViewer;
    QLabel* pathViewer;
    QLineEdit* commandLine;
    QFileSystemModel* model;

    CommandMaster commandMaster;
    CommandCompletion commandCompletion;
};
