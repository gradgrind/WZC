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

    int nrooms = ldata.rooms_needed.size();
//TODO: It might be sensible to do the resizing just once, when the lesson
// is created.
    if (ldata.rooms_choice.empty()) ldata.rooms.resize(nrooms);
    else ldata.rooms.resize(nrooms + 1);
    for (int i = 0; i < nrooms; ++i) {
        int r = ldata.rooms_needed[i];
        basic_constraints->r_weeks[r][newday][newhour] = lesson;
        ldata.rooms[i] = r;
    }
    if (!ldata.rooms_choice.empty()) {
        //TODO: Use first available one?
        for (int r : ldata.rooms_choice) {
            if (basic_constraints->r_weeks[r][newday][newhour] < 0) {
                basic_constraints->r_weeks[r][newday][newhour] = lesson;
                ldata.rooms[nrooms] = r;
                goto chosen;
            }
        }
        //TODO: Clarify the handling of unallocated rooms! Maybe one of the
        // choices should be placed here even though it is already in use?
        ldata.rooms[nrooms] = -1;
    chosen:;
    }
}

//TODO: Then the corresponding tile will need placing and showing.
