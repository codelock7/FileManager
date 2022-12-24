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
};

enum class EKey {
    NONE,
    ESCAPE,
    META,
    SHIFT,
    CONTROL,
    COLON,
    D,
    G,
    H,
    J,
    K,
    L,

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

    COUNT,
};


class NormalOperation {
public:
    using Seq = std::vector<EKey>;

    NormalOperation(ENormalOperation, Seq);
    int eq(const Seq&) const;
    ENormalOperation getType() const;

private:
    ENormalOperation operation;
    std::vector<EKey> keySequence;
};

class NormalMode {
public:
    using Status = std::pair<bool, ENormalOperation>;

    bool isLast(EKey) const;
    bool isEmptySequence() const;
    void addCommand(NormalOperation);
    void addKey(EKey);
    Status handle();
    void reset();

private:
    std::vector<NormalOperation> commands;
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

private slots:
    void goToSelected();
    void goToParent();
    void goToDown();
    void goToUp();
    void goToFall();
    void selectFirst();
    void deleteFile();
    void handleCommand();
    void onDataChanged();
    void onRowsRemoved(const QModelIndex& parent, int first, int last);

private:
    QTableView* getView();

private:
    ViMode viMode;
    Ui::MainWindow *ui;
    QTableView* filesView;
    QLabel* pathView;
    QLineEdit* commandLine;
    QFileSystemModel* model;
    NormalMode normalMode;
};
