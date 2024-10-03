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
 - *start_cells*: A vector with one entry for each school day. Each entry is a list of possible starting hours for the day. It is set only when the lesson is not "fixed".
The constraints referenced below are owned by the *BasicConstraints* object.
 - *parallel*: An optional reference to a hard *SameStartingTime* constraint. The Constraint object is owned by the *BasicConstraints* object.
 - *day_constraints*: A list of references to other hard constraints which can be handled immediately, in particular to ensure lessons are on different days. The Constraint objects are owned by the *BasicConstraints* object.

**Shared with other lessons belonging to the course**

 - *teachers*: List of week-block indexes (int) of the teachers involved in the lesson
 - *atomic_groups*: List of week-block indexes (int) of the atomic groups involved in the lesson
 - *fixed_rooms*: List of week-block indexes (int) of the rooms necessary for the lesson
 - *subject*: The subject's database id (int)
