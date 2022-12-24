#pragma once

#include <QMainWindow>
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
    void onDirectoryLoaded();
    void deleteFile();
    void handleCommand();
    void onIndexPressed(const QModelIndex&);
    void onDataChanged();

private:
    ViMode viMode;
    Ui::MainWindow *ui;
    QTableView* filesView;
    QLabel* pathView;
    QLineEdit* commandLine;
    QFileSystemModel* model;
};
