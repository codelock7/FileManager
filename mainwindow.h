#pragma once

#include <QMainWindow>
#include <QFileInfo>
#include <functional>

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

    COUNT,
};


class NormalOperation {
public:
    using Seq = std::vector<EKey>;
    enum EqRes {
        MAYBE,
        FALSE,
        TRUE,
    };

    NormalOperation(ENormalOperation, Seq);
    EqRes eq(const Seq&) const;
    ENormalOperation getType() const;

private:
    ENormalOperation operation;
    std::vector<EKey> keys;
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
    std::vector<EKey> keySequence;
};

class ViMode {
public:
    Mode getCurrentMode() const;
    void setMode(Mode);
    void reset();
    bool hasSequence() const;

private:
    Mode mode = Mode::NORMAL;
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


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    QString getCurrentFile() const;
    QFileInfo getCurrentFileInfo() const;
    QString getCurrentDirectory() const;
    int getCurrentRow() const;
    void keyPressEvent(QKeyEvent*) override;
    bool handleKeyPress(QObject*, QKeyEvent*);
    void handleNormalOperation();

private slots:
    void goToSelected();
    void goToParent();
    void goToDown();
    void goToUp();
    void goToFall();
    void selectFirst();
    void deleteFile();
    void onCommandLineEnter();
    void onRowsInserted(const QModelIndex& parent, int first, int last);
    void onMessageChange(const QString&);

private:
    QTableView* getView() const;
    QModelIndex getCurrentIndex() const;
    void switchToNormalMode();
    void copyFile(const QString&);
    void showStatus(const QString&, int seconds = 0);

private:
    ViMode viMode;
    Ui::MainWindow *ui;
    QTableView* filesView;
    QLabel* pathView;
    QLineEdit* commandLine;
    QFileSystemModel* model;
    NormalMode normalMode;
    Mode mode = Mode::NORMAL;
    QString pathCopy;
    QString lastSearch;
};
