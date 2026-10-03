#include "Scheduler.cpp"
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

// ---------- ANSI color codes for a nicer terminal UI ----------
const string RESET  = "\033[0m";
const string RED    = "\033[31m";
const string GREEN  = "\033[32m";
const string YELLOW = "\033[33m";
const string CYAN   = "\033[36m";
const string BOLD   = "\033[1m";

#ifdef _WIN32
// Enables ANSI escape code support on older Windows consoles (classic cmd.exe).
// Modern Windows Terminal / VS Code integrated terminal usually support this already.
void enableAnsiColors() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    if (hOut == INVALID_HANDLE_VALUE || hOut == nullptr) {
        return;
    }

    DWORD dwMode = 0;

    if (!GetConsoleMode(hOut, &dwMode)) {
        return;
    }

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
}
#endif

// ---------- Small print helpers for consistent messages ----------
void printSuccess(const string &msg) { cout << GREEN  << "[SUCCESS] " << msg << RESET << "\n"; }
void printError(const string &msg)   { cout << RED    << "[ERROR] "   << msg << RESET << "\n"; }
void printWarning(const string &msg) { cout << YELLOW << "[WARNING] " << msg << RESET << "\n"; }
void printHeading(const string &msg) {
    cout << "\n" << BOLD << CYAN << "===== " << msg << " =====" << RESET << "\n";
}

// ---------- Input helpers ----------
string getLineInput(const string &prompt) {
    cout << prompt;
    string value;
    getline(cin, value);
    return value;
}

int getIntInput(const string &prompt) {
    while (true) {
        string input = getLineInput(prompt);
        try {
            return toInt(input);
        } catch (...) {
            printError("Please enter a valid whole number.");
        }
    }
}

void pauseScreen() {
    cout << "\nPress Enter to continue...";
    string dummy;
    getline(cin, dummy);
}

// ---------- Table display helper ----------
void displayList(const vector<Appointment> &list, const string &title) {
    printHeading(title);
    if (list.empty()) {
        cout << "No appointments found.\n";
        return;
    }
    cout << left
         << setw(4)  << "ID"
         << setw(20) << "Title"
         << setw(12) << "Date"
         << setw(8)  << "Start"
         << setw(8)  << "End"
         << setw(15) << "User"
         << setw(12) << "Status"
         << "\n";
    cout << string(79, '-') << "\n";
    for (const Appointment &a : list) {
        a.printRow();
    }
}

// ---------- Feature flows ----------

void addAppointmentFlow(Scheduler &scheduler) {
    printHeading("Add New Appointment");
    string title        = getLineInput("Title: ");
    string date         = getLineInput("Date (YYYY-MM-DD): ");
    string startTime    = getLineInput("Start Time (HH:MM, 24-hour): ");
    string endTime      = getLineInput("End Time (HH:MM, 24-hour): ");
    string description  = getLineInput("Description: ");
    string userName     = getLineInput("User Name: ");
    string reminderTime = getLineInput("Reminder Time (HH:MM, optional - Enter to skip): ");

    string error = scheduler.addAppointment(title, date, startTime, endTime,
                                             description, userName, reminderTime);
    if (error.empty()) printSuccess("Appointment added successfully.");
    else printError(error);
}

void viewAllFlow(Scheduler &scheduler) {
    displayList(scheduler.getAll(), "All Appointments");
}

void viewDailyFlow(Scheduler &scheduler) {
    string date = getLineInput("Enter date to view (YYYY-MM-DD): ");
    if (!Scheduler::isValidDate(date)) { printError("Invalid date format."); return; }
    displayList(scheduler.getByDate(date), "Appointments on " + date);
}

void viewWeeklyFlow(Scheduler &scheduler) {
    string date = getLineInput("Enter start date of the week (YYYY-MM-DD): ");
    if (!Scheduler::isValidDate(date)) { printError("Invalid date format."); return; }
    string endDate = Scheduler::addDays(date, 6);
    displayList(scheduler.getByDateRange(date, endDate), "Weekly Schedule: " + date + " to " + endDate);
}

void viewMonthlyFlow(Scheduler &scheduler) {
    string yearMonth = getLineInput("Enter month to view (YYYY-MM): ");
    if (!Scheduler::isValidYearMonth(yearMonth)) { printError("Invalid month format."); return; }
    displayList(scheduler.getByMonth(yearMonth), "Appointments in " + yearMonth);
}

void editAppointmentFlow(Scheduler &scheduler) {
    printHeading("Edit Appointment");
    int id = getIntInput("Enter appointment ID to edit: ");
    Appointment* existing = scheduler.findById(id);
    if (existing == nullptr) { printError("Appointment not found."); return; }

    existing->printDetails();
    cout << "Leave a field blank and press Enter to keep its current value.\n";
    string title        = getLineInput("New Title: ");
    string date         = getLineInput("New Date (YYYY-MM-DD): ");
    string startTime    = getLineInput("New Start Time (HH:MM): ");
    string endTime      = getLineInput("New End Time (HH:MM): ");
    string description  = getLineInput("New Description: ");
    string userName     = getLineInput("New User Name: ");
    string reminderTime = getLineInput("New Reminder Time (HH:MM): ");

    string error = scheduler.editAppointment(id, title, date, startTime, endTime,
                                              description, userName, reminderTime);
    if (error.empty()) printSuccess("Appointment updated successfully.");
    else printError(error);
}

void cancelAppointmentFlow(Scheduler &scheduler) {
    int id = getIntInput("Enter appointment ID to cancel: ");
    string confirm = getLineInput("Cancel appointment #" + intToString(id) + "? (y/n): ");
    if (confirm != "y" && confirm != "Y") { printWarning("Cancel action aborted."); return; }

    string error = scheduler.cancelAppointment(id);
    if (error.empty()) printSuccess("Appointment cancelled.");
    else printError(error);
}

void deleteAppointmentFlow(Scheduler &scheduler) {
    int id = getIntInput("Enter appointment ID to delete: ");
    string confirm = getLineInput("This permanently deletes appointment #" + intToString(id) +
                                  " and renumbers the rest. Continue? (y/n): ");
    if (confirm != "y" && confirm != "Y") { printWarning("Delete action aborted."); return; }

    string error = scheduler.deleteAppointment(id);
    if (error.empty()) {
        printSuccess("Appointment deleted.");
        printWarning("Remaining appointments were renumbered. Use 'View All' to see the new IDs.");
    }
    else printError(error);
}

void searchFlow(Scheduler &scheduler) {
    printHeading("Search Appointments");
    cout << "1. Search by Title\n2. Search by Date\n3. Search by User\n";
    int choice = getIntInput("Choose search type: ");
    if (choice == 1) {
        string keyword = getLineInput("Enter title keyword: ");
        displayList(scheduler.searchByTitle(keyword), "Search Results (Title: " + keyword + ")");
    } else if (choice == 2) {
        string date = getLineInput("Enter date (YYYY-MM-DD): ");
        displayList(scheduler.searchByDate(date), "Search Results (Date: " + date + ")");
    } else if (choice == 3) {
        string user = getLineInput("Enter user name: ");
        displayList(scheduler.searchByUser(user), "Search Results (User: " + user + ")");
    } else {
        printError("Invalid choice.");
    }
}

void setStatusFlow(Scheduler &scheduler) {
    int id = getIntInput("Enter appointment ID: ");
    cout << "1. Scheduled\n2. Completed\n3. Cancelled\n";
    int choice = getIntInput("Choose new status: ");
    string status;
    if (choice == 1) status = "Scheduled";
    else if (choice == 2) status = "Completed";
    else if (choice == 3) status = "Cancelled";
    else { printError("Invalid choice."); return; }

    string error = scheduler.setStatus(id, status);
    if (error.empty()) printSuccess("Status updated to " + status + ".");
    else printError(error);
}

void exportFlow(Scheduler &scheduler) {
    int id = getIntInput("Enter appointment ID to export: ");
    string error = scheduler.exportAppointment(id);
    if (!error.empty()) printError(error);
}

void printMenu() {
    printHeading("APPOINTMENT SCHEDULING SYSTEM");
    cout << " 1. Add Appointment\n";
    cout << " 2. View All Appointments\n";
    cout << " 3. View Daily Schedule\n";
    cout << " 4. View Weekly Schedule\n";
    cout << " 5. View Monthly Schedule\n";
    cout << " 6. Edit Appointment\n";
    cout << " 7. Cancel Appointment\n";
    cout << " 8. Delete Appointment\n";
    cout << " 9. Search Appointments\n";
    cout << "10. Set Appointment Status\n";
    cout << "11. View Reminders\n";
    cout << "12. Export Appointment (Mock Calendar API)\n";
    cout << "13. Exit\n";
    cout << string(40, '-') << "\n";
}

int main() {
#ifdef _WIN32
    enableAnsiColors();
#endif

    Scheduler scheduler;

    cout << BOLD << CYAN << "\nWelcome to the Appointment Scheduling System!\n" << RESET;
    scheduler.showReminders();

    bool running = true;
    while (running) {
        printMenu();
        int choice = getIntInput("Enter your choice (1-13): ");

        switch (choice) {
            case 1:  addAppointmentFlow(scheduler); break;
            case 2:  viewAllFlow(scheduler); break;
            case 3:  viewDailyFlow(scheduler); break;
            case 4:  viewWeeklyFlow(scheduler); break;
            case 5:  viewMonthlyFlow(scheduler); break;
            case 6:  editAppointmentFlow(scheduler); break;
            case 7:  cancelAppointmentFlow(scheduler); break;
            case 8:  deleteAppointmentFlow(scheduler); break;
            case 9:  searchFlow(scheduler); break;
            case 10: setStatusFlow(scheduler); break;
            case 11: scheduler.showReminders(); break;
            case 12: exportFlow(scheduler); break;
            case 13:
                running = false;
                cout << GREEN << "\nGoodbye! Your appointments have been saved.\n" << RESET;
                break;
            default:
                printError("Invalid choice. Please enter a number between 1 and 13.");
        }

        if (running) pauseScreen();
    }
    return 0;
}
