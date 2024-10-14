#ifndef SOFTACTIVITYTIMES_H
#define SOFTACTIVITYTIMES_H

#include "basicconstraints.h"

class SoftActivityTimes : public Constraint
{
public:
    SoftActivityTimes(BasicConstraints *constraint_data,
                      int weight,
                      // For each day a list of allowed time slots
                      slot_constraint &ttslots,
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

void multi_slot_constraints(
    BasicConstraints *bcp,
    std::vector<ActivitySelectionSlots> &alist,
    bool allslots // false for starting times, true for slots
);

#endif // SOFTACTIVITYTIMES_H
