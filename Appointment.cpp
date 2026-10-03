#ifndef APPOINTMENT_CPP
#define APPOINTMENT_CPP

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cerrno>
#include <climits>
#include <stdexcept>

using namespace std;

// Portable replacements for std::stoi / std::to_string. Old MinGW builds
// (e.g. GCC 6.3.0 on Windows) do not provide those two functions.
inline int toInt(const string &text) {
    const char *start = text.c_str();
    char *end = nullptr;
    errno = 0;
    long value = strtol(start, &end, 10);
    if (end == start) throw invalid_argument("toInt: no digits");
    if (errno == ERANGE || value > INT_MAX || value < INT_MIN)
        throw out_of_range("toInt: out of range");
    return (int)value;
}

inline string intToString(int value) {
    ostringstream out;
    out << value;
    return out.str();
}

// Represents a single appointment with all its details.
// Demonstrates ENCAPSULATION: all data members are private,
// accessed only through public getters/setters.
class Appointment {
private:
    int id;
    string title;
    string date;          // format: YYYY-MM-DD
    string startTime;      // format: HH:MM (24-hour)
    string endTime;        // format: HH:MM (24-hour)
    string description;
    string userName;
    string reminderTime;   // format: HH:MM - a time on the SAME day to show a reminder
    string status;         // "Scheduled", "Completed", "Cancelled"

public:
    // Default constructor
    Appointment() {
        id = 0;
        status = "Scheduled";
    }

    // Parameterized constructor
    Appointment(int id, string title, string date, string startTime, string endTime,
                string description, string userName, string reminderTime,
                string status = "Scheduled") {
        this->id = id;
        this->title = title;
        this->date = date;
        this->startTime = startTime;
        this->endTime = endTime;
        this->description = description;
        this->userName = userName;
        this->reminderTime = reminderTime;
        this->status = status;
    }

    // ---------- Getters ----------
    int getId() const { return id; }
    string getTitle() const { return title; }
    string getDate() const { return date; }
    string getStartTime() const { return startTime; }
    string getEndTime() const { return endTime; }
    string getDescription() const { return description; }
    string getUserName() const { return userName; }
    string getReminderTime() const { return reminderTime; }
    string getStatus() const { return status; }

    // ---------- Setters ----------
    // Used by the Scheduler when IDs are renumbered after a delete.
    void setId(int newId) { id = newId; }
    void setTitle(const string &t) { title = t; }
    void setDate(const string &d) { date = d; }
    void setStartTime(const string &s) { startTime = s; }
    void setEndTime(const string &e) { endTime = e; }
    void setDescription(const string &d) { description = d; }
    void setUserName(const string &u) { userName = u; }
    void setReminderTime(const string &r) { reminderTime = r; }
    void setStatus(const string &s) { status = s; }

    // Converts this appointment into one line of text for file storage.
    // Uses '|' as a delimiter (instead of comma) so descriptions containing
    // commas do not break the file format.
    string toFileLine() const {
        stringstream ss;
        ss << id << "|" << title << "|" << date << "|" << startTime << "|" << endTime << "|"
           << description << "|" << userName << "|" << reminderTime << "|" << status;
        return ss.str();
    }

    // Rebuilds an Appointment object from a stored line of text.
    // May throw (e.g. std::invalid_argument from toInt) if the line is corrupted;
    // the caller (FileManager) catches this.
    static Appointment fromFileLine(const string &line) {
        stringstream ss(line);
        string field;
        string values[9];
        int index = 0;
        while (getline(ss, field, '|') && index < 9) {
            values[index] = field;
            index++;
        }
        Appointment a;
        a.id = toInt(values[0]);
        a.title = values[1];
        a.date = values[2];
        a.startTime = values[3];
        a.endTime = values[4];
        a.description = values[5];
        a.userName = values[6];
        a.reminderTime = values[7];
        a.status = values[8];
        return a;
    }

    // Displays this appointment as one formatted table row.
    void printRow() const {
        cout << left
             << setw(4)  << id
             << setw(20) << title.substr(0, 18)
             << setw(12) << date
             << setw(8)  << startTime
             << setw(8)  << endTime
             << setw(15) << userName.substr(0, 13)
             << setw(12) << status
             << "\n";
    }

    // Displays full details of this appointment.
    void printDetails() const {
        cout << "----------------------------------------\n";
        cout << "ID          : " << id << "\n";
        cout << "Title       : " << title << "\n";
        cout << "Date        : " << date << "\n";
        cout << "Start Time  : " << startTime << "\n";
        cout << "End Time    : " << endTime << "\n";
        cout << "Description : " << description << "\n";
        cout << "User        : " << userName << "\n";
        cout << "Reminder At : " << reminderTime << "\n";
        cout << "Status      : " << status << "\n";
        cout << "----------------------------------------\n";
    }
};

#endif
