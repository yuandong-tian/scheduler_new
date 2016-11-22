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

#include <iostream>
#include <iomanip>
#include <regex>
#include <sstream>
#include <time.h>
#include <map>
#include "schedule_lib.h"

using namespace std;

string trim(string& str) {
    str.erase(0, str.find_first_not_of(' '));       //prefixing spaces
    str.erase(str.find_last_not_of(' ')+1);         //surfixing spaces
    return str;
}

vector<string> split(const string &s, char delim) {
    stringstream ss(s);
    string item;
    vector<string> elems;
    while (getline(ss, item, delim)) {
        elems.push_back(move(item)); 
    }
    return elems;
}

int read_duration(const string& t, const string& suffix) {
    // cout << "Read duration " << t << " " << suffix << endl;
    int duration = stoi(t);
    if (!suffix.empty()) {
        if (suffix[0] == 'm') duration *= 60;
        else if (suffix[0] == 'h') duration *= 3600; 
    }
    return duration;
}

int read_time(const string& s1, const string& s2, const string& suffix) {
    // cout << "Read time " << s1 << " " << s2 << " " << suffix << endl;
    int part1 = stoi(s1);
    int part2 = stoi(s2);
    if (!suffix.empty() && suffix[0] == 'p') part1 += 12;

    return part1 * 3600 + part2 * 60;
}

bool set_task_time(const string &s, TimeSegment *t) {
    regex pattern("([\\+~><=\\$xcl]?)(\\d+):?(\\d+)?([smhap]?)");
    auto words_begin = sregex_iterator(s.begin(), s.end(), pattern);
    auto words_end = sregex_iterator();

    t->pattern = "";

    int start_time = -1;
    int uncertainty = 0;
    int after = -1;
    int before = -1;

    int duration = 0;
    int deadline = -1;
    int cool_down = 0;

    int priority = 10;
    bool involved = true;

    for (auto it = words_begin; it != words_end; ++it) {
        smatch sm = *it;
        t->pattern += sm[0].str() + " ";

        if (sm[1].str().empty()) {
            if (sm[3].str().empty()) {
                char prefix = sm[4].str().empty() ? 's' : sm[4].str()[0];
                if (prefix == 'm' || prefix == 'h' || prefix == 's') {
                    duration += read_duration(sm[2].str(), sm[4].str());
                } else { 
                    start_time = read_time(sm[2].str(), sm[3].str(), sm[4].str());
                }
            } else {
                start_time = read_time(sm[2].str(), sm[3].str(), sm[4].str());
            } 
            continue;
        }
        // First command
        switch(sm[1].str()[0]) {
            case 'x':
            case 'c':
                involved = false;
                break;
            case '+':
                // Cooldown time. 
                cool_down = read_duration(sm[2].str(), sm[4].str());
                break;
            case '~':
                // Uncertainty.
                uncertainty = read_duration(sm[2].str(), sm[4].str());
                break;
            case '>':
                // After given time.
                after = read_time(sm[2].str(), sm[3].str(), sm[4].str());
                break;
            case '<':
                // Before given time.
                before = read_time(sm[2].str(), sm[3].str(), sm[4].str());
                break;
            case '=':
                // Duration 
                // Note that this is accumulative.
                duration += read_duration(sm[2].str(), sm[4].str());
                break;
            case '$':
                // Deadline
                deadline = read_time(sm[2].str(), sm[3].str(), sm[4].str());
                break;
            case 'l':
                // Priority
                priority = stoi(sm[2].str());
                break;
        }
    }

    if (!involved) return false;

    // Then we set the structure accordingly.
    t->duration = duration;
    t->cool_down = cool_down;
    t->deadline = deadline;
    t->priority = priority;

    if (start_time >= 0) {
        // Start time and duration.
        t->start_time_intervals.push_back(make_pair(start_time - uncertainty, start_time + uncertainty));
    } else if (after >= 0) {
        t->start_time_intervals.push_back(make_pair(after, numeric_limits<int>::max()));
    } else if (before >= 0) {
        t->start_time_intervals.push_back(make_pair(0, before));
    }
    return true;
}

void set_task_labels(const string& dep_str, Task *task) {
    regex pattern("([#,])([A-Za-z0-9\\-_]+)");
    auto words_begin = sregex_iterator(dep_str.begin(), dep_str.end(), pattern);
    auto words_end = sregex_iterator();

    for (auto it = words_begin; it != words_end; ++it) {
        smatch sm = *it;
        switch(sm[1].str()[0]) {
            case '#':
                // self_label.
                task->label = sm[2].str();
                break;
            case ',':
                // dependency label.
                task->pre_reqs.push_back(sm[2].str());
                break;
        }
    }

    trim(task->label);
    for (auto & d : task->pre_reqs) {
        trim(d);
    }
}

void compute_task_indices(Tasks *tasks) {
    // Dependency conversion.
    map<string, vector<int> > label_to_indices;
    for (int i = 0; i < tasks->tasks.size(); ++i) {
        Task& task = tasks->tasks[i];        
        auto it = label_to_indices.find(task.label);
        if (it == label_to_indices.end()) {
            label_to_indices.insert(make_pair(task.label, vector<int>()));
        }  
        label_to_indices[task.label].push_back(i);
        task.idx = i;
    }

    for (Task &task : tasks->tasks) {
        task.pre_req_indices.clear();

        for (const auto& pre_req : task.pre_reqs) {
            // cout << "Task " << task.idx << " has prereq " << task.pre_req_ids[j] << endl;
            for (int dep_id : label_to_indices[pre_req]) {
                task.pre_req_indices.push_back(dep_id);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        cout << "Usage: schedule_new strings to specify the events." << endl;
        return 0;
    }

    time_t rawtime;
    tm * timeinfo;

    time (&rawtime);
    timeinfo = localtime (&rawtime);
    // printf ("Current local time and date: %s", asctime(timeinfo));
    
    /*
    auto tp = chrono::system_clock::now();
    time_t tt = chrono::system_clock::to_time_t(tp);
    tm local_tm = *localtime(&tt);

    auto dp = date::floor<date::days>(tp);  // dp is a sys_days, which is a
    // type alias for a C::time_point
    auto ymd = date::year_month_day{dp};
    auto time = date::make_time(chrono::duration_cast<chrono::milliseconds>(tp - dp));

    int hour = time.hours().count();
    int minute = time.minutes().count();
    int seconds = time.seconds().count();
    */

    int hour = 0, minute = 0, seconds = 0;
    hour = timeinfo->tm_hour;
	minute = timeinfo->tm_min;
	seconds = timeinfo->tm_sec;
	// hour = 8;

    Tasks tasks;
    tasks.global_start_time = hour * 3600 + minute * 60 + seconds;
    tasks.rest_time = 300;

    cout << "Current time: " << convert_to_time(tasks.global_start_time) << endl;

    regex pattern("\\[(.*?)\\](\\[(.*?)\\])?\\s+(.*?)$");
    smatch sm;
    for (const auto &s : split(string(argv[1]), '\n')) {
        // Use regular expression to analyze the string.
        // cout << "Parse " << s << endl;
        if (!regex_match(s, sm, pattern)) continue;

        // It matches, setup the task.
        string time_str = sm[1].str();
        string dep_str = sm[3].str(); 
        string name = sm[4].str();

        Task task;
        task.name = name; 
        trim(task.name);

        if (!set_task_time(time_str, &task.time)) continue;
        set_task_labels(dep_str, &task);

        tasks.tasks.push_back(task);
    }

    compute_task_indices(&tasks);

    cout << tasks.get_summary() << endl;

    Schedules schedules;
    if (make_schedule(tasks, &schedules)) {
        // print schedules
        cout << "#steps = " << schedules.search_steps << endl;
        for (int i = 0; i < schedules.schedules.size(); ++i) {
            const Schedule& schedule = schedules.schedules[i];
            const Task& task = tasks.tasks[schedule.idx]; 
            cout << setw(30) <<  task.name << " [" << setw(20) << task.time.pattern << "]" << "   " << convert_to_time(schedule.start) << " - " << convert_to_time(schedule.end) << endl;
        }
        if (schedules.status == Schedules::FinalStatus::INCOMPLETE) {
            cout << "Task not yet assigned: " << endl;
            for (const auto &idx : schedules.incomplete_tasks) {
                cout << tasks.tasks[idx].name << endl;
            }
        } 
        cout << "Utility: " << schedules.used_duration << "/" << schedules.total_duration << "(" << 100 * schedules.used_duration / schedules.total_duration << "%)" << endl;
    }
    return 0;
}
