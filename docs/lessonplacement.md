# Lesson Placement

Determining where to place the lessons – without breaking "local" hard constraints – needs to be efficient and fast. "Local" refers here to constraints which can be tested without having the whole timetable available. Before performing a placement, the destination slot needs to be checked, primarily for "collisions" (teachers, students and rooms can only be in any slot once). For reasons of efficiency it may also make sense to check other hard constraints before performing a placement.

## Find available slots

When placing lessons automatically, each will normally be put in an available slot, if there is one. Typically the possible slots would be found and one of these chosen according to some algorithm, possibly just a random choice.

Also when performing manual placements it is useful to know which time-slots are available without conflicts.

A function, *available_slots* (a wrapper around *find_slots*), is provided to determine which slots are possible destinations for a given lesson. If the lesson has (hard) "parallel" lessons (same starting time), also these must be checked, and if the lesson covers multiple slots (double lesson, etc.), they must all be tested.

Each lesson uses a data structure, indexed by *slot_constraint_index*, which lists those time-slots where the lesson can be placed without (hard) conflicts. It does not, however, cover conflicts with non-fixed lessons. If there are hard-parallel lessons, these share a single *slot_constraint* structure.

Thus the hard constraints concerning the placement of a lesson which can be determined statically (without taking non-fixed lessons into account) are effectively bundled into the *slot_constraint* structure. Those hard constraints which depend on non-fixed lessons – collisions (which must take all parallel lessons and every period of a multi-period lesson into account) and different-day constraints — must be tested at every placement.

The lessons listed for each lesson in *different_days* may not be placed on the same day. They can make testing slots on particular days unnecessary and should possibly be checked before the detailed testing of each slot.

## Testing whether a particular placement is possible

If the test is part of a search for available slots, it may be handled a bit differently from the case where just a single slot is tested.

### Testing a time-slot as part of a search for placement possibilities

The reduction of time-slots by the *different_days* list can be performed as mentioned above, then each slot under consideration will simply be tested for conflicts. This can be faster than a detailed test because the test can be aborted as soon as the first conflict is found, and no details of the conflict are required.

## When no slot is available for a lesson

If there is no slot available, an alternative algorithm would be needed to decide where to place the lesson, replacing one or more others. The chosen placement should definitely be one of those in the static *slot_constraint* structure, to ensure no clashes with fixed lessons. Again, a random choice would be possible, or one which causes a minimum of disruption (in some sense). The latter would probably require an analysis of all the slots under consideration. The simple random choice would only need to investigate the chosen destination.

### Detailed testing of a time-slot

If a lesson is to be placed at a time which is not actually available to the lesson because of conflicts with other non-fixed lessons, it is necessary to know what is causing the conflicts. In some cases it may be enough to know just the lessons concerned, so that removing these would enable the placement. For automatic placement that might be enough. Especially for manual placement it might also be helpful to know which resource or constraint is causing the clash.

Thus there will probably be two versions of the placement test for a particular slot.

As the bulk of the clash testing concerning fixed items is achieved via the *slot_constraint* structure, getting details of clashes with fixed items (should this be wanted) may be a bit more complicated.

## Actual Placement of a Lesson

Once it is clear that a placement is possible (if necessary, by removing other lessons), the data structures can be updated to include the new placement.

 - LessonData.day: Set to the (0-based) index of the day.
 - LessonData.hour: Set to the (0-based) index of the hour.
 - LessonData.flexible_room: Set to -1. If there is a list of rooms from which one is to be chosen (in the database that is the optional "ROOM_CHOICE" field of the **course**), this will be set later by the corresponding soft constraint, when the timetable is complete.
 - BasicConstraints.sg_weeks: The lesson index will be placed in the appropriate cells, i.e. for each of the atomic groups involved in the lesson.
 - BasicConstraints.t_weeks: The lesson index will be placed in the appropriate cells, i.e. for each of the teachers involved in the lesson.
 - BasicConstraints.r_weeks: The lesson index will be placed in the appropriate cells, i.e. for each of the fixed rooms involved in the lesson.

In addition, the database must be updated. When performing automatic placement, this will be postponed until the operation is complete, updating the whole lot in one go. In the case of manual placement, the database should be updated immediately – and, of course, the GUI too.

The database fields that need setting are in the "LESSONS" record, fields "DAY" and "HOUR". If there is a flexible room which gets set, also "FLEXIBLE_ROOM" must be set. Note that the values which get placed in these fields are the database keys (id) of the corresponding records. For the day and hour this can be got by means of the *dix_id* and *hix_id* lists of the database *DBData* object. For rooms the database keys can be got from *BasicConstraints.i_r*.

## Removing a Lesson

This is basically the reverse of the placement.

 - LessonData.day: Set to -1.
 - LessonData.hour: not relevant.
 - LessonData.flexible_room: not relevant.
 - BasicConstraints.sg_weeks: Set the appropriate cells to 0, i.e. for each of the atomic groups involved in the lesson.
 - BasicConstraints.t_weeks: Set the appropriate cells to 0, i.e. for each of the teachers involved in the lesson.
 - BasicConstraints.r_weeks: Set the appropriate cells to 0, i.e. for each of the fixed rooms involved in the lesson.

In the database, the fields in the "LESSONS" record must be removed: "DAY", "HOUR" and, if present, "FLEXIBLE_ROOM".


## Special considerations

When placing a lesson which occupies more than one time slot, the operation (placing or removing) must be performed on all the relevant time slots.

If there are hard-parallel lessons, the operation must also be performed on each of these.
