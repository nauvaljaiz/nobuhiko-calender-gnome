# Feature audit

Audit baseline: GNOME Calendar 51.0 at upstream commit `978546c2`, reviewed on
2026-09-09. “Google” refers to the Google Calendar web product and public APIs,
not an undocumented private service.

## Current foundation

| Area | GNOME Calendar 51 baseline | Google parity status |
|---|---|---|
| Native UI | GTK 4, libadwaita, adaptive layouts, keyboard navigation and accessibility | Strong native foundation |
| Data and accounts | Evolution Data Server (EDS), local calendars, CalDAV, WebDAV discovery and GNOME Online Accounts | Google calendars work through the system account stack, but Google-specific fields are incomplete |
| Views | Month, week and agenda | Day, year and schedule-style density controls are missing |
| Event editing | Title, location, notes, calendar, date/time, all-day, timezone and basic recurrence | Core editing works; advanced recurrence and several Google fields are missing |
| Reminders | Multiple display alarms from presets plus **custom 1–40,320 minute offsets in this fork**; imported alarm types remain separate and removable | Email delivery and calendar-level defaults still need provider support |
| Invitations | Organizer and attendee data are represented; attendee status can be displayed | Adding/removing guests, permissions, RSVP actions, rooms and proposed times are incomplete |
| Meetings | Common meeting URLs can be detected and opened | Creating Google Meet conference data is missing |
| Files | ICS import is present | Export workflow and Google Drive attachments UI are incomplete |
| Search | In-app search and GNOME Shell search provider | Advanced filters and Google-like search grammar are missing |
| Availability and visibility | **Implemented in this fork:** Busy/Free maps to iCalendar `TRANSP`; Default/Public/Private maps to `CLASS` | Matches the corresponding Google event controls, while preserving confidential imports |

## Google feature gaps

The public Calendar Events resource exposes writable fields for transparency,
visibility, reminders, attendees, guest permissions, attachments, conference
data, extended properties, colors, recurrence, and special event types. It also
contains read-only or server-managed values that a client must preserve. See the
[official Events reference](https://developers.google.com/workspace/calendar/api/v3/reference/events).

| Capability group | Missing or incomplete work | Delivery approach |
|---|---|---|
| Event details | Per-event color, custom recurrence, custom reminders, attachments and complete metadata preservation | Add provider-neutral event model APIs first, then editor sections gated by source capabilities |
| Collaboration | Guest editor, RSVP, optional/required guests, guest permissions, rooms/resources, availability grid and suggested times | Use iCalendar scheduling and EDS where supported; add Google API operations only for fields EDS cannot round-trip |
| Conferencing | Create/remove Google Meet; preserve all conference metadata | Google adapter using `conferenceDataVersion=1`; generic join-link UI remains provider-neutral |
| Calendars | ACL sharing, subscriptions, color/default reminders, working hours and resource browsing | Extend calendar management with capability checks and Google-specific pages where necessary |
| Views and productivity | Day/year views, tasks panel, print, richer search, keyboard commands | Implement in GTK without provider coupling |
| Google status events | Focus time, out of office and working location, including auto-decline settings | Model special event types explicitly; Google documents these through its [status event API](https://developers.google.com/workspace/calendar/api/guides/calendar-status) |
| Tasks | Task lists, due dates, completion and ordering | Separate adapter because [Google Tasks has its own API](https://developers.google.com/workspace/tasks/reference/rest) |
| Appointment schedules | Booking pages, availability rules, buffers, limits, forms, payments and reminders | Offer link/open support first; full parity depends on Google-hosted and subscription features described in [Google Calendar Help](https://support.google.com/calendar/answer/10729749) |
| Smart/cloud features | Gmail-created events, Gemini suggestions, Time Insights and organization policy features | Preserve and display server data when available; do not claim local reimplementation of closed services |

## Compatibility rules

1. Never drop iCalendar properties that the editor does not understand.
2. Do not show an editable control unless the selected calendar can save it.
3. New provider-neutral fields must round-trip through EDS before being enabled
   for CalDAV accounts.
4. Google-only writes go through a dedicated adapter and use server IDs, ETags,
   sync tokens and capability metadata.
5. Canceling the editor must leave the original event byte-for-byte unchanged.
6. Editing one field must preserve recurrence, organizer, attendees,
   classification, attachments and extension properties.

The first implementations follow these rules: absent or unknown `TRANSP` is
treated as Busy, confidential `CLASS` is shown as Private, and selecting the
same effective value does not rewrite the EDS component. EDS may normalize a
nonstandard `CLASS` value when it initially parses the imported iCalendar data;
the editor does not perform an additional rewrite.
