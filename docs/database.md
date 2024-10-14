# The database

The basis for the timetable is the database. This is essentially a collection of items represented as JSON objects. Each of these has an (integer) key which can be used to reference it in other objects.

A convenient way to store the database is as a simple table ("NODES") in an SQLite database, the table having just two fields, "Id" for the key and "DATA" for the JSON object. However, as the key is also present in the "DATA" objects it could all be stored as a JSON array.

When the raw database is loaded into the program a data structure is created which does some processing of the raw data to provide more convenient access. See [The DBData Structure](dbdata.md#the-dbdata-structure).

## Object types

Apart from its main content each object has an "Id" field, which is just the object's key, and a "TYPE" field, which serves to group similar objects together. The currently supported types are listed below:

 - DAYS
 - HOURS
 - SUBJECTS
 - TEACHERS
 - ROOMS
 - CLASSES
 - GROUPS
 - COURSES
 - LESSONS
 - LOCAL_CONSTRAINTS

The database objects are used to build the internal structures necessary for displaying, editing and constructing the timetable.

### DAYS

 - X (int): The 0-based index of the day within the school week.
 - TAG (string): The short name for the day.
 - NAME (string): THe full name for the day.

### HOURS

 - X (int): The 0-based index of the hour within the school day.
 - TAG (string): The short name for the hour.
 - NAME (string): The full name for the hour.
 - START_TIME (string, optional): The time (h:mm) at which the hour starts.
 - END_TIME (string, optional): The time (h:mm) at which the hour ends.

### SUBJECTS

 - X (int): The 0-based index of the subject, for ordering.
 - TAG (string): The short name (abbreviation) for the subject, its primary identifier.
 - NAME (string): THe full name for the subject.

### TEACHERS

 - X (int): The 0-based index of the teacher, for ordering.
 - TAG (string): The short name (abbreviation) for the teacher, their primary identifier.
 - NAME (string): THe full name for the teacher (not used for timetable processing, can be empty).
 - NOT_AVAILABLE: An Array of pairs, each pair containing a day and hour in which the teacher is not available. The day and hour members are keys to the node table.

### ROOMS

 - X (int): The 0-based index of the room, for ordering.
 - TAG (string): The short name (abbreviation) for the room, its primary identifier.
 - NAME (string): The full name for the room (not used for timetable processing, can be empty).
 - NOT_AVAILABLE: An Array of pairs, each pair containing a day and hour in which the room is not available. The day and hour members are keys to the node table.
 - ROOMS_NEEDED: An optional Array of ROOMS keys. When present it defines the room as a room group, specifying that it is not a real room itself, but requires multiple real rooms.
 - $REF (string, optional): Where the data is imported from another program, this can be used as to reference a room (or room group) in the other program.

### CLASSES

 - X (int): The 0-based index of the class, for ordering.
 - TAG (string): The short name (abbreviation) for the class, its primary identifier.
 - NAME (string): The full name for the class (not used for timetable processing, can be empty).
 - DIVISIONS: An Array of Objects describing the ways in which the class can be divided into groups. Each Object has a "DivTag" field (string), which is just a name for the division, and a "Groups" field, which is an Array of GROUPS keys. The order of the groups should be retained in the display of divided lessons. The "whole class" group has the special DivTag "*" and only one GROUPS key.

### GROUPS

 - TAG (string): The short name (abbreviation) for the group, its primary identifier. This does not include the class.
 - CLASS (int). The "CLASSES" key of the class to which this group belongs.
 - NOT_AVAILABLE: An Array of pairs, each pair containing a day and hour in which the group is not available. The day and hour members are keys to the node table.
 - STUDENTS: An Array of the "STUDENTS" keys of the students belonging to this group. This is not relevant for the timetable and can be empty.
 - SUBGROUPS: An Array of strings representing the "atomic" subgroups of students comprising this group. The exact nature of the strings is not relevant, but it can be helpful if the class tag is included somehow.

**Atomic Subgroups** are here defined as the groups of students which are never divided in lessons. This corresponds roughly to the Cartesian product of the class divisions and can be constructed from these. They are useful in several places in the timetable algorithms.
 

### STUDENTS

TODO: Not yet defined, unnecessary for the timetable.

### COURSES

Note that entries of this type need not be courses in the classical sense, but types of activities which bind resources (teachers and/or students and/or rooms) under a given name (the "SUBJECT").

 - SUBJECT (int): SUBJECTS key, the course subject.
 - TEACHERS: An Array of TEACHERS keys, the teachers involved.
 - GROUPS: An Array of GROUPS keys, the student groups involved.
 - ROOMSPEC: An Array of ROOMS keys. **TODO**: Currently this is a bit complicated, I might rework it. It is probably a bit of a compromise, allowing a compulsory room (which can be a room group, thus multiple compulsory rooms) and/or (?) a single choice from a list of possible rooms (not room groups).

### LESSONS

Note that the entries of this type need not be lessons in the classical sense, but activities in general which bind resources (teachers and/or students and/or rooms) in timetable slots.

 - COURSE (int): The COURSES key of the course to which this lesson belongs.
 - LENGTH (int): Number of hours/periods covered by the lesson.
 - FIXED (bool): Determines whether the lesson may be moved.
 - DAY (int): 0-based index of the weekday, -1 if the lesson is unplaced.
 - HOUR (int) 0-based index of the hour/period within the school day, undefined if the lesson is unplaced.
 - ROOMS: An Array of ROOMS keys, the rooms occupied by the lesson. The entries must be real rooms (not room groups) and are only relevant when the lesson is placed (DAY >= 0).
 - ACTIVITY_TAG0S: An Array of strings used to build groups of lessons.
 - $REF (string, optional): Where the data is imported from another program, this can be used as to reference a lesson in the other program.

### LOCAL_CONSTRAINTS

 - CTYPE (string): The type of constraint.
 - WEIGHT (int): The "weight" of the constraint, i.e. some measure of its importance. It is basically the penalty score if the constraint fails. A value of 0 means the constraint is ineffective (not active).

Hard constraints currently have weight 10, but a larger value may be desirable, to give a wider range.

There will be other fields, based on the CTYPE field. See the [Constraints](constraints.md#constraints) documentation.