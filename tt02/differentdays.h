#ifndef DIFFERENTDAYS_H
#define DIFFERENTDAYS_H

#include "basicconstraints.h"

class DifferentDays : public Constraint
{
public:
    DifferentDays(
        BasicConstraints *constraint_data,
        QJsonObject node);
    //~DifferentDays() { qDebug() << "~DifferentDays"; }

    int evaluate(BasicConstraints *constraint_data) override;
    bool test(BasicConstraints *constraint_data, int l_id, int day);

private:
    std::vector<int> lesson_indexes; // no fixed lessons
    int gap;
    std::vector<bool> fixed; // flag days blocked by fixed lessons
};

#endif // DIFFERENTDAYS_H
