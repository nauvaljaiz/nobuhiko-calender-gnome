# Roadmap

The sequence favors small vertical slices: model, UI, persistence, preservation
tests and user-visible behavior ship together. A phase advances only after the
existing test suite passes and unrelated event metadata survives round-trips.

## Phase 0 — safe fork baseline

- [x] Base the fork on GNOME Calendar 51.0.
- [x] Use a separate binary, app ID, desktop file, D-Bus service, search provider,
  icon names and GSettings schema.
- [x] Keep the upstream remote read-only in practice by naming it `upstream`.
- [x] Establish audit, architecture and validation records.
- [ ] Choose a permanent reverse-DNS application ID and create original artwork.
- [ ] Add reproducible Flatpak CI and signed development bundles.

## Phase 1 — complete ordinary event editing

- [x] Busy/Free availability with iCalendar preservation tests.
- [x] Default/Public/Private visibility with confidential and unknown-value preservation.
- [ ] Custom recurrence rules, exceptions and end conditions.
- [x] Custom reminder offsets with safe preservation of imported alarm types.
- [ ] Calendar-level default reminders and Google email reminder delivery.
- [ ] Per-event color and generic attachments.
- [ ] Export one event, a date range, or a calendar to ICS.

## Phase 2 — native productivity

- [ ] Day and year views.
- [ ] Richer agenda grouping and density controls.
- [ ] Advanced search filters and keyboard commands.
- [ ] Print and accessible export layouts.
- [ ] Time-zone comparison and secondary time zone.

## Phase 3 — collaboration

- [ ] Edit guests and their roles without losing scheduling metadata.
- [ ] Accept, decline, tentatively accept and propose a new time.
- [ ] Guest permissions and organizer-safe updates.
- [ ] Free/busy grid, suggested times, working-hours hints and rooms/resources.
- [ ] Generic conferencing link creation hooks.

## Phase 4 — Google Calendar adapter

- [ ] OAuth account connection and secret storage.
- [ ] Calendar list, ACL and settings capability discovery.
- [ ] Incremental sync cursor, ETag conflict handling and durable operation queue.
- [ ] Google Meet conference creation and Drive attachment metadata.
- [ ] Google event colors, extended properties and guest controls.
- [ ] Integration tests against recorded fixtures and an opt-in live test account.

## Phase 5 — tasks and status events

- [ ] Provider-neutral tasks panel plus Google Tasks adapter.
- [ ] Focus time, out-of-office and working-location event models.
- [ ] Google auto-decline and Chat-status options when the account supports them.
- [ ] Birthdays and other read-only special events with correct presentation.

## Phase 6 — hosted and advanced features

- [ ] Appointment-schedule discovery, booking-link display and open-in-browser.
- [ ] Optional self-hosted booking pages if the product scope requires them.
- [ ] Local time-allocation insights with explicit privacy controls.
- [ ] Preserve Gmail-generated and other server-managed events without pretending
  they can be created through public APIs.

## Phase 7 — release readiness

- [ ] Migration and rollback tests against real EDS profiles.
- [ ] Accessibility, localization, performance and offline test matrices.
- [ ] Privacy review, OAuth verification and account removal flows.
- [ ] Stable packaging, update channel, crash reporting policy and user docs.

## Current next slice

Add custom recurrence rules for intervals, weekdays, month positions, end
conditions and exceptions while preserving recurrence properties the editor
cannot represent.
