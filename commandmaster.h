#pragma once
#include <map>
#include <QStringList>


class ICommandMasterOwner {
public:
    virtual QString getCurrentDirectory() const = 0;
    virtual void showStatus(const QString&, int secTimeout = 0) = 0;
    virtual QString getCurrentFile() const = 0;
    virtual void mkdir(const QString&) = 0;
    virtual bool changeDirectoryIfCan(const QString& dirPath) = 0;
    virtual void setColorSchemeName(const QString&) = 0;
};


template<>
struct std::less<QString> {
    bool operator()(const QString& lhs, const QString& rhs) const { return lhs < rhs; }
};


class CommandMaster final {
public:
    using Command = void(CommandMaster::*)(const QStringList&);
    using Commands = std::map<QString, Command>;

    explicit CommandMaster(ICommandMasterOwner&);

    Commands::const_iterator cbegin() const;
    Commands::const_iterator cend() const;

    bool runIfHas(const QStringList& args);

    void createEmptyFile(const QStringList& args);
    void changeDirectory(const QStringList& args);
    void openFile(const QStringList& args);
    void makeDirectory(const QStringList& args);
    void setColorScheme(const QStringList&);

private:
    ICommandMasterOwner* owner;
    Commands commands;
};
