# The *BasicConstraints* Structure

This structure is initialized by calling the constructor with a reference to the database as argument. It retains this reference as a member variable, *db_data*.

## Other member variables

 - *ndays*: Set to the number of school days in each week.
 - *nhours*: Set to the maximum number of lesson slots in a school day.
 - *g2sg*: Mapping, group id (int) -> list of atomic groups (strings)
 - *sg2i*: Mapping, atomic group (string) -> atomic-group-week-block index (int)
 - *i_sg*: Vector of atomic groups (strings), used to map week-block indexes to atomic groups
 - *sg_weeks*: Vector of atomic-group-week-blocks
 - *t2i*: Mapping, teacher id (int) -> teacher-week-block index (int)
 - *i_t*: Vector of teacher ids (ints), used to map week-block indexes to teacher ids
 - *t_weeks*: Vector of teacher-week-blocks
 - *r2i*: Mapping, (real) room id (int) -> room-week-block index (int)
 - *i_r*: Vector of (real) room ids (ints), used to map week-block indexes to room ids
 - *r_weeks*: Vector of room-week-blocks
 - *lid2lix*: Mapping, lesson id -> lesson index
 - *lessons*: Vector of *LessonData* structures. The 0-indexed entry is a dummy, so that index 0 is invalid (used in references to signify "no lesson").

The week-blocks are 2D arrays indexed by day and hour. The individual cells contain references to "lessons", 0 if no reference. 

**TODO:** Check the correctness of the following constraint structures:
 - *general_constraints*: Vector of *Constraint* references. This is the structure that "owns" these *Constraint* objects.
 - *local_hard_constraints*: Vector of *Constraint* references. This is the structure that "owns" these *Constraint* objects.
    
