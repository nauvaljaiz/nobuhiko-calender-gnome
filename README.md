# Nobuhiko Calendar

Nobuhiko Calendar is an experimental native Linux calendar based on GNOME
Calendar. It keeps the GTK 4, libadwaita, and Evolution Data Server foundation
while adding deeper Google Calendar compatibility in small, testable stages.

The current development build uses the application ID
`local.nobuhiko.Calendar.Devel`, so it can be installed beside GNOME Calendar.
The first fork features add event controls for **Busy/Free** availability and
**Default/Public/Private** visibility, plus **custom reminder offsets** that
preserve imported alarm types. Recurring events support custom intervals,
weekday selection, monthly patterns, occurrence counts and end dates while
retaining recurrence rules that the editor cannot represent. Events can also
override their calendar color without discarding imported provider metadata.

## Project plan

- [Feature audit](docs/AUDIT.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Roadmap](docs/ROADMAP.md)
- [Validation record](docs/VALIDATION.md)

## Build

The project uses Meson. GNOME Builder is the recommended development
environment because it can build the supplied Flatpak manifest and provide the
required GNOME platform dependencies.

For a host build with the development packages installed:

```sh
meson setup _build -Dprofile=development
meson compile -C _build
meson test -C _build --print-errorlogs
```

Run the development binary with the build-time settings schema:

```sh
GSETTINGS_SCHEMA_DIR="$PWD/_build/data" _build/src/nobuhiko-calendar
```

## Upstream and license

This is an independent fork of
[GNOME Calendar](https://gitlab.gnome.org/GNOME/gnome-calendar), currently based
on version 51.0. GNOME Calendar and this fork are licensed under GPL-3.0-or-later.
The existing icon artwork and translations retain their upstream attribution.
Changes made for this fork should be reviewed and distributed as fork changes;
the upstream project has its own contribution policy in
[CONTRIBUTING.md](CONTRIBUTING.md).
