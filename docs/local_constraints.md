# Local Constraints

These are constraints which are concerned with local properties of lessons: preferred time slots, relationships to other lessons which can be tested without the whole timetable being complete.

## SAME_STARTING_TIME

This specifies a list of lessons which should/must start at the same time. It can be hard or soft.

## PREFERRED_STARTING_TIMES

This specifies time slots which are permissible as starting times for a specified lesson. If this is a hard constraint, the possible starting times are saved with the lesson when the data is loaded. These can be affected by more than one constraint.

## ACTIVITIES_PREFERRED_STARTING_TIMES

This specifies time slots which are permissible as starting times for a group of lessons. The lessons concerned are specified by means of their parameters: teacher, group, subject, length, activity-tag. Only those fields with a (non-null) value are considered in the filter.

The list of affected lessons is built when the data is loaded, so the filter is only applied once, rather than every time the constraint is checked. If this is a hard constraint, the possible starting times for a lesson are saved with the lesson when the data is loaded. These can be affected by more than one constraint.

## ACTIVITIES_PREFERRED_TIME_SLOTS

This is very similar to ACTIVITIES_PREFERRED_STARTING_TIMES, but  for lessons with length > 1 it is more restrictive as it refers to the slots which may be occupied. If this is a hard constraint, the possible starting times for a lesson are determined (using the length) and saved with the lesson when the data is loaded. These can be affected by more than one constraint.
