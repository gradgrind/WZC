#ifndef BASICCONSTRAINTS_H
#define BASICCONSTRAINTS_H

#include "database.h"
#include "with_roaring/roaring.hh"

struct lesson_data{
    // The contained values (int) are the indexes into the week-blocks
    // for the corresponding items.
    std::vector<int> teachers;
    std::vector<int> groups;
    std::vector<int> rooms;
    std::vector<std::vector<int>> roomspec;
};

class BasicConstraints
{
public:
    BasicConstraints(DBData *dbdata);

    bool test_place_lesson(lesson_data *ldata, int day, int hour);

    DBData * db_data;
    int ndays;
    int nhours;
    // group index -> list of atomic subgroups:
    QHash<int, QList<QString>> g2sg;
    // atomic subgroup -> week-block index:
    QHash<QString, int> sg2i;
    // week-block index -> atomic subgroup:
    QList<QString> i_sg;
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
    // week-block index -> room index:
    QList<int> i_r;
    // week-blocks for rooms:
    std::vector<std::vector<std::vector<int>>> r_weeks;
    // placement resources for each lesson
    std::unordered_map<int, lesson_data> lesson_resources;

private:
    void slot_blockers();
    void initial_place_lessons();
};

#endif // BASICCONSTRAINTS_H
