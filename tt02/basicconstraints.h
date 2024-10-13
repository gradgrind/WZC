#ifndef BASICCONSTRAINTS_H
#define BASICCONSTRAINTS_H

#include "database.h"

// Vector has one list per day of available slots
typedef std::vector<std::vector<int>> slot_constraint;

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
    slot_constraint ttslots;

    bool isHard() { return is_hard(weight); }
};

// Restrict possible starting times on the basis of various constraints
// on activities.
struct time_constraints {
    // Collect groups of hard-parallel lessons for later processing
    slot_constraint parallel_lessons;
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

protected:
    int penalty;
};

struct LessonData{
    // Only hard constraints are handled here.
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

    int day{-1}; // -1 indicates unplaced lesson
    int hour;           // only valid when day >= 0
    int flexible_room;  // only valid when day >= 0

    // This indexes the structure start_cells_list in the BasicConstraints
    // object. The indexed item contains a list of possible starting hours for
    // each day, irrelevant when "fixed". For parallel lessons it is shared.
    int slot_constraint_index{0};
    // Indexes of lessons which may not be on the same day:
    std::vector<int> different_days;
    // Indexes of hard-parallel lessons:
    std::vector<int> parallel_lessons;
};

struct FlexiRoom {
    int lesson_index;
    int room_index;
};

class BasicConstraints
{
public:
    BasicConstraints(DBData *dbdata);
    ~BasicConstraints() {
        for (const auto &p : general_constraints) delete p;
    }

    QString pr_lesson(int lix);
    QString pr_week_block_sg(int ix);

    void update_db_field(int id, QString field, QJsonValue val);
    void remove_db_field(int id, QString field);

    void set_start_cells_id(int lesson_id, slot_constraint &week_slots);
    void set_different_days(std::vector<int> &lesson_ids);
    bool test_possible_place(LessonData &ldata, int day, int hour);
    void initial_place_lessons();
    void initial_place_lessons2(time_constraints &tconstraints);
    bool test_single_slot(LessonData &ldata, int day, int hour);
    std::map<int, std::string> find_clashes(
        int lesson_index, int day, int hour);

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
    std::vector<slot_constraint> sg_weeks;
    // teacher id -> week-block index:
    QHash<int, int> t2i;
    // week-block index -> teacher id:
    QList<int> i_t;
    // week-blocks for teachers:
    std::vector<slot_constraint> t_weeks;
    // room id -> week-block index:
    QHash<int, int> r2i;
    // week-block index -> room id:
    QList<int> i_r;
    // week-blocks for rooms:
    std::vector<slot_constraint> r_weeks;

    std::unordered_map<int, int> lid2lix;
    std::vector<LessonData> lessons;
    std::vector<Constraint *> general_constraints;

    std::vector<TTSlot> available_slots(int lesson_index);
    void merge_slot_constraints(
        LessonData &ldata, const slot_constraint &newslots);
    void setup_parallels(slot_constraint &parallels);

private:
    void find_clashes2(
        std::map<int, std::string> &clashmap,
        LessonData &ldata, int day, int hour);
    void place_lesson(int lesson_index);
    void place_fixed_lesson(int lesson_index);
    void slot_blockers();
    int set_start_cells(LessonData &ldata, slot_constraint &week_slots);
    // start_cells_list contains the slot_constraint objects for the lessons.
    // Indexing is by index. The first entry is a default entry which blocks
    // and shouldn't be changed.
    std::vector<slot_constraint> start_cells_list;

    std::vector<FlexiRoom> flexirooms; // used internally for pending rooms
    // Return a list of possible starting slots in found_slots:
    void find_slots(int lesson_index);
    std::vector<TTSlot> found_slots;
    std::vector<bool> blocked_days; // used internally by find_slots
    // Test the given slot:
    bool test_slot(int lesson_index, int day, int hour);
};

#endif // BASICCONSTRAINTS_H
