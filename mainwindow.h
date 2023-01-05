#pragma once

#include <QMainWindow>
#include <QFileInfo>
#include <QItemSelection>
#include <functional>
#include <array>
#include <map>
#include "vimodel.h"
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


class IFileViewer : public IRowInfo {
public:
    virtual QItemSelectionModel* getSelectionModel() = 0;
    virtual QModelIndex getCurrentIndex() const = 0;
    virtual QModelIndex getIndexForRow(int) const = 0;
};


class MultiRowSelector final : IRowSelectionStrategy {
public:
    explicit MultiRowSelector(IFileViewer&);
    IRowSelectionStrategy* init();
    void selectRow(int) override;

private:
    void deselectCurrentRow();
    void setRowSelected(bool, const QModelIndex&);

private:
    IFileViewer* owner;
    int startingRow;
};


class MainWindow : public QMainWindow, IViView, IFileViewer {
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
    bool handleKeyPress(QKeyEvent*);
    bool eventFilter(QObject*, QEvent*) override;

private slots:
    void openCurrentDirectory() override;
    void openParentDirectory() override;
    void selectRow(int) override;
    int getRowCount() const override;
    int getCurrentRow() const override;
    void onCommandLineEnter();
    void onCommandEdit();
    void onRowsInserted(const QModelIndex& parent, int first, int last);
    void onMessageChange(const QString&);

private:
    void completeCommand();
    bool resetCompletionIfEndReached();
    QModelIndex getCurrentIndex() const override;
    void showStatus(const QString&, int secTimeout = 0) override;
    void mkdir(const QString& dirName) override;
    void setColorSchemeName(const QString&) override;
    bool changeDirectoryIfCan(const QString& dirPath) override;
    void setRootIndex(const QModelIndex&);
    void searchForward(const QString&) override;

    void focusToCommandLine(const QString& line = {}) override;
    void activateFileViewer() override;
    void setMultiSelectionEnabled(bool) override;
    bool isMultiSelectionEnabled() const override;
    bool showQuestion(const QString&) override;

    QItemSelectionModel* getSelectionModel() override;
    QModelIndex getIndexForRow(int) const override;

    bool isRowVisible(int) const override;

private:
    Ui::MainWindow *ui;
    QTableView* fileViewer;
    QLabel* pathViewer;
    QLineEdit* commandLine;
    QFileSystemModel* model;
    MultiRowSelector multiRowSelector;
    IRowSelectionStrategy* rowSelectionStrategy = nullptr;

    ViModel viModel;
    CommandCompletion commandSuggestor;
};
