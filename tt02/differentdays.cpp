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
    QJsonObject node)
{
    penalty = node.value("WEIGHT").toInt();
    gap = node.value("NDAYS").toInt();
    // Flag days occupied by fixed lessons,
    // add non-fixed lessons to "lesson_indexes".
    fixed.resize(constraint_data->ndays, false);
    const auto llist = node.value("LESSONS").toArray();
    for (const auto &i : llist) {
        int lix = constraint_data->lid2lix[i.toInt()];
        const LessonData &ld{constraint_data->lessons[lix]};
        if (ld.fixed) {
            fixed[ld.day] = true;
        } else {
            lesson_indexes.push_back(lix);
        }
    }
}

// Count the lessons not meeting the criterion, summing the penalties.
int DifferentDays::evaluate(BasicConstraints *constraint_data)
{
    std::vector<bool> dmap{fixed};
    int penalties{0};
    for (int lix : lesson_indexes) {
        int d = constraint_data->lessons[lix].day;
        if (d < 0) continue; // unplaced lesson
        if (dmap[d]) {
            penalties += penalty;
            continue;
        }
        if (gap > 1) {
            for (int gg = 1; gg < gap; ++gg) {
                int dd = d - gg;
                if (dd >= 0 && dmap[dd]) {
                    penalties += penalty;
                    break;
                }
                dd = d + gg;
                if (dd < constraint_data->ndays && dmap[dd]) {
                    penalties += penalty;
                    break;
                }
            }
        }
        dmap[d] = true;
    }
    return penalties;
}
