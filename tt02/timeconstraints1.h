#ifndef TIMECONSTRAINTS1_H
#define TIMECONSTRAINTS1_H

#include "basicconstraints.h"
#include <vector>

class Constraint
{
public:
    virtual int evaluate(BasicConstraints *constraint_data) = 0;

protected:
    int penalty;
};

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

#endif // TIMECONSTRAINTS1_H
