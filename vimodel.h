#pragma once
#include <map>
#include <array>
#include <QStringList>
#include "searchcontroller.h"
#include <functional>
#include <variant>


enum class ENormalOperation {
    NONE,
    VISUAL_MODE,
    OPEN_PARENT_DIRECTORY,
    OPEN_CURRENT_DIRECTORY,
    SELECT_NEXT,
    SELECT_PREVIOUS,
    SELECT_FIRST,
    SELECT_LAST,
    SELECT_HIGH,
    SELECT_LOW,
    SELECT_MIDDLE,
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


class IRowInfo {
public:
    virtual int getRowCount() const = 0;
    virtual int getCurrentRow() const = 0;
};

class IRowSelectionStrategy {
public:
    virtual void selectRow(int) = 0;
};


struct IViView : ICurrentDirectoryGetter, IStatusDisplayer,
    ISearchControllerOwner, IRowSelectionStrategy, IRowInfo
{
    virtual QString getCurrentFile() const = 0;
    virtual QString getCurrentDir() const = 0;
    virtual void mkdir(const QString&) = 0;
    virtual bool changeDirectoryIfCan(const QString& dirPath) = 0;
    virtual void setColorSchemeName(const QString&) = 0;
    virtual void openCurrentDirectory() = 0;
    virtual void openParentDirectory() = 0;
    virtual void focusToCommandLine(const QString& = {}) = 0;
    virtual void activateFileViewer() = 0;
    virtual void setMultiSelectionEnabled(bool) = 0;
    virtual bool isMultiSelectionEnabled() const = 0;
    virtual bool showQuestion(const QString&) = 0;
    virtual bool isRowVisible(int) const = 0;
};


class ViModel;
struct PasteFileCommand {
    void paste();
    void pasteWithNewName(QString);

public:
    ViModel* owner;
    QString pathCopy;
};


struct Key {
    constexpr Key();
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


template<EKey key>
struct StaticKeyImpl
{
    static constexpr Key fill(Key result)
    {
        if constexpr (key == EKey::SHIFT) {
            Q_ASSERT(!result.shift);
            result.shift = true;
        } else if constexpr (key == EKey::CONTROL) {
            Q_ASSERT(!result.control);
            result.control = true;
        } else if constexpr (key == EKey::META) {
            Q_ASSERT(!result.meta);
            result.meta = true;
        } else if constexpr (isChar<key>()) {
            Q_ASSERT(result.value == EKey::NONE);
            result.value = key;
        } else {
            Q_ASSERT(!"Invalid key");
        }
        return result;
    }

    template<EKey ekey>
    static constexpr bool isChar()
    {
        return static_cast<int>(ekey) >= static_cast<int>(EKey::A) &&
               static_cast<int>(ekey) <= static_cast<int>(EKey::Z);
    }
};


template<EKey...> struct StaticKey;

template<EKey key, EKey... otherKeys>
struct StaticKey<key, otherKeys...> : StaticKeyImpl<key>
{
    inline static const Key result = fill(StaticKey<otherKeys...>::result);
};

template<EKey key>
struct StaticKey<key> : StaticKeyImpl<key>
{
    inline static const Key result = fill({});
};


enum class CompareResult
{
    FALSE,
    TRUE,
    PARTIALLY_TRUE,
};


struct KeySequence {
    using Keys = std::array<Key, 4>;

    constexpr KeySequence();

    template<typename...T>
    constexpr KeySequence(T... params)
        : keys{params...}
        , length(sizeof...(params))
    {}

    Keys::const_iterator cbegin() const;
    Keys::const_iterator cend() const;

    CompareResult compare(const KeySequence&) const;

    void operator+=(Key);

    void clear();
    bool isEmpty() const;
    size_t getLength() const;

public:
    Keys keys;
    uint8_t length;
};


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


struct SelectionData {
    std::variant<QString, QVector<QString>> data;
};


class ViModel final {
public:
    using Command = void(ViModel::*)(const QStringList&);
    using Commands = std::map<QString, Command>;
    using OperationFunction = void(ViModel::*)();
    using NormalOperations = std::array<OperationFunction, static_cast<size_t>(ENormalOperation::COUNT)>;

    explicit ViModel(IViView&);

    Commands::const_iterator cbegin() const;
    Commands::const_iterator cend() const;

    IViView& getUi();

    void handleKeyPress(Key);
    void handleCommandEnter(QString);

    void switchToNormalMode();
    void switchToCommandMode();
    void switchToSearchMode();
    void switchToFileRenameMode(QString oldName);
    void switchToVisualMode();

    SearchController& getSearchController() { return searchController; }

    bool runIfHas(const QStringList& args);

    void openCurrentDirectory();
    void openParentDirectory();
    void selectPrevious();
    void selectNext();
    void selectFirst();
    void selectLast();
    void selectHigh();
    void selectMiddle();
    void selectLow();
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

    int findHighRow(int sourceRow);
    int findLowRow(int sourceRow);
    int findMiddleRow(int sourceRow);

public:
    NormalOperations normalOperations;

private:
    IViView* view;
    SearchController searchController;
    Commands commands;
    PasteFileCommand pasteFileCommand;
    std::function<void(QString)> clStrategy;
    NormalMode normalMode;
};
