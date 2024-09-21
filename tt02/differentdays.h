#ifndef DIFFERENTDAYS_H
#define DIFFERENTDAYS_H

#include "basicconstraints.h"

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

#endif // DIFFERENTDAYS_H
