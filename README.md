# Appointment Scheduling System (C++ / OOP)

A CLI-based appointment scheduler written in C++ using core OOP principles
(encapsulation, abstraction, constructors, composition). No header files —
each .cpp file uses an include guard and includes the others it depends on.

## Classes
- **Appointment** — one appointment's data + file serialization
- **FileManager** — reads/writes `appointments.txt`
- **Notification / CalendarAPI** — same-day reminders + mock calendar export
- **Scheduler** — business logic (add/edit/delete/search/conflict detection)

## Files
```
AppointmentScheduler/
├── main.cpp
├── Appointment.cpp
├── Scheduler.cpp
├── FileManager.cpp
├── Notification.cpp
├── appointments.txt      (auto-created on first save)
└── README.md
```

## Check the compiler
```powershell
g++ --version
```
If this fails, install MinGW-w64 and add its `bin` folder to your Windows PATH.

## Build
```powershell
g++ main.cpp Appointment.cpp Scheduler.cpp FileManager.cpp Notification.cpp -o AppointmentScheduler.exe
```

## Run
```powershell
.\AppointmentScheduler.exe
```

## Build and run in one command

PowerShell 7+:
```powershell
g++ main.cpp Appointment.cpp Scheduler.cpp FileManager.cpp Notification.cpp -o AppointmentScheduler.exe && .\AppointmentScheduler.exe
```

Windows PowerShell 5.1 (no `&&`):
```powershell
g++ main.cpp Appointment.cpp Scheduler.cpp FileManager.cpp Notification.cpp -o AppointmentScheduler.exe; if ($LASTEXITCODE -eq 0) { .\AppointmentScheduler.exe }
```

cmd.exe:
```cmd
g++ main.cpp Appointment.cpp Scheduler.cpp FileManager.cpp Notification.cpp -o AppointmentScheduler.exe && AppointmentScheduler.exe
```

## Data format
Pipe-delimited, one appointment per line in `appointments.txt`:
```
id|title|date|startTime|endTime|description|userName|reminderTime|status
```
Dates are `YYYY-MM-DD`, times are `HH:MM` (24-hour). These fixed-width formats
sort correctly with plain string comparison, which is why no date library is needed.

## Why there are no .h files
Every class method is defined inside its class body, making it implicitly `inline`.
Inline functions may appear in multiple translation units without causing
"multiple definition" linker errors, so the `.cpp` files can safely `#include`
each other. Include guards prevent the same file being pulled in twice.

## ID numbering
Appointment IDs are always kept as 1, 2, 3, ... with no gaps. When an appointment is
permanently deleted (menu option 8), the remaining appointments are renumbered in
their existing order, so deleting #1 turns the old #2 into the new #1. Cancelling
(option 7) is a soft delete and does **not** change any IDs. Because IDs can change
after a delete, use "View All" to check the current ID before editing or deleting.

## Known limitations (deliberate, to keep the project simple)
- Date validation checks ranges (month 1–12, day 1–31) but not days-per-month,
  so `2026-02-30` passes format validation.
- Editing treats a blank field as "keep old value", so there is no way to
  explicitly clear a reminder time through the edit menu.
- `CalendarAPI` is a mock/offline stub — it makes no real network calls.
