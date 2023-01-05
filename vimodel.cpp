#include "vimodel.h"
#include "platform.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QApplication>
#include "util.h"
#include "searchcontroller.h"


void toStringg(EKey key, QString& result)
{
    switch (key) {
    case EKey::CONTROL:
        result += 'C';
        break;
    case EKey::SHIFT:
        result += 'S';
        break;
    case EKey::META:
        result += 'M';
        break;
    default:
        if (key >= EKey::A && key <= EKey::Z) {
            const int offset = static_cast<int>(key) - static_cast<int>(EKey::A);
            result += 'a' + offset;
        }
        break;
    }
}


QString toString(const std::vector<EKey>& keys)
{
    QString result;
    const int reserveSize = static_cast<int>(keys.size());
    Q_ASSERT(reserveSize == keys.size());
    result.reserve(reserveSize);

    for (EKey key : keys) {
        switch (key) {
        case EKey::CONTROL:
            result += 'C';
            break;
        case EKey::SHIFT:
            result += 'S';
            break;
        case EKey::META:
            result += 'M';
            break;
        default:
            if (key >= EKey::A && key <= EKey::Z) {
                const int offset = static_cast<int>(key) - static_cast<int>(EKey::A);
                result += 'a' + offset;
            }
            break;
        }
    }
    return result;
}


bool isChar(EKey key)
{
    return static_cast<int>(key) >= static_cast<int>(EKey::A) &&
           static_cast<int>(key) <= static_cast<int>(EKey::Z);
}


ViModel::ViModel(IViView& newView)
    : view(&newView)
    , searchController(newView)
{
    using CommandOwner = ViModel;

    normalMode.addCommand({ENormalOperation::VISUAL_MODE, {StaticKey<EKey::V>::result}});
    normalMode.addCommand({ENormalOperation::OPEN_PARENT_DIRECTORY, {StaticKey<EKey::H>::result}});
    normalMode.addCommand({ENormalOperation::OPEN_CURRENT_DIRECTORY, {StaticKey<EKey::L>::result}});
    normalMode.addCommand({ENormalOperation::SELECT_NEXT, {StaticKey<EKey::J>::result}});
    normalMode.addCommand({ENormalOperation::SELECT_PREVIOUS, {StaticKey<EKey::K>::result}});
    normalMode.addCommand({ENormalOperation::SELECT_FIRST, {StaticKey<EKey::G>::result, StaticKey<EKey::G>::result}});
    normalMode.addCommand({ENormalOperation::SELECT_LAST, {StaticKey<EKey::SHIFT, EKey::G>::result}});
    normalMode.addCommand({ENormalOperation::DELETE_FILE, {StaticKey<EKey::SHIFT, EKey::D>::result}});
    normalMode.addCommand({ENormalOperation::RENAME_FILE, {StaticKey<EKey::C>::result, StaticKey<EKey::W>::result}});
    normalMode.addCommand({ENormalOperation::YANK_FILE, {StaticKey<EKey::Y>::result, StaticKey<EKey::Y>::result}});
    normalMode.addCommand({ENormalOperation::PASTE_FILE, {StaticKey<EKey::P>::result}});
    normalMode.addCommand({ENormalOperation::SEARCH_NEXT, {StaticKey<EKey::N>::result}});
    normalMode.addCommand({ENormalOperation::EXIT, {StaticKey<EKey::CONTROL, EKey::Q>::result}});

    normalOperations[static_cast<size_t>(ENormalOperation::VISUAL_MODE)] = &CommandOwner::switchToVisualMode;
    normalOperations[static_cast<size_t>(ENormalOperation::OPEN_PARENT_DIRECTORY)] = &CommandOwner::openParentDirectory;
    normalOperations[static_cast<size_t>(ENormalOperation::OPEN_CURRENT_DIRECTORY)] = &CommandOwner::openCurrentDirectory;
    normalOperations[static_cast<size_t>(ENormalOperation::SELECT_NEXT)] = &CommandOwner::selectNext;
    normalOperations[static_cast<size_t>(ENormalOperation::SELECT_PREVIOUS)] = &CommandOwner::selectPrevious;
    normalOperations[static_cast<size_t>(ENormalOperation::SELECT_FIRST)] = &CommandOwner::selectFirst;
    normalOperations[static_cast<size_t>(ENormalOperation::SELECT_LAST)] = &CommandOwner::selectLast;
    normalOperations[static_cast<size_t>(ENormalOperation::DELETE_FILE)] = &CommandOwner::removeCurrent;
    normalOperations[static_cast<size_t>(ENormalOperation::RENAME_FILE)] = &CommandOwner::renameCurrent;
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

    pasteFileCommand.owner = this;
}

ViModel::Commands::const_iterator ViModel::cbegin() const
{
    return commands.cbegin();
}

ViModel::Commands::const_iterator ViModel::cend() const
{
    return commands.cend();
}

IViView &ViModel::getUi()
{
    return *view;
}

//QString toStringgg(const KeySeq& keys)
//{
//    QString result;
//    const int reserveSize = static_cast<int>(keys.length);
//    Q_ASSERT(reserveSize == keys.length);
//    result.reserve(reserveSize);

//    for (EKey key : keys.value) {
//        switch (key) {
//        case EKey::CONTROL:
//            result += 'C';
//            break;
//        case EKey::SHIFT:
//            result += 'S';
//            break;
//        case EKey::META:
//            result += 'M';
//            break;
//        default:
//            if (key >= EKey::A && key <= EKey::Z) {
//                const int offset = static_cast<int>(key) - static_cast<int>(EKey::A);
//                result += 'a' + offset;
//            }
//            break;
//        }
//    }
//    return result;
//}

void ViModel::handleKeyPress(Key key)
{
    switch (key.value) {
    case EKey::ESCAPE:
        switchToNormalMode();
        break;

    case EKey::COLON:
        switchToCommandMode();
        break;

    case EKey::SLASH:
        switchToSearchMode();
        break;

    default:
        if (isChar(key.value)) {
            normalMode.addKey(key);
            const NormalMode::Status status = normalMode.handle();
            if (std::get<bool>(status)) {
                const auto& operation = std::get<ENormalOperation>(status);
                switch (operation) {
                case ENormalOperation::NONE:
                    return;
                default:
                    Q_ASSERT(operation != ENormalOperation::COUNT);
                    const size_t i = static_cast<size_t>(operation);
                    auto ptr = normalOperations[i];
                    ((this)->*ptr)();
                    break;
                }
            }
            normalMode.reset();
            break;
        }
        break;
    }
}

void ViModel::handleCommandEnter(QString line)
{
    Q_ASSERT(clStrategy);
    clStrategy(std::move(line));
    switchToNormalMode();
}

void ViModel::switchToNormalMode()
{
    qDebug("Normal mode");
    view->activateFileViewer();
    if (view->isMultiSelectionEnabled()) {
        view->setMultiSelectionEnabled(false);
        view->selectRow(view->getCurrentRow());
    }
}

void ViModel::switchToCommandMode()
{
    qDebug("Command mode");
    clStrategy = [&](QString line) {
        if (line.isEmpty())
            return;
        const QStringList& args = line.split(' ', QString::SkipEmptyParts);
        if (args.isEmpty())
            return;
        if (!runIfHas(args))
            view->showStatus("Unknown command", 4);
    };
    view->focusToCommandLine();
}

void ViModel::switchToSearchMode()
{
    qDebug("Search mode");
    clStrategy = [&](QString line) {
        if (line.isEmpty())
            return;
        searchController.enterSearchLine(std::move(line));
    };
    view->focusToCommandLine();
}

void ViModel::switchToFileRenameMode(QString oldName)
{
    clStrategy = [&](QString newName) {
        pasteFileCommand.pasteWithNewName(newName);
    };
    view->focusToCommandLine(oldName);
}

void ViModel::switchToVisualMode()
{
    view->setMultiSelectionEnabled(true);
}

void ViModel::exit()
{
    qApp->exit();
}

bool ViModel::runIfHas(const QStringList &args)
{
    auto iter = commands.find(args.first());
    if (iter == commands.end())
        return false;
    (this->*(iter->second))(args);
    return true;
}

void ViModel::openCurrentDirectory()
{
    view->openCurrentDirectory();
}

void ViModel::openParentDirectory()
{
    view->openParentDirectory();
}

void ViModel::selectPrevious()
{
    const int currRow = view->getCurrentRow();
    if (currRow == -1) {
        view->selectRow(0);
        return;
    }
    view->selectRow(currRow - 1);
}

void ViModel::selectNext()
{
    const int currRow = view->getCurrentRow();
    if (currRow == -1) {
        view->selectRow(0);
        return;
    }
    view->selectRow(currRow + 1);
}

void ViModel::selectFirst()
{
    view->selectRow(0);
}

void ViModel::selectLast()
{
    const int rowCount = view->getRowCount();
    view->selectRow(rowCount - 1);
}

void ViModel::yankFile()
{
    pasteFileCommand.pathCopy = view->getCurrentFile();
}

void ViModel::pasteFile()
{
    pasteFileCommand.paste();
}

void ViModel::searchNext()
{
    searchController.searchNext();
}

void ViModel::renameCurrent()
{
    const QFileInfo fi(view->getCurrentFile());
    view->focusToCommandLine(fi.fileName());
    clStrategy = [=](QString newName) {
        if (!newName.isEmpty())
            QFile::rename(fi.filePath(), fi.path() / newName);
    };
}

void ViModel::removeCurrent()
{
    const QFileInfo fi(view->getCurrentFile());
    if (!view->showQuestion(QString("Do you want to remove?\n%1").arg(fi.fileName())))
        return;
    if (fi.isDir())
        QDir(fi.filePath()).removeRecursively();
    else
        QFile::remove(fi.filePath());
}

void ViModel::createEmptyFile(const QStringList& args)
{
    const QString& currDir = view->getCurrentDirectory();
    for (int i = 1; i < args.length(); ++i) {
        QFile newFile(currDir / args[i]);
        newFile.open(QIODevice::WriteOnly);
    }
}

void ViModel::changeDirectory(const QStringList& args)
{
    if (args.size() != 2) {
        view->showStatus("Invalid command signature", 4);
        return;
    }
    QString newDirPath = args[1];
    QFileInfo newDirInfo(newDirPath);
    if (newDirInfo.isRelative()) {
        newDirInfo.setFile(view->getCurrentDirectory() / newDirPath);
        newDirPath = newDirInfo.absoluteFilePath();
    }
    if (!newDirInfo.exists()) {
        view->showStatus("Target directory does not exist", 4);
        return;
    }
    if (!view->changeDirectoryIfCan(newDirPath))
        view->showStatus("Unexpected error", 4);
}

void ViModel::openFile(const QStringList&)
{
    Platform::open(view->getCurrentFile().toStdWString().c_str());
}

void ViModel::makeDirectory(const QStringList& args)
{
    for (int i = 1; i < args.length(); ++i)
        view->mkdir(args[i]);
}

void ViModel::setColorScheme(const QStringList& args)
{
    if (args.size() != 2) {
        view->showStatus("Invalid command signature", 4);
        return;
    }
    view->setColorSchemeName(args.back());
}

void PasteFileCommand::paste()
{
    if (pathCopy.isEmpty())
        return;
    const QFileInfo fileInfo(pathCopy);
    const QString& newFileName = fileInfo.fileName();
    const QString& newFilePath = owner->getUi().getCurrentDirectory() / newFileName;
    if (QFile::copy(pathCopy, newFilePath))
        return;
    if (!fileInfo.exists()) {
        owner->getUi().showStatus("The file being copied no longer exists", 4);
    } else if (QFileInfo::exists(newFilePath)) {
        owner->switchToFileRenameMode(newFileName);
        owner->getUi().showStatus("Set a new name for the destination file");
    } else {
        owner->getUi().showStatus("Enexpected copy error", 4);
    }
}

void PasteFileCommand::pasteWithNewName(QString newName)
{
    const QString& destPath = owner->getUi().getCurrentDirectory() / newName;
    if (QFile::copy(pathCopy, destPath))
        return;
    if (!QFileInfo::exists(pathCopy))
        owner->getUi().showStatus("The file being copied no longer exists", 4);
    else if (QFileInfo::exists(destPath))
        owner->getUi().showStatus("The file being copied with that name already exists", 4);
    else
        owner->getUi().showStatus("Unexpected copy error", 4);
}


constexpr KeySequence::KeySequence()
    : length(0)
{}

KeySequence::Keys::const_iterator KeySequence::cbegin() const
{
    return keys.cbegin();
}

KeySequence::Keys::const_iterator KeySequence::cend() const
{
    return keys.cbegin() + length;
}

CompareResult KeySequence::compare(const KeySequence& rhs) const
{
    const size_t len = std::min(length, rhs.length);
    for (size_t i = 0; i < len; ++i)
        if (keys[i] != rhs.keys[i])
            return CompareResult::FALSE;
    if (length != rhs.length)
        return CompareResult::PARTIALLY_TRUE;
    return CompareResult::TRUE;
}

void KeySequence::operator+=(Key rhs)
{
    keys[length++] = rhs;
}

void KeySequence::clear()
{
    length = 0;
}

bool KeySequence::isEmpty() const
{
    return length == 0;
}

size_t KeySequence::getLength() const
{
    return length;
}

void NormalMode::addCommand(NormalOperation operation)
{
    operations.push_back(std::move(operation));
}

bool NormalMode::isEmptySequence() const
{
    return keySequence.isEmpty();
}

void NormalMode::addKey(Key key)
{
    keySequence += key;
}

NormalMode::Status NormalMode::handle()
{
    auto result = std::make_pair(false, ENormalOperation::NONE);
    for (const NormalOperation& operation : operations) {
        switch (operation.compare(keySequence)) {
        case CompareResult::FALSE:
            break;
        case CompareResult::PARTIALLY_TRUE:
            std::get<bool>(result) = true;
            break;
        case CompareResult::TRUE:
            std::get<bool>(result) = true;
            std::get<ENormalOperation>(result) = operation.getType();
            break;
        }
    }
    return result;
}

void NormalMode::reset()
{
    keySequence.clear();
}

NormalOperation::NormalOperation(ENormalOperation operation, KeySequence keySequence)
    : operation(operation)
    , keySequence(std::move(keySequence))
{
}

CompareResult NormalOperation::compare(const KeySequence& rhs) const
{
    return keySequence.compare(rhs);
}

ENormalOperation NormalOperation::getType() const
{
    return operation;
}

constexpr Key::Key()
    : value(EKey::NONE)
    , shift(false)
    , control(false)
    , meta(false)
{}

Key::Key(std::initializer_list<EKey> keys)
    : Key()
{
    for (const EKey& key : keys)
        this->operator+=(key);
    Q_ASSERT(value != EKey::NONE);
}

bool Key::operator==(Key rhs) const
{
    return value == rhs.value &&
           shift == rhs.shift &&
           control == rhs.control &&
           meta == rhs.meta;
}

bool Key::operator!=(const Key& rhs) const
{
    return !this->operator==(rhs);
}

void Key::operator+=(EKey key)
{
    switch (key) {
    case EKey::SHIFT:
        Q_ASSERT(!shift);
        shift = true;
        break;

    case EKey::CONTROL:
        Q_ASSERT(!control);
        control = true;
        break;

    case EKey::META:
        Q_ASSERT(!meta);
        meta = true;
        break;

    default:
        Q_ASSERT(value == EKey::NONE);
        value = key;
    }
}
