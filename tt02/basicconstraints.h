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

class BasicConstraints;

class Constraint
{
public:
    virtual ~Constraint() = default;

    virtual int evaluate(BasicConstraints *constraint_data) = 0;

    int penalty;
};

class SameStartingTime : public Constraint
{
public:
    SameStartingTime(QJsonObject node);

    int evaluate(BasicConstraints *constraint_data) override;
    bool test(BasicConstraints *constraint_data, int l_id, int day);

    std::vector<int> lesson_indexes;
};

struct lesson_data{
    // Only 100%-constraints are handled here.
    ~lesson_data()
    {
        //TODO: probably need to use shared pointers here, so no destructor
        // would be needed
        for (const auto *o : day_constraints) {
            delete o;
        }
    }

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

    std::shared_ptr<SameStartingTime> parallel;
    std::vector<Constraint *> day_constraints;
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
    void initial_place_lessons(time_constraints &tconstraints);

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
    void with_slots(
        std::vector<ActivitySelectionSlots> &alist,
        lesson_data *ld,
        bool starting_time);
    void find_slots(time_constraints &constraints, lesson_data *ld);
};

/*#DOC

## Idea for handling hard constraints.

When looking for slots in which to place a lesson tile there can be
constraints which limit the available days (like the different-days
constraints). If I run one of these before doing any other searches, it
could return a list of possible days, which can then restrict the range
of subsequent tests.

Initially I have the generally possible slots organized as a list of
day-lists, these latter being lists of possible hours. The day-restrictor
constraints can reduce these slots.

Then the basic tests can be run on the lesson (groups, teachers, rooms),
but only testing the subset of slots. The result will be a still more
restricted subset.

There can also be (hard-)parallel lessons. These also need to be tested,
possibly leading to even smaller slot subsets.

Actually, it might be sensible to run the parallel lessons through their
own day-restrictor constraints at the beginning of the procedure.
*/

#endif // BASICCONSTRAINTS_H
