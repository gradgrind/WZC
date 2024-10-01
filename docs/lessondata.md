# The *LessonData* Structure

This structure is used when dealing with the timetable. It represents a single lesson. A lesson is here not just a teaching unit, but any activity with a justification for being represented in the timetable because it occupies "resources" (teachers, student groups and rooms).

## Member variables

 - *lesson_id*: Reference to the database entry for the lesson (int)
 - *teachers*: Week-block indexes (int) of the teachers involved in the lesson
 - *atomic_groups*: Week-block indexes (int) of the atomic groups involved in the lesson
 - *rooms_needed*: Week-block indexes (int) of the rooms necessary for the lesson

**TODO:** Make room-choice lists only soft constraints. There would probably still need to be some storage here for the chosen rooms.
 - *rooms_choice*: An optional list of room-week-block indexes (int), one of which is required.
 
 - *subject*: The subject's database id (int)
 - *tags*: An optional list of tags (string) used to identify groups of lessons
 - *length*: Lesson length (int), the number of slots occupied by this lesson
 - *fixed*: The placement of this lesson is fixed (bool)
 - *day*: The 0-based index (int) of the day on which this lesson is placed, -1 if not placed.
 - *hour*: The 0-based index of the hour at which this lessons starts (valid only if *day* is not 0).  
 - *start_cells*: A vector with one entry for each school day. Each entry is a list of possible starting hours for the day. It is set only when the lesson is not "fixed".

**TODO:** The following member is awaiting clarification of room handling
 - *rooms*: A list of the occupied rooms (valid only if the lesson is placed, i.e. *day* is not 0).

**TODO:** To what extent is the following still relevant?

The constraints referenced below are owned by the *BasicConstraints* object.
 - *parallel*: An optional reference to a hard *SameStartingTime* constraint
 - *day_constraints*: A list of references to other hard constraints which can be handled immediately.
