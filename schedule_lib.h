/*
Copyright (c) 2016 by Yuandong Tian

Permission is hereby granted, free of charge, to any person obtaining a copy 
of this software and associated documentation files (the "Software"), to deal 
in the Software without restriction, including without limitation the rights 
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
of the Software, and to permit persons to whom the Software is furnished to do so, 
subject to the following conditions:

The above copyright notice and this permission notice shall be included 
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef _SCHEDULE_LIB_H_
#define _SCHEDULE_LIB_H_

#include <vector>
#include <string>
#include <sstream>

// All units are in seconds.
struct TimeSegment {
    // How long the current time segment will last.
    int duration;

    // Cool down time for the next dependent task to start.
    int cool_down;

    // If deadline is negative, them omits it. 
    int deadline;

    // The starting time of this task falls in the union of [e_i, l_i].
    // if it is empty, then there is no such constraint.
    std::vector<std::pair<int, int> > start_time_intervals;

    // Priority (1-10).
    int priority;

    // The original patterns.
    std::string pattern;

    // Default Constructor
    TimeSegment() : duration(0), cool_down(0), deadline(-1), priority(10)  { }
    std::string get_summary() const {
        std::stringstream ss;
        ss << "[" << pattern << "]";
        ss << "(";
        ss << "dur=" << duration << ",";
        ss << "cd=" << cool_down << ",";
        if (deadline >= 0) ss << "ddl=" << deadline << ",";

        if (!start_time_intervals.empty()) {
            ss << "int=";
            for (const auto& se : start_time_intervals) {
                ss << "(" << se.first << "," << se.second << ")";
            }
        }
        ss << "pr=" << priority;
        return ss.str();
    }
};

// Specifying a task.
struct Task {
    int idx;
    std::string name; 
    std::string label;
    std::string group;
    
    // Pre-req task ids
    std::vector<std::string> pre_reqs;
    std::vector<int> pre_req_indices;

    // Time specification of the task.
    TimeSegment time;
    std::string get_summary() const {
        std::stringstream ss;
        ss << time.get_summary();
        ss << "[#" << label;
        for (const auto& pre_req : pre_reqs) {
            ss << "," << pre_req;
        }
        ss << "] " << name << std::endl;
        return ss.str();
    }
};

struct Tasks {
    std::vector<Task> tasks;

    // Scheduling parameters.
    int global_start_time;
    int rest_time;
    int max_heap_size;

    Tasks() : global_start_time(0), rest_time(0), max_heap_size(500000) { } 
    std::string get_summary() const {
        std::stringstream ss;
        ss << "Start time: " << global_start_time << std::endl;
        ss << "Rest time: " << rest_time << std::endl;
        ss << "Max Heap size: " << max_heap_size << std::endl;
        for (int i = 0; i < tasks.size(); ++i) ss << tasks[i].get_summary();
        return ss.str();
    }
};

// Output schedules.
struct Schedule {
    int idx;
    int start, end;
};

// Output a complete schedule given the tasks.
struct Schedules {
    enum FinalStatus { SUCCESS = 0, INCOMPLETE = 1 };
    std::vector<Schedule> schedules;
    
    // Output statistics.
    int search_steps;
    FinalStatus status;
    std::vector<int> incomplete_tasks;
    int total_duration, used_duration;
};

bool make_schedule(const Tasks& tasks, Schedules* schedules);

std::string convert_to_time(int t); 

#endif
