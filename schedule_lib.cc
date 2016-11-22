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

#include "schedule_lib.h"

#include <queue>
#include <vector>
#include <utility>
#include <string>
#include <map>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <limits>
#include <time.h>
#include <set>

using namespace std;

string convert_to_time(int t) {
    int hour = t / 3600;
    int minute = (t - hour * 3600) / 60;
    int second = t % 60;
    stringstream ss;
    // ss << setfill('0') << setw(2) << hour << ":" << setw(2) << minute << ":" << setw(2) << second;
    ss << setfill('0') << setw(2) << hour << ":" << setw(2) << minute;
    return ss.str();
}

/////////////////////////////////Heap////////////////////////////////////////////
template <typename Key, typename T>
class Heap {
private:
    struct HeapSlot {
        Key key;
        T content;
        // The pointer will maintain its location in the heap.
        int *backIndexPtr;
        HeapSlot() {
        }
        HeapSlot(const Key &k, const T &c, int *const ptr) 
            :key(k), content(c), backIndexPtr(ptr) {
        }
    };
    std::vector<HeapSlot> m_heap;
    int m_heapSize;

    void GenerateZeroPos() {
        // Always the first element.
        m_heap[0].key = -std::numeric_limits<Key>::max();
    }

    inline bool set_back_ptr(int index, int val) {
        int *back_ptr = m_heap[index].backIndexPtr;
        if (back_ptr != nullptr) {
            *back_ptr = val;
            return true;
        } else return false;
    }

public:
    Heap() {
        //
        m_heap.push_back(HeapSlot());
        m_heapSize = 0;
        GenerateZeroPos();
    }

    int GetSize() const { return m_heapSize; }

    void SiftUp(int index) {
        assert(index > 0);
        assert(index <= m_heapSize);
        while (m_heap[index].key < m_heap[index >> 1].key) {
            set_back_ptr(index >> 1, index);
            swap(m_heap[index], m_heap[index >> 1]);
            index >>= 1;
        }
        set_back_ptr(index, index);
    }
    void SiftDown(int index) {
        assert(index > 0);
        assert(index <= m_heapSize);
        while (2*index <= m_heapSize) {
            int nextIndex = index << 1;
            if (   (nextIndex + 1 <= m_heapSize) 
                && (m_heap[nextIndex + 1].key < m_heap[nextIndex].key) ) 
                nextIndex ++;
            if (m_heap[nextIndex].key < m_heap[index].key) {
                set_back_ptr(nextIndex, index);
                swap(m_heap[nextIndex], m_heap[index]);
            }
            else break;
            index = nextIndex;
        }
        set_back_ptr(index, index);
    }
    void MakeHeap(const std::vector<Key> &keys, const std::vector<T> &contents, const std::vector<int *> &backIndexPointers) {
        assert(keys.size() == contents.size());
        m_heapSize = keys.size();
        m_heap.assign(m_heapSize + 1, HeapSlot());

        GenerateZeroPos();
        for (int i = 1; i <= m_heapSize; i ++) {
            m_heap[i].key = keys[i - 1];
            m_heap[i].content = contents[i - 1];
            m_heap[i].backIndexPtr = backIndexPointers[i - 1];
            *(m_heap[i].backIndexPtr) = i;
        }
        for (int i = m_heapSize >> 1; i >= 1; i --) {
            SiftDown(i);
        }
    }
    void Insert(const Key &key, const T &content, int *const backPtr) {
        m_heapSize++;
        m_heap.push_back(HeapSlot(key, content, backPtr));
        SiftUp(m_heapSize);
    }
    bool Delete(int index) {
        // Set the key to be the smallest.
        if (m_heapSize == 0)
            return false;        
        m_heap[index].key = -std::numeric_limits<Key>::max() / 2;
        SiftUp(index);
        // Then delete it from the top
        return DeleteMin(nullptr, nullptr);
    }

    bool IsEmpty()const {
        return m_heapSize < 1;
    }
    Key &GetKey(int index) {
        assert(index > 0 && index <= m_heapSize);
        return m_heap[index].key;
    }
    T &GetContent(int index) {
        assert(index > 0 && index <= m_heapSize);
        return m_heap[index].content;
    }

    // Get the minimal element and delete it.
    bool DeleteMin(Key* key, T* content) {
        if (m_heapSize == 0)
            return false;
        if (key != nullptr) *key = m_heap[1].key;
        if (content != nullptr) *content = m_heap[1].content;

        set_back_ptr(1, -1);
        if (m_heapSize > 1) {
            set_back_ptr(m_heapSize, 1);
            swap(m_heap[1], m_heap[m_heapSize]);
        }
        m_heapSize --;
        typename std::vector<HeapSlot>::iterator it = m_heap.end();
        --it;
        m_heap.erase(it);
        if (m_heapSize >= 1)
            SiftDown(1);
        return true;
    }
    bool CheckIndices(int &errValue)const {
        for (int i = 1; i <= m_heapSize; i++) {
            if ( *(m_heap[i].backIndexPtr) != i ) {
                errValue = i;
                return false;
            }
        }
        return true;
    }
};

void test_heap() {
    cout << "Testing heap" << endl;
    Heap<int, int> q;
    const int N = 100000;

    for (int i = 0; i < N; ++i)
        q.Insert(rand(), rand(), nullptr);

    vector<int> sorted_key;

    while (!q.IsEmpty()) {
        int key, val;
        q.DeleteMin(&key, &val);
        sorted_key.push_back(key);
    }

    // Check sorted
    for (int i = 1; i < N; ++i) {
        if (sorted_key[i - 1] > sorted_key[i]) {
            cout << "Heap output at " << i << " is not sorted!!";
        }
    }
    cout << "Done" << endl;
}

// Compact representation of schedule internal status.
struct ScheduleItem {
    int num_scheduled = 0;
    vector<time_t> end_timestamps;
    // The most recent ending timestamp.
    time_t end_timestamp = -1;
    int slot_index = -1;

    // Specify the number of tasks beforehand.
    // If a task is not scheduled, its end_timestamp is -1
    ScheduleItem() {
    }

    ScheduleItem(int N) : end_timestamps(N, -1) {
    }

    ScheduleItem next(int new_task, time_t end_timestamp) const {
        ScheduleItem new_item = *this;
        new_item.num_scheduled++;
        new_item.end_timestamps[new_task] = end_timestamp;
        new_item.end_timestamp = max(new_item.end_timestamp, end_timestamp);

        return new_item;
    }

    vector<int> GetOrder() const {
        vector<pair<time_t, int>> sort_pairs;
        for (int i = 0; i < end_timestamps.size(); ++i) {
            if (end_timestamps[i] > 0) {
                sort_pairs.emplace_back(make_pair(end_timestamps[i], i));
            }
        }

        sort(sort_pairs.begin(), sort_pairs.end());
        vector<int> order(sort_pairs.size());
        for (int i = 0; i < sort_pairs.size(); ++i) {
            order[i] = sort_pairs[i].second;
        }
        return order;
    }

    void PrintDebugInfo(const Tasks& tasks) const {
		// Sort the indices.
		vector<int> indices;
        for (int i = 0; i < end_timestamps.size(); ++i) {
			indices.push_back(i);
		}
		sort(indices.begin(), indices.end(), [&](int i, int j) -> bool { return end_timestamps[i] < end_timestamps[j]; });

		for (int i : indices) {
            if (end_timestamps[i] < 0) continue;
			string start_str = convert_to_time(end_timestamps[i] - tasks.tasks[i].time.duration);
			string end_str = convert_to_time(end_timestamps[i]);
            cout << "Task = " << tasks.tasks[i].name << "  " << start_str << " - " << end_str << endl;
        }
    }

    friend bool operator<(const ScheduleItem& s1, const ScheduleItem& s2) {
        // Note since the priority queue in c++ always returns the greatest element, 
        // we reverse the definition of <.
        return s1.end_timestamp > s2.end_timestamp;
    }
};

typedef pair<float, ScheduleItem> SchedulePair;

time_t earliest_given_pre_req(time_t global_start_time, const Tasks& tasks, int curr_task_idx, const ScheduleItem& completed) {
    // Find the earliest starting time.
    time_t start_time = completed.num_scheduled > 0 ? completed.end_timestamp : global_start_time;
    for (const int& pre_index : tasks.tasks[curr_task_idx].pre_req_indices) {
        if (completed.end_timestamps[pre_index] < 0) return -1;
        start_time = max(start_time, completed.end_timestamps[pre_index] + tasks.tasks[pre_index].time.cool_down);
    }
    return start_time;
}

// Can the current task start with the given start_time (or later)
// If not, return -1, else return the earliest start time for the task.
time_t earliest_given_constraint(const Task& task, time_t start_time) {
    // Cannot miss the deadline.
    const TimeSegment& time = task.time;
    if (time.deadline > 0 && start_time + time.duration > time.deadline) return -1;

    if (time.start_time_intervals.empty()) return start_time;

    bool fit_in = false;
    for (int i = 0; i < time.start_time_intervals.size(); ++i) {
        // If the start_time is earlier than then the latest starting time, we could schedule.
        if (start_time <= time.start_time_intervals[i].second) {
            start_time = max(start_time, (time_t)time.start_time_intervals[i].first);
            fit_in = true;
            break;
        }
    }

    if (fit_in) return start_time;
    else return -1;
}

bool get_lb(const Tasks& tasks, const ScheduleItem& completed, float* score) {
    // Compute the heuristic function.
    time_t lower_bound = 0;
    for (int i = 0; i < tasks.tasks.size(); ++i) {
        // If the task is already placed in the partial solution, skip
        if (completed.end_timestamps[i] >= 0) continue;

        // The task is notyet added.
        const Task& task = tasks.tasks[i];

        time_t opt_start_time = earliest_given_constraint(task, completed.end_timestamp);
        // You can never start this job, set the score to be very low.
        if (opt_start_time < 0) {
            // Penalty for not achieving the goal.
            lower_bound += task.time.duration * task.time.priority;
        } else {
            lower_bound += task.time.duration + tasks.rest_time;
        }
    }

    *score = completed.end_timestamp + lower_bound;
    return true;
}

// Input a few tasks and return a complete schedule.
bool make_schedule(const Tasks& tasks, Schedules* schedules) {
    // test_heap();
    const int N = tasks.tasks.size();
    // cout << "#Task = " << N << endl;

    vector<int> back_container(tasks.max_heap_size + N, -1);
    set<int> unused_slot;
    for (int i = 0; i < tasks.max_heap_size + N; ++i) {
        unused_slot.insert(i);
    }

    ScheduleItem best_schedule(N);
    float best_score;

    int num_steps = 0;
    Heap<float, ScheduleItem> q;
    Heap<float, int> back_q;

    ScheduleItem completed(N);
    completed.slot_index = 0;
    q.Insert(0.0, completed, &back_container[0]);    
    back_q.Insert(0.0, 0, nullptr);
    unused_slot.erase(0);

    float score;
    while (!q.IsEmpty()) {
        q.DeleteMin(&score, &completed);
        unused_slot.insert(completed.slot_index);

		/*
        cout << score << endl;
        completed.PrintDebugInfo(tasks);
        cout << endl;
		*/

        // const float score = q.top().first;
        // // One extra copy here.
        // ScheduleItem completed = q.top().second;
        // q.pop();
        num_steps++;

        if (completed.num_scheduled > best_schedule.num_scheduled) {
            best_schedule = completed;
            best_score = score;

			/*
			cout << score << endl;
			completed.PrintDebugInfo(tasks);
			cout << endl;
			*/
        }

        if (best_schedule.num_scheduled == N) break;

        // Make 
        for (int i = 0; i < N; ++i) {
			// If the event is already scheduled, go to the next one. 
            if (completed.end_timestamps[i] >= 0) continue;

            // Else try scheduling it.
            time_t start_time = earliest_given_pre_req(tasks.global_start_time, tasks, i, completed);

            if (start_time < 0) continue;
            start_time = earliest_given_constraint(tasks.tasks[i], start_time + tasks.rest_time);

            if (start_time < 0) continue;
            time_t end_time = start_time + tasks.tasks[i].time.duration;

            ScheduleItem next_item = completed.next(i, end_time);
            float next_score;
            if (get_lb(tasks, next_item, &next_score)) {
                int slot_index = *unused_slot.begin();
                next_item.slot_index = slot_index;
                q.Insert(next_score, next_item, &back_container[slot_index]);
                back_q.Insert(-next_score, slot_index, nullptr);
                unused_slot.erase(slot_index);
            }
        }

        // If queue is too large, remove the worst one.
        while (q.GetSize() > tasks.max_heap_size) {
            while (true) {
                int slot_index;
                back_q.DeleteMin(nullptr, &slot_index);
                int heap_index = back_container[slot_index];
                if (heap_index >= 0) {
                    // Remove
                    q.Delete(heap_index);
                    unused_slot.insert(slot_index);
                    break;
                }
            }
        }
    }

	cout << "Search finished. #Step = " << num_steps << " Size of queue " << q.GetSize() << endl;

    // Get the best schedule.
    vector<int> order = best_schedule.GetOrder();

    if (order.size() < N) {
        schedules->status = Schedules::FinalStatus::INCOMPLETE;
        for (int i = 0; i < N; ++i) {
              // Save incompleted tasks.
              if (best_schedule.end_timestamps[i] < 0) {
                  schedules->incomplete_tasks.push_back(i);
              }
        }
    } else {
        schedules->status = Schedules::FinalStatus::SUCCESS;
    }
    schedules->search_steps = num_steps;
    schedules->total_duration = best_schedule.end_timestamp - tasks.global_start_time;

    // From the order, construct the best schedule and get their start/end timestamp.
    schedules->schedules.clear();
    int duration = 0;
    for (int i = 0; i < order.size(); ++i) {
        Schedule s;

        const int task_index = order[i];
        const Task& task = tasks.tasks[task_index];

        s.idx = task.idx;
        s.end = best_schedule.end_timestamps[task_index];        
        s.start = s.end - task.time.duration;
        // Add the schedule into the scheduler.
        schedules->schedules.push_back(s);

        duration += task.time.duration;
    }

    schedules->used_duration = duration;
    return true;
}
