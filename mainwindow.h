#pragma once

#include <QMainWindow>
#include <QFileInfo>
#include <functional>
#include <array>
#include <map>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


class QTableView;
class QShortcut;
class QFileSystemModel;
class QLabel;
class QLineEdit;


enum class Mode {
    NORMAL,
    COMMAND,
    SEARCH,
    RENAME,
    RENAME_FOR_COPY,
};

enum class EKey {
    NONE,
    ESCAPE,
    META,
    SHIFT,
    CONTROL,
    COLON,
    A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

    COUNT,
};


using KeySequence = std::vector<EKey>;


enum class ENormalOperation {
    NONE,
    RESET,
    COMMAND_MODE,
    OPEN_PARENT_DIRECTORY,
    OPEN_CURRENT_DIRECTORY,
    SELECT_NEXT,
    SELECT_PREVIOUS,
    SELECT_FIRST,
    SELECT_LAST,
    DELETE_FILE,
    RENAME_FILE,
    YANK_FILE,
    PASTE_FILE,
    SEARCH_NEXT,
    EXIT,

    COUNT,
};


class NormalOperation {
public:
    enum CompareResult {
        MAYBE,
        FALSE,
        TRUE,
    };

    NormalOperation(ENormalOperation, KeySequence);
    CompareResult compareKeySequence(const KeySequence&) const;
    ENormalOperation getType() const;

private:
    ENormalOperation operation;
    KeySequence keys;
};


QString toString(const std::vector<EKey>&);


class NormalMode {
public:
    using Status = std::pair<bool, ENormalOperation>;

    bool isLastEqual(EKey) const;
    bool isEmptySequence() const;
    void addCommand(NormalOperation);
    void addKey(EKey);
    Status handle();
    void reset();
    QString seq() const {return toString(keySequence);}

private:
    std::vector<NormalOperation> operations;
    KeySequence keySequence;
};


class KeyPressEater : public QObject {
public:
    using Handler = std::function<bool(QObject*, QKeyEvent*)>;
    explicit KeyPressEater(Handler);

protected:
    bool eventFilter(QObject*, QEvent*) override;

private:
    Handler keyEventHandler;
};


template<>
struct std::less<QString> {
    bool operator()(const QString& lhs, const QString& rhs) const { return lhs < rhs; }
};


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    using OperationFunction = void(MainWindow::*)();
    using NormalOperations = std::array<OperationFunction, static_cast<size_t>(ENormalOperation::COUNT)>;
    using Command = void(MainWindow::*)(const QStringList&);
    using Commands = std::map<QString, Command>;

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    QString getCurrentFile() const;
    QFileInfo getCurrentFileInfo() const;
    QString getCurrentDirectory() const;
    int getCurrentRow() const;
    void keyPressEvent(QKeyEvent*) override;
    bool eventFilter(QObject*, QEvent*) override;
    bool handleKeyPress(QObject*, QKeyEvent*);
    void handleNormalOperation();

private slots:
    void openCurrentDirectory();
    void openParentDirectory();
    void selectNext();
    void selectPrevious();
    void selectLast();
    void selectFirst();
    void deleteFile();
    void onCommandLineEnter();
    void onRowsInserted(const QModelIndex& parent, int first, int last);
    void onMessageChange(const QString&);

private:
    void completeCommand();
    QTableView* getView() const;
    QModelIndex getCurrentIndex() const;
    void switchToNormalMode();
    void showStatus(const QString&, int seconds = 0);
    void handleCommand();
    void handleRename();
    void handleRenameForCopy();
    QString getCommandLineString() const;
    void activateCommandLine(const QString& initialValue = {});
    void setRootIndex(const QModelIndex&);

    void activateCommandMode();
    void renameFile();
    void copyFilePath();
    void pasteFile();
    void searchNext();
    void exit();

    void createEmptyFile(const QStringList& args);
    void changeDirectory(const QStringList& args);
    void openFile(const QStringList& args);
    void makeDirectory(const QStringList& args);
    void setColorScheme(const QStringList&);

private:
    Ui::MainWindow *ui;
    QTableView* fileViewer;
    QLabel* pathViewer;
    QLineEdit* commandLine;
    QFileSystemModel* model;
    NormalMode normalMode;
    Mode mode = Mode::NORMAL;
    QString pathCopy;
    QString lastSearch;
    NormalOperations normalOperations;
    Commands commands;
};
