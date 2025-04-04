#ifndef RPQDB_Profiler_H
#define RPQDB_Profiler_H

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <stack>
#include <ctime>
#include <iomanip>
#include <sys/stat.h>

using namespace std;
using namespace std::chrono;

namespace rpqdb {
    struct ProfileEvent {
        string name;
        steady_clock::time_point start;
        long long duration;
    };
    
    static string log_time_human(const steady_clock::time_point& tp) {
        // Convert steady_clock to system_clock for calendar time
        auto system_now = system_clock::now() + duration_cast<system_clock::duration>(tp - steady_clock::now());
        time_t time = system_clock::to_time_t(system_now);
        
        stringstream ss;
        // Format as local time
        ss << "[" << put_time(localtime(&time), "%Y-%m-%d %H:%M:%S");
        
        // Add milliseconds
        auto ms = duration_cast<milliseconds>(tp.time_since_epoch()) % 1000;
        ss << "." << setfill('0') << setw(3) << ms.count() << "] ";
        return ss.str();
    }

    static string generate_timestamp() {
        auto now = system_clock::now();
        time_t tt = system_clock::to_time_t(now);
        tm tm = *localtime(&tt);
        stringstream ss;
        ss << put_time(&tm, "%Y%m%d_%H%M%S");
        return ss.str();
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
                cout << log_time_human(now) << "Start event " << name << endl;
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
                cout << log_time_human(end) << "End event " << name << endl;
            }
            long long duration = duration_cast<milliseconds>(end - start).count();
            profile_data[name].push_back({name, start, duration});
        }
    
        // Start a named event (must be ended explicitly)
        static void start_event(const string& name) {
            auto now = steady_clock::now();

            if (verbose) {
                cout << log_time_human(now) << " start event " << name << endl;
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
            long long duration = duration_cast<milliseconds>(end - it->second).count();
            // long long duration = (end - it->second).count();
            profile_data[name].push_back({name, it -> second, duration});
            if (verbose) {
                cout << log_time_human(end) << "End event " << name << endl;
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
                    total_ms += event.duration;
                }
                
                cout << name << ": " << events.size() << " calls, "
                     << total_ms << "ms total, "
                     << total_ms/events.size() << "ms avg\n";
            }
        }

        static void export_to_file(const string& new_filename = "profile.dat") {
            namespace fs = std::filesystem;
    
            // Generate timestamp for backup filename
            string timestamp = generate_timestamp();
        
            // Rename existing file if it exists
            struct stat buffer;
            if (stat(new_filename.c_str(), &buffer) == 0) {
                try {
                    string backup_filename = new_filename + timestamp;
                    if (std::rename(new_filename.c_str(), backup_filename.c_str()) != 0) {
                        throw runtime_error("Failed to rename existing file: " + new_filename);
                    }
                    cout << "Existing profile.dat renamed to: " << backup_filename << endl;
                } catch (const std::runtime_error& e) {
                    throw runtime_error("Failed to rename existing file: " + string(e.what()));
                }
            }
        
            // Sort events by start time
            vector<ProfileEvent> sorted_events;
            for (const auto& [name, events] : profile_data) {
                sorted_events.insert(sorted_events.end(), events.begin(), events.end());
            }
            sort(sorted_events.begin(), sorted_events.end(),
                [](const auto& a, const auto& b) { return a.start < b.start; });
        
            // Write to profile.dat
            ofstream file(new_filename);
            if (!file.is_open()) throw runtime_error("Failed to create profile.dat");
        
            // Write header
            file << "# Event\tDuration(ms)\n";

            for (const auto& event : sorted_events) {
                file << quoted(event.name) << "\t" << to_string(event.duration) << "\n";
            }
            cout << "Data saved to: " << new_filename << endl;
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

    #define IGNORE(...)
    #define VERSIONED_IMPLEMENTATION(...)
}; 
#endif