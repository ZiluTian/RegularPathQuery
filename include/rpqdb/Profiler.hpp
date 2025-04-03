#ifndef RPQDB_Profiler_H
#define RPQDB_Profiler_H

#include <iostream>
#include <string>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <stack>
#include <ctime>
#include <iomanip>

using namespace std;
using namespace std::chrono;

namespace rpqdb {
    struct ProfileEvent {
        string name;
        duration<double> duration;
    };
    
    void log_time_human(const steady_clock::time_point& tp) {
        // Convert steady_clock to system_clock for calendar time
        auto system_now = system_clock::now() + duration_cast<system_clock::duration>(tp - steady_clock::now());
        time_t time = system_clock::to_time_t(system_now);
        
        // Format as local time
        cout << "[" << put_time(localtime(&time), "%Y-%m-%d %H:%M:%S");
        
        // Add milliseconds
        auto ms = duration_cast<milliseconds>(tp.time_since_epoch()) % 1000;
        cout << "." << setfill('0') << setw(3) << ms.count() << "] ";
    }

    class EventProfiler {
        static unordered_map<string, vector<ProfileEvent>> profile_data;
        static mutex mtx;
        
        // Stack for local events (no names needed for ending)
        static thread_local stack<pair<string, steady_clock::time_point>> local_stack;
        
        // Map for named events (must end with same name)
        static unordered_map<string, steady_clock::time_point> active_events;

    public:
        const static bool verbose = true;

        // Start a local event (automatically tracked in stack)
        static void start_local(const string& name) {
            auto now = steady_clock::now();
            lock_guard<mutex> lock(mtx);
            if (verbose) {
                log_time_human(now);
                cout << "Start event " << name << endl;
            }
            local_stack.push({name, now});
        }
    
        // End the most recent local event
        static void end_local() {
            lock_guard<mutex> lock(mtx);
            if (local_stack.empty()) {
                cerr << "Warning: No local event to end\n";
                return;
            }
            
            auto [name, start] = local_stack.top();
            local_stack.pop();
            
            auto end = steady_clock::now();
            if (verbose) {
                log_time_human(end);
                cout << "End event " << name << endl;
            }
            profile_data[name].push_back({name, end - start});
        }
    
        // Start a named event (must be ended explicitly)
        static void start_event(const string& name) {
            auto now = steady_clock::now();

            if (verbose) {
                log_time_human(now);
                cout << " start event " << name << endl;
            }
            lock_guard<mutex> lock(mtx);
            if (active_events.count(name)) {
                cerr << "Warning: Event '" << name << "' already started\n";
                return;
            }
            active_events[name] = now;
        }
    
        // End a named event
        static void end_event(const string& name) {
            lock_guard<mutex> lock(mtx);
            auto it = active_events.find(name);
            if (it == active_events.end()) {
                cerr << "Warning: Event '" << name << "' not found\n";
                return;
            }
            
            auto end = steady_clock::now();
            profile_data[name].push_back({name, end - it->second});
            if (verbose) {
                log_time_human(end);
                cout << "End event " << name << endl;
            }
            active_events.erase(it);
        }
    
        static void print_stats() {
            lock_guard<mutex> lock(mtx);
            cout << "\n===== Profiling Results =====\n";
            
            for (const auto& [name, events] : profile_data) {
                if (events.empty()) continue;
                
                double total_ms = 0;
                for (const auto& event : events) {
                    total_ms += duration_cast<milliseconds>(event.duration).count();
                }
                
                cout << name << ": " << events.size() << " calls, "
                     << total_ms << "ms total, "
                     << total_ms/events.size() << "ms avg\n";
            }
        }
        
        static void reset() {
            lock_guard<mutex> lock(mtx);
            profile_data.clear();
            while (!local_stack.empty()) local_stack.pop();
            active_events.clear();
        }
    };
    
    // Static member definitions
    unordered_map<string, vector<ProfileEvent>> EventProfiler::profile_data;
    mutex EventProfiler::mtx;
    thread_local stack<pair<string, steady_clock::time_point>> EventProfiler::local_stack;
    unordered_map<string, steady_clock::time_point> EventProfiler::active_events;
    
    // Macros for cleaner syntax
    #define START_LOCAL(name) EventProfiler::start_local(name)
    #define END_LOCAL() EventProfiler::end_local()
    
    #define START_EVENT(name) EventProfiler::start_event(name)
    #define END_EVENT(name) EventProfiler::end_event(name)
}; 
#endif