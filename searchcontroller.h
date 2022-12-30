#pragma once

#include <QString>


class ISearchControllerOwner {
public:
    virtual void searchForward(const QString&) = 0;
};


class SearchController {
public:
    explicit SearchController(ISearchControllerOwner&);
    void enterSearchLine(QString);
    void searchNext();

private:
    ISearchControllerOwner* owner;
    QString lastSearchLine;
};
