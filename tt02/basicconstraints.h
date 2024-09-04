#ifndef BASICCONSTRAINTS_H
#define BASICCONSTRAINTS_H

#include "database.h"

struct TTSlot {
    int day, hour;
};

struct lesson_data{
    // Only 100%-constraints are handled here.
    int lesson_id;

    // The contained values (int) are the indexes into the week-blocks
    // for the corresponding items.
    std::vector<int> teachers;
    std::vector<int> groups;
    std::vector<int> rooms_needed;
    std::vector<int> rooms_choice;

//?
    int length;
    bool fixed;
    // used only for placed lessons:
    int day;
    int hour;
    std::vector<int> rooms;

//??
    std::vector<int> parallel;
    std::vector<int> different_days;
    std::vector<TTSlot> preferred_slots; // starting times!
    // PreferredSlots constraints should be converted to
    // PreferredStartingTimes.
};

class BasicConstraints
{
public:
    BasicConstraints(DBData *dbdata);

    bool test_place_lesson(lesson_data *ldata, int day, int hour);
    std::vector<int> find_clashes(lesson_data *ldata, int day, int hour);

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
//?
    // placement resources for each lesson
    //std::unordered_map<int, lesson_data> lesson_resources;

    std::unordered_map<int, int> lid2lix;
    std::vector<lesson_data> lessons;

private:
    void slot_blockers();
    void initial_place_lessons();
};

#endif // BASICCONSTRAINTS_H
