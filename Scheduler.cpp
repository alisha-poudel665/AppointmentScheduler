#ifndef SCHEDULER_CPP
#define SCHEDULER_CPP

#include "Appointment.cpp"
#include "FileManager.cpp"
#include "Notification.cpp"
#include <vector>
#include <algorithm>
#include <iostream>
#include <string>
#include <cctype>
#include <ctime>
#include <cstdio>

using namespace std;

// The Scheduler is the central controller of the application.
// It owns the list of appointments and a FileManager (COMPOSITION:
// Scheduler "has-a" FileManager, it does not inherit from it).
class Scheduler {
private:
    vector<Appointment> appointments;
    FileManager fileManager;
    int nextId;

    // Recalculates the next available ID based on the highest existing ID.
    void refreshNextId() {
        nextId = 1;
        for (const Appointment &a : appointments) {
            if (a.getId() >= nextId) nextId = a.getId() + 1;
        }
    }

    // Renumbers all appointments so IDs are always 1, 2, 3, ... with no gaps.
    // The list is first put back in ID order so the relative order of the
    // appointments is preserved (e.g. old #2 becomes #1, old #3 becomes #2).
    // Returns true if any ID actually changed.
    bool renumberIds() {
        sort(appointments.begin(), appointments.end(),
             [](const Appointment &a, const Appointment &b) {
                 return a.getId() < b.getId();
             });

        bool changed = false;
        for (size_t i = 0; i < appointments.size(); i++) {
            int newId = (int)i + 1;
            if (appointments[i].getId() != newId) {
                appointments[i].setId(newId);
                changed = true;
            }
        }
        refreshNextId();
        return changed;
    }

public:
    Scheduler() : fileManager("appointments.txt") {
        appointments = fileManager.loadAppointments();
        // Clean up any gaps left in an older appointments.txt file.
        if (renumberIds()) fileManager.saveAppointments(appointments);
    }

    // ---------- Validation helpers ----------

    // Checks that a date string roughly matches YYYY-MM-DD with valid ranges.
    // NOTE: this is a simplified check (e.g. it allows Feb 30) - acceptable
    // for this project's scope, but a known limitation.
    static bool isValidDate(const string &date) {
        if (date.length() != 10) return false;
        if (date[4] != '-' || date[7] != '-') return false;
        for (int i = 0; i < 10; i++) {
            if (i == 4 || i == 7) continue;
            if (!isdigit((unsigned char)date[i])) return false;
        }
        int month = toInt(date.substr(5, 2));
        int day = toInt(date.substr(8, 2));
        if (month < 1 || month > 12) return false;
        if (day < 1 || day > 31) return false;
        return true;
    }

    // Checks a YYYY-MM string (used for monthly view).
    static bool isValidYearMonth(const string &ym) {
        if (ym.length() != 7) return false;
        if (ym[4] != '-') return false;
        for (int i = 0; i < 7; i++) {
            if (i == 4) continue;
            if (!isdigit((unsigned char)ym[i])) return false;
        }
        int month = toInt(ym.substr(5, 2));
        return month >= 1 && month <= 12;
    }

    // Checks that a time string matches HH:MM in 24-hour format.
    static bool isValidTime(const string &t) {
        if (t.length() != 5) return false;
        if (t[2] != ':') return false;
        if (!isdigit((unsigned char)t[0]) || !isdigit((unsigned char)t[1]) ||
            !isdigit((unsigned char)t[3]) || !isdigit((unsigned char)t[4])) return false;
        int hour = toInt(t.substr(0, 2));
        int minute = toInt(t.substr(3, 2));
        if (hour < 0 || hour > 23) return false;
        if (minute < 0 || minute > 59) return false;
        return true;
    }

    // Adds "days" to a YYYY-MM-DD date string, handling month/year rollover.
    static string addDays(const string &date, int days) {
        tm t = {};
        t.tm_year = toInt(date.substr(0, 4)) - 1900;
        t.tm_mon  = toInt(date.substr(5, 2)) - 1;
        t.tm_mday = toInt(date.substr(8, 2)) + days;
        time_t tt = mktime(&t);
        tm* result = localtime(&tt);
        char buffer[11];
        snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d",
                 1900 + result->tm_year, 1 + result->tm_mon, result->tm_mday);
        return string(buffer);
    }

    // Two time ranges on the SAME date overlap if startA < endB AND startB < endA.
    static bool timesOverlap(const string &startA, const string &endA,
                              const string &startB, const string &endB) {
        return (startA < endB) && (startB < endA);
    }

    // Checks whether a new/edited appointment would conflict with an existing
    // one on the same date. excludeId is used when editing, so the appointment
    // being edited does not conflict with itself.
    bool hasConflict(const string &date, const string &startTime, const string &endTime,
                      int excludeId = -1) {
        for (const Appointment &a : appointments) {
            if (a.getId() == excludeId) continue;
            if (a.getStatus() == "Cancelled") continue;
            if (a.getDate() != date) continue;
            if (timesOverlap(startTime, endTime, a.getStartTime(), a.getEndTime())) {
                return true;
            }
        }
        return false;
    }

    // ---------- Core features ----------

    // Adds a new appointment after validating dates/times and checking conflicts.
    // Returns "" on success, or an error message on failure.
    string addAppointment(string title, string date, string startTime, string endTime,
                           string description, string userName, string reminderTime) {
        if (!isValidDate(date)) return "Invalid date format. Use YYYY-MM-DD.";
        if (!isValidTime(startTime)) return "Invalid start time format. Use HH:MM (24-hour).";
        if (!isValidTime(endTime)) return "Invalid end time format. Use HH:MM (24-hour).";
        if (!reminderTime.empty() && !isValidTime(reminderTime))
            return "Invalid reminder time format. Use HH:MM.";
        if (startTime >= endTime) return "Start time must be earlier than end time.";
        if (!reminderTime.empty() && reminderTime >= startTime)
            return "Reminder time must be earlier than start time.";
        if (hasConflict(date, startTime, endTime))
            return "Time slot conflicts with an existing appointment.";

        Appointment a(nextId, title, date, startTime, endTime, description, userName,
                      reminderTime, "Scheduled");
        appointments.push_back(a);
        nextId++;
        fileManager.saveAppointments(appointments);
        return "";
    }

    // Finds an appointment by ID. Returns nullptr if not found.
    Appointment* findById(int id) {
        for (Appointment &a : appointments) {
            if (a.getId() == id) return &a;
        }
        return nullptr;
    }

    // Edits an existing appointment. Blank string fields keep their old value.
    string editAppointment(int id, string title, string date, string startTime, string endTime,
                            string description, string userName, string reminderTime) {
        Appointment* a = findById(id);
        if (a == nullptr) return "Appointment not found.";

        string newDate     = date.empty() ? a->getDate() : date;
        string newStart    = startTime.empty() ? a->getStartTime() : startTime;
        string newEnd      = endTime.empty() ? a->getEndTime() : endTime;
        string newReminder = reminderTime.empty() ? a->getReminderTime() : reminderTime;

        if (!isValidDate(newDate)) return "Invalid date format. Use YYYY-MM-DD.";
        if (!isValidTime(newStart)) return "Invalid start time format. Use HH:MM.";
        if (!isValidTime(newEnd)) return "Invalid end time format. Use HH:MM.";
        if (newStart >= newEnd) return "Start time must be earlier than end time.";
        if (!newReminder.empty() && newReminder >= newStart)
            return "Reminder time must be earlier than start time.";
        if (hasConflict(newDate, newStart, newEnd, id))
            return "Time slot conflicts with another appointment.";

        if (!title.empty()) a->setTitle(title);
        a->setDate(newDate);
        a->setStartTime(newStart);
        a->setEndTime(newEnd);
        if (!description.empty()) a->setDescription(description);
        if (!userName.empty()) a->setUserName(userName);
        a->setReminderTime(newReminder);

        fileManager.saveAppointments(appointments);
        return "";
    }

    // Cancels an appointment (soft delete - keeps a record with status Cancelled).
    string cancelAppointment(int id) {
        Appointment* a = findById(id);
        if (a == nullptr) return "Appointment not found.";
        a->setStatus("Cancelled");
        fileManager.saveAppointments(appointments);
        return "";
    }

    // Permanently removes an appointment from the list and file.
    // The remaining appointments are renumbered so IDs stay 1, 2, 3, ...
    // (deleting #1 makes the old #2 become the new #1, and so on).
    string deleteAppointment(int id) {
        for (size_t i = 0; i < appointments.size(); i++) {
            if (appointments[i].getId() == id) {
                appointments.erase(appointments.begin() + i);
                renumberIds();
                fileManager.saveAppointments(appointments);
                return "";
            }
        }
        return "Appointment not found.";
    }

    // Updates only the status field of an appointment.
    string setStatus(int id, const string &status) {
        if (status != "Scheduled" && status != "Completed" && status != "Cancelled") {
            return "Invalid status. Use Scheduled, Completed, or Cancelled.";
        }
        Appointment* a = findById(id);
        if (a == nullptr) return "Appointment not found.";
        a->setStatus(status);
        fileManager.saveAppointments(appointments);
        return "";
    }

    // ---------- Sorting ----------

    void sortByDateTime() {
        sort(appointments.begin(), appointments.end(), [](const Appointment &a, const Appointment &b) {
            if (a.getDate() != b.getDate()) return a.getDate() < b.getDate();
            return a.getStartTime() < b.getStartTime();
        });
    }

    // ---------- Viewing ----------

    // "View All" lists appointments in ID order (1, 2, 3, ...) so that the
    // renumbering after a delete is easy to see. Daily/weekly/monthly views
    // below are still sorted by date and time.
    vector<Appointment> getAll() {
        vector<Appointment> sorted = appointments;
        sort(sorted.begin(), sorted.end(), [](const Appointment &a, const Appointment &b) {
            return a.getId() < b.getId();
        });
        return sorted;
    }

    vector<Appointment> getByDate(const string &date) {
        sortByDateTime();
        vector<Appointment> result;
        for (const Appointment &a : appointments) {
            if (a.getDate() == date) result.push_back(a);
        }
        return result;
    }

    // Inclusive date range - string comparison works because dates are
    // always stored as fixed-width YYYY-MM-DD.
    vector<Appointment> getByDateRange(const string &startDate, const string &endDate) {
        sortByDateTime();
        vector<Appointment> result;
        for (const Appointment &a : appointments) {
            if (a.getDate() >= startDate && a.getDate() <= endDate) result.push_back(a);
        }
        return result;
    }

    vector<Appointment> getByMonth(const string &yearMonth) {
        sortByDateTime();
        vector<Appointment> result;
        for (const Appointment &a : appointments) {
            if (a.getDate().substr(0, 7) == yearMonth) result.push_back(a);
        }
        return result;
    }

    // ---------- Searching ----------

    vector<Appointment> searchByTitle(const string &keyword) {
        vector<Appointment> result;
        for (const Appointment &a : appointments) {
            if (a.getTitle().find(keyword) != string::npos) result.push_back(a);
        }
        return result;
    }

    vector<Appointment> searchByDate(const string &date) {
        return getByDate(date);
    }

    vector<Appointment> searchByUser(const string &userName) {
        vector<Appointment> result;
        for (const Appointment &a : appointments) {
            if (a.getUserName().find(userName) != string::npos) result.push_back(a);
        }
        return result;
    }

    // ---------- Reminders ----------

    void showReminders() {
        Notification::showDueReminders(appointments);
    }

    // ---------- Mock API ----------

    string exportAppointment(int id) {
        Appointment* a = findById(id);
        if (a == nullptr) return "Appointment not found.";
        CalendarAPI::exportAppointment(*a);
        return "";
    }
};

#endif
