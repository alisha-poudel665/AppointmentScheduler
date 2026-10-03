#ifndef NOTIFICATION_CPP
#define NOTIFICATION_CPP

#include "Appointment.cpp"
#include <vector>
#include <iostream>
#include <ctime>
#include <cstdio>
#include <string>

using namespace std;

// Handles reminder checking and display.
// Demonstrates single-responsibility: Notification only cares about
// telling the user about upcoming appointments, not storage or scheduling logic.
class Notification {
public:
    // Returns the current date as YYYY-MM-DD using the system clock.
    static string getCurrentDate() {
        time_t now = time(0);
        tm* ltm = localtime(&now);
        char buffer[11];
        snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d",
                 1900 + ltm->tm_year, 1 + ltm->tm_mon, ltm->tm_mday);
        return string(buffer);
    }

    // Returns the current time as HH:MM using the system clock.
    static string getCurrentTime() {
        time_t now = time(0);
        tm* ltm = localtime(&now);
        char buffer[6];
        snprintf(buffer, sizeof(buffer), "%02d:%02d", ltm->tm_hour, ltm->tm_min);
        return string(buffer);
    }

    // Prints a reminder for any appointment that:
    // - is scheduled for TODAY
    // - has status "Scheduled"
    // - current time is between reminderTime and startTime
    static void showDueReminders(const vector<Appointment> &appointments) {
        string today = getCurrentDate();
        string nowTime = getCurrentTime();
        bool any = false;

        cout << "\n===== REMINDERS =====\n";
        for (const Appointment &a : appointments) {
            if (a.getStatus() != "Scheduled") continue;
            if (a.getDate() != today) continue;
            if (a.getReminderTime().empty()) continue;

            if (nowTime >= a.getReminderTime() && nowTime < a.getStartTime()) {
                cout << "[REMINDER] \"" << a.getTitle() << "\" with " << a.getUserName()
                     << " starts at " << a.getStartTime() << " today.\n";
                any = true;
            }
        }
        if (!any) {
            cout << "No reminders due right now.\n";
        }
        cout << "======================\n";
    }
};

// A simple MOCK API class simulating export of an appointment to an
// external calendar service (e.g. Google Calendar). It makes NO real
// network calls and needs NO API key or internet connection.
// This exists to show how a real integration could be plugged in later
// without touching the rest of the application.
class CalendarAPI {
public:
    static bool exportAppointment(const Appointment &a) {
        cout << "\n[MOCK API] Connecting to external calendar service... (simulated)\n";
        cout << "[MOCK API] Exporting appointment #" << a.getId()
             << " - \"" << a.getTitle() << "\"\n";
        cout << "[MOCK API] Export successful (simulated). No real API key was used.\n";
        return true;
    }
};

#endif
