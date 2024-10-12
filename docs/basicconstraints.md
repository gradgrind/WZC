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
 - *general_constraints*: Vector of *Constraint* references. This is the structure that "owns" these *Constraint* objects.
 
The week-blocks are 2D arrays indexed by day and hour. The individual cells contain references to "lessons", 0 if no reference. 
   
## Representing the Timetable State

The database contains all the necessary information about the timetable. While working on the timetable it is convenient to have other data structures, especially ones which can be accessed quickly, To this end the data in the database is read into the internal structures centred on a *BasicConstraints* object and a series of *LessonData* objects.

### The Week-Blocks

Teachers, student groups and rooms are "primary" resources in that each of the individual elements of these categories can only be associated with a single lesson in any time-slot. This assumption is taken as a very hard constraint, so that no lesson may be placed at a particular time when that would result in one of these elements being "double-booked". To give automatic timetable generation a chance, checking these constraints needs to be very fast. This could be done in a number of ways, but for the present I have chosen the week-blocks.

Each element (teacher, ["atomic" student group](atomic_groups.md#atomic-student-groups) and room) has an array of cells, each cell representing a time-slot, with days and hours (teaching periods) as axes. The arrays are organized contiguously in memory, forming, in effect, 3-D arrays. There is a separate such array for each of the categories. The first index is the element, the second the day, the third the hour. In these week-blocks, 0 is a valid index, referring to the first entry in the array.

The cells of these arrays contain 0 by default, which indicates that the time-slot is not occupied. Otherwise they contain lesson indexes, or the special value -1.

The [*LessonData*](lessondata.md#the-lessondata-structure) structures contain a lot of significant information about the lessons. Also these structures are arranged contiguously in memory to allow simple indexed access. The first entry (index 0) is not a valid lesson, allowing the use of index 0 to indicate "no lesson". The lesson index in a week-block cell specifies which lesson is using that resource in this time slot.