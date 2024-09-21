#ifndef SAMESTARTINGTIME_H
#define SAMESTARTINGTIME_H

#include "basicconstraints.h"

class SameStartingTime : public Constraint
{
public:
    SameStartingTime(QJsonObject node);

    int evaluate(BasicConstraints *constraint_data) override;
    bool test(BasicConstraints *constraint_data, int l_id, int day);

private:
    std::vector<int> lesson_indexes;
};

#endif // SAMESTARTINGTIME_H
