#ifndef BASICCONSTRAINTS_H
#define BASICCONSTRAINTS_H

#include "database.h"
#include "with_roaring/roaring.hh"

class BasicConstraints
{
public:
    BasicConstraints(DBData *dbdata);

    DBData * db_data;
    int ndays;
    int nhours;
    QHash<QString, int> sg2i;
    std::vector<std::vector<std::vector<int>>> sg_weeks;
    QHash<int, int> t2i;
    std::vector<std::vector<std::vector<int>>> t_weeks;
    QHash<int, int> r2i;
    std::vector<std::vector<std::vector<int>>> r_weeks;

private:
    void slot_blockers();
};

#endif // BASICCONSTRAINTS_H
