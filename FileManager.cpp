#ifndef FILEMANAGER_CPP
#define FILEMANAGER_CPP

#include "Appointment.cpp"
#include <fstream>
#include <vector>
#include <iostream>
#include <string>

using namespace std;

// Handles all reading and writing of appointment data to a text file.
// Demonstrates ABSTRACTION: the rest of the program does not need to know
// how or where data is stored on disk, only that FileManager can save/load it.
class FileManager {
private:
    string filename;

public:
    FileManager() {
        filename = "appointments.txt";
    }

    FileManager(const string &fname) {
        filename = fname;
    }

    // Saves the full list of appointments to the file, overwriting old contents.
    // Returns false (and prints an error) if the file cannot be opened.
    bool saveAppointments(const vector<Appointment> &appointments) {
        ofstream outFile(filename);
        if (!outFile.is_open()) {
            cout << "[ERROR] Could not open file for saving: " << filename << "\n";
            return false;
        }
        for (const Appointment &a : appointments) {
            outFile << a.toFileLine() << "\n";
        }
        outFile.close();
        return true;
    }

    // Loads appointments from the file. If the file does not exist yet
    // (first run), this is treated as "no appointments", not an error.
    vector<Appointment> loadAppointments() {
        vector<Appointment> appointments;
        ifstream inFile(filename);
        if (!inFile.is_open()) {
            return appointments; // first run - empty list, not an error
        }
        string line;
        while (getline(inFile, line)) {
            if (line.empty()) continue;
            try {
                Appointment a = Appointment::fromFileLine(line);
                appointments.push_back(a);
            } catch (...) {
                cout << "[WARNING] Skipped a corrupted line in " << filename << "\n";
            }
        }
        inFile.close();
        return appointments;
    }
};

#endif
