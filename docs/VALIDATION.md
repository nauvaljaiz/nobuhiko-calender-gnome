# Validation record

## 2026-09-09 — phase 0 and event details slices

Environment: Fedora 44, GNOME Calendar 51.0 source baseline, GCC 16.2.1, GTK
4.22.4, libadwaita 1.9.3, Evolution Data Server 3.60.2 and Meson 1.11.2.

Results:

- Clean Meson development build: 370/370 targets completed.
- Full Meson suite: 11/11 tests passed.
- Event tests cover Busy as the effective default; Free/Busy round-trips;
  Default/Public/Private visibility; confidential and unknown classifications;
  clone isolation; equality; and preservation of recurrence, organizer,
  attendees, attachments and custom properties.
- Strict GSettings compilation passed.
- Desktop file validation passed.
- AppStream validation passed with one informational note for the temporary
  `local.*` application ID.
- Indonesian message catalog validation passed, including the new editor labels.
- Development installation contains only the fork binary and fork registration
  names; no `org.gnome.Calendar` desktop registration is installed.
- Version smoke check returns `nobuhiko-calendar: Version 51.0-978546c2`.
- The application stayed running for an eight-second graphical smoke test under
  an isolated D-Bus session, X server and empty user-data directories. No app
  crash or GTK critical error was reported.

Interactive editor automation and live Google account synchronization are not
covered by this slice. The latter requires an opt-in test Google account and is
a release gate before Google adapter features ship.

## 2026-09-09 — custom reminders

- Added precise reminder offsets from 1 to 40,320 minutes before an event, the
  range accepted by [Google Calendar event reminder overrides](https://developers.google.com/workspace/calendar/api/v3/reference/events/update).
- Corrected alarm trigger classification so absolute alarms, alarms relative to
  event end, after-start alarms and sub-minute alarms are not mistaken for a
  normal before-start offset.
- Imported alarms are listed separately, retained during unrelated edits and
  removable by their exact alarm UID.
- Alarm equality now compares complete serialized alarm components, including
  trigger type, action and additional properties.
- Targeted event tests cover four mixed alarm types, full-content equality even
  when alarm counts match, adding and replacing custom reminders, exact removal
  and complete removal. Duplicate imported offsets are re-indexed after removing
  one alarm, so the remaining reminder stays editable.
- The full Meson suite remains green at 11/11 tests; strict schema, desktop file,
  AppStream and Indonesian catalog validation also pass.
- A fresh eight-second graphical smoke test confirms both the availability and
  reminders editor templates load from the application resources without a GTK
  critical error.
