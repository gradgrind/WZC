# Basics

## The database

The basis for the timetable is the database. This is essentially a collection of items represented as JSON objects. Each of these has an (integer) key which can be used to reference it in other objects.

A convenient way to store the database is as a simple table ("NODES") in an SQLite database, the table having just two fields, "Id" for the key and "DATA" for the JSON object. However, as the key is also present in the "DATA" objects it could all be stored as a JSON array.

### Object types

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

The database objects are used to build the internal structures necessary for displaying, editing and constructting the timetable.

#### DAYS

 - X (int): The 0-based index of the day within the school week.
 - TAG (string): The short name for the day.
 - NAME (string): THe full name for the day.

#### HOURS

 - X (int): The 0-based index of the hour within the school day.
 - TAG (string): The short name for the hour.
 - NAME (string): The full name for the hour.
 - START_TIME (string, optional): The time (h:mm) at which the hour starts.
 - END_TIME (string, optional): The time (h:mm) at which the hour ends.

#### SUBJECTS

 - X (int): The 0-based index of the subject, for ordering.
 - TAG (string): The short name (abbreviation) for the subject.
 - NAME (string): THe full name for the subject.

#### TEACHERS

 - X (int): The 0-based index of the teacher, for ordering.
 - TAG (string): The short name (abbreviation) for the teacher.
 - NAME (string): THe full name for the teacher (not used for timetable processing, can be empty).
 - NOT_AVAILABLE: An Array of pairs, each pair containing a day and hour in which the teacher is not available. The day and hour members are keys to the node table.

#### ROOMS
