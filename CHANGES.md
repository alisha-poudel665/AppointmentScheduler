# What Changed

Goal: when an appointment is permanently deleted, the remaining appointments are
renumbered so IDs are always 1, 2, 3, ... with no gaps
(delete #1 -> old #2 becomes #1, old #3 becomes #2, and so on).

Lines starting with `-` were removed, lines starting with `+` were added.

## Summary

| File | Status | What changed |
|---|---|---|
| `Scheduler.cpp` | CHANGED | New `renumberIds()`; called after delete and when the file is loaded; "View All" now lists by ID |
| `Appointment.cpp` | CHANGED | New `setId()`; new `toInt()` / `intToString()` helpers (old MinGW has no `stoi` / `to_string`) |
| `main.cpp` | CHANGED | Delete prompt mentions renumbering; note shown after delete; uses the helpers |
| `README.md` | CHANGED | Added an "ID numbering" section |
| `FileManager.cpp` | unchanged | |
| `Notification.cpp` | unchanged | |
| `appointments.txt` | empty | No dummy data - you enter your own |

## Why the old code did not renumber
`deleteAppointment()` only removed the record. The other appointments kept their old
IDs, so after deleting #1 the next one was still #2. Also, "View All" was sorted by
date, so IDs could look mixed up on screen.

## How to build and run
```powershell
del *.exe
g++ main.cpp Appointment.cpp Scheduler.cpp FileManager.cpp Notification.cpp -o AppointmentScheduler.exe
.\AppointmentScheduler.exe
```

## Detailed changes (diff against your original files)

### Scheduler.cpp
```diff
--- original/Scheduler.cpp
+++ updated/Scheduler.cpp
@@ -31,10 +31,33 @@
         }
     }
 
+    // Renumbers all appointments so IDs are always 1, 2, 3, ... with no gaps.
+    // The list is first put back in ID order so the relative order of the
+    // appointments is preserved (e.g. old #2 becomes #1, old #3 becomes #2).
+    // Returns true if any ID actually changed.
+    bool renumberIds() {
+        sort(appointments.begin(), appointments.end(),
+             [](const Appointment &a, const Appointment &b) {
+                 return a.getId() < b.getId();
+             });
+
+        bool changed = false;
+        for (size_t i = 0; i < appointments.size(); i++) {
+            int newId = (int)i + 1;
+            if (appointments[i].getId() != newId) {
+                appointments[i].setId(newId);
+                changed = true;
+            }
+        }
+        refreshNextId();
+        return changed;
+    }
+
 public:
     Scheduler() : fileManager("appointments.txt") {
         appointments = fileManager.loadAppointments();
-        refreshNextId();
+        // Clean up any gaps left in an older appointments.txt file.
+        if (renumberIds()) fileManager.saveAppointments(appointments);
     }
 
     // ---------- Validation helpers ----------
@@ -49,8 +72,8 @@
             if (i == 4 || i == 7) continue;
             if (!isdigit((unsigned char)date[i])) return false;
         }
-        int month = stoi(date.substr(5, 2));
-        int day = stoi(date.substr(8, 2));
+        int month = toInt(date.substr(5, 2));
+        int day = toInt(date.substr(8, 2));
         if (month < 1 || month > 12) return false;
         if (day < 1 || day > 31) return false;
         return true;
@@ -64,7 +87,7 @@
             if (i == 4) continue;
             if (!isdigit((unsigned char)ym[i])) return false;
         }
-        int month = stoi(ym.substr(5, 2));
+        int month = toInt(ym.substr(5, 2));
         return month >= 1 && month <= 12;
     }
 
@@ -74,8 +97,8 @@
         if (t[2] != ':') return false;
         if (!isdigit((unsigned char)t[0]) || !isdigit((unsigned char)t[1]) ||
             !isdigit((unsigned char)t[3]) || !isdigit((unsigned char)t[4])) return false;
-        int hour = stoi(t.substr(0, 2));
-        int minute = stoi(t.substr(3, 2));
+        int hour = toInt(t.substr(0, 2));
+        int minute = toInt(t.substr(3, 2));
         if (hour < 0 || hour > 23) return false;
         if (minute < 0 || minute > 59) return false;
         return true;
@@ -84,9 +107,9 @@
     // Adds "days" to a YYYY-MM-DD date string, handling month/year rollover.
     static string addDays(const string &date, int days) {
         tm t = {};
-        t.tm_year = stoi(date.substr(0, 4)) - 1900;
-        t.tm_mon  = stoi(date.substr(5, 2)) - 1;
-        t.tm_mday = stoi(date.substr(8, 2)) + days;
+        t.tm_year = toInt(date.substr(0, 4)) - 1900;
+        t.tm_mon  = toInt(date.substr(5, 2)) - 1;
+        t.tm_mday = toInt(date.substr(8, 2)) + days;
         time_t tt = mktime(&t);
         tm* result = localtime(&tt);
         char buffer[11];
@@ -192,10 +215,13 @@
     }
 
     // Permanently removes an appointment from the list and file.
+    // The remaining appointments are renumbered so IDs stay 1, 2, 3, ...
+    // (deleting #1 makes the old #2 become the new #1, and so on).
     string deleteAppointment(int id) {
         for (size_t i = 0; i < appointments.size(); i++) {
             if (appointments[i].getId() == id) {
                 appointments.erase(appointments.begin() + i);
+                renumberIds();
                 fileManager.saveAppointments(appointments);
                 return "";
             }
@@ -226,9 +252,15 @@
 
     // ---------- Viewing ----------
 
+    // "View All" lists appointments in ID order (1, 2, 3, ...) so that the
+    // renumbering after a delete is easy to see. Daily/weekly/monthly views
+    // below are still sorted by date and time.
     vector<Appointment> getAll() {
-        sortByDateTime();
-        return appointments;
+        vector<Appointment> sorted = appointments;
+        sort(sorted.begin(), sorted.end(), [](const Appointment &a, const Appointment &b) {
+            return a.getId() < b.getId();
+        });
+        return sorted;
     }
 
     vector<Appointment> getByDate(const string &date) {
```

### Appointment.cpp
```diff
--- original/Appointment.cpp
+++ updated/Appointment.cpp
@@ -5,9 +5,32 @@
 #include <string>
 #include <sstream>
 #include <iomanip>
+#include <cstdlib>
+#include <cerrno>
+#include <climits>
+#include <stdexcept>
 
 using namespace std;
 
+// Portable replacements for std::stoi / std::to_string. Old MinGW builds
+// (e.g. GCC 6.3.0 on Windows) do not provide those two functions.
+inline int toInt(const string &text) {
+    const char *start = text.c_str();
+    char *end = nullptr;
+    errno = 0;
+    long value = strtol(start, &end, 10);
+    if (end == start) throw invalid_argument("toInt: no digits");
+    if (errno == ERANGE || value > INT_MAX || value < INT_MIN)
+        throw out_of_range("toInt: out of range");
+    return (int)value;
+}
+
+inline string intToString(int value) {
+    ostringstream out;
+    out << value;
+    return out.str();
+}
+
 // Represents a single appointment with all its details.
 // Demonstrates ENCAPSULATION: all data members are private,
 // accessed only through public getters/setters.
@@ -57,6 +80,8 @@
     string getStatus() const { return status; }
 
     // ---------- Setters ----------
+    // Used by the Scheduler when IDs are renumbered after a delete.
+    void setId(int newId) { id = newId; }
     void setTitle(const string &t) { title = t; }
     void setDate(const string &d) { date = d; }
     void setStartTime(const string &s) { startTime = s; }
@@ -77,7 +102,7 @@
     }
 
     // Rebuilds an Appointment object from a stored line of text.
-    // May throw (e.g. std::invalid_argument from stoi) if the line is corrupted;
+    // May throw (e.g. std::invalid_argument from toInt) if the line is corrupted;
     // the caller (FileManager) catches this.
     static Appointment fromFileLine(const string &line) {
         stringstream ss(line);
@@ -89,7 +114,7 @@
             index++;
         }
         Appointment a;
-        a.id = stoi(values[0]);
+        a.id = toInt(values[0]);
         a.title = values[1];
         a.date = values[2];
         a.startTime = values[3];
```

### main.cpp
```diff
--- original/main.cpp
+++ updated/main.cpp
@@ -51,7 +51,7 @@
     while (true) {
         string input = getLineInput(prompt);
         try {
-            return stoi(input);
+            return toInt(input);
         } catch (...) {
             printError("Please enter a valid whole number.");
         }
@@ -151,7 +151,7 @@
 
 void cancelAppointmentFlow(Scheduler &scheduler) {
     int id = getIntInput("Enter appointment ID to cancel: ");
-    string confirm = getLineInput("Cancel appointment #" + to_string(id) + "? (y/n): ");
+    string confirm = getLineInput("Cancel appointment #" + intToString(id) + "? (y/n): ");
     if (confirm != "y" && confirm != "Y") { printWarning("Cancel action aborted."); return; }
 
     string error = scheduler.cancelAppointment(id);
@@ -161,11 +161,15 @@
 
 void deleteAppointmentFlow(Scheduler &scheduler) {
     int id = getIntInput("Enter appointment ID to delete: ");
-    string confirm = getLineInput("This permanently deletes appointment #" + to_string(id) + ". Continue? (y/n): ");
+    string confirm = getLineInput("This permanently deletes appointment #" + intToString(id) +
+                                  " and renumbers the rest. Continue? (y/n): ");
     if (confirm != "y" && confirm != "Y") { printWarning("Delete action aborted."); return; }
 
     string error = scheduler.deleteAppointment(id);
-    if (error.empty()) printSuccess("Appointment deleted.");
+    if (error.empty()) {
+        printSuccess("Appointment deleted.");
+        printWarning("Remaining appointments were renumbered. Use 'View All' to see the new IDs.");
+    }
     else printError(error);
 }
 
```

### README.md
```diff
--- original/README.md
+++ updated/README.md
@@ -69,6 +69,13 @@
 "multiple definition" linker errors, so the `.cpp` files can safely `#include`
 each other. Include guards prevent the same file being pulled in twice.
 
+## ID numbering
+Appointment IDs are always kept as 1, 2, 3, ... with no gaps. When an appointment is
+permanently deleted (menu option 8), the remaining appointments are renumbered in
+their existing order, so deleting #1 turns the old #2 into the new #1. Cancelling
+(option 7) is a soft delete and does **not** change any IDs. Because IDs can change
+after a delete, use "View All" to check the current ID before editing or deleting.
+
 ## Known limitations (deliberate, to keep the project simple)
 - Date validation checks ranges (month 1–12, day 1–31) but not days-per-month,
   so `2026-02-30` passes format validation.
```
