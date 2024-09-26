#include "differentdays.h"
#include <qjsonarray.h>

/*
 * This is an important constraint which could be implemented in various ways.
 * Especially important – and presumably common – is the version with one-day
 * separation and a hard requirement. It might be sensible to implement this
 * version as part of the main allocation strategy even though that might be
 * a bit complicated.
 * As the comparisons are different, it might make sense to have a separate
 * constraint for gaps of more than one day ("DaysBetween"?).
 * There are also various conceivable invocations. The simplest is perhaps
 * the evaluation scenario when all lessons have been placed. In that case
 * it would just be a question of checking that each covered lesson has a
 * different day (gap=1) or that the days between satisfy the constraint.
 * This invocation would return a standard penalty (0 if OK or if disabled).
 * Such an invocation would need the placements of all the component lessons.
 * The placements are available in the vector "BasicConstraints::lessons",
 * which contains "lesson_data" structures (including day and hour) – see
 * "basicconstraints.h".
 *
 */
DifferentDays::DifferentDays(
    BasicConstraints *constraint_data,
    QJsonObject node) : Constraint()
{
    penalty = node.value("WEIGHT").toInt();
    gap = node.value("NDAYS").toInt();
    auto llist = node.value("LESSONS").toArray();
    int n = llist.size();
    lesson_indexes.resize(n);
    for (int i = 0; i < n; ++i) {
        lesson_indexes[i] = constraint_data->lid2lix[llist[i].toInt()];
    }
//TODO--
//    qDebug() << "DifferentDays" << penalty << lesson_indexes;
}

// Handle constraints "different days" and "days between".
int DifferentDays::evaluate(BasicConstraints *constraint_data)
{
    std::vector<int> dlist;
    dlist.reserve(lesson_indexes.size());
    for (int lid : lesson_indexes) {
        int d = constraint_data->lessons[lid].day;
        if (d < 0) continue; // unplaced lesson
        for (int dd : dlist) {
            //TODO: It is perhaps possible that more than two lessons share a day.
            // Should this increase the penalty?
            if (dd == d) return penalty; // "different days" only
            if (abs(dd - d) < gap) return penalty; // any gap
        }
        dlist.push_back(d);
    }
    return 0;
}

//TODO: I'm not sure about this one. If it is used for a lessons which is
// currently placed, that placement would block a placement on the same day!
// This method would be called by the allocation algorithm, before all
// lessons are placed. As such it would only be used for hard constraints,
// so the penalty value is irrelevant. Also, any included lessons will
// already satisfy the constraint. It is a test to discover whether a new
// lesson placement also satisfies the constraint. But the lesson to be
// tested can already have a placement.
// I don't really want to change the lesson data before the new placement is
// done, so maybe I would need, for example, the lesson-id too. There may,
// however, be a better way of doing it ...
bool DifferentDays::test(BasicConstraints *constraint_data, int l_id, int day)
{
    for (int lid : lesson_indexes) {
        int d = constraint_data->lessons[lid].day;
        if (d < 0) continue; // unplaced lesson
        if (lid == l_id) {
            if (d == day) return true;
            continue;
        }
        if (d == day) {
            return false; // "different days" only
        }
        if (abs(day - d) < gap) return false; // any gap
    }
    return true;
}
