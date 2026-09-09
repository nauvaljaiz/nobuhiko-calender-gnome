/* gcal-recurrence.c
 *
 * Copyright (C) 2017 Yash Singh <yashdev10p@gmail.com>
 *
 * gnome-calendar is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * gnome-calendar is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gcal-recurrence.h"
#include "gcal-utils.h"
#include "gcal-event.h"

#include <glib.h>

G_DEFINE_BOXED_TYPE (GcalRecurrence, gcal_recurrence, gcal_recurrence_ref, gcal_recurrence_unref)

static void
gcal_recurrence_free (GcalRecurrence *self)
{
  g_assert (self);
  g_assert_cmpint (self->ref_count, ==, 0);

  gcal_clear_date_time (&self->limit.until);
  g_slice_free (GcalRecurrence, self);
}

/**
 * gcal_recurrence_new:
 *
 * Creates a new #GcalRecurrence
 *
 * Returns: (transfer full): a #GcalRecurrence
 */
GcalRecurrence*
gcal_recurrence_new (void)
{
  GcalRecurrence *new_recur;

  new_recur = g_slice_new (GcalRecurrence);

  new_recur->frequency = GCAL_RECURRENCE_NO_REPEAT;
  new_recur->limit_type = GCAL_RECURRENCE_FOREVER;
  new_recur->interval = 1;
  new_recur->week_days = 0;
  new_recur->monthly_pattern = GCAL_RECURRENCE_MONTHLY_DAY_OF_MONTH;
  new_recur->month_day = 0;
  new_recur->month_week = 0;
  new_recur->month_weekday = G_DATE_BAD_WEEKDAY;

  new_recur->limit.until = NULL;
  new_recur->limit.count = 0;

  g_atomic_ref_count_init (&new_recur->ref_count);

  return new_recur;
}

/**
 * gcal_recurrence_copy:
 * @recur: a #GcalRecurrence
 *
 * Creates a copy of @recur
 *
 * Returns: (transfer full): a #GcalRecurrence
 */
GcalRecurrence*
gcal_recurrence_copy (GcalRecurrence *recur)
{
  GcalRecurrence *new_recur;

  g_return_val_if_fail (recur != NULL, NULL);

  new_recur = gcal_recurrence_new ();

  new_recur->frequency = recur->frequency;
  new_recur->limit_type = recur->limit_type;
  new_recur->interval = recur->interval;
  new_recur->week_days = recur->week_days;
  new_recur->monthly_pattern = recur->monthly_pattern;
  new_recur->month_day = recur->month_day;
  new_recur->month_week = recur->month_week;
  new_recur->month_weekday = recur->month_weekday;

  if (recur->limit_type == GCAL_RECURRENCE_UNTIL && recur->limit.until)
    new_recur->limit.until = g_date_time_ref (recur->limit.until);

  new_recur->limit.count = recur->limit.count;

  return new_recur;
}

GcalRecurrence *
gcal_recurrence_ref (GcalRecurrence *self)
{
  g_return_val_if_fail (self, NULL);
  g_return_val_if_fail (self->ref_count, NULL);

  g_atomic_ref_count_inc (&self->ref_count);

  return self;
}

void
gcal_recurrence_unref (GcalRecurrence *self)
{
  g_return_if_fail (self);
  g_return_if_fail (self->ref_count);

  if (g_atomic_ref_count_dec (&self->ref_count))
    gcal_recurrence_free (self);
}

/**
 * gcal_recurrence_is_equal:
 * @recur1: (nullable): a #GcalRecurrence
 * @recur2: (nullable): a #GcalRecurrence
 *
 * Checks if @recur1 and @recur2 are equal or not
 *
 * Returns: %TRUE if both are equal, %FALSE otherwise
 */

gboolean
gcal_recurrence_is_equal (GcalRecurrence *recur1,
                          GcalRecurrence *recur2)
{
  if (recur1 == recur2)
    return TRUE;
  else if (!recur1 || !recur2)
    return FALSE;

  if (recur1->frequency != recur2->frequency)
    return FALSE;

  if (recur1->limit_type != recur2->limit_type)
    return FALSE;

  if (recur1->frequency != GCAL_RECURRENCE_NO_REPEAT &&
      recur1->frequency != GCAL_RECURRENCE_OTHER &&
      recur1->interval != recur2->interval)
    return FALSE;

  if ((recur1->frequency == GCAL_RECURRENCE_WEEKLY ||
       recur1->frequency == GCAL_RECURRENCE_MON_FRI) &&
      recur1->week_days != recur2->week_days)
    return FALSE;

  if (recur1->frequency == GCAL_RECURRENCE_MONTHLY)
    {
      if (recur1->monthly_pattern != recur2->monthly_pattern)
        return FALSE;

      if (recur1->monthly_pattern == GCAL_RECURRENCE_MONTHLY_DAY_OF_MONTH &&
          recur1->month_day != recur2->month_day)
        return FALSE;

      if (recur1->monthly_pattern == GCAL_RECURRENCE_MONTHLY_NTH_WEEKDAY &&
          (recur1->month_week != recur2->month_week ||
           recur1->month_weekday != recur2->month_weekday))
        return FALSE;
    }

  if (recur1->limit_type == GCAL_RECURRENCE_UNTIL)
    {
      if (!g_date_time_equal (recur1->limit.until, recur2->limit.until))
        return FALSE;
    }
  else if (recur1->limit_type == GCAL_RECURRENCE_COUNT)
    {
      if (recur1->limit.count != recur2->limit.count)
        return FALSE;
    }

  return TRUE;
}

static gboolean
rrule_has_only_supported_parts (ICalRecurrence          *rrule,
                                ICalRecurrenceFrequency  frequency)
{
  g_auto (GStrv) parts = NULL;
  g_autofree gchar *rule = NULL;

  rule = i_cal_recurrence_to_string (rrule);
  parts = g_strsplit (rule, ";", -1);

  for (guint i = 0; parts[i]; i++)
    {
      const gchar *part = parts[i];

      if (g_str_has_prefix (part, "FREQ=") ||
          g_str_has_prefix (part, "INTERVAL=") ||
          g_str_has_prefix (part, "COUNT=") ||
          g_str_has_prefix (part, "UNTIL="))
        continue;

      if (frequency == I_CAL_WEEKLY_RECURRENCE && g_str_has_prefix (part, "BYDAY="))
        continue;

      if (frequency == I_CAL_MONTHLY_RECURRENCE &&
          (g_str_has_prefix (part, "BYDAY=") || g_str_has_prefix (part, "BYMONTHDAY=")))
        continue;

      return FALSE;
    }

  return TRUE;
}

static GDateWeekday
weekday_from_ical (ICalRecurrenceWeekday weekday)
{
  switch (weekday)
    {
    case I_CAL_MONDAY_WEEKDAY:    return G_DATE_MONDAY;
    case I_CAL_TUESDAY_WEEKDAY:   return G_DATE_TUESDAY;
    case I_CAL_WEDNESDAY_WEEKDAY: return G_DATE_WEDNESDAY;
    case I_CAL_THURSDAY_WEEKDAY:  return G_DATE_THURSDAY;
    case I_CAL_FRIDAY_WEEKDAY:    return G_DATE_FRIDAY;
    case I_CAL_SATURDAY_WEEKDAY:  return G_DATE_SATURDAY;
    case I_CAL_SUNDAY_WEEKDAY:    return G_DATE_SUNDAY;
    default:                      return G_DATE_BAD_WEEKDAY;
    }
}

static ICalRecurrenceWeekday
weekday_to_ical (GDateWeekday weekday)
{
  switch (weekday)
    {
    case G_DATE_MONDAY:    return I_CAL_MONDAY_WEEKDAY;
    case G_DATE_TUESDAY:   return I_CAL_TUESDAY_WEEKDAY;
    case G_DATE_WEDNESDAY: return I_CAL_WEDNESDAY_WEEKDAY;
    case G_DATE_THURSDAY:  return I_CAL_THURSDAY_WEEKDAY;
    case G_DATE_FRIDAY:    return I_CAL_FRIDAY_WEEKDAY;
    case G_DATE_SATURDAY:  return I_CAL_SATURDAY_WEEKDAY;
    case G_DATE_SUNDAY:    return I_CAL_SUNDAY_WEEKDAY;
    default:               return I_CAL_NO_WEEKDAY;
    }
}

static gboolean
parse_week_days (ICalRecurrence *rrule,
                 guint          *week_days)
{
  guint parsed_days = 0;

  for (guint i = 0; i < I_CAL_BY_DAY_SIZE; i++)
    {
      GDateWeekday weekday;
      gshort encoded_day;
      guint mask;

      encoded_day = i_cal_recurrence_get_by_day (rrule, i);
      if (encoded_day == I_CAL_RECURRENCE_ARRAY_MAX)
        break;

      if (i_cal_recurrence_day_position (encoded_day) != 0)
        return FALSE;

      weekday = weekday_from_ical (i_cal_recurrence_day_day_of_week (encoded_day));
      if (weekday == G_DATE_BAD_WEEKDAY)
        return FALSE;

      mask = GCAL_RECURRENCE_WEEKDAY_MASK (weekday);
      if (parsed_days & mask)
        return FALSE;

      parsed_days |= mask;
    }

  *week_days = parsed_days;
  return TRUE;
}

static gboolean
parse_monthly_pattern (ICalRecurrence *rrule,
                       GcalRecurrence *recur)
{
  gshort by_day = i_cal_recurrence_get_by_day (rrule, 0);
  gshort next_by_day = i_cal_recurrence_get_by_day (rrule, 1);
  gshort by_month_day = i_cal_recurrence_get_by_month_day (rrule, 0);
  gshort next_by_month_day = i_cal_recurrence_get_by_month_day (rrule, 1);
  gboolean has_by_day = by_day != I_CAL_RECURRENCE_ARRAY_MAX;
  gboolean has_by_month_day = by_month_day != I_CAL_RECURRENCE_ARRAY_MAX;

  if ((has_by_day && has_by_month_day) ||
      next_by_day != I_CAL_RECURRENCE_ARRAY_MAX ||
      next_by_month_day != I_CAL_RECURRENCE_ARRAY_MAX)
    return FALSE;

  if (has_by_month_day)
    {
      if ((by_month_day < 1 || by_month_day > 31) && by_month_day != -1)
        return FALSE;

      recur->monthly_pattern = GCAL_RECURRENCE_MONTHLY_DAY_OF_MONTH;
      recur->month_day = by_month_day;
    }
  else if (has_by_day)
    {
      gint position = i_cal_recurrence_day_position (by_day);
      GDateWeekday weekday = weekday_from_ical (i_cal_recurrence_day_day_of_week (by_day));

      if ((position != -1 && (position < 1 || position > 5)) ||
          weekday == G_DATE_BAD_WEEKDAY)
        return FALSE;

      recur->monthly_pattern = GCAL_RECURRENCE_MONTHLY_NTH_WEEKDAY;
      recur->month_week = position;
      recur->month_weekday = weekday;
    }

  return TRUE;
}

/**
 * gcal_recurrence_parse_recurrence_rules:
 * @comp: an #ECalComponent
 *
 * Parses the rrule of the @comp and
 * sets the GcalRecurrence struct accordingly.
 *
 * Returns: (transfer full): a #GcalRecurrence
 */
GcalRecurrence*
gcal_recurrence_parse_recurrence_rules (ECalComponent *comp)
{
  GcalRecurrence *recur;
  ICalRecurrence *rrule;
  ICalRecurrenceFrequency frequency;
  GSList *rrules;

  if (!e_cal_component_has_recurrences (comp))
    return NULL;

  recur = gcal_recurrence_new ();
  rrules = e_cal_component_get_rrules (comp);

  /* Multiple regular rules, inclusion dates, and exclusion rules are valid
   * iCalendar, but cannot be represented by this editor. Keep them imported. */
  if (!rrules || rrules->next || e_cal_component_has_rdates (comp) || e_cal_component_has_exrules (comp))
    {
      recur->frequency = GCAL_RECURRENCE_OTHER;
      g_slist_free_full (rrules, g_object_unref);
      return recur;
    }

  rrule = g_object_ref (rrules->data);
  g_slist_free_full (rrules, g_object_unref);
  frequency = i_cal_recurrence_get_freq (rrule);

  if (!rrule_has_only_supported_parts (rrule, frequency))
    recur->frequency = GCAL_RECURRENCE_OTHER;

  recur->interval = MAX (1, i_cal_recurrence_get_interval (rrule));

  switch (frequency)
    {
      case I_CAL_DAILY_RECURRENCE:
        if (recur->frequency != GCAL_RECURRENCE_OTHER)
          recur->frequency = GCAL_RECURRENCE_DAILY;
        break;

      case I_CAL_WEEKLY_RECURRENCE:
        {
          if (!parse_week_days (rrule, &recur->week_days))
            recur->frequency = GCAL_RECURRENCE_OTHER;
          else if (recur->frequency != GCAL_RECURRENCE_OTHER)
            recur->frequency = recur->interval == 1 && recur->week_days == GCAL_RECURRENCE_WEEKDAYS_MON_FRI
                                 ? GCAL_RECURRENCE_MON_FRI
                                 : GCAL_RECURRENCE_WEEKLY;
          break;
        }

      case I_CAL_MONTHLY_RECURRENCE:
        if (!parse_monthly_pattern (rrule, recur))
          recur->frequency = GCAL_RECURRENCE_OTHER;
        else if (recur->frequency != GCAL_RECURRENCE_OTHER)
          recur->frequency = GCAL_RECURRENCE_MONTHLY;
        break;

      case I_CAL_YEARLY_RECURRENCE:
        if (recur->frequency != GCAL_RECURRENCE_OTHER)
          recur->frequency = GCAL_RECURRENCE_YEARLY;
        break;

      default:
        recur->frequency = GCAL_RECURRENCE_OTHER;
    }

  if (i_cal_recurrence_get_count (rrule) > 0)
    {
      recur->limit_type = GCAL_RECURRENCE_COUNT;
      recur->limit.count = i_cal_recurrence_get_count (rrule);
    }
  else
    {
      ICalTime *until;

      until = i_cal_recurrence_get_until (rrule);

      if (i_cal_time_get_year (until) != 0)
        {
          recur->limit_type = GCAL_RECURRENCE_UNTIL;
          recur->limit.until = gcal_date_time_from_icaltime (until);
          if (!recur->limit.until)
            recur->limit_type = GCAL_RECURRENCE_FOREVER;
        }
      else
        {
          recur->limit_type = GCAL_RECURRENCE_FOREVER;
        }

      g_clear_object (&until);
    }

  g_clear_object (&rrule);

  return recur;
}

/**
 * gcal_recurrence_to_rrule:
 * @recur: a #GcalRecurrence
 *
 * Converts @recur into corresponding rrule.
 *
 * Returns: (transfer full): an #ICalRecurrence
 */
ICalRecurrence*
gcal_recurrence_to_rrule (GcalRecurrence *recur)
{
  ICalRecurrence *rrule;

  if (!recur)
    return NULL;

  if (recur->frequency == GCAL_RECURRENCE_OTHER)
    return NULL;

  /* Initialize and clear the rrule to get rid of unwanted fields */
  rrule = i_cal_recurrence_new ();

  switch (recur->frequency)
    {
    case GCAL_RECURRENCE_DAILY:
      i_cal_recurrence_set_freq (rrule, I_CAL_DAILY_RECURRENCE);
      break;

    case GCAL_RECURRENCE_WEEKLY:
      {
        guint index = 0;

        i_cal_recurrence_set_freq (rrule, I_CAL_WEEKLY_RECURRENCE);
        for (GDateWeekday day = G_DATE_MONDAY; day <= G_DATE_SUNDAY; day++)
          {
            if (recur->week_days & GCAL_RECURRENCE_WEEKDAY_MASK (day))
              i_cal_recurrence_set_by_day (rrule, index++, weekday_to_ical (day));
          }
      }
      break;

    case GCAL_RECURRENCE_MONTHLY:
      i_cal_recurrence_set_freq (rrule, I_CAL_MONTHLY_RECURRENCE);
      if (recur->monthly_pattern == GCAL_RECURRENCE_MONTHLY_DAY_OF_MONTH && recur->month_day != 0)
        {
          i_cal_recurrence_set_by_month_day (rrule, 0, recur->month_day);
        }
      else if (recur->monthly_pattern == GCAL_RECURRENCE_MONTHLY_NTH_WEEKDAY)
        {
          ICalRecurrenceWeekday weekday = weekday_to_ical (recur->month_weekday);
          gint encoded_day = (ABS (recur->month_week) << 3) | weekday;

          i_cal_recurrence_set_by_day (rrule, 0, recur->month_week < 0 ? -encoded_day : encoded_day);
        }
      break;

    case GCAL_RECURRENCE_YEARLY:
      i_cal_recurrence_set_freq (rrule, I_CAL_YEARLY_RECURRENCE);
      break;

    case GCAL_RECURRENCE_NO_REPEAT:
      i_cal_recurrence_set_freq (rrule, I_CAL_NO_RECURRENCE);
      break;

    case GCAL_RECURRENCE_MON_FRI:
      {
        i_cal_recurrence_set_freq (rrule, I_CAL_WEEKLY_RECURRENCE);
        i_cal_recurrence_set_by_day (rrule, 0, I_CAL_MONDAY_WEEKDAY);
        i_cal_recurrence_set_by_day (rrule, 1, I_CAL_TUESDAY_WEEKDAY);
        i_cal_recurrence_set_by_day (rrule, 2, I_CAL_WEDNESDAY_WEEKDAY);
        i_cal_recurrence_set_by_day (rrule, 3, I_CAL_THURSDAY_WEEKDAY);
        i_cal_recurrence_set_by_day (rrule, 4, I_CAL_FRIDAY_WEEKDAY);
        break;
      }

    default:
      i_cal_recurrence_set_freq (rrule, I_CAL_NO_RECURRENCE);
      break;
    }

  if (recur->frequency != GCAL_RECURRENCE_NO_REPEAT && recur->interval > 1)
    i_cal_recurrence_set_interval (rrule, recur->interval);

  switch (recur->limit_type)
    {
    case GCAL_RECURRENCE_COUNT:
      i_cal_recurrence_set_count (rrule, recur->limit.count);
      break;

    case GCAL_RECURRENCE_UNTIL:
      {
        ICalTime *until;

        until = i_cal_time_new_null_time ();
        i_cal_time_set_date (until,
                             g_date_time_get_year (recur->limit.until),
                             g_date_time_get_month (recur->limit.until),
                             g_date_time_get_day_of_month (recur->limit.until));
        i_cal_time_set_time (until,
                             g_date_time_get_hour (recur->limit.until),
                             g_date_time_get_minute (recur->limit.until),
                             g_date_time_get_second (recur->limit.until));

        i_cal_recurrence_set_until (rrule, until);

        g_clear_object (&until);
        break;
      }

    case GCAL_RECURRENCE_FOREVER:
      break;

    default:
      break;
    }

  return rrule;
}
