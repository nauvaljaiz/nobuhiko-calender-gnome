/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "gcal-event-color-section.h"

struct _GcalEventColorSection
{
  GcalEventEditorSection  parent;

  AdwSwitchRow           *custom_color_row;
  GtkColorDialogButton   *event_color_button;
  GtkWidget              *event_color_row;
  GtkWidget              *imported_color_row;
};

G_DEFINE_FINAL_TYPE (GcalEventColorSection, gcal_event_color_section, GCAL_TYPE_EVENT_EDITOR_SECTION)

static void
on_custom_color_changed_cb (AdwSwitchRow         *row,
                            GParamSpec           *pspec,
                            GcalEventColorSection *self)
{
  GcalEvent *event;

  event = gcal_event_editor_section_get_event (GCAL_EVENT_EDITOR_SECTION (self));
  if (!event)
    return;

  if (adw_switch_row_get_active (row))
    {
      const GdkRGBA *color = gcal_event_get_color (event);

      gtk_color_dialog_button_set_rgba (self->event_color_button, color);
      gcal_event_set_color_override (event, color);
    }
  else
    {
      gcal_event_set_color_override (event, NULL);
    }

  gtk_widget_set_visible (self->event_color_row, adw_switch_row_get_active (row));
  gtk_widget_set_visible (self->imported_color_row, gcal_event_has_imported_color (event));
}

static void
on_event_color_changed_cb (GtkColorDialogButton *button,
                           GParamSpec           *pspec,
                           GcalEventColorSection *self)
{
  GcalEvent *event;

  if (!adw_switch_row_get_active (self->custom_color_row))
    return;

  event = gcal_event_editor_section_get_event (GCAL_EVENT_EDITOR_SECTION (self));
  if (event)
    gcal_event_set_color_override (event, gtk_color_dialog_button_get_rgba (button));
}

static void
gcal_event_color_section_event_set_cb (GcalEventEditorSection *section,
                                       GcalEvent              *event)
{
  GcalEventColorSection *self = GCAL_EVENT_COLOR_SECTION (section);
  const GdkRGBA *color = event ? gcal_event_get_color_override (event) : NULL;
  gboolean has_custom_color = color != NULL;

  g_signal_handlers_block_by_func (self->custom_color_row, on_custom_color_changed_cb, self);
  g_signal_handlers_block_by_func (self->event_color_button, on_event_color_changed_cb, self);

  adw_switch_row_set_active (self->custom_color_row, has_custom_color);
  gtk_widget_set_visible (self->event_color_row, has_custom_color);
  gtk_widget_set_visible (self->imported_color_row, event && gcal_event_has_imported_color (event));

  if (event)
    gtk_color_dialog_button_set_rgba (self->event_color_button, color ? color : gcal_event_get_color (event));

  g_signal_handlers_unblock_by_func (self->event_color_button, on_event_color_changed_cb, self);
  g_signal_handlers_unblock_by_func (self->custom_color_row, on_custom_color_changed_cb, self);
}

static void
gcal_event_color_section_class_init (GcalEventColorSectionClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GcalEventEditorSectionClass *section_class = GCAL_EVENT_EDITOR_SECTION_CLASS (klass);

  section_class->event_set_cb = gcal_event_color_section_event_set_cb;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/gnome/calendar/ui/event-editor/gcal-event-color-section.ui");
  gtk_widget_class_bind_template_child (widget_class, GcalEventColorSection, custom_color_row);
  gtk_widget_class_bind_template_child (widget_class, GcalEventColorSection, event_color_button);
  gtk_widget_class_bind_template_child (widget_class, GcalEventColorSection, event_color_row);
  gtk_widget_class_bind_template_child (widget_class, GcalEventColorSection, imported_color_row);
  gtk_widget_class_bind_template_callback (widget_class, on_custom_color_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_event_color_changed_cb);
}

static void
gcal_event_color_section_init (GcalEventColorSection *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
