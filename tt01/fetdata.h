#ifndef FETDATA_H
#define FETDATA_H

#include "readxml.h"

struct Day {
    int index;
    QString Name;
    QString Long_Name;
};

struct Hour {
    int index;
    QString Name;
    QString Long_Name;
    QString start;
    QString end;
};

class FetData
{
public:
    FetData(XMLNode xmlin);
};

#endif // FETDATA_H
