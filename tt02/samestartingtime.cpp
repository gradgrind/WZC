#include "samestartingtime.h"
#include <qjsonarray.h>

SameStartingTime::SameStartingTime(QJsonObject node)// : Constraint()
{
    penalty = node.value("WEIGHT").toInt();
    auto llist = node.value("LESSONS").toArray();
    lesson_indexes.resize(llist.size());
    for (const auto lid : llist) {
        lesson_indexes.push_back(lid.toInt());
    }
}

//TODO
int SameStartingTime::evaluate(BasicConstraints *constraint_data) { return 0; }
