#ifndef BASICCONSTRAINTS_H
#define BASICCONSTRAINTS_H

#include "database.h"

struct TTSlot {
    int day, hour;
};

struct ActivitySelectionSlots {
    QString tag;
    int tid;
    int gid;
    int sid;
    int l;
    std::vector<std::vector<int>> ttslots;
};

struct time_constraints {
    std::unordered_map<int, std::vector<std::vector<int>>> lesson_starting_times;
    std::vector<ActivitySelectionSlots> activities_starting_times;
    std::vector<ActivitySelectionSlots> activities_slots;
    //TODO ...
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

    int subject;
    QStringList tags;
    int length;
    bool fixed = false;
    std::vector<std::vector<int>> start_cells; // set only when not "fixed"
    // used only for placed lessons:
//TODO: Actually, at least "day" should always be set, to -1 for unplaced
// lessons.
    int day;
    int hour;
    std::vector<int> rooms;

//??
    std::vector<int> parallel;
    std::vector<int> different_days;
};

class BasicConstraints
{
public:
    BasicConstraints(DBData *dbdata);

    bool test_single_slot(lesson_data *ldata, int day, int hour);
    std::vector<int> find_clashes(lesson_data *ldata, int day, int hour);
    std::vector<std::vector<int>> find_possible_places(lesson_data *ldata);
    bool test_possible_place(lesson_data *ldata, int day, int hour);
    bool test_place(lesson_data *ldata, int day, int hour);

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
    time_constraints activity_slot_constraints();
    void initial_place_lessons(time_constraints &tconstraints);
    void with_slots(
        std::vector<ActivitySelectionSlots> &alist,
        lesson_data *ld,
        bool starting_time);
    void find_slots(time_constraints &constraints, lesson_data *ld);
};

#endif // BASICCONSTRAINTS_H
