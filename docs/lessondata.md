# The *LessonData* Structure

This structure is used when dealing with the timetable. It represents a single lesson. A lesson is here not just a teaching unit, but any activity with a justification for being represented in the timetable because it occupies "resources" (teachers, student groups and rooms).

## Member variables

**Specific to the lesson**

 - *lesson_id*: Reference to the database entry for the lesson (int)
 - *flexible_room*: Index (int) of room chosen from a list of possibilities, -1 if no room. The acceptable rooms are specified in a soft constraint.
 - *tags*: An optional list of tags (string) used to identify groups of lessons
 - *length*: Lesson length (int), the number of slots occupied by this lesson
 - *fixed*: The placement of this lesson is fixed (bool)
 - *day*: The 0-based index (int) of the day on which this lesson is placed, -1 if not placed.
 - *hour*: The 0-based index of the hour at which this lessons starts (valid only if *day* is not 0). 
 - *parallel*: A list of lesson-indexes, lessons bound to the current one by a hard *SameStartingTime* constraint.

The constraints referenced below are owned by the *BasicConstraints* object.

 - *start_cells*: A vector with one entry for each school day. Each entry is a list of possible starting hours for the day. Lessons hard-constrained to have the same starting time ("parallel") share a single *start_cells* structure.
 - *day_constraints*: A list of references to other hard constraints which can be handled immediately, in particular to ensure lessons are on different days. The Constraint objects are owned by the *BasicConstraints* object.

**Shared with other lessons belonging to the course**

 - *teachers*: List of week-block indexes (int) of the teachers involved in the lesson
 - *atomic_groups*: List of week-block indexes (int) of the atomic groups involved in the lesson
 - *fixed_rooms*: List of week-block indexes (int) of the rooms necessary for the lesson
 - *subject*: The subject's database id (int)

## Initialization

When the database is processed to prepare for handling the timetable, a lot of data structures are set up, one of these being the *LessonData* items.

### Lesson-Indexes

Internally the lessons will be referenced primarily by means of an index. All *LessonData* items are stored in a vector, *lessons*, owned by the *BasicConstraints* object. The item with index 0 is not a valid lesson, this index can be used to signify "no lesson". As some lesson fields reference other lessons, all the lesson items must be allocated before some of the initialization can be performed.

To assist the conversion from the data in the database to the internal timetable structures, the *BasicConstraints* object has a mapping, *lid2lix*, which is set up during the construction of the *lessons* vector and maps the database key of the lesson to its lesson-index. The reverse mapping is available via the *lesson_id* field of the lesson.

To get the interrelationships right, the initialization loops over the lesson data more than once. This is handled at the top level by the function *localConstraints*.

In the first pass (function *initial_place_lessons*), the database "COURSES"  and their associated "LESSONS" are read to set up the *lessons* vector and add the immediately available data to the entries. "Fixed" lessons are also "placed", which means they are added to the structures governing resource allocation in the timetable (function *place_fixed_lesson*). There should be no problems with these placements, but they should certainly be checked for collisions (handle by aborting, or "unfixing" them?) in case changes to the underlying data have invalidated something.

The function *activity_slot_constraints* reads "LOCAL_CONSTRAINTS" from the database. If a constraint is "hard", it is either added directly to the lesson data or it is stored in an intermediate form in the data structure *time_constraints*. Soft constraints are added to the *general_constraints* list in the *BasicConstraints* object.

The function *initial_place_lessons2* goes over the lesson structures once more and, with the *time_constraints* information, completes the construction. The first step is to handle the groups of parallel lessons, linking them to each other and adjusting their *start_cells* so that they share the data. Then – in a separate pass over the data! – the further time constraints from the *day_constraints* list can be processed. The final adjustment of the *start_cells* involves finding all permissible slots without placing any of the non-fixed lessons (the fixed ones were placed much earlier). To conclude the initialization, all the non-fixed cells with placements are placed (if possible).

