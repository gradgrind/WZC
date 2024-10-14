#ifndef SAMESTARTINGTIME_H
#define SAMESTARTINGTIME_H

#include "basicconstraints.h"

class SameStartingTime : public Constraint
{
public:
    SameStartingTime(std::vector<int> &lesson_indexes, int weight);
    //~SameStartingTime() { qDebug() << "~SameStartingTime"; }

    int evaluate(BasicConstraints *constraint_data) override;

    std::vector<int> lesson_indexes;
};


#endif // SAMESTARTINGTIME_H
