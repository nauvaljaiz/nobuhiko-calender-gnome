/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include "gcal-event-editor-section.h"

G_BEGIN_DECLS

#define GCAL_TYPE_AVAILABILITY_SECTION (gcal_availability_section_get_type ())
G_DECLARE_FINAL_TYPE (GcalAvailabilitySection, gcal_availability_section, GCAL, AVAILABILITY_SECTION, GcalEventEditorSection)

G_END_DECLS
