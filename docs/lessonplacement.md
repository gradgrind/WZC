# Lesson Placement

Determining where to place the lessons – without breaking "local" hard constraints – needs to be efficient and fast. Before performing a placement, the destination slot needs to be checked, primarily for "collisions" (teachers, students and rooms can only be in any slot once). For reasons of efficiency it may also make sense to check other hard constraints before performing a placement.

A function, *find_slots*, is provided to determine which slots are possible destinations for a given lesson. If the lesson has (hard) "parallel" lessons, also these must be checked, and if the lesson covers multiple slots, they must all be tested.

By considering certain hard constraints which limit the availability of slots for a teacher, students group or room it is possible to reduce the number of slots which must be tested. The permissible starting slots for the lesson are available in the structure *start_cells*. Also the fixed lessons are taken into consideration when building this structure.