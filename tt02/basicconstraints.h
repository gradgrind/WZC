#ifndef BASICCONSTRAINTS_H
#define BASICCONSTRAINTS_H

#include "database.h"

struct TTSlot {
    int day, hour;
};

struct WeightedHour {
    int weight, hour;
};

struct ActivitySelectionSlots {
    int weight;
    QString tag;
    int tid;
    int gid;
    int sid;
    int l;
    std::vector<std::vector<int>> ttslots;

    bool isHard() { return (weight == 10); }
};

struct LessonStartingSlots {
    int weight;
    std::vector<std::vector<int>> days;

    bool isHard() { return (weight == 10); }
};

// Restrict possible starting times on the basis of various constraints
// on activities.
struct time_constraints {
    // From constraint Activity has preferred starting times
    std::unordered_map<int, LessonStartingSlots> lesson_starting_times;
    // From constraint Activities have preferred starting times
    std::vector<ActivitySelectionSlots> activities_starting_times;
    // From constraint Activities have preferred slots
    std::vector<ActivitySelectionSlots> activities_slots;
};

class BasicConstraints; // forward declaration

class Constraint
{
public:
    virtual ~Constraint() = default;

    virtual int evaluate(BasicConstraints *constraint_data) = 0;
    bool isHard() { return (penalty == 10); }

protected:
    int penalty;
};

class SameStartingTime : public Constraint
{
public:
    SameStartingTime(
        BasicConstraints *constraint_data,
        QJsonObject node);
    //~SameStartingTime() { qDebug() << "~SameStartingTime"; }

    int evaluate(BasicConstraints *constraint_data) override;
    bool test(BasicConstraints *constraint_data, int l_id, int day);

    std::vector<int> lesson_indexes;
};

class SoftActivityTimes : public Constraint
{
public:
    SoftActivityTimes(
        BasicConstraints *constraint_data,
        int weight,
        std::vector<std::vector<int>> days,
        std::vector<int> lesson_ids,
        bool allslots
    );
    //~SoftActivityTimes() { qDebug() << "~SoftActivityTimes"; }

    int evaluate(BasicConstraints *constraint_data) override;

    std::vector<int> lesson_indexes;
    const std::vector<std::vector<int>> week_slots;
    const bool all_slots;
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
    int day = -1; // -1 indicates unplaced lesson
    int hour;
    std::vector<int> rooms;

    // The referenced constraints are owned by BasicConstraints, so no
    // destructor is needed here.
    SameStartingTime *parallel = nullptr;
    std::vector<Constraint *> day_constraints;
    std::vector<LessonStartingSlots> soft_start_cells;
};

class BasicConstraints
{
public:
    BasicConstraints(DBData *dbdata);
    ~BasicConstraints() {
        qDebug() << "~BasicConstraints"
                 << general_constraints.size()
                 << local_hard_constraints.size();
        for (const auto &p : general_constraints) delete p;
        for (const auto &p : local_hard_constraints) delete p;
    }

    bool test_single_slot(lesson_data *ldata, int day, int hour);
    std::vector<int> find_clashes(lesson_data *ldata, int day, int hour);
    std::vector<std::vector<int>> find_possible_places(lesson_data *ldata);
    bool test_possible_place(lesson_data *ldata, int day, int hour);
    bool test_place(lesson_data *ldata, int day, int hour);
    std::vector<int> initial_place_lessons();
    void initial_place_lessons2(
        std::vector<int> to_place, time_constraints &tconstraints);

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

    std::unordered_map<int, int> lid2lix;
    std::vector<lesson_data> lessons;
    std::vector<Constraint *> general_constraints;
    std::vector<Constraint *> local_hard_constraints;

private:
    void slot_blockers();
};

/*#DOC

## Idea for handling hard constraints.

When looking for slots in which to place a lesson tile there can be
constraints which limit the available days (like the different-days
constraints). If I run one of these before doing any other searches, it
could return a list of possible days, which can then restrict the range
of subsequent tests.

Initially I have the generally possible slots organized as a list of
day-lists, these latter being lists of possible hours. The day-restricting
constraints can reduce these slots.

Then the basic tests can be run on the lesson (groups, teachers, rooms),
but only testing the subset of slots. The result will be a still more
restricted subset.

There can also be (hard-)parallel lessons. These also need to be tested,
possibly leading to even smaller slot subsets.

Actually, it might be sensible to run the parallel lessons through their
own day-restricting constraints at the beginning of the procedure.
*/

#endif // BASICCONSTRAINTS_H
