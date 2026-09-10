/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include "gcal-event-editor-section.h"

G_BEGIN_DECLS

#define GCAL_TYPE_EVENT_COLOR_SECTION (gcal_event_color_section_get_type ())
G_DECLARE_FINAL_TYPE (GcalEventColorSection, gcal_event_color_section, GCAL, EVENT_COLOR_SECTION, GcalEventEditorSection)

G_END_DECLS
