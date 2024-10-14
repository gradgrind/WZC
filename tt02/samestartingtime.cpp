#include "samestartingtime.h"

SameStartingTime::SameStartingTime(std::vector<int> &l_indexes, int weight)
{
    penalty = weight;
    lesson_indexes = l_indexes;
    //TODO--
    //    qDebug() << "SameStartingTime" << penalty << lesson_indexes;
}

//TODO
int SameStartingTime::evaluate(BasicConstraints *constraint_data) { return 0; }

