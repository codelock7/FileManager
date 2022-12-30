#pragma once
#include <map>
#include <array>
#include <QStringList>
#include "searchcontroller.h"


enum class ENormalOperation {
    NONE,
    RESET,
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


template<>
struct std::less<QString> {
    bool operator()(const QString& lhs, const QString& rhs) const { return lhs < rhs; }
};


class ICommandLineStrategy {
public:
    virtual QString getInitialValue() const { return {}; }
    virtual void handleEnter(QString) = 0;
};



struct PasteFileCommand;
class CLFileRenameStrategy : public ICommandLineStrategy {
public:
    QString getInitialValue() const override;
    void handleEnter(QString) override;

public:
    QString initialValue;
    PasteFileCommand* owner;
};


struct ICurrentDirectoryGetter {
    virtual QString getCurrentDirectory() const = 0;
};


struct IStatusDisplayer {
    virtual void showStatus(const QString&, int secTimeout = 0) = 0;
};


struct ICommandLineActivator {
    virtual void activateCommandLine(ICommandLineStrategy*) = 0;
};


struct ICommandMasterOwner : ICurrentDirectoryGetter, IStatusDisplayer, ICommandLineActivator, ISearchControllerOwner {
    virtual QString getCurrentFile() const = 0;
    virtual void mkdir(const QString&) = 0;
    virtual bool changeDirectoryIfCan(const QString& dirPath) = 0;
    virtual void setColorSchemeName(const QString&) = 0;
    virtual void selectPrevious() = 0;
    virtual void selectNext() = 0;
    virtual void selectFirst() = 0;
    virtual void selectLast() = 0;
    virtual void openCurrentDirectory() = 0;
    virtual void openParentDirectory() = 0;
};


struct PasteFileCommand {
    void paste();
    void pasteWithNewName(QString);

public:
    ICommandMasterOwner* owner;
    QString pathCopy;

private:
    CLFileRenameStrategy clFileRenameStrategy;
};


class CommandMaster final {
public:
    using Command = void(CommandMaster::*)(const QStringList&);
    using Commands = std::map<QString, Command>;
    using OperationFunction = void(CommandMaster::*)();
    using NormalOperations = std::array<OperationFunction, static_cast<size_t>(ENormalOperation::COUNT)>;

    explicit CommandMaster(ICommandMasterOwner&);

    Commands::const_iterator cbegin() const;
    Commands::const_iterator cend() const;

    SearchController& getSearchController() { return searchController; }

    bool runIfHas(const QStringList& args);

    void openCurrentDirectory();
    void openParentDirectory();
    void selectPrevious();
    void selectNext();
    void selectFirst();
    void selectLast();
    void yankFile();
    void pasteFile();
    void searchNext();
    void exit();

    void createEmptyFile(const QStringList& args);
    void changeDirectory(const QStringList& args);
    void openFile(const QStringList& args);
    void makeDirectory(const QStringList& args);
    void setColorScheme(const QStringList&);

public:
    NormalOperations normalOperations;

private:
    ICommandMasterOwner* owner;
    SearchController searchController;
    Commands commands;
    PasteFileCommand pasteFileCommand;
};
