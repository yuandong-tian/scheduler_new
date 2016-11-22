# scheduler_new
New version of scheduler. See https://github.com/yuandong-tian/scheduler for the old version.

The motivation of this project is to schedule a set of todo tasks, possibly dependent, for the rest of the day. The program takes a list of tasks as the input, and output a schedule, starting from right now.

Example Input and Output
---
Input:

(Assuming it is 8:00am in the morning)

```
[8:30a=1h~10m] Task 1
[9:30a=30m~15m] Task 2
[1h] Task 3
[3h<10:00] Task 4
[2h>12:00] Task 5
[x11:20] Task 6
```

Output:
```
 Task 1 [     8:30a =1h ~10m ]   08:20 - 09:20
 Task 2 [    9:30a =30m ~15m ]   09:25 - 09:55
 Task 4 [          3h <10:00 ]   10:00 - 13:00
 Task 3 [                 1h ]   13:05 - 14:05
 Task 5 [          2h >12:00 ]   14:10 - 16:10
```

Compilation and Example
----

```bash
make
./run.sh
```

Usage
------
Each row looks like the following:  
`[Scheduling][Dependency] Task name`

Scheduling:

| Symbol | Meaning | Example
|--------|---------|--------
| a      |  morning | 7:00a
| p      | afternoon| 3:15p
| m      | minutes  | 30m
| h      | hours    | 1h
| s      | seconds  | 30s
| >      | Start after | >12:00
| <      | Start before| <15:00
| =      | How long this task takes | =1h
| ~      | Uncertainty of the starting time | ~20m
| +      | Cool down before next dependent task | +20m
| $      | Deadline (finish before)  | $16:00

Dependency:

| Symbol | Meaning | Example
|--------|---------|--------
| #      | tag     | #task1
| ,      | dependent tags.| ,task2

Note that dependency means the current task starts only after *all* tasks with the dependent tags are completed (plus their respective cooldown).


Example:
1. `[8:30a=1h~10m] Task 1`   
Task 1 starts at 8:20-8:40am, and last for 1 hour.
2. `[1h30m] Task 2`  
Task 2 lasts for 1.5 hours. Starting time is flexible.
3. `[30m<13:00] Task 3`   
Task 3 lasts for 30 minutes and needs to start before 13:00
4. `[50m$16:00] Task 4`  
Task 4 lasts for 50 minutes and shall be completed before 16:00.

Example that uses dependency:
1. `[20m+10m][#first] Task 1`   
   `[30m][#second,first] Task 2`   
Task 2 starts after at least 10 minutes after the completion of Task 1, which takes 20 minutes.

License
----------

MIT License
