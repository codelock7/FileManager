#include "commandmaster.h"
#include "platform.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include "util.h"


CommandMaster::CommandMaster(ICommandMasterOwner& owner)
    : owner(&owner)
{
    using CommandOwner = CommandMaster;
    commands = Commands({
        std::make_pair(QString("cd"), &CommandOwner::changeDirectory),
        std::make_pair(QString("touch"), &CommandOwner::createEmptyFile),
        std::make_pair(QString("open"), &CommandOwner::openFile),
        std::make_pair(QString("mkdir"), &CommandOwner::makeDirectory),
        std::make_pair(QString("colorscheme"), &CommandOwner::setColorScheme)
    });
}

CommandMaster::Commands::const_iterator CommandMaster::cbegin() const
{
    return commands.cbegin();
}

CommandMaster::Commands::const_iterator CommandMaster::cend() const
{
    return commands.cend();
}

bool CommandMaster::runIfHas(const QStringList &args)
{
    auto iter = commands.find(args.first());
    if (iter == commands.end())
        return false;
    (this->*(iter->second))(args);
    return true;
}

void CommandMaster::createEmptyFile(const QStringList& args)
{
    const QString& currDir = owner->getCurrentDirectory();
    for (int i = 1; i < args.length(); ++i) {
        QFile newFile(currDir / args[i]);
        newFile.open(QIODevice::WriteOnly);
    }
}

void CommandMaster::changeDirectory(const QStringList& args)
{
    if (args.size() != 2) {
        owner->showStatus("Invalid command signature", 4);
        return;
    }
    QString newDirPath = args[1];
    QFileInfo newDirInfo(newDirPath);
    if (newDirInfo.isRelative()) {
        newDirInfo.setFile(owner->getCurrentDirectory() / newDirPath);
        newDirPath = newDirInfo.absoluteFilePath();
    }
    if (!newDirInfo.exists()) {
        owner->showStatus("Target directory does not exist", 4);
        return;
    }
    if (!owner->changeDirectoryIfCan(newDirPath))
        owner->showStatus("Unexpected error", 4);
}

void CommandMaster::openFile(const QStringList&)
{
    Platform::open(owner->getCurrentFile().toStdWString().c_str());
}

void CommandMaster::makeDirectory(const QStringList& args)
{
    for (int i = 1; i < args.length(); ++i)
        owner->mkdir(args[i]);
}

void CommandMaster::setColorScheme(const QStringList& args)
{
    if (args.size() != 2) {
        owner->showStatus("Invalid command signature", 4);
        return;
    }
    owner->setColorSchemeName(args.back());
}
