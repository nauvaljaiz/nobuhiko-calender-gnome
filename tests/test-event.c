/* test-event.c
 *
 * Copyright © 2018 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
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
 */

#include <glib.h>

#include "gcal-event-attendee.h"
#include "gcal-event-organizer.h"
#include "gcal-event.h"
#include "gcal-recurrence.h"
#include "gcal-stub-calendar.h"
#include "gcal-utils.h"
#include "libecal/libecal.h"

#define STUB_EVENT "BEGIN:VEVENT\n"             \
                   "SUMMARY:Stub event\n"       \
                   "UID:example@uid\n"          \
                   "DTSTAMP:19970114T170000Z\n" \
                   "DTSTART:20180714T170000Z\n" \
                   "DTEND:20180715T035959Z\n"   \
                   "END:VEVENT\n"

#define STUB_EVENT_DTEND "BEGIN:VEVENT\n"                \
                         "SUMMARY:Stub all day\n"        \
                         "UID:example@uid\n"             \
                         "DTSTART;VALUE=DATE:20231108\n" \
                         "DTEND;VALUE=DATE:20231109\n"   \
                         "END:VEVENT\n"

#define STUB_EVENT_NODTEND "BEGIN:VEVENT\n"                \
                           "SUMMARY:Stub all day\n"        \
                           "UID:example@uid\n"             \
                           "DTSTART;VALUE=DATE:20231108\n" \
                           "END:VEVENT\n"

#define EVENT_STRING_FOR_DATE(dtstart,dtend)    \
                   "BEGIN:VEVENT\n"             \
                   "SUMMARY:Stub event\n"       \
                   "UID:example@uid\n"          \
                   "DTSTAMP:19970114T170000Z\n" \
                   "DTSTART"dtstart"\n"        \
                   "DTEND"dtend"\n"            \
                   "END:VEVENT\n"

#define EVENT_STRING_FOR_DATE_START(dtstart)    \
                   EVENT_STRING_FOR_DATE(dtstart, ":20180715T035959Z")

#define EVENT_STRING_FOR_DATE_END(dtend)        \
                   EVENT_STRING_FOR_DATE(":19970101T170000Z", dtend)


#define STUB_EVENT_WITH_ATTENDEES "BEGIN:VEVENT\n"                                                                           \
                                  "UID:123456789@example.com\n"                                                              \
                                  "DTSTAMP:20240210T120000Z\n"                                                               \
                                  "DTSTART:20240215T150000Z\n"                                                               \
                                  "DTEND:20240215T160000Z\n"                                                                 \
                                  "SUMMARY:Project Meeting\n"                                                                \
                                  "DESCRIPTION:Discuss project updates and next steps.\n"                                    \
                                  "ORGANIZER;CN=Alice Smith;SENT-BY=\"MAILTO:jane_doe@host.com\":mailto:alice@example.com\n" \
                                  "ATTENDEE;CN=Bob Johnson;RSVP=TRUE;ROLE=REQ-PARTICIPANT:mailto:bob@example.com\n"          \
                                  "ATTENDEE;CN=Carol Lee;RSVP=FALSE;ROLE=OPT-PARTICIPANT:mailto:carol@example.com\n"         \
                                  "END:VEVENT\n"

#define STUB_EVENT_WITH_RECURRENCE "BEGIN:VEVENT\n"                   \
                                   "SUMMARY:Stub event\n"             \
                                   "UID:example@uid\n"                \
                                   "DTSTAMP:19970114T000000Z\n"       \
                                   "DTSTART:20180714T000000Z\n"       \
                                   "DTEND:20180714T235959Z\n"         \
                                   "RRULE;FREQ=DAILY;COUNT=3\n"       \
                                   "RECURRENCE-ID:20180714T000000Z\n" \
                                   "END:VEVENT\n"

#define STUB_EVENT_WITH_METADATA "BEGIN:VEVENT\n"                                      \
                                 "SUMMARY:Planning\n"                         \
                                 "UID:availability@example.test\n"            \
                                 "DTSTAMP:20260907T000000Z\n"                 \
                                 "DTSTART;TZID=Asia/Jakarta:20260907T090000\n" \
                                 "DTEND;TZID=Asia/Jakarta:20260907T100000\n"   \
                                 "RRULE:FREQ=WEEKLY;COUNT=3\n"                 \
                                 "ORGANIZER:mailto:owner@example.test\n"       \
                                 "ATTENDEE:mailto:guest@example.test\n"        \
                                 "CLASS:PRIVATE\n"                            \
                                 "ATTACH:https://example.test/file\n"          \
                                 "X-NOBUHIKO-KEEP:preserved\n"                \
                                 "END:VEVENT\n"

#define STUB_EVENT_WITH_MIXED_ALARMS "BEGIN:VEVENT\n"                          \
                                     "SUMMARY:Alarm preservation\n"             \
                                     "UID:alarms@example.test\n"                 \
                                     "DTSTART:20260909T090000Z\n"                \
                                     "DTEND:20260909T100000Z\n"                  \
                                     "BEGIN:VALARM\n"                            \
                                     "ACTION:DISPLAY\n"                          \
                                     "DESCRIPTION:Supported\n"                   \
                                     "TRIGGER:-PT45M\n"                          \
                                     "END:VALARM\n"                              \
                                     "BEGIN:VALARM\n"                            \
                                     "ACTION:DISPLAY\n"                          \
                                     "DESCRIPTION:Absolute\n"                    \
                                     "TRIGGER;VALUE=DATE-TIME:20260909T083000Z\n" \
                                     "END:VALARM\n"                              \
                                     "BEGIN:VALARM\n"                            \
                                     "ACTION:DISPLAY\n"                          \
                                     "DESCRIPTION:Relative to end\n"             \
                                     "TRIGGER;RELATED=END:-PT10M\n"              \
                                     "END:VALARM\n"                              \
                                     "BEGIN:VALARM\n"                            \
                                     "ACTION:DISPLAY\n"                          \
                                     "DESCRIPTION:After start\n"                 \
                                     "TRIGGER:PT5M\n"                            \
                                     "END:VALARM\n"                              \
                                     "END:VEVENT\n"

#define STUB_EVENT_WITH_DUPLICATE_ALARMS "BEGIN:VEVENT\n"              \
                                         "SUMMARY:Duplicate alarms\n"    \
                                         "UID:duplicate@example.test\n"  \
                                         "DTSTART:20260909T090000Z\n"   \
                                         "DTEND:20260909T100000Z\n"     \
                                         "BEGIN:VALARM\n"               \
                                         "ACTION:DISPLAY\n"             \
                                         "DESCRIPTION:First\n"          \
                                         "TRIGGER:-PT45M\n"             \
                                         "END:VALARM\n"                 \
                                         "BEGIN:VALARM\n"               \
                                         "ACTION:DISPLAY\n"             \
                                         "DESCRIPTION:Second\n"         \
                                         "TRIGGER:-PT45M\n"             \
                                         "END:VALARM\n"                 \
                                         "END:VEVENT\n"

#define STUB_EVENT_WITH_CUSTOM_WEEKLY_RECURRENCE "BEGIN:VEVENT\n"                         \
                                                  "SUMMARY:Custom weekly\n"              \
                                                  "UID:custom-weekly@example.test\n"     \
                                                  "DTSTART:20260909T090000Z\n"           \
                                                  "DTEND:20260909T100000Z\n"             \
                                                  "RRULE:FREQ=WEEKLY;INTERVAL=2;BYDAY=MO,WE;COUNT=6\n" \
                                                  "EXDATE:20260923T090000Z\n"            \
                                                  "X-NOBUHIKO-KEEP:recurrence\n"         \
                                                  "END:VEVENT\n"

#define STUB_EVENT_WITH_MONTH_DAY_RECURRENCE "BEGIN:VEVENT\n"                    \
                                              "SUMMARY:Month day\n"               \
                                              "UID:month-day@example.test\n"      \
                                              "DTSTART:20260909T090000Z\n"        \
                                              "DTEND:20260909T100000Z\n"          \
                                              "RRULE:FREQ=MONTHLY;INTERVAL=3;BYMONTHDAY=9;UNTIL=20270909T090000Z\n" \
                                              "END:VEVENT\n"

#define STUB_EVENT_WITH_NTH_WEEKDAY_RECURRENCE "BEGIN:VEVENT\n"                   \
                                                "SUMMARY:Nth weekday\n"             \
                                                "UID:nth-weekday@example.test\n"    \
                                                "DTSTART:20260927T090000Z\n"         \
                                                "DTEND:20260927T100000Z\n"           \
                                                "RRULE:FREQ=MONTHLY;BYDAY=-1SU\n"   \
                                                "END:VEVENT\n"

#define STUB_EVENT_WITH_IMPORTED_RECURRENCE "BEGIN:VEVENT\n"                    \
                                            "SUMMARY:Imported recurrence\n"       \
                                            "UID:imported-recurrence@example.test\n" \
                                            "DTSTART:20260909T090000Z\n"          \
                                            "DTEND:20260909T100000Z\n"            \
                                            "RRULE:FREQ=MONTHLY;BYDAY=MO,TU,WE,TH,FR;BYSETPOS=1\n" \
                                            "RDATE:20261001T090000Z\n"            \
                                            "END:VEVENT\n"

/*
 * Auxiliary methods
 */

static GcalEvent*
create_event_for_string (const gchar  *string,
                         GError      **error)
{
  g_autoptr (ECalComponent) component = NULL;
  g_autoptr (GcalCalendar) calendar = NULL;

  component = e_cal_component_new_from_string (string);
  calendar = gcal_stub_calendar_new (NULL, error);

  return component ? gcal_event_new (calendar, component, error) : NULL;
}


/*********************************************************************************************************************/

static void
event_new (void)
{
  g_autoptr (GcalEvent) event = NULL;
  g_autoptr (GError) error = NULL;

  event = create_event_for_string (STUB_EVENT, &error);

  g_assert_no_error (error);
  g_assert_nonnull (event);
}

/*********************************************************************************************************************/

static void
event_clone (void)
{
  g_autoptr (GcalEvent) clone1 = NULL;
  g_autoptr (GcalEvent) clone2 = NULL;
  g_autoptr (GcalEvent) event = NULL;
  g_autoptr (GError) error = NULL;

  event = create_event_for_string (STUB_EVENT, &error);

  g_assert_no_error (error);
  g_assert_nonnull (event);

  clone1 = gcal_event_new_from_event (event);
  g_assert_nonnull (clone1);

  gcal_event_set_summary (event, "Another summary");

  clone2 = gcal_event_new_from_event (event);
  g_assert_nonnull (clone2);
}


/*********************************************************************************************************************/

static void
event_uid (void)
{
  g_autoptr (GcalEvent) event = NULL;

  event = create_event_for_string (STUB_EVENT, NULL);

  g_assert_cmpstr (gcal_event_get_uid (event), ==, "stub:example@uid");
}

/*********************************************************************************************************************/

static void
event_summary (void)
{
  g_autoptr (GcalEvent) event = NULL;

  event = create_event_for_string (STUB_EVENT, NULL);

  g_assert_cmpstr (gcal_event_get_summary (event), ==, "Stub event");
}

/*********************************************************************************************************************/

static void
event_availability (void)
{
  g_autoptr (GcalEvent) event = create_event_for_string (STUB_EVENT, NULL);
  g_autoptr (GcalEvent) free_event = NULL;
  g_autofree gchar *before = NULL;
  g_autofree gchar *after = NULL;
  ICalComponent *ical_component;

  ical_component = e_cal_component_get_icalcomponent (gcal_event_get_component (event));
  before = i_cal_component_as_ical_string (ical_component);

  g_assert_true (gcal_event_get_busy (event));
  gcal_event_set_busy (event, TRUE);
  after = i_cal_component_as_ical_string (ical_component);
  g_assert_cmpstr (before, ==, after);

  gcal_event_set_busy (event, FALSE);
  g_assert_false (gcal_event_get_busy (event));
  g_assert_cmpint (e_cal_component_get_transparency (gcal_event_get_component (event)),
                   ==,
                   E_CAL_COMPONENT_TRANSP_TRANSPARENT);

  free_event = gcal_event_new_from_event (event);
  g_assert_true (gcal_event_availability_equal (event, free_event));

  gcal_event_set_busy (free_event, TRUE);
  g_assert_true (gcal_event_get_busy (free_event));
  g_assert_false (gcal_event_availability_equal (event, free_event));
  g_assert_false (gcal_event_get_busy (event));
}

/*********************************************************************************************************************/

static void
event_availability_preserves_metadata (void)
{
  g_autoptr (GcalEvent) event = create_event_for_string (STUB_EVENT_WITH_METADATA, NULL);
  g_autofree gchar *serialized = NULL;

  gcal_event_set_busy (event, FALSE);
  serialized = i_cal_component_as_ical_string (
      e_cal_component_get_icalcomponent (gcal_event_get_component (event)));

  g_assert_nonnull (strstr (serialized, "TRANSP:TRANSPARENT"));
  g_assert_nonnull (strstr (serialized, "RRULE:FREQ=WEEKLY;COUNT=3"));
  g_assert_nonnull (strstr (serialized, "ORGANIZER:mailto:owner@example.test"));
  g_assert_nonnull (strstr (serialized, "ATTENDEE:mailto:guest@example.test"));
  g_assert_nonnull (strstr (serialized, "CLASS:PRIVATE"));
  g_assert_nonnull (strstr (serialized, "ATTACH:https://example.test/file"));
  g_assert_nonnull (strstr (serialized, "X-NOBUHIKO-KEEP:preserved"));
}

/*********************************************************************************************************************/

static void
event_visibility (void)
{
  g_autoptr (GcalEvent) event = create_event_for_string (STUB_EVENT, NULL);
  g_autoptr (GcalEvent) public_event = NULL;
  g_autofree gchar *before = NULL;
  g_autofree gchar *after = NULL;
  ICalComponent *ical_component;

  ical_component = e_cal_component_get_icalcomponent (gcal_event_get_component (event));
  before = i_cal_component_as_ical_string (ical_component);

  g_assert_cmpint (gcal_event_get_visibility (event), ==, GCAL_EVENT_VISIBILITY_DEFAULT);
  gcal_event_set_visibility (event, GCAL_EVENT_VISIBILITY_DEFAULT);
  after = i_cal_component_as_ical_string (ical_component);
  g_assert_cmpstr (before, ==, after);

  gcal_event_set_visibility (event, GCAL_EVENT_VISIBILITY_PUBLIC);
  g_assert_cmpint (gcal_event_get_visibility (event), ==, GCAL_EVENT_VISIBILITY_PUBLIC);
  g_assert_cmpint (e_cal_component_get_classification (gcal_event_get_component (event)),
                   ==,
                   E_CAL_COMPONENT_CLASS_PUBLIC);

  public_event = gcal_event_new_from_event (event);
  g_assert_true (gcal_event_visibility_equal (event, public_event));

  gcal_event_set_visibility (public_event, GCAL_EVENT_VISIBILITY_PRIVATE);
  g_assert_cmpint (gcal_event_get_visibility (public_event), ==, GCAL_EVENT_VISIBILITY_PRIVATE);
  g_assert_false (gcal_event_visibility_equal (event, public_event));
  g_assert_cmpint (gcal_event_get_visibility (event), ==, GCAL_EVENT_VISIBILITY_PUBLIC);

  gcal_event_set_visibility (public_event, GCAL_EVENT_VISIBILITY_DEFAULT);
  g_assert_cmpint (e_cal_component_get_classification (gcal_event_get_component (public_event)),
                   ==,
                   E_CAL_COMPONENT_CLASS_NONE);
}

/*********************************************************************************************************************/

static void
event_visibility_preserves_special_values (void)
{
  const gchar *confidential_ical =
    "BEGIN:VEVENT\n"
    "SUMMARY:Confidential event\n"
    "UID:confidential@example.test\n"
    "DTSTART:20260909T090000Z\n"
    "DTEND:20260909T100000Z\n"
    "CLASS:CONFIDENTIAL\n"
    "END:VEVENT\n";
  const gchar *unknown_ical =
    "BEGIN:VEVENT\n"
    "SUMMARY:Imported event\n"
    "UID:unknown-class@example.test\n"
    "DTSTART:20260909T090000Z\n"
    "DTEND:20260909T100000Z\n"
    "CLASS:X-NOBUHIKO-CUSTOM\n"
    "END:VEVENT\n";
  g_autoptr (GcalEvent) confidential = create_event_for_string (confidential_ical, NULL);
  g_autoptr (GcalEvent) private_event = create_event_for_string (STUB_EVENT_WITH_METADATA, NULL);
  g_autoptr (GcalEvent) unknown = create_event_for_string (unknown_ical, NULL);
  g_autofree gchar *confidential_before = NULL;
  g_autofree gchar *confidential_after = NULL;
  g_autofree gchar *unknown_before = NULL;
  g_autofree gchar *unknown_after = NULL;

  confidential_before = i_cal_component_as_ical_string (
      e_cal_component_get_icalcomponent (gcal_event_get_component (confidential)));
  g_assert_cmpint (gcal_event_get_visibility (confidential), ==, GCAL_EVENT_VISIBILITY_PRIVATE);
  g_assert_true (gcal_event_visibility_equal (confidential, private_event));
  gcal_event_set_visibility (confidential, GCAL_EVENT_VISIBILITY_PRIVATE);
  confidential_after = i_cal_component_as_ical_string (
      e_cal_component_get_icalcomponent (gcal_event_get_component (confidential)));
  g_assert_cmpstr (confidential_before, ==, confidential_after);
  g_assert_nonnull (strstr (confidential_after, "CLASS:CONFIDENTIAL"));

  unknown_before = i_cal_component_as_ical_string (
      e_cal_component_get_icalcomponent (gcal_event_get_component (unknown)));
  g_assert_cmpint (gcal_event_get_visibility (unknown), ==, GCAL_EVENT_VISIBILITY_DEFAULT);
  gcal_event_set_visibility (unknown, GCAL_EVENT_VISIBILITY_DEFAULT);
  unknown_after = i_cal_component_as_ical_string (
      e_cal_component_get_icalcomponent (gcal_event_get_component (unknown)));
  g_assert_cmpstr (unknown_before, ==, unknown_after);
}

/*********************************************************************************************************************/

static void
event_visibility_preserves_metadata (void)
{
  g_autoptr (GcalEvent) event = create_event_for_string (STUB_EVENT_WITH_METADATA, NULL);
  g_autofree gchar *serialized = NULL;

  gcal_event_set_visibility (event, GCAL_EVENT_VISIBILITY_PUBLIC);
  serialized = i_cal_component_as_ical_string (
      e_cal_component_get_icalcomponent (gcal_event_get_component (event)));

  g_assert_nonnull (strstr (serialized, "CLASS:PUBLIC"));
  g_assert_nonnull (strstr (serialized, "RRULE:FREQ=WEEKLY;COUNT=3"));
  g_assert_nonnull (strstr (serialized, "ORGANIZER:mailto:owner@example.test"));
  g_assert_nonnull (strstr (serialized, "ATTENDEE:mailto:guest@example.test"));
  g_assert_nonnull (strstr (serialized, "ATTACH:https://example.test/file"));
  g_assert_nonnull (strstr (serialized, "X-NOBUHIKO-KEEP:preserved"));
}

/*********************************************************************************************************************/

static void
event_reminders_preserve_imported_alarms (void)
{
  g_autoptr (GcalEvent) event = create_event_for_string (STUB_EVENT_WITH_MIXED_ALARMS, NULL);
  g_autoptr (GcalEvent) clone = gcal_event_new_from_event (event);
  g_autofree gchar *serialized = NULL;
  GList *alarms;
  guint supported = 0;
  guint unsupported = 0;

  alarms = gcal_event_get_alarms (event);
  g_assert_cmpuint (g_list_length (alarms), ==, 4);

  for (GList *l = alarms; l; l = l->next)
    {
      if (get_alarm_trigger_minutes (event, l->data) >= 0)
        supported++;
      else
        unsupported++;
    }

  g_assert_cmpuint (supported, ==, 1);
  g_assert_cmpuint (unsupported, ==, 3);
  g_assert_true (gcal_event_has_alarm_at_minutes (event, 45));
  g_assert_true (gcal_event_alarms_equal (event, clone));
  g_list_free_full (alarms, e_cal_component_alarm_free);

  /* A replacement can change alarm contents without changing the count. */
  gcal_event_add_alarm_minutes (clone, 45);
  g_assert_false (gcal_event_alarms_equal (event, clone));

  gcal_event_add_alarm_minutes (event, 90);
  g_assert_true (gcal_event_has_alarm_at_minutes (event, 90));
  g_assert_false (gcal_event_alarms_equal (event, clone));

  alarms = gcal_event_get_alarms (event);
  g_assert_cmpuint (g_list_length (alarms), ==, 5);
  g_list_free_full (alarms, e_cal_component_alarm_free);

  gcal_event_remove_alarm (event, 90);
  g_assert_false (gcal_event_has_alarm_at_minutes (event, 90));
  gcal_event_add_alarm_minutes (event, 90);

  serialized = i_cal_component_as_ical_string (
      e_cal_component_get_icalcomponent (gcal_event_get_component (event)));
  g_assert_nonnull (strstr (serialized, "DESCRIPTION:Absolute"));
  g_assert_nonnull (strstr (serialized, "DESCRIPTION:Relative to end"));
  g_assert_nonnull (strstr (serialized, "DESCRIPTION:After start"));

  /* Adding the same supported offset replaces only that alarm. */
  gcal_event_add_alarm_minutes (event, 45);
  alarms = gcal_event_get_alarms (event);
  g_assert_cmpuint (g_list_length (alarms), ==, 5);

  for (GList *l = alarms; l; l = l->next)
    {
      if (get_alarm_trigger_minutes (event, l->data) < 0)
        {
          g_autofree gchar *uid = g_strdup (e_cal_component_alarm_get_uid (l->data));

          g_assert_nonnull (uid);
          gcal_event_remove_alarm_by_uid (event, uid);
          break;
        }
    }

  g_list_free_full (alarms, e_cal_component_alarm_free);

  alarms = gcal_event_get_alarms (event);
  g_assert_cmpuint (g_list_length (alarms), ==, 4);
  g_list_free_full (alarms, e_cal_component_alarm_free);

  gcal_event_remove_all_alarms (event);
  alarms = gcal_event_get_alarms (event);
  g_assert_null (alarms);
  g_assert_false (gcal_event_has_alarm_at_minutes (event, 45));
  g_assert_false (gcal_event_has_alarm_at_minutes (event, 90));
}

/*********************************************************************************************************************/

static void
event_reminders_reindex_duplicate_offsets (void)
{
  g_autoptr (GcalEvent) event = create_event_for_string (STUB_EVENT_WITH_DUPLICATE_ALARMS, NULL);
  GList *alarms;

  alarms = gcal_event_get_alarms (event);
  g_assert_cmpuint (g_list_length (alarms), ==, 2);
  g_list_free_full (alarms, e_cal_component_alarm_free);
  g_assert_true (gcal_event_has_alarm_at_minutes (event, 45));

  gcal_event_remove_alarm (event, 45);
  alarms = gcal_event_get_alarms (event);
  g_assert_cmpuint (g_list_length (alarms), ==, 1);
  g_list_free_full (alarms, e_cal_component_alarm_free);
  g_assert_true (gcal_event_has_alarm_at_minutes (event, 45));

  gcal_event_remove_alarm (event, 45);
  g_assert_false (gcal_event_has_alarm_at_minutes (event, 45));
  alarms = gcal_event_get_alarms (event);
  g_assert_null (alarms);
}

/*********************************************************************************************************************/

static void
event_custom_recurrence_roundtrips (void)
{
  g_autoptr (GcalEvent) event = create_event_for_string (STUB_EVENT_WITH_CUSTOM_WEEKLY_RECURRENCE, NULL);
  g_autoptr (GcalRecurrence) edited = NULL;
  ICalRecurrence *rrule = NULL;
  g_autofree gchar *rule = NULL;
  g_autofree gchar *serialized = NULL;
  GcalRecurrence *recur = gcal_event_get_recurrence (event);

  g_assert_nonnull (recur);
  g_assert_cmpint (recur->frequency, ==, GCAL_RECURRENCE_WEEKLY);
  g_assert_cmpuint (recur->interval, ==, 2);
  g_assert_cmpuint (recur->week_days, ==,
                    GCAL_RECURRENCE_WEEKDAY_MASK (G_DATE_MONDAY) |
                    GCAL_RECURRENCE_WEEKDAY_MASK (G_DATE_WEDNESDAY));
  g_assert_cmpint (recur->limit_type, ==, GCAL_RECURRENCE_COUNT);
  g_assert_cmpuint (recur->limit.count, ==, 6);

  rrule = gcal_recurrence_to_rrule (recur);
  rule = i_cal_recurrence_to_string (rrule);
  g_assert_nonnull (strstr (rule, "FREQ=WEEKLY"));
  g_assert_nonnull (strstr (rule, "INTERVAL=2"));
  g_assert_nonnull (strstr (rule, "BYDAY=MO,WE"));
  g_assert_nonnull (strstr (rule, "COUNT=6"));
  g_clear_object (&rrule);

  edited = gcal_recurrence_copy (recur);
  edited->interval = 3;
  g_assert_false (gcal_recurrence_is_equal (recur, edited));
  gcal_event_set_recurrence (event, edited);

  serialized = i_cal_component_as_ical_string (
      e_cal_component_get_icalcomponent (gcal_event_get_component (event)));
  g_assert_nonnull (strstr (serialized, "INTERVAL=3"));
  g_assert_nonnull (strstr (serialized, "BYDAY=MO,WE"));
  g_assert_nonnull (strstr (serialized, "EXDATE:20260923T090000Z"));
  g_assert_nonnull (strstr (serialized, "X-NOBUHIKO-KEEP:recurrence"));
}

/*********************************************************************************************************************/

static void
event_monthly_recurrence_patterns (void)
{
  g_autoptr (GcalEvent) month_day = create_event_for_string (STUB_EVENT_WITH_MONTH_DAY_RECURRENCE, NULL);
  g_autoptr (GcalEvent) nth_weekday = create_event_for_string (STUB_EVENT_WITH_NTH_WEEKDAY_RECURRENCE, NULL);
  ICalRecurrence *rrule = NULL;
  g_autofree gchar *rule = NULL;
  GcalRecurrence *recur;

  recur = gcal_event_get_recurrence (month_day);
  g_assert_cmpint (recur->frequency, ==, GCAL_RECURRENCE_MONTHLY);
  g_assert_cmpuint (recur->interval, ==, 3);
  g_assert_cmpint (recur->monthly_pattern, ==, GCAL_RECURRENCE_MONTHLY_DAY_OF_MONTH);
  g_assert_cmpint (recur->month_day, ==, 9);
  g_assert_cmpint (recur->limit_type, ==, GCAL_RECURRENCE_UNTIL);

  recur = gcal_event_get_recurrence (nth_weekday);
  g_assert_cmpint (recur->frequency, ==, GCAL_RECURRENCE_MONTHLY);
  g_assert_cmpint (recur->monthly_pattern, ==, GCAL_RECURRENCE_MONTHLY_NTH_WEEKDAY);
  g_assert_cmpint (recur->month_week, ==, -1);
  g_assert_cmpint (recur->month_weekday, ==, G_DATE_SUNDAY);

  rrule = gcal_recurrence_to_rrule (recur);
  rule = i_cal_recurrence_to_string (rrule);
  g_assert_nonnull (strstr (rule, "BYDAY=-1SU"));
  g_clear_object (&rrule);
}

/*********************************************************************************************************************/

static void
event_imported_recurrence_is_not_rewritten (void)
{
  g_autoptr (GcalEvent) event = create_event_for_string (STUB_EVENT_WITH_IMPORTED_RECURRENCE, NULL);
  g_autoptr (GcalEvent) clone = gcal_event_new_from_event (event);
  g_autofree gchar *before = NULL;
  g_autofree gchar *after = NULL;
  GcalRecurrence *recur = gcal_event_get_recurrence (event);

  g_assert_nonnull (recur);
  g_assert_cmpint (recur->frequency, ==, GCAL_RECURRENCE_OTHER);
  g_assert_null (gcal_recurrence_to_rrule (recur));
  g_assert_true (gcal_event_schedule_equal (event, clone));

  before = i_cal_component_as_ical_string (
      e_cal_component_get_icalcomponent (gcal_event_get_component (event)));
  gcal_event_set_busy (event, FALSE);
  after = i_cal_component_as_ical_string (
      e_cal_component_get_icalcomponent (gcal_event_get_component (event)));

  g_assert_nonnull (strstr (after, "BYSETPOS=1"));
  g_assert_nonnull (strstr (after, "RDATE:20261001T090000Z"));
  g_assert_cmpuint (strlen (after), >=, strlen (before));
}

/*********************************************************************************************************************/

static void
event_no_dtend (void)
{
  g_autoptr (GcalEvent) event_dtend = NULL;
  g_autoptr (GcalEvent) event_nodtend = NULL;
  GDateTime *end_dtend = NULL;
  GDateTime *end_nodtend = NULL;

  g_test_bug ("327");

  event_dtend = create_event_for_string (STUB_EVENT_DTEND, NULL);
  event_nodtend = create_event_for_string (STUB_EVENT_NODTEND, NULL);
  end_dtend = gcal_event_get_date_end (event_dtend);
  end_nodtend = gcal_event_get_date_end (event_nodtend);

  // assert event with DTEND is working correctly
  g_assert_true (gcal_event_get_all_day (event_dtend));
  g_assert_true (end_dtend != NULL);

  // assert event without DTEND is working correctly, has a date_end assigned
  // for all day
  g_assert_true (gcal_event_get_all_day (event_nodtend));
  g_assert_true (end_nodtend != NULL);

  // The event without dtend should be all day and should have the same
  // date_end than the event that explicitly specified it.
  g_assert_true (g_date_time_equal (end_dtend, end_nodtend));
}

/*********************************************************************************************************************/

static void
event_date_start (void)
{
  struct
    {
      const gchar *string;
      GDateTime   *datetime;
    }
  start_dates[] = {
    { EVENT_STRING_FOR_DATE_START (":20160229T000000Z"), g_date_time_new_utc (2016, 2, 29, 00, 00, 00.) },
    { EVENT_STRING_FOR_DATE_START (":20180228T170000Z"), g_date_time_new_utc (2018, 2, 28, 17, 00, 00.) },
    { EVENT_STRING_FOR_DATE_START (":20180714T170000Z"), g_date_time_new_utc (2018, 7, 14, 17, 00, 00.) },
  };

  guint i;

  for (i = 0; i < G_N_ELEMENTS (start_dates); i++)
    {
      g_autoptr (GDateTime) dtstart = NULL;
      g_autoptr (GcalEvent) event = NULL;
      g_autoptr (GError) error = NULL;

      event = create_event_for_string (start_dates[i].string, &error);
      dtstart = start_dates[i].datetime;

      g_assert_no_error (error);
      g_assert_true (g_date_time_equal (gcal_event_get_date_start (event), dtstart));
    }
}

/*********************************************************************************************************************/

static void
event_date_end (void)
{
  struct
    {
      const gchar *string;
      GDateTime   *datetime;
    }
  end_dates[] = {
    { EVENT_STRING_FOR_DATE_END (":20160229T000000Z"), g_date_time_new_utc (2016, 2, 29, 00, 00, 00.) },
    { EVENT_STRING_FOR_DATE_END (":20180228T170000Z"), g_date_time_new_utc (2018, 2, 28, 17, 00, 00.) },
    { EVENT_STRING_FOR_DATE_END (":20180714T170000Z"), g_date_time_new_utc (2018, 7, 14, 17, 00, 00.) },
  };

  guint i;

  for (i = 0; i < G_N_ELEMENTS (end_dates); i++)
    {
      g_autoptr (GDateTime) dtend = NULL;
      g_autoptr (GcalEvent) event = NULL;
      g_autoptr (GError) error = NULL;

      event = create_event_for_string (end_dates[i].string, &error);
      dtend = end_dates[i].datetime;

      g_assert_no_error (error);
      g_assert_true (g_date_time_equal (gcal_event_get_date_end (event), dtend));
    }
}

/*********************************************************************************************************************/

static void
event_date_singleday (void)
{
  const gchar * const events[] = {
    EVENT_STRING_FOR_DATE (":20160229T000000Z", ":20160229T030000Z"),
    EVENT_STRING_FOR_DATE (":20160229T000000Z", ":20160301T000000Z"),
    EVENT_STRING_FOR_DATE (":20160229T000000Z", ":20160229T235959Z"),
  };

  guint i;

  for (i = 0; i < G_N_ELEMENTS (events); i++)
    {
      g_autoptr (GcalEvent) event = NULL;
      g_autoptr (GError) error = NULL;

      event = create_event_for_string (events[i], &error);

      g_assert_no_error (error);
      g_assert_false (gcal_event_is_multiday (event));
    }
}

/*********************************************************************************************************************/

static void
event_date_multiday (void)
{
  const gchar * const events[] = {
    EVENT_STRING_FOR_DATE (":20160229T000000Z", ":20160302T000001Z"),
    EVENT_STRING_FOR_DATE (":20160229T020000Z", ":20160310T004500Z"),
  };

  guint i;

  for (i = 0; i < G_N_ELEMENTS (events); i++)
    {
      g_autoptr (GcalEvent) event = NULL;
      g_autoptr (GError) error = NULL;

      event = create_event_for_string (events[i], &error);

      g_assert_no_error (error);
      g_assert_true (gcal_event_is_multiday (event));
    }
}

/*********************************************************************************************************************/

static void
event_date_create_tzid (void)
{
  const gchar *event_str = EVENT_STRING_FOR_DATE (";TZID=Europe/London:20170818T010000", ";TZID=Europe/London:20170818T020000");

  g_autoptr (GcalEvent) event = NULL;
  g_autoptr (GError) error = NULL;
  g_autoptr (GDateTime) datetime = NULL;
  g_autoptr (GTimeZone) tz = NULL;
  g_autofree gchar *ical_string = NULL;

  ECalComponent *comp = NULL;
  ICalComponent *ical_comp = NULL;
  ECalComponentDateTime *dt_end = NULL;

  g_test_bug ("172");

  event = create_event_for_string (event_str, &error);
  g_assert_no_error (error);

  comp = gcal_event_get_component (event);
  dt_end = e_cal_component_get_dtend (comp);
  g_assert_cmpstr (e_cal_component_datetime_get_tzid (dt_end), ==, "Europe/London");
  e_cal_component_datetime_free (dt_end);

  ical_comp = e_cal_component_get_icalcomponent (comp);

  ical_string = i_cal_component_as_ical_string (ical_comp);
  g_assert_true (strstr (ical_string, "DTSTART;TZID=Europe/London:20170818T010000\r\n") != NULL);
  g_assert_true (strstr (ical_string, "DTEND;TZID=Europe/London:20170818T020000\r\n") != NULL);
  g_clear_pointer (&ical_string, g_free);

  // Edit the event to check that the modification is stored correctly
  g_setenv ("TZ", "Europe/London", TRUE);

  tz = g_time_zone_new_identifier ("Europe/London");
  datetime = g_date_time_new (tz, 2017, 8, 18, 3, 00, 00.);
  gcal_event_set_date_end (event, datetime);

  ical_string = i_cal_component_as_ical_string (ical_comp);
  g_assert_true (strstr (ical_string, "DTSTART;TZID=Europe/London:20170818T010000\r\n") != NULL);
  g_assert_true (strstr (ical_string, "DTEND;TZID=/freeassociation.sourceforge.net/Europe/London:\r\n 20170818T030000\r\n") != NULL);
  g_clear_pointer (&ical_string, g_free);

  g_setenv ("TZ", "UTC", TRUE);
}

/*********************************************************************************************************************/

static void
event_date_check_tz (void)
{
  struct
    {
      const gchar *string;
      const gchar *tz;
    }
  timezones[] = {
    {
      STUB_EVENT_DTEND, "UTC"
    },
    {
      EVENT_STRING_FOR_DATE (";TZID=America/New_York:20240709T170000", ";TZID=America/New_York:20240709T180000"),
      "America/New_York"
    },
    {
      EVENT_STRING_FOR_DATE (";TZID=Australia/Melbourne:20240709T170000", ";TZID=Australia/Melbourne:20240709T180000"),
      "Australia/Melbourne"
    },
    {
      EVENT_STRING_FOR_DATE (":20240709T170000", ":20240709T180000"), "Europe/London"
    },
    {
      EVENT_STRING_FOR_DATE (":20240709T180000Z", ":20240709T190000Z"), "UTC"
    },
  };

  g_test_bug ("171");

  for (gsize i = 0; i < G_N_ELEMENTS (timezones); i++)
    {
      g_autoptr (GcalEvent) event = NULL;
      g_autoptr (GError) error = NULL;
      GTimeZone *date_start = NULL;
      GTimeZone *date_end = NULL;

      /* Change environment variable to test local timezone, when it's the case. */
      g_setenv ("TZ", "Europe/London", TRUE);

      event = create_event_for_string (timezones[i].string, &error);

      g_setenv ("TZ", "UTC", TRUE);
      g_assert_no_error (error);

      date_start = g_date_time_get_timezone (gcal_event_get_date_start (event));
      date_end = g_date_time_get_timezone (gcal_event_get_date_end (event));

      g_assert_cmpstr (g_time_zone_get_identifier (date_start), ==, timezones[i].tz);
      g_assert_cmpstr (g_time_zone_get_identifier (date_end), ==, timezones[i].tz);
    }
}

/*********************************************************************************************************************/

static void
event_get_attendees (void)
{
  /* "ATTENDEE;CN=Bob Johnson;RSVP=TRUE;ROLE=REQ-PARTICIPANT:mailto:bob@example.com\n" \ */
  /* "ATTENDEE;CN=Carol Lee;RSVP=FALSE;ROLE=OPT-PARTICIPANT:mailto:carol@example.com\n" \ */
  g_autoptr (GcalEvent) event = NULL;
  g_autoptr (GError) error = NULL;
  GListModel *attendees = NULL;

  event = create_event_for_string (STUB_EVENT_WITH_ATTENDEES, &error);

  attendees = gcal_event_get_attendees (event);

  g_assert_nonnull (attendees);

  for (guint i = 0; i < g_list_model_get_n_items (attendees); ++i)
    {
      g_autoptr (GcalEventAttendee) at = (GcalEventAttendee *) g_list_model_get_item (attendees, i);

      g_assert_cmpstr (NULL, !=, gcal_event_attendee_get_name (at));
      g_assert_cmpstr (NULL, !=, gcal_event_attendee_get_uri (at));
      g_assert_cmpuint (GCAL_EVENT_ATTENDEE_TYPE_NONE, !=, gcal_event_attendee_get_attendee_type (at));
    }

  /* organizer should be set */
  /* "ORGANIZER;CN=Alice Smith;SENT-BY=\"MAILTO:jane_doe@host.com\":mailto:alice@example.com\n" \ */
  GcalEventOrganizer *organizer = gcal_event_get_organizer (event);
  g_assert_nonnull (organizer);
  g_assert_cmpstr ("Alice Smith", ==, gcal_event_organizer_get_name (organizer));
  g_assert_cmpstr ("MAILTO:jane_doe@host.com", ==, gcal_event_organizer_get_sent_by (organizer));
  g_assert_cmpstr ("mailto:alice@example.com", ==, gcal_event_organizer_get_uri (organizer));

  g_autofree const gchar *uri_trimmed = gcal_get_email_from_mailto_uri (gcal_event_organizer_get_uri (organizer));
  g_assert_cmpstr ("alice@example.com", ==, uri_trimmed);
}

/*********************************************************************************************************************/

static GDateTime *
dt_add_second_random (GDateTime *dt)
{
  gint32 seconds = 0;
  do
    {
      // numbers chosen to cover a fair amount of years
      seconds = g_test_rand_int_range (-1000000000, 1000000000);
    }
  while (seconds == 0);

  return g_date_time_add_seconds (dt, seconds);
};

static void
event_schedule_equal (void)
{
  g_test_bug ("1612");

  const gchar *const events[] = {
    EVENT_STRING_FOR_DATE (":20160229T000000Z", ":20160229T030000Z"),
    EVENT_STRING_FOR_DATE (":20160229T000000Z", ":20160301T000000Z"),
    EVENT_STRING_FOR_DATE (":20160229T000000Z", ":20160229T235959Z"),
    EVENT_STRING_FOR_DATE (":20160229T000000Z", ":20160302T000001Z"),
    EVENT_STRING_FOR_DATE (":20160229T020000Z", ":20160310T004500Z"),
    STUB_EVENT_WITH_RECURRENCE,
  };

  for (guint i = 0; i < G_N_ELEMENTS (events); i++)
    {
      g_autoptr (GcalEvent) event_orig = NULL;
      g_autoptr (GcalEvent) event_mod = NULL;
      g_autoptr (GError) error = NULL;

      event_orig = create_event_for_string (events[i], &error);
      g_assert_no_error (error);

      event_mod = create_event_for_string (events[i], &error);
      g_assert_no_error (error);

      g_assert_true (gcal_event_schedule_equal (event_orig, event_mod));

      g_autoptr (GDateTime) dt_start = g_date_time_ref (gcal_event_get_date_start (event_mod));
      g_autoptr (GDateTime) dt_end = g_date_time_ref (gcal_event_get_date_end (event_mod));

      /* == type changed == */

      /* use a separate event instance in case flipping the type also changes the stored date time */
      {
        g_autoptr (GcalEvent) event_mod_mod = gcal_event_new_from_event (event_mod);

        gcal_event_set_all_day (event_mod_mod, !gcal_event_get_all_day (event_mod_mod));

        g_assert_false (gcal_event_schedule_equal (event_orig, event_mod_mod));
      }

      /* == start and/or end date time changed == */

      /* poor person's fuzz test I suppose */
      for (guint j = 0; j < 10; j++)
        {
          g_autoptr (GDateTime) dt_start_mod = dt_add_second_random (dt_start);
          g_autoptr (GDateTime) dt_end_mod = dt_add_second_random (dt_end);

          gcal_event_set_date_start (event_mod, dt_start_mod);
          g_assert_false (gcal_event_schedule_equal (event_orig, event_mod));

          gcal_event_set_date_start (event_mod, dt_start);
          g_assert_true (gcal_event_schedule_equal (event_orig, event_mod));

          gcal_event_set_date_end (event_mod, dt_end_mod);
          g_assert_false (gcal_event_schedule_equal (event_orig, event_mod));

          gcal_event_set_date_end (event_mod, dt_end);
          g_assert_true (gcal_event_schedule_equal (event_orig, event_mod));
        }

      /* == start and/or end timezone changed == */

      {
        g_autoptr (GDateTime) dt_start_mod = g_date_time_to_timezone (dt_start, g_time_zone_new_identifier ("Australia/Melbourne"));
        g_autoptr (GDateTime) dt_end_mod = g_date_time_to_timezone (dt_end, g_time_zone_new_identifier ("Australia/Melbourne"));

        gcal_event_set_date_start (event_mod, dt_start_mod);
        g_assert_false (gcal_event_schedule_equal (event_orig, event_mod));

        gcal_event_set_date_start (event_mod, dt_start);
        g_assert_true (gcal_event_schedule_equal (event_orig, event_mod));

        gcal_event_set_date_end (event_mod, dt_end_mod);
        g_assert_false (gcal_event_schedule_equal (event_orig, event_mod));

        gcal_event_set_date_end (event_mod, dt_end);
        g_assert_true (gcal_event_schedule_equal (event_orig, event_mod));
      }

      /* == recurrence changed == */

      if (gcal_event_has_recurrence (event_mod))
        {
          g_autoptr (GcalRecurrence) no_rec = gcal_recurrence_new ();

          gcal_event_set_recurrence (event_mod, no_rec);
        }
      else
        {
          g_autoptr (ECalComponent) comp = e_cal_component_new_from_string (STUB_EVENT_WITH_RECURRENCE);
          g_autoptr (GcalRecurrence) some_rec = gcal_recurrence_parse_recurrence_rules (comp);

          gcal_event_set_recurrence (event_mod, some_rec);
        }

      g_assert_false (gcal_event_schedule_equal (event_orig, event_mod));

      /* event_mod uses different recurrence now, don't add additional tests below */
    }
}

/*********************************************************************************************************************/

gint
main (gint   argc,
      gchar *argv[])
{
  g_setenv ("TZ", "UTC", TRUE);

  g_test_init (&argc, &argv, NULL);
  g_test_bug_base ("https://gitlab.gnome.org/GNOME/gnome-calendar/-/issues/");

  g_test_add_func ("/event/new", event_new);
  g_test_add_func ("/event/clone", event_clone);
  g_test_add_func ("/event/uid", event_uid);
  g_test_add_func ("/event/summary", event_summary);
  g_test_add_func ("/event/availability", event_availability);
  g_test_add_func ("/event/availability/preserves-metadata", event_availability_preserves_metadata);
  g_test_add_func ("/event/visibility", event_visibility);
  g_test_add_func ("/event/visibility/preserves-special-values", event_visibility_preserves_special_values);
  g_test_add_func ("/event/visibility/preserves-metadata", event_visibility_preserves_metadata);
  g_test_add_func ("/event/reminders/preserve-imported-alarms", event_reminders_preserve_imported_alarms);
  g_test_add_func ("/event/reminders/reindex-duplicate-offsets", event_reminders_reindex_duplicate_offsets);
  g_test_add_func ("/event/recurrence/custom-roundtrip", event_custom_recurrence_roundtrips);
  g_test_add_func ("/event/recurrence/monthly-patterns", event_monthly_recurrence_patterns);
  g_test_add_func ("/event/recurrence/imported-preserved", event_imported_recurrence_is_not_rewritten);
  g_test_add_func ("/event/nodtend", event_no_dtend);
  g_test_add_func ("/event/date/start", event_date_start);
  g_test_add_func ("/event/date/end", event_date_end);
  g_test_add_func ("/event/date/singleday", event_date_singleday);
  g_test_add_func ("/event/date/multiday", event_date_multiday);
  g_test_add_func ("/event/date/create-tzid", event_date_create_tzid);
  g_test_add_func ("/event/date/check-tz", event_date_check_tz);
  g_test_add_func ("/event/date/get-attendees", event_get_attendees);
  g_test_add_func ("/event/equal/schedule", event_schedule_equal);

  return g_test_run ();
}
