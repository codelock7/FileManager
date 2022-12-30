#include "searchcontroller.h"


SearchController::SearchController(ISearchControllerOwner& owner)
    : owner(&owner)
{
}

void SearchController::enterSearchLine(QString line)
{
    lastSearchLine = std::move(line);
    owner->searchForward(lastSearchLine);
}

void SearchController::searchNext()
{
    owner->searchForward(lastSearchLine);
}
