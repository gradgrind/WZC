# Lesson Placement

Determining where to place the lessons – without breaking "local" hard constraints – needs to be efficient and fast. "Local" refers here to constraints which can be tested without having the whole timetable available. Before performing a placement, the destination slot needs to be checked, primarily for "collisions" (teachers, students and rooms can only be in any slot once). For reasons of efficiency it may also make sense to check other hard constraints before performing a placement.

## Find available slots

When placing lessons automatically, each will normally be put in an available slot, if there is one. Typically the possible slots would be found and one of these chosen according to some algorithm, possibly just a random choice.

Also when performing manual placements it is useful to know which time-slots are available without clashes.

A function, *find_slots*, is provided to determine which slots are possible destinations for a given lesson. If the lesson has (hard) "parallel" lessons (same starting time), also these must be checked, and if the lesson covers multiple slots (double lesson, etc.), they must all be tested.

Each lesson has a data structure, *start_cells*, which lists those time-slots where the lesson can be placed without (hard) conflicts. It does not, however, cover conflicts with non-fixed lessons. If there are hard-parallel lessons, these share a single *start_cells* structure.

Thus the hard constraints concerning the placement of a lesson which can be determined statically (without taking non-fixed lessons into account) are effectively bundled into the structure referenced by *start_cells*. Those hard constraints which depend on non-fixed lessons – collisions (which must take all parallel lessons and every period of a multi-period lesson into account) and different-day constraints — must be tested at every placement.

The hard constraints listed for each lesson in its *day_constraints* structure ensure that particular lessons are not placed on the same day (or, potentially, have a minimum number of days between them). They can make testing slots on particular days unnecessary and should possibly be performed before the detailed testing of each slot.

## Testing whether a particular placement is possible

If the test is part of a search for available slots, it may be handled a bit differently from the case where just a single slot is tested.

### Testing a time-slot as part of a search for placement possibilities

The reduction of time-slots by the *day_constraints* can be performed as mentioned above, then each slot under consideration will simply be tested for conflicts. This can be faster than a detailed test because the test can be aborted as soon as the first conflict is found, and no details of the conflict are required.

## When no slot is available for a lesson

If there is no slot available, an alternative algorithm would be needed to decide where to place the lesson, replacing one or more others. The chosen placement should definitely be one of those in the static *start_cells* structure, to ensure no clashes with fixed lessons. Again, a random choice would be possible, or one which causes a minimum of disruption (in some sense). The latter would probably require an analysis of all the slots under consideration. The simple random choice would only need to investigate the chosen destination.

### Detailed testing of a time-slot

If a lesson is to be placed at a time which is not actually available to the lesson because of conflicts with other non-fixed lessons, it is necessary to know what is causing the conflicts. In some cases it may be enough to know just the lessons concerned, so that removing these would enable the placement. For automatic placement that might be enough. Especially for manual placement it might also be helpful to know which resource or constraint is causing the clash.

Thus there will probably be two versions of the placement test for a particular slot.

As the bulk of the clash testing concerning fixed items is achieved via the *start_cells* structure, getting details of clashes with fixed items (should this be wanted) may be a bit more complicated.
