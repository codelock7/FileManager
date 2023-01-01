#pragma once
#include <map>
#include <array>
#include <QStringList>
#include "searchcontroller.h"
#include <functional>


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


enum class EKey {
    NONE,
    ESCAPE,
    META,
    SHIFT,
    CONTROL,
    COLON,
    SLASH,
    SEMICOLON,
    A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

    COUNT,
};


bool isChar(EKey key);


template<>
struct std::less<QString> {
    bool operator()(const QString& lhs, const QString& rhs) const { return lhs < rhs; }
};


struct ICurrentDirectoryGetter {
    virtual QString getCurrentDirectory() const = 0;
};


struct IStatusDisplayer {
    virtual void showStatus(const QString&, int secTimeout = 0) = 0;
};


struct ICommandMasterOwner : ICurrentDirectoryGetter, IStatusDisplayer, ISearchControllerOwner {
    virtual QString getCurrentFile() const = 0;
    virtual QString getCurrentDir() const = 0;
    virtual void mkdir(const QString&) = 0;
    virtual bool changeDirectoryIfCan(const QString& dirPath) = 0;
    virtual void setColorSchemeName(const QString&) = 0;
    virtual void openCurrentDirectory() = 0;
    virtual void openParentDirectory() = 0;
    virtual void selectRow(int) = 0;
    virtual int getRowCount() const = 0;
    virtual int getCurrentRow() const = 0;
    virtual void focusToCommandLine(const QString& = {}) = 0;
    virtual void activateFileViewer() = 0;
};


class CommandMaster;
struct PasteFileCommand {
    void paste();
    void pasteWithNewName(QString);

public:
    CommandMaster* owner;
    QString pathCopy;
};




//template<EKey... ekeys>
//struct KeyFiller {
//    template<int n>
//    static constexpr Key get()
//    {
//        if constexpr (ekey == EKey::SHIFT)
//            key.shift = true;
//        else if constexpr (ekey == EKey::CONTROL)
//            key.control = true;
//        else if constexpr (ekey == EKey::META)
//            key.meta = true;
//        else if constexpr (isChar<ekey>())
//            key.value = ekey;
//        else
//            Q_ASSERT(false);
//    }

//    static constexpr

//    template<EKey ekey>
//    static constexpr bool isChar()
//    {
//        return static_cast<int>(ekey) >= static_cast<int>(EKey::A) &&
//               static_cast<int>(ekey) <= static_cast<int>(EKey::Z);
//    }

//    Key value;
//};


struct Key {
    Key();
    Key(std::initializer_list<EKey>);

    bool operator==(Key) const;
    bool operator!=(const Key &rhs) const;
private:
    void operator+=(EKey);
public:
    EKey value;
    bool shift : 1;
    bool control : 1;
    bool meta : 1;
};


enum class CompareResult
{
    FALSE,
    TRUE,
    PARTIALLY_TRUE,
};


struct KeySeq {
    using Keys = std::array<Key, 4>;

    KeySeq();
    KeySeq(Keys, size_t);
    KeySeq(std::initializer_list<Key>);

//    template<typename... T>
//    KeySeq(T... params)
//        : keys{Key::make<params>...}
//        , length(sizeof...(params))
//    {
//    }

    Keys::const_iterator cbegin() const;
    Keys::const_iterator cend() const;

    CompareResult compare(const KeySeq&) const;

    void operator+=(Key);

    void clear();
    bool isEmpty() const;
    size_t getLength() const;

public:
    Keys keys;
    uint8_t length;
};


using KeySequence = KeySeq;


class NormalOperation {
public:
    NormalOperation(ENormalOperation, KeySequence);
    CompareResult compare(const KeySequence&) const;
    ENormalOperation getType() const;

private:
    ENormalOperation operation;
    KeySequence keySequence;
};


QString toString(const std::vector<EKey>&);




class NormalMode {
public:
    using Status = std::pair<bool, ENormalOperation>;

    bool isEmptySequence() const;
    void addCommand(NormalOperation);
    void addKey(Key);
    Status handle();
    void reset();
//    QString seq() const {return toString(keySequence);}

private:
    std::vector<NormalOperation> operations;
    KeySequence keySequence;
};



class QKeyEvent;


class CommandMaster final {
public:
    using Command = void(CommandMaster::*)(const QStringList&);
    using Commands = std::map<QString, Command>;
    using OperationFunction = void(CommandMaster::*)();
    using NormalOperations = std::array<OperationFunction, static_cast<size_t>(ENormalOperation::COUNT)>;

    explicit CommandMaster(ICommandMasterOwner&);

    Commands::const_iterator cbegin() const;
    Commands::const_iterator cend() const;

    ICommandMasterOwner& getUi();

    void handleKeyPress(Key);
    void handleCommandEnter(QString);

    void switchToNormalMode();
    void switchToCommandMode();
    void switchToSearchMode();
    void switchToFileRenameMode(QString oldName);

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
    void renameCurrent();
    void removeCurrent();
    void exit();

    void createEmptyFile(const QStringList& args);
    void changeDirectory(const QStringList& args);
    void openFile(const QStringList& args);
    void makeDirectory(const QStringList& args);
    void setColorScheme(const QStringList&);

public:
    NormalOperations normalOperations;

private:
    ICommandMasterOwner* ui;
    SearchController searchController;
    Commands commands;
    PasteFileCommand pasteFileCommand;
    std::function<void(QString)> clStrategy;
    NormalMode normalMode;
};
