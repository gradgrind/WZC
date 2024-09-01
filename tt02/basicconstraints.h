#ifndef BASICCONSTRAINTS_H
#define BASICCONSTRAINTS_H

#include "database.h"
#include "with_roaring/roaring.hh"

struct lesson_data{
    std::vector<int> teachers;
    std::vector<int> groups;
    std::vector<int> rooms;
    std::vector<std::vector<int>> roomspec;
};

class BasicConstraints
{
public:
    BasicConstraints(DBData *dbdata);

    DBData * db_data;
    int ndays;
    int nhours;
    // group index -> list of atomic subgroups:
    QHash<int, QList<QString>> g2sg;
    // atomic subgroup -> week-block index:
    QHash<QString, int> sg2i;
    // week-blocks for atomic subgroups:
    std::vector<std::vector<std::vector<int>>> sg_weeks;
    // teacher index -> week-block index:
    QHash<int, int> t2i;
    // week-block index -> teacher index:
    QList<int> i_t;
    // week-blocks for teachers:
    std::vector<std::vector<std::vector<int>>> t_weeks;
    // room index -> week-block index:
    QHash<int, int> r2i;
    // week-blocks for rooms:
    std::vector<std::vector<std::vector<int>>> r_weeks;
    // placement resources for each lesson
    std::unordered_map<int, lesson_data> lesson_resources;

private:
    void slot_blockers();
    void place_lessons();
};

#endif // BASICCONSTRAINTS_H
