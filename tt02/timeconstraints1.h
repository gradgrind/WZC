#ifndef TIMECONSTRAINTS1_H
#define TIMECONSTRAINTS1_H

#include "basicconstraints.h"
#include <vector>

class DifferentDays : public Constraint
{
public:
    DifferentDays();

    int evaluate(BasicConstraints *constraint_data) override;
    bool test(BasicConstraints *constraint_data, int l_id, int day);

private:
    std::vector<int> lesson_indexes;
    int gap;
};

class SameStartingTime : public Constraint
{
public:
    SameStartingTime(QJsonObject node);

    int evaluate(BasicConstraints *constraint_data) override;
    bool test(BasicConstraints *constraint_data, int l_id, int day);

private:
    std::vector<int> lesson_indexes;
};

#endif // TIMECONSTRAINTS1_H
