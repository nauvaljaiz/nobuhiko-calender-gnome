/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "gcal-availability-section.h"

struct _GcalAvailabilitySection
{
  GcalEventEditorSection  parent;

  AdwComboRow            *availability_row;
  AdwComboRow            *visibility_row;
};

G_DEFINE_FINAL_TYPE (GcalAvailabilitySection, gcal_availability_section, GCAL_TYPE_EVENT_EDITOR_SECTION)

static void
on_availability_changed_cb (GcalAvailabilitySection *self)
{
  GcalEvent *event;

  event = gcal_event_editor_section_get_event (GCAL_EVENT_EDITOR_SECTION (self));
  if (event)
    gcal_event_set_busy (event, adw_combo_row_get_selected (self->availability_row) == 0);
}

static void
on_visibility_changed_cb (GcalAvailabilitySection *self)
{
  GcalEvent *event;

  event = gcal_event_editor_section_get_event (GCAL_EVENT_EDITOR_SECTION (self));
  if (event)
    gcal_event_set_visibility (event,
                               (GcalEventVisibility) adw_combo_row_get_selected (self->visibility_row));
}

static void
gcal_availability_section_event_set_cb (GcalEventEditorSection *section,
                                        GcalEvent              *event)
{
  GcalAvailabilitySection *self = GCAL_AVAILABILITY_SECTION (section);

  g_signal_handlers_block_by_func (self->availability_row, on_availability_changed_cb, self);
  adw_combo_row_set_selected (self->availability_row, event && !gcal_event_get_busy (event) ? 1 : 0);
  g_signal_handlers_unblock_by_func (self->availability_row, on_availability_changed_cb, self);

  g_signal_handlers_block_by_func (self->visibility_row, on_visibility_changed_cb, self);
  adw_combo_row_set_selected (self->visibility_row,
                              event ? gcal_event_get_visibility (event) : GCAL_EVENT_VISIBILITY_DEFAULT);
  g_signal_handlers_unblock_by_func (self->visibility_row, on_visibility_changed_cb, self);
}

static void
gcal_availability_section_class_init (GcalAvailabilitySectionClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GcalEventEditorSectionClass *section_class = GCAL_EVENT_EDITOR_SECTION_CLASS (klass);

  section_class->event_set_cb = gcal_availability_section_event_set_cb;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/gnome/calendar/ui/event-editor/gcal-availability-section.ui");
  gtk_widget_class_bind_template_child (widget_class, GcalAvailabilitySection, availability_row);
  gtk_widget_class_bind_template_child (widget_class, GcalAvailabilitySection, visibility_row);
  gtk_widget_class_bind_template_callback (widget_class, on_availability_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_visibility_changed_cb);
}

static void
gcal_availability_section_init (GcalAvailabilitySection *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
