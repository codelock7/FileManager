#include "commandmaster.h"
#include "platform.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QApplication>
#include "util.h"
#include "searchcontroller.h"


CommandMaster::CommandMaster(ICommandMasterOwner& newOwner)
    : owner(&newOwner)
    , searchController(newOwner)
{
    using CommandOwner = CommandMaster;

    normalOperations[static_cast<size_t>(ENormalOperation::OPEN_PARENT_DIRECTORY)] = &CommandOwner::openParentDirectory;
    normalOperations[static_cast<size_t>(ENormalOperation::OPEN_CURRENT_DIRECTORY)] = &CommandOwner::openCurrentDirectory;
    normalOperations[static_cast<size_t>(ENormalOperation::SELECT_NEXT)] = &CommandOwner::selectNext;
    normalOperations[static_cast<size_t>(ENormalOperation::SELECT_PREVIOUS)] = &CommandOwner::selectPrevious;
    normalOperations[static_cast<size_t>(ENormalOperation::SELECT_FIRST)] = &CommandOwner::selectFirst;
    normalOperations[static_cast<size_t>(ENormalOperation::SELECT_LAST)] = &CommandOwner::selectLast;
//    normalOperations[static_cast<size_t>(ENormalOperation::DELETE_FILE)] = &CommandOwner::deleteFile;
//    normalOperations[static_cast<size_t>(ENormalOperation::RENAME_FILE)] = &CommandOwner::renameFile;
    normalOperations[static_cast<size_t>(ENormalOperation::YANK_FILE)] = &CommandOwner::yankFile;
    normalOperations[static_cast<size_t>(ENormalOperation::PASTE_FILE)] = &CommandOwner::pasteFile;
    normalOperations[static_cast<size_t>(ENormalOperation::SEARCH_NEXT)] = &CommandOwner::searchNext;
    normalOperations[static_cast<size_t>(ENormalOperation::EXIT)] = &CommandOwner::exit;


    commands = Commands({
        std::make_pair(QString("cd"), &CommandOwner::changeDirectory),
        std::make_pair(QString("touch"), &CommandOwner::createEmptyFile),
        std::make_pair(QString("open"), &CommandOwner::openFile),
        std::make_pair(QString("mkdir"), &CommandOwner::makeDirectory),
        std::make_pair(QString("colorscheme"), &CommandOwner::setColorScheme)
    });

    pasteFileCommand.owner = owner;
}

CommandMaster::Commands::const_iterator CommandMaster::cbegin() const
{
    return commands.cbegin();
}

CommandMaster::Commands::const_iterator CommandMaster::cend() const
{
    return commands.cend();
}

void CommandMaster::exit()
{
    qApp->exit();
}

bool CommandMaster::runIfHas(const QStringList &args)
{
    auto iter = commands.find(args.first());
    if (iter == commands.end())
        return false;
    (this->*(iter->second))(args);
    return true;
}

void CommandMaster::openCurrentDirectory()
{
    owner->openCurrentDirectory();
}

void CommandMaster::openParentDirectory()
{
    owner->openParentDirectory();
}

void CommandMaster::selectPrevious()
{
    owner->selectPrevious();
}

void CommandMaster::selectNext()
{
    owner->selectNext();
}

void CommandMaster::selectFirst()
{
    owner->selectFirst();
}

void CommandMaster::selectLast()
{
    owner->selectLast();
}

void CommandMaster::yankFile()
{
    pasteFileCommand.pathCopy = owner->getCurrentFile();
}

void CommandMaster::pasteFile()
{
    pasteFileCommand.paste();
}

void CommandMaster::searchNext()
{
    searchController.searchNext();
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

void PasteFileCommand::paste()
{
    if (pathCopy.isEmpty())
        return;
    const QFileInfo fileInfo(pathCopy);
    const QString& newFileName = fileInfo.fileName();
    const QString& newFilePath = owner->getCurrentDirectory() / newFileName;
    if (QFile::copy(pathCopy, newFilePath))
        return;
    if (!fileInfo.exists()) {
        owner->showStatus("The file being copied no longer exists", 4);
    } else if (QFileInfo::exists(newFilePath)) {
        clFileRenameStrategy.owner = this;
        clFileRenameStrategy.initialValue = newFileName;
        owner->activateCommandLine(&clFileRenameStrategy);
        owner->showStatus("Set a new name for the destination file");
    } else {
        owner->showStatus("Enexpected copy error", 4);
    }
}

void PasteFileCommand::pasteWithNewName(QString newName)
{
    const QString& destPath = owner->getCurrentDirectory() / newName;
    if (QFile::copy(pathCopy, destPath))
        return;
    if (!QFileInfo::exists(pathCopy))
        owner->showStatus("The file being copied no longer exists", 4);
    else if (QFileInfo::exists(destPath))
        owner->showStatus("The file being copied with that name already exists", 4);
    else
        owner->showStatus("Unexpected copy error", 4);
}

QString CLFileRenameStrategy::getInitialValue() const
{
    return initialValue;
}

void CLFileRenameStrategy::handleEnter(QString newName)
{
    owner->pasteWithNewName(newName);
}
