#include "placelesson.h"

// Place the given lesson in the specified slot.
// Important: assume that the hard constraints are fulfilled (i.e. that they
// have already been checked).

//TODO: The DAYS_BETWEEN constraints (well, all constraints in the
// day_constraints field) would need checking, presumably before calling this
// function?
void placeLesson(
    BasicConstraints *basic_constraints, int lesson, int newday, int newhour)
{

    //TODO: Should the lesson be currently unplaced, or can its removal be
    // handled here too?

    auto &ldata = basic_constraints->lessons[lesson];
    ldata.day = newday;
    ldata.hour = newhour;
    // Note that no checks are done here, so the prior checking of the
    // validity of this operation is very important!
    for (int x : ldata.teachers) {
        basic_constraints->t_weeks[x][newday][newhour] = lesson;
    }
    for (int x : ldata.atomic_groups) {
        basic_constraints->sg_weeks[x][newday][newhour] = lesson;
    }
    for (int r : ldata.fixed_rooms) {
        basic_constraints->r_weeks[r][newday][newhour] = lesson;
    }
}

//TODO: Then the corresponding tile will need placing and showing.
