/* gcal-schedule-section.c
 *
 * Copyright 2020 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define G_LOG_DOMAIN "GcalScheduleSection"

#include "gcal-context.h"
#include "gcal-date-selector.h"
#include "gcal-date-chooser-row.h"
#include "gcal-date-time-chooser.h"
#include "gcal-debug.h"
#include "gcal-event.h"
#include "gcal-event-schedule.h"
#include "gcal-recurrence.h"
#include "gcal-schedule-section.h"
#include "gcal-utils.h"

#include <glib/gi18n.h>

struct _GcalScheduleSection
{
  GcalEventEditorSection parent;

  GcalEventSchedule *values;

  AdwViewStack        *schedule_type_stack;
  GcalDateChooserRow  *start_date_row;
  GcalDateChooserRow  *end_date_row;
  GcalDateTimeChooser *start_date_time_chooser;
  GcalDateTimeChooser *end_date_time_chooser;
  GtkToggleButton     *friday_button;
  GtkWidget           *imported_recurrence_row;
  GtkToggleButton     *monday_button;
  GtkWidget           *monthly_pattern_combo;
  GtkWidget           *number_of_occurrences_spin;
  GtkWidget           *repeat_combo;
  GtkWidget           *repeat_duration_combo;
  GtkWidget           *repeat_interval_row;
  GtkWidget           *repeat_interval_spin;
  GtkLabel            *repeat_interval_unit_label;
  GtkToggleButton     *saturday_button;
  GtkToggleButton     *sunday_button;
  GtkToggleButton     *thursday_button;
  GtkToggleButton     *tuesday_button;
  GtkWidget           *until_date_selector;
  GtkToggleButton     *wednesday_button;
  GtkWidget           *weekday_row;
};

/* Desired state of the widgets, computed from GcalEventSchedule.
 *
 * There is some subtle logic to decide how to update the widgets based on how
 * each of them causes the GcalEventSchedule to be modified.  We encapsulate
 * the desired state in this struct, so we can have tests for it.
 */
typedef struct
{
  gboolean schedule_type_all_day;
  gboolean date_widgets_visible;
  gboolean date_time_widgets_visible;

  GcalTimeFormat time_format;

  /* Note that the displayed date/time may not correspond to the real data in the
   * GcalEventSchedule.  See the comment in widget_state_from_values().
   */
  GDateTime *date_time_start;
  GDateTime *date_time_end;

  struct {
    gboolean duration_combo_visible;
    gboolean number_of_occurrences_visible;
    gboolean until_date_visible;
    gboolean interval_visible;
    gboolean weekdays_visible;
    gboolean monthly_pattern_visible;
    gboolean imported_visible;
    GcalRecurrenceFrequency frequency;
    GcalRecurrenceLimitType limit_type;
    GcalRecurrenceMonthlyPattern monthly_pattern;
    guint interval;
    guint week_days;
    gint month_day;
    gint month_week;
    GDateWeekday month_weekday;
    guint limit_count;
    GDateTime *limit_until;
  } recurrence;
} WidgetState;

static void widget_state_free (WidgetState *state);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (WidgetState, widget_state_free);

G_DEFINE_FINAL_TYPE (GcalScheduleSection, gcal_schedule_section, GCAL_TYPE_EVENT_EDITOR_SECTION)

static void          on_start_date_changed_cb                    (GtkWidget          *widget,
                                                                  GParamSpec         *pspec,
                                                                  GcalScheduleSection *self);

static void          on_end_date_changed_cb                      (GtkWidget          *widget,
                                                                  GParamSpec         *pspec,
                                                                  GcalScheduleSection *self);

static void          on_start_date_time_changed_cb               (GtkWidget          *widget,
                                                                  GParamSpec         *pspec,
                                                                  GcalScheduleSection *self);

static void          on_end_date_time_changed_cb                 (GtkWidget          *widget,
                                                                  GParamSpec         *pspec,
                                                                  GcalScheduleSection *self);

static void          on_schedule_type_changed_cb                 (GtkWidget           *widget,
                                                                  GParamSpec          *pspec,
                                                                  GcalScheduleSection *self);

static void          on_number_of_occurrences_changed_cb         (GcalScheduleSection *self);
static void          on_until_date_changed_cb                    (GcalScheduleSection *self);
static void          on_repeat_duration_changed_cb               (GtkWidget           *widget,
                                                                  GParamSpec          *pspec,
                                                                  GcalScheduleSection *self);
static void          on_repeat_type_changed_cb                   (GtkWidget           *widget,
                                                                  GParamSpec          *pspec,
                                                                  GcalScheduleSection *self);
static void          on_repeat_interval_changed_cb               (GcalScheduleSection *self);
static void          on_monthly_pattern_changed_cb               (GtkWidget           *widget,
                                                                  GParamSpec          *pspec,
                                                                  GcalScheduleSection *self);
static void          on_weekday_toggled_cb                       (GtkToggleButton      *button,
                                                                  GcalScheduleSection *self);

static WidgetState *
widget_state_from_values (const GcalEventSchedule *values)
{
  WidgetState *state = g_new0 (WidgetState, 1);

  state->schedule_type_all_day = values->curr.all_day;
  state->date_widgets_visible = values->curr.all_day;
  state->date_time_widgets_visible = !values->curr.all_day;
  state->time_format = values->time_format;

  if (values->curr.date_start && values->curr.date_end)
    {
      state->date_time_start = g_date_time_ref (values->curr.date_start);

      if (values->curr.all_day)
        {
          /* While in iCalendar a single-day all-day event goes from $day to $day+1, we don't
           * want to show the user a single-day all-day event as starting on Feb 1 and ending
           * on Feb 2, for example.
           *
           * So, we maintain the "real" data in GcalEventSchedule with the $day+1 as iCalendar wants,
           * but for display purposes, we subtract a day from the end date to show the expected thing to the
           * user.  When the widget changes, we add the day back - see gcal_schedule_values_set_end_date().
           *
           * See https://bugzilla.gnome.org/show_bug.cgi?id=769300 for the original bug about this.
           *
           * Keep in sync with gcal_schedule_values_set_end_date().
           */
          state->date_time_end = g_date_time_add_days (values->curr.date_end, -1);
        }
      else
        {
          state->date_time_end = g_date_time_ref (values->curr.date_end);
        }
    }
  else
    {
      /* The event editor can be instantiated but not shown yet, and we may get a
       * notification that the time_format changed.  In that case, the event will not be
       * set yet, and so the dates will not be set either - hence the test above for
       * date_start and date_end not being NULL.  Handle that by setting NULL dates for
       * the widgets.
       */
      state->date_time_start = NULL;
      state->date_time_end = NULL;
    }

  if (values->curr.recur && values->curr.recur->frequency != GCAL_RECURRENCE_NO_REPEAT)
    {
      state->recurrence.frequency = values->curr.recur->frequency;
      state->recurrence.limit_type = values->curr.recur->limit_type;
      state->recurrence.imported_visible = values->curr.recur->frequency == GCAL_RECURRENCE_OTHER;
      state->recurrence.duration_combo_visible = !state->recurrence.imported_visible;
      state->recurrence.interval_visible = !state->recurrence.imported_visible;
      state->recurrence.weekdays_visible = values->curr.recur->frequency == GCAL_RECURRENCE_WEEKLY ||
                                           values->curr.recur->frequency == GCAL_RECURRENCE_MON_FRI;
      state->recurrence.monthly_pattern_visible = values->curr.recur->frequency == GCAL_RECURRENCE_MONTHLY;
      state->recurrence.interval = values->curr.recur->interval;
      state->recurrence.week_days = values->curr.recur->week_days;
      state->recurrence.monthly_pattern = values->curr.recur->monthly_pattern;
      state->recurrence.month_day = values->curr.recur->month_day;
      state->recurrence.month_week = values->curr.recur->month_week;
      state->recurrence.month_weekday = values->curr.recur->month_weekday;

      if (state->recurrence.weekdays_visible && state->recurrence.week_days == 0 && values->curr.date_start)
        state->recurrence.week_days = GCAL_RECURRENCE_WEEKDAY_MASK (g_date_time_get_day_of_week (values->curr.date_start));

      if (state->recurrence.monthly_pattern_visible &&
          state->recurrence.monthly_pattern == GCAL_RECURRENCE_MONTHLY_DAY_OF_MONTH &&
          state->recurrence.month_day == 0 && values->curr.date_start)
        state->recurrence.month_day = g_date_time_get_day_of_month (values->curr.date_start);

      switch (state->recurrence.imported_visible ? GCAL_RECURRENCE_FOREVER : values->curr.recur->limit_type)
        {
        case GCAL_RECURRENCE_COUNT:
          state->recurrence.limit_count = values->curr.recur->limit.count;
          state->recurrence.number_of_occurrences_visible = TRUE;
          break;

        case GCAL_RECURRENCE_UNTIL:
          state->recurrence.until_date_visible = TRUE;
          state->recurrence.limit_until = g_date_time_ref (values->curr.recur->limit.until);
          break;

        case GCAL_RECURRENCE_FOREVER:
          state->recurrence.limit_count = 0;
          break;

        default:
          g_assert_not_reached ();
        }
    }
  else
    {
      state->recurrence.duration_combo_visible = FALSE;
      state->recurrence.number_of_occurrences_visible = FALSE;
      state->recurrence.until_date_visible = FALSE;
      state->recurrence.interval_visible = FALSE;
      state->recurrence.weekdays_visible = FALSE;
      state->recurrence.monthly_pattern_visible = FALSE;
      state->recurrence.imported_visible = FALSE;
      state->recurrence.frequency = GCAL_RECURRENCE_NO_REPEAT;
      state->recurrence.limit_type = GCAL_RECURRENCE_FOREVER;
      state->recurrence.monthly_pattern = GCAL_RECURRENCE_MONTHLY_DAY_OF_MONTH;
      state->recurrence.interval = 1;
      state->recurrence.week_days = 0;
      state->recurrence.month_day = 0;
      state->recurrence.month_week = 0;
      state->recurrence.month_weekday = G_DATE_BAD_WEEKDAY;
      state->recurrence.limit_count = 0;
      state->recurrence.limit_until = NULL;
    }

  return state;
}

static void
widget_state_free (WidgetState *state)
{
  g_date_time_unref (state->date_time_start);
  g_date_time_unref (state->date_time_end);

  if (state->recurrence.limit_until)
    {
      g_date_time_unref (state->recurrence.limit_until);
    }

  g_free (state);
}

/*
 * Auxiliary methods
 */

static gboolean
all_day_selected (GcalScheduleSection *self)
{
  const gchar *active = adw_view_stack_get_visible_child_name (self->schedule_type_stack);
  return g_strcmp0 (active, "all-day") == 0;
}


static void
block_date_signals (GcalScheduleSection *self)
{
  g_signal_handlers_block_by_func (self->start_date_row, on_start_date_changed_cb, self);
  g_signal_handlers_block_by_func (self->end_date_row, on_end_date_changed_cb, self);

  g_signal_handlers_block_by_func (self->start_date_time_chooser, on_start_date_time_changed_cb, self);
  g_signal_handlers_block_by_func (self->end_date_time_chooser, on_end_date_time_changed_cb, self);
}

static void
unblock_date_signals (GcalScheduleSection *self)
{
  g_signal_handlers_unblock_by_func (self->end_date_time_chooser, on_end_date_time_changed_cb, self);
  g_signal_handlers_unblock_by_func (self->start_date_time_chooser, on_start_date_time_changed_cb, self);

  g_signal_handlers_unblock_by_func (self->end_date_row, on_end_date_changed_cb, self);
  g_signal_handlers_unblock_by_func (self->start_date_row, on_start_date_changed_cb, self);
}

static void
remove_recurrence_properties (GcalEvent *event,
                              gboolean   remove_exceptions)
{
  ECalComponent *comp;

  comp = gcal_event_get_component (event);

  e_cal_component_set_recurid (comp, NULL);
  e_cal_component_set_rrules (comp, NULL);

  if (remove_exceptions)
    {
      e_cal_component_set_rdates (comp, NULL);
      e_cal_component_set_exrules (comp, NULL);
      e_cal_component_set_exdates (comp, NULL);
    }

  e_cal_component_commit_sequence (comp);
}

static GtkToggleButton *
weekday_button (GcalScheduleSection *self,
                GDateWeekday        day)
{
  switch (day)
    {
    case G_DATE_MONDAY:    return self->monday_button;
    case G_DATE_TUESDAY:   return self->tuesday_button;
    case G_DATE_WEDNESDAY: return self->wednesday_button;
    case G_DATE_THURSDAY:  return self->thursday_button;
    case G_DATE_FRIDAY:    return self->friday_button;
    case G_DATE_SATURDAY:  return self->saturday_button;
    case G_DATE_SUNDAY:    return self->sunday_button;
    default: g_assert_not_reached ();
    }
}

static void
block_recurrence_signals (GcalScheduleSection *self)
{
  g_signal_handlers_block_by_func (self->repeat_combo, on_repeat_type_changed_cb, self);
  g_signal_handlers_block_by_func (self->repeat_duration_combo, on_repeat_duration_changed_cb, self);
  g_signal_handlers_block_by_func (self->repeat_interval_spin, on_repeat_interval_changed_cb, self);
  g_signal_handlers_block_by_func (self->monthly_pattern_combo, on_monthly_pattern_changed_cb, self);

  for (GDateWeekday day = G_DATE_MONDAY; day <= G_DATE_SUNDAY; day++)
    g_signal_handlers_block_by_func (weekday_button (self, day), on_weekday_toggled_cb, self);
}

static void
unblock_recurrence_signals (GcalScheduleSection *self)
{
  g_signal_handlers_unblock_by_func (self->repeat_combo, on_repeat_type_changed_cb, self);
  g_signal_handlers_unblock_by_func (self->repeat_duration_combo, on_repeat_duration_changed_cb, self);
  g_signal_handlers_unblock_by_func (self->repeat_interval_spin, on_repeat_interval_changed_cb, self);
  g_signal_handlers_unblock_by_func (self->monthly_pattern_combo, on_monthly_pattern_changed_cb, self);

  for (GDateWeekday day = G_DATE_MONDAY; day <= G_DATE_SUNDAY; day++)
    g_signal_handlers_unblock_by_func (weekday_button (self, day), on_weekday_toggled_cb, self);
}

static const gchar *
recurrence_unit (GcalRecurrenceFrequency frequency,
                 guint                    interval)
{
  switch (frequency)
    {
    case GCAL_RECURRENCE_DAILY:
      return ngettext ("day", "days", interval);
    case GCAL_RECURRENCE_MON_FRI:
    case GCAL_RECURRENCE_WEEKLY:
      return ngettext ("week", "weeks", interval);
    case GCAL_RECURRENCE_MONTHLY:
      return ngettext ("month", "months", interval);
    case GCAL_RECURRENCE_YEARLY:
      return ngettext ("year", "years", interval);
    default:
      return "";
    }
}

static const gchar *
weekday_name (GDateWeekday weekday)
{
  switch (weekday)
    {
    case G_DATE_MONDAY:    return _("Monday");
    case G_DATE_TUESDAY:   return _("Tuesday");
    case G_DATE_WEDNESDAY: return _("Wednesday");
    case G_DATE_THURSDAY:  return _("Thursday");
    case G_DATE_FRIDAY:    return _("Friday");
    case G_DATE_SATURDAY:  return _("Saturday");
    case G_DATE_SUNDAY:    return _("Sunday");
    default:               return "";
    }
}

static const gchar *
ordinal_name (gint position)
{
  switch (position)
    {
    case 1:  return _("First");
    case 2:  return _("Second");
    case 3:  return _("Third");
    case 4:  return _("Fourth");
    case 5:  return _("Fifth");
    case -1: return _("Last");
    default: return "";
    }
}

static gchar *
monthly_pattern_description (WidgetState *state)
{
  if (!state->recurrence.monthly_pattern_visible)
    return NULL;

  if (state->recurrence.monthly_pattern == GCAL_RECURRENCE_MONTHLY_DAY_OF_MONTH)
    {
      if (state->recurrence.month_day == -1)
        return g_strdup (_("Last day of the month"));

      return g_strdup_printf (_("Day %d"), state->recurrence.month_day);
    }

  return g_strdup_printf (_("%1$s %2$s"),
                          ordinal_name (state->recurrence.month_week),
                          weekday_name (state->recurrence.month_weekday));
}

static void
update_widgets (GcalScheduleSection *self,
                WidgetState         *state)
{
  g_autofree gchar *monthly_description = monthly_pattern_description (state);

  g_signal_handlers_block_by_func (self->schedule_type_stack, on_schedule_type_changed_cb, self);
  g_signal_handlers_block_by_func (self->number_of_occurrences_spin, on_number_of_occurrences_changed_cb, self);
  g_signal_handlers_block_by_func (self->until_date_selector, on_until_date_changed_cb, self);
  block_recurrence_signals (self);
  block_date_signals (self);

  adw_view_stack_set_visible_child_name (self->schedule_type_stack,
                                         state->schedule_type_all_day ? "all-day" : "time-slot");

  gcal_date_time_chooser_set_time_format (self->start_date_time_chooser, state->time_format);
  gcal_date_time_chooser_set_time_format (self->end_date_time_chooser, state->time_format);

  /* date */
  if (state->date_time_start)
    {
      gcal_date_chooser_row_set_date (self->start_date_row, state->date_time_start);
      gcal_date_time_chooser_set_date_time (self->start_date_time_chooser, state->date_time_start);
    }

  if (state->date_time_end)
    {
      gcal_date_chooser_row_set_date (self->end_date_row, state->date_time_end);
      gcal_date_time_chooser_set_date_time (self->end_date_time_chooser, state->date_time_end);
    }

  /* Recurrences */
  gtk_widget_set_visible (self->repeat_duration_combo, state->recurrence.duration_combo_visible);
  gtk_widget_set_visible (self->repeat_interval_row, state->recurrence.interval_visible);
  gtk_widget_set_visible (self->weekday_row, state->recurrence.weekdays_visible);
  gtk_widget_set_visible (self->monthly_pattern_combo, state->recurrence.monthly_pattern_visible);
  gtk_widget_set_visible (self->imported_recurrence_row, state->recurrence.imported_visible);

  adw_combo_row_set_selected (ADW_COMBO_ROW (self->repeat_combo), state->recurrence.frequency);
  adw_combo_row_set_selected (ADW_COMBO_ROW (self->repeat_duration_combo), state->recurrence.limit_type);
  adw_combo_row_set_selected (ADW_COMBO_ROW (self->monthly_pattern_combo), state->recurrence.monthly_pattern);
  adw_action_row_set_subtitle (ADW_ACTION_ROW (self->monthly_pattern_combo), monthly_description);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (self->repeat_interval_spin), state->recurrence.interval);
  gtk_label_set_label (self->repeat_interval_unit_label,
                       recurrence_unit (state->recurrence.frequency, state->recurrence.interval));

  for (GDateWeekday day = G_DATE_MONDAY; day <= G_DATE_SUNDAY; day++)
    gtk_toggle_button_set_active (weekday_button (self, day),
                                  (state->recurrence.week_days & GCAL_RECURRENCE_WEEKDAY_MASK (day)) != 0);

  gtk_widget_set_visible (self->until_date_selector, state->recurrence.until_date_visible);
  if (state->recurrence.until_date_visible)
    {
      gcal_date_selector_set_date (GCAL_DATE_SELECTOR (self->until_date_selector), state->recurrence.limit_until);
    }

  gtk_widget_set_visible (self->number_of_occurrences_spin, state->recurrence.number_of_occurrences_visible);
  if (state->recurrence.number_of_occurrences_visible)
    {
      gtk_spin_button_set_value (GTK_SPIN_BUTTON (self->number_of_occurrences_spin), state->recurrence.limit_count);
    }

  unblock_date_signals (self);
  unblock_recurrence_signals (self);
  g_signal_handlers_unblock_by_func (self->until_date_selector, on_until_date_changed_cb, self);
  g_signal_handlers_unblock_by_func (self->number_of_occurrences_spin, on_number_of_occurrences_changed_cb, self);
  g_signal_handlers_unblock_by_func (self->schedule_type_stack, on_schedule_type_changed_cb, self);
}

/* Recomputes and sets the widget state.  Assumes self->values has already been updated. */
static void
refresh (GcalScheduleSection *self)
{
  g_autoptr (WidgetState) state = widget_state_from_values (self->values);
  update_widgets (self, state);
}

/* Updates the widgets, and the current values, from the specified ones.
 *
 * This will free the current values and replace them with new ones.
 */
static void
update_from_event_schedule (GcalScheduleSection *self,
                            GcalEventSchedule *values)
{
  GcalEvent *event;

  GCAL_ENTRY;

  event = gcal_event_editor_section_get_event (GCAL_EVENT_EDITOR_SECTION (self));

  gcal_event_schedule_free (self->values);
  self->values = values;

  gcal_event_set_all_day (event, self->values->curr.all_day);
  gcal_event_set_date_start (event, self->values->curr.date_start);
  gcal_event_set_date_end (event, self->values->curr.date_end);

  /* Keep the original recurrence properties byte-for-byte during unrelated
   * edits. Replacing an imported rule deliberately clears recurrence parts
   * that this editor cannot represent; editing a supported rule keeps EXDATE
   * exceptions intact. */
  if (!gcal_recurrence_is_equal (self->values->curr.recur, gcal_event_get_recurrence (event)))
    {
      GcalRecurrence *old_recur = gcal_event_get_recurrence (event);
      gboolean replacing_imported = old_recur && old_recur->frequency == GCAL_RECURRENCE_OTHER;

      remove_recurrence_properties (event, replacing_imported || !self->values->curr.recur);

      gcal_event_set_recurrence (event, self->values->curr.recur);
    }

  refresh (self);

  GCAL_EXIT;
}


/*
 * Callbacks
 */

static void
on_schedule_type_changed_cb (GtkWidget           *widget,
                             GParamSpec          *pspec,
                             GcalScheduleSection *self)
{
  gboolean all_day = all_day_selected (self);
  GcalEventSchedule *updated = gcal_event_schedule_set_all_day (self->values, all_day);
  update_from_event_schedule (self, updated);
}

static void
on_start_date_changed_cb (GtkWidget           *widget,
                          GParamSpec          *pspec,
                          GcalScheduleSection *self)
{
  GDateTime *start = gcal_date_chooser_row_get_date (self->start_date_row);
  GcalEventSchedule *updated = gcal_event_schedule_set_start_date (self->values, start);
  update_from_event_schedule (self, updated);
}

static void
on_end_date_changed_cb (GtkWidget           *widget,
                        GParamSpec          *pspec,
                        GcalScheduleSection *self)
{
  GDateTime *end = gcal_date_chooser_row_get_date (self->end_date_row);
  GcalEventSchedule *updated = gcal_event_schedule_set_end_date (self->values, end);
  update_from_event_schedule (self, updated);
}

static void
on_start_date_time_changed_cb (GtkWidget           *widget,
                               GParamSpec          *pspec,
                               GcalScheduleSection *self)
{
  GDateTime *start = gcal_date_time_chooser_get_date_time (self->start_date_time_chooser);
  GcalEventSchedule *updated = gcal_event_schedule_set_start_date_time (self->values, start);
  update_from_event_schedule (self, updated);
}

static void
on_end_date_time_changed_cb (GtkWidget           *widget,
                             GParamSpec          *pspec,
                             GcalScheduleSection *self)
{
  GDateTime *end = gcal_date_time_chooser_get_date_time (self->end_date_time_chooser);
  GcalEventSchedule *updated = gcal_event_schedule_set_end_date_time (self->values, end);
  update_from_event_schedule (self, updated);
}

static void
on_number_of_occurrences_changed_cb (GcalScheduleSection *self)
{
  guint count = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (self->number_of_occurrences_spin));
  GcalEventSchedule *updated = gcal_event_schedule_set_recurrence_count (self->values, count);
  update_from_event_schedule (self, updated);
}

static void
on_until_date_changed_cb (GcalScheduleSection *self)
{
  GDateTime *until = gcal_date_selector_get_date (GCAL_DATE_SELECTOR (self->until_date_selector));
  GcalEventSchedule *updated = gcal_event_schedule_set_recurrence_until (self->values, until);
  update_from_event_schedule (self, updated);
}

static void
on_repeat_interval_changed_cb (GcalScheduleSection *self)
{
  guint interval = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (self->repeat_interval_spin));
  GcalEventSchedule *updated = gcal_event_schedule_set_recurrence_interval (self->values, interval);
  update_from_event_schedule (self, updated);
}

static void
on_monthly_pattern_changed_cb (GtkWidget           *widget,
                               GParamSpec          *pspec,
                               GcalScheduleSection *self)
{
  guint selected = adw_combo_row_get_selected (ADW_COMBO_ROW (self->monthly_pattern_combo));
  GcalEventSchedule *updated;

  g_assert (selected <= GCAL_RECURRENCE_MONTHLY_NTH_WEEKDAY);
  updated = gcal_event_schedule_set_monthly_pattern (self->values, selected);
  update_from_event_schedule (self, updated);
}

static void
on_weekday_toggled_cb (GtkToggleButton      *button,
                       GcalScheduleSection *self)
{
  guint week_days = 0;

  for (GDateWeekday day = G_DATE_MONDAY; day <= G_DATE_SUNDAY; day++)
    {
      if (gtk_toggle_button_get_active (weekday_button (self, day)))
        week_days |= GCAL_RECURRENCE_WEEKDAY_MASK (day);
    }

  if (week_days == 0)
    {
      g_signal_handlers_block_by_func (button, on_weekday_toggled_cb, self);
      gtk_toggle_button_set_active (button, TRUE);
      g_signal_handlers_unblock_by_func (button, on_weekday_toggled_cb, self);
      return;
    }

  update_from_event_schedule (self,
                              gcal_event_schedule_set_recurrence_week_days (self->values, week_days));
}

static GcalRecurrenceLimitType
get_recurence_limit_type (GcalScheduleSection *self)
{
  guint item = adw_combo_row_get_selected (ADW_COMBO_ROW (self->repeat_duration_combo));

  g_assert (item >= GCAL_RECURRENCE_FOREVER && item <= GCAL_RECURRENCE_UNTIL);

  return (GcalRecurrenceLimitType) item;
}

static GcalRecurrenceFrequency
get_recurrence_frequency (GcalScheduleSection *self)
{
  guint item = adw_combo_row_get_selected (ADW_COMBO_ROW (self->repeat_combo));

  g_assert (item >= GCAL_RECURRENCE_NO_REPEAT && item <= GCAL_RECURRENCE_OTHER);

  return (GcalRecurrenceFrequency) item;
}

static void
on_repeat_duration_changed_cb (GtkWidget           *widget,
                               GParamSpec          *pspec,
                               GcalScheduleSection *self)
{
  if (get_recurrence_frequency (self) != GCAL_RECURRENCE_NO_REPEAT &&
      get_recurrence_frequency (self) != GCAL_RECURRENCE_OTHER)
    {
      GcalRecurrenceLimitType limit_type = get_recurence_limit_type (self);
      GcalEventSchedule *updated = gcal_event_schedule_set_recur_limit_type (self->values, limit_type);
      update_from_event_schedule (self, updated);
    }
}

static void
on_repeat_type_changed_cb (GtkWidget           *widget,
                           GParamSpec          *pspec,
                           GcalScheduleSection *self)
{
  GcalRecurrenceFrequency frequency = get_recurrence_frequency (self);
  GcalEventSchedule *updated;

  if (frequency == GCAL_RECURRENCE_OTHER &&
      (!self->values->curr.recur || self->values->curr.recur->frequency != GCAL_RECURRENCE_OTHER))
    {
      refresh (self);
      return;
    }

  updated = gcal_event_schedule_set_recur_frequency (self->values, frequency);
  update_from_event_schedule (self, updated);
}

static void
on_time_format_changed_cb (GcalScheduleSection *self)
{
  GcalContext *context;

  if (gcal_event_editor_section_get_event (GCAL_EVENT_EDITOR_SECTION (self)))
    {
      context = gcal_application_get_context (GCAL_DEFAULT_APPLICATION);

      /* Apparently we can be notified that the time format changed, even when
       * there has not been an event set yet.  This breaks the code downstream.
       */
      GcalTimeFormat time_format = gcal_context_get_time_format (context);

      GcalEventSchedule *updated = gcal_event_schedule_set_time_format (self->values, time_format);

      update_from_event_schedule (self, updated);
    }
}


/*
 * GtkWidget overrides
 */

static void
gcal_schedule_section_event_set_cb (GcalEventEditorSection *section,
                                    GcalEvent              *event)
{
  GcalScheduleSection *self = GCAL_SCHEDULE_SECTION (section);
  GcalTimeFormat time_format;
  GcalContext *context;

  context = gcal_application_get_context (GCAL_DEFAULT_APPLICATION);

  time_format = gcal_context_get_time_format (context);
  self->values = gcal_event_schedule_from_event (event, time_format);

  if (event)
    refresh (self);
}


/*
 * GObject overrides
 */

static void
gcal_schedule_section_finalize (GObject *object)
{
  GcalScheduleSection *self = (GcalScheduleSection *)object;

  g_clear_pointer (&self->values, gcal_event_schedule_free);

  G_OBJECT_CLASS (gcal_schedule_section_parent_class)->finalize (object);
}

static void
gcal_schedule_section_class_init (GcalScheduleSectionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GcalEventEditorSectionClass *section_class = GCAL_EVENT_EDITOR_SECTION_CLASS (klass);

  g_type_ensure (GCAL_TYPE_DATE_TIME_CHOOSER);

  object_class->finalize = gcal_schedule_section_finalize;

  section_class->event_set_cb = gcal_schedule_section_event_set_cb;

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/calendar/ui/event-editor/gcal-schedule-section.ui");

  gtk_widget_class_bind_template_child (widget_class, GcalScheduleSection, schedule_type_stack);
  gtk_widget_class_bind_template_child (widget_class, GcalScheduleSection, start_date_row);
  gtk_widget_class_bind_template_child (widget_class, GcalScheduleSection, end_date_row);
  gtk_widget_class_bind_template_child (widget_class, GcalScheduleSection, start_date_time_chooser);
  gtk_widget_class_bind_template_child (widget_class, GcalScheduleSection, end_date_time_chooser);
  gtk_widget_class_bind_template_child (widget_class, GcalScheduleSection, friday_button);
  gtk_widget_class_bind_template_child (widget_class, GcalScheduleSection, imported_recurrence_row);
  gtk_widget_class_bind_template_child (widget_class, GcalScheduleSection, monday_button);
  gtk_widget_class_bind_template_child (widget_class, GcalScheduleSection, monthly_pattern_combo);
  gtk_widget_class_bind_template_child (widget_class, GcalScheduleSection, number_of_occurrences_spin);
  gtk_widget_class_bind_template_child (widget_class, GcalScheduleSection, repeat_combo);
  gtk_widget_class_bind_template_child (widget_class, GcalScheduleSection, repeat_duration_combo);
  gtk_widget_class_bind_template_child (widget_class, GcalScheduleSection, repeat_interval_row);
  gtk_widget_class_bind_template_child (widget_class, GcalScheduleSection, repeat_interval_spin);
  gtk_widget_class_bind_template_child (widget_class, GcalScheduleSection, repeat_interval_unit_label);
  gtk_widget_class_bind_template_child (widget_class, GcalScheduleSection, saturday_button);
  gtk_widget_class_bind_template_child (widget_class, GcalScheduleSection, sunday_button);
  gtk_widget_class_bind_template_child (widget_class, GcalScheduleSection, thursday_button);
  gtk_widget_class_bind_template_child (widget_class, GcalScheduleSection, tuesday_button);
  gtk_widget_class_bind_template_child (widget_class, GcalScheduleSection, until_date_selector);
  gtk_widget_class_bind_template_child (widget_class, GcalScheduleSection, wednesday_button);
  gtk_widget_class_bind_template_child (widget_class, GcalScheduleSection, weekday_row);

  gtk_widget_class_bind_template_callback (widget_class, on_start_date_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_end_date_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_start_date_time_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_end_date_time_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_monthly_pattern_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_repeat_duration_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_repeat_interval_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_repeat_type_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_number_of_occurrences_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_until_date_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_weekday_toggled_cb);
}

static void
gcal_schedule_section_init (GcalScheduleSection *self)
{
  GcalContext *context = gcal_application_get_context (GCAL_DEFAULT_APPLICATION);

  gtk_widget_init_template (GTK_WIDGET (self));

  g_signal_connect (self->schedule_type_stack,
                    "notify::visible-child-name",
                    G_CALLBACK (on_schedule_type_changed_cb),
                    self);

  g_signal_connect_object (context,
                           "notify::time-format",
                           G_CALLBACK (on_time_format_changed_cb),
                           self,
                           G_CONNECT_SWAPPED);
}

static void
test_all_day_displays_sensible_dates_and_roundtrips (void)
{
  /* Start with this, per iCalendar's convention for a single-day all-day event:
   *
   *   start = midnight
   *   end = next midnight
   *   all_day = true
   *
   * Then, pass that on to the widgets, and check that they display the same date for start/end,
   * since users see "all day, start=2025/03/03, end=2025/03/03" and think sure, a single all-day.
   *
   * Then, change the end date to one day later, to get a two-day event.
   *
   * Check that the values gets back end = (start + 2 days)
   */

  g_autoptr (GcalEventSchedule) values = gcal_event_schedule_with_date_times ("20250303T00:00:00-06:00",
                                                                              "20250304T00:00:00-06:00",
                                                                              TRUE);

  /* Compute the widget state from the values; check the displayed dates (should be the same) */

  g_autoptr (WidgetState) state = widget_state_from_values (values);

  g_assert_true (state->schedule_type_all_day);
  g_assert_true (state->date_widgets_visible);
  g_assert_false (state->date_time_widgets_visible);

  g_assert_cmpint (g_date_time_get_year (state->date_time_start), ==, 2025);
  g_assert_cmpint (g_date_time_get_month (state->date_time_start), ==, 3);
  g_assert_cmpint (g_date_time_get_day_of_month (state->date_time_start), ==, 3);

  g_assert_cmpint (g_date_time_get_year (state->date_time_end), ==, 2025);
  g_assert_cmpint (g_date_time_get_month (state->date_time_end), ==, 3);
  g_assert_cmpint (g_date_time_get_day_of_month (state->date_time_end), ==, 3);

  /* Add a day to the end date */

  g_autoptr (GDateTime) displayed_end_date_plus_one_day = g_date_time_new_from_iso8601 ("20250304T00:00:00-06:00", NULL);
  g_assert (displayed_end_date_plus_one_day != NULL);
  g_autoptr (GcalEventSchedule) new_values = gcal_event_schedule_set_end_date (values, displayed_end_date_plus_one_day);

  /* Check the real iCalendar value for the new date, should be different from the displayed value */

  g_assert_cmpint (g_date_time_get_day_of_month (new_values->curr.date_end), ==, 5);

  /* Roundtrip back to widgets and check the displayed value */

  g_autoptr (WidgetState) new_state = widget_state_from_values (new_values);
  g_assert_cmpint (g_date_time_get_day_of_month (new_state->date_time_end), ==, 4);
}

static void
test_turning_on_recurrence_count_turns_on_its_widget (void)
{
  /* Start with some configuration */
  g_autoptr (GcalEventSchedule) values = gcal_event_schedule_with_date_times("20250411T10:00:00-06:00",
                                                                             "20250411T11:30:00-06:00",
                                                                             FALSE);

  /* Turn on recurrence */
  g_autoptr (GcalEventSchedule) with_recur =
    gcal_event_schedule_set_recur_frequency (values, GCAL_RECURRENCE_DAILY);

  /* Set it to "count" */
  g_autoptr (GcalEventSchedule) with_count =
    gcal_event_schedule_set_recur_limit_type (with_recur, GCAL_RECURRENCE_COUNT);

  g_autoptr (WidgetState) state = widget_state_from_values (with_count);
  g_assert_true (state->recurrence.duration_combo_visible);
  g_assert_true (state->recurrence.interval_visible);
  g_assert_true (state->recurrence.number_of_occurrences_visible);
  g_assert_false (state->recurrence.until_date_visible);
}

static void
test_turning_on_recurrence_until_turns_on_its_widget (void)
{
  /* Start with some configuration */
  g_autoptr (GcalEventSchedule) values = gcal_event_schedule_with_date_times("20250411T10:00:00-06:00",
                                                                             "20250411T11:30:00-06:00",
                                                                             FALSE);

  /* Turn on recurrence */
  g_autoptr (GcalEventSchedule) with_recur =
    gcal_event_schedule_set_recur_frequency (values, GCAL_RECURRENCE_DAILY);

  /* Set it to "count" */
  g_autoptr (GcalEventSchedule) with_until =
    gcal_event_schedule_set_recur_limit_type (with_recur, GCAL_RECURRENCE_UNTIL);

  g_autoptr (WidgetState) state = widget_state_from_values (with_until);
  g_assert_true (state->recurrence.duration_combo_visible);
  g_assert_true (state->recurrence.interval_visible);
  g_assert_false (state->recurrence.number_of_occurrences_visible);
  g_assert_true (state->recurrence.until_date_visible);
}

static void
test_imported_recurrence_only_shows_preservation_state (void)
{
  g_autoptr (GcalEventSchedule) values =
    gcal_event_schedule_with_date_times ("20250411T10:00:00-06:00",
                                         "20250411T11:30:00-06:00",
                                         FALSE);
  g_autoptr (GcalEventSchedule) imported =
    gcal_event_schedule_set_recur_frequency (values, GCAL_RECURRENCE_OTHER);
  g_autoptr (WidgetState) state = widget_state_from_values (imported);

  g_assert_true (state->recurrence.imported_visible);
  g_assert_false (state->recurrence.duration_combo_visible);
  g_assert_false (state->recurrence.interval_visible);
  g_assert_false (state->recurrence.weekdays_visible);
  g_assert_false (state->recurrence.monthly_pattern_visible);
}

void
gcal_schedule_section_add_tests (void)
{
  g_test_add_func ("/event_editor/schedule_section/all_day_displays_sensible_dates_and_roundtrips",
                   test_all_day_displays_sensible_dates_and_roundtrips);
  g_test_add_func ("/event_editor/schedule_section/turning_on_recurrence_count_turns_on_its_widget",
                   test_turning_on_recurrence_count_turns_on_its_widget);
  g_test_add_func ("/event_editor/schedule_section/turning_on_recurrence_until_turns_on_its_widget",
                   test_turning_on_recurrence_until_turns_on_its_widget);
  g_test_add_func ("/event_editor/schedule_section/imported_recurrence_only_shows_preservation_state",
                   test_imported_recurrence_only_shows_preservation_state);
}
