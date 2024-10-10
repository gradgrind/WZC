#ifndef BASICCONSTRAINTS_H
#define BASICCONSTRAINTS_H

#include "database.h"

struct TTSlot {
    int day, hour;
};

inline bool is_hard(int w) { return (w == 10); };

struct ActivitySelectionSlots {
    int weight;
    QString tag;
    int tid;
    int gid;
    int sid;
    int l;
    // This has a list for each day containing the allowed times
    std::vector<std::vector<int>> ttslots;

    bool isHard() { return is_hard(weight); }
};

// Restrict possible starting times on the basis of various constraints
// on activities.
struct time_constraints {
    // Collect groups of hard-parallel lessons for later processing
    std::vector<std::vector<int>> parallel_lessons;
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
    virtual bool test(BasicConstraints *constraint_data, int l_ix, int day)
        { return false; }
    virtual bool testx(
            BasicConstraints *constraint_data, int l_ix, int day,
            std::vector<int> &conflicts)
        { return false; }
    bool isHard() { return is_hard(penalty); }

protected:
    int penalty;
};

class SameStartingTime : public Constraint
{
public:
    SameStartingTime(std::vector<int> &lesson_indexes, int weight);
    //~SameStartingTime() { qDebug() << "~SameStartingTime"; }

    int evaluate(BasicConstraints *constraint_data) override;

    std::vector<int> lesson_indexes;
};

class SoftActivityTimes : public Constraint
{
public:
    SoftActivityTimes(BasicConstraints *constraint_data,
        int weight,
        // For each day a list of allowed time slots
        std::vector<std::vector<int>> &ttslots,
        // The times can refer to starting times or permissible
        // slots, true for slots
        bool allslots // false for starting times, true for slots
    );
    //~SoftActivityTimes() { qDebug() << "~SoftActivityTimes"; }

    void add_lesson_id(
        BasicConstraints *constraint_data, int lesson_id);
    int evaluate(BasicConstraints *constraint_data) override;

    std::vector<int> lesson_indexes;
    // This should have a full week of slots containing true for
    // permitted slots
    std::vector<std::vector<bool>> week_slots;
    const bool all_slots;
};

struct LessonData{
    // Only 100%-constraints are handled here.
    int lesson_id;

    // The contained values (int) are the indexes into the week-blocks
    // for the corresponding items (not the db ids).
    std::vector<int> teachers;
    std::vector<int> atomic_groups;
    std::vector<int> fixed_rooms;
//    std::vector<int> rooms_choice; // -> soft constraint?

    int subject; // the subject's db id
    QStringList tags;
    int length;
    bool fixed{false};
    // This has an array of possible starting hours for each day,
    // irrelevant when "fixed". For parallel lessons it is shared.
    // The vector is owned by the BasicConstraints object.
    std::vector<std::vector<int>> *start_cells;

    int day{-1}; // -1 indicates unplaced lesson
    int hour;
    int flexible_room{-1};

    std::vector<int> parallel_lessons;  // indexes of hard-parallel lessons

    // Certain hard constraints are relevant for placement of the lesson, in
    // particular when other lessons should have the same starting time
    // or be on distinct days. These constraints are accessible via the
    // references in day_constraints. For "fixed" lessons these are not
    // relevant.
    // The referenced constraints are owned by BasicConstraints, so no
    // destructor is needed here.
    std::vector<Constraint *> day_constraints;
};

class BasicConstraints
{
public:
    BasicConstraints(DBData *dbdata);
    ~BasicConstraints() {
        for (const auto &p : general_constraints) delete p;
        for (const auto &p : local_hard_constraints) delete p;
    }

    // Use the hard day constraints with the currently placed lessons
    // to reduce the available slots. The array passed as reference in
    // start_slots may be modified.
    void filter_day_slots(
        std::vector<std::vector<int>> &start_slots,
        int lesson_index);
    // Returns a list of possible starting slots in found_slots.
    void find_slots(
        std::vector<std::vector<int>> &start_slots,
        int lesson_index);
    std::vector<TTSlot> found_slots;

//TODO: All needed?
    bool test_possible_place(LessonData &ldata, int day, int hour);
    bool test_place(LessonData &ldata, int day, int hour);
    void initial_place_lessons();
    void initial_place_lessons2(time_constraints &tconstraints);
    bool test_single_slot(LessonData &ldata, int day, int hour);
    std::vector<int> find_clashes(LessonData *ldata, int day, int hour);

    DBData * db_data;
    int ndays;
    int nhours;
    // group id -> list of atomic subgroups:
    QHash<int, QList<QString>> g2sg;
    // atomic subgroup -> week-block index:
    QHash<QString, int> sg2i;
    // week-block index -> atomic subgroup:
    QList<QString> i_sg;
    // week-blocks for atomic subgroups:
    std::vector<std::vector<std::vector<int>>> sg_weeks;
    // teacher id -> week-block index:
    QHash<int, int> t2i;
    // week-block index -> teacher id:
    QList<int> i_t;
    // week-blocks for teachers:
    std::vector<std::vector<std::vector<int>>> t_weeks;
    // room id -> week-block index:
    QHash<int, int> r2i;
    // week-block index -> room id:
    QList<int> i_r;
    // week-blocks for rooms:
    std::vector<std::vector<std::vector<int>>> r_weeks;

    std::unordered_map<int, int> lid2lix;
    std::vector<LessonData> lessons;
    std::vector<Constraint *> general_constraints;
    std::vector<Constraint *> local_hard_constraints;

    void set_start_cells_array(int lid, std::vector<std::vector<int>> &days);

private:
    void place_fixed_lesson(int lesson_index);
    void multi_slot_constraints(
        std::vector<ActivitySelectionSlots> &alist,
        bool allslots // false for starting times, true for slots
    );
    void slot_blockers();
    // The actual object referenced by the return pointer is owned by
    // start_cells_arrays:
    std::vector<std::vector<int>> * defaultStartCells(int lesson_index);
    void setup_parallels(std::vector<std::vector<int>> &parallels);
    // start_cells_arrays is primarily provided to manage the memory for
    // the start_cells arrays of the lessons. It is a mapping because of the
    // way it is built and used during construction.
    // The keys are lesson indexes (in the lessons vector).
    std::unordered_map<int, std::vector<std::vector<int>>> start_cells_arrays;
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
