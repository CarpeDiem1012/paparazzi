/*
 * Copyright (C) 2021 Freek van Tienen <freek.v.tienen@gmail.com>
 *
 * This file is part of paparazzi
 *
 * paparazzi is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * paparazzi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with paparazzi; see the file COPYING.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/**
 * @file "modules/ctrl/target_pos.c"
 * @author Freek van Tienen <freek.v.tienen@gmail.com>
 * Control a rotorcraft to follow at a defined distance from the target
 */

#include "target_pos.h"

#include "subsystems/datalink/telemetry.h"

// The timeout when receiving GPS messages from the ground in ms
#ifndef TARGET_POS_TIMEOUT
#define TARGET_POS_TIMEOUT 5000
#endif

#ifndef TARGET_OFFSET_HEADING
#define TARGET_OFFSET_HEADING 180.0
#endif

#ifndef TARGET_OFFSET_DISTANCE
#define TARGET_OFFSET_DISTANCE 10.0
#endif

#ifndef TARGET_OFFSET_HEIGHT
#define TARGET_OFFSET_HEIGHT 10.0
#endif

#ifndef TARGET_INTEGRATE
#define TARGET_INTEGRATE true
#endif

/* Initialize the main structure */
struct target_t target = {
  .pos = {0},
  .offset = {
    .heading = TARGET_OFFSET_HEADING,
    .distance = TARGET_OFFSET_DISTANCE,
    .height = TARGET_OFFSET_HEIGHT,
  },
  .target_pos_timeout = TARGET_POS_TIMEOUT,
  .integrate = TARGET_INTEGRATE
};

/**
 * Receive a TARGET_POS message from the ground
 */
void target_parse_target_pos(uint8_t *buf)
{
  if(DL_TARGET_POS_ac_id(buf) != AC_ID)
    return;

  // Save the received values
  target.pos.recv_time = get_sys_time_msec();
  target.pos.tow = gps_tow_from_sys_ticks(sys_time.nb_tick); // FIXME: need to get from the real GPS
  target.pos.lla.lat = DL_TARGET_POS_lat(buf);
  target.pos.lla.lon = DL_TARGET_POS_lon(buf);
  target.pos.lla.alt = DL_TARGET_POS_alt(buf);
  target.pos.ground_speed = DL_TARGET_POS_speed(buf);
  target.pos.climb = DL_TARGET_POS_climb(buf);
  target.pos.course = DL_TARGET_POS_course(buf);
  target.pos.valid = true;
}

/**
 * Get the current target position (NED) and heading
 */
bool target_get_pos(struct NedCoor_f *pos, float *heading) {

  /* When we have a valid target_pos message, state ned is initialized and no timeout */
  if(target.pos.valid && state.ned_initialized_i && (target.pos.recv_time+target.target_pos_timeout) > get_sys_time_msec()) {
    struct NedCoor_i target_pos_cm;

    // Convert from LLA to NED using origin from the UAV
    ned_of_lla_point_i(&target_pos_cm, &state.ned_origin_i, &target.pos.lla);

    // Convert to floating point (cm to meters)
    pos->x = target_pos_cm.x * 0.01;
    pos->y = target_pos_cm.y * 0.01;
    pos->z = target_pos_cm.z * 0.01;

    // If we have a velocity measurement try to integrate the position when enabled
    struct NedCoor_f vel = {0};
    if(target.integrate && target_get_vel(&vel)) {
      // In seconds, overflow uint32_t in 49,7 days
      float time_diff = (get_sys_time_msec() - target.pos.recv_time) * 0.001; // FIXME: should be based on TOW of ground gps

      // Integrate
      pos->x = pos->x + vel.x * time_diff;
      pos->y = pos->y + vel.y * time_diff;
      pos->z = pos->z + vel.z * time_diff;
    }

    // Offset the target
    pos->x += target.offset.distance * cosf((target.pos.course + target.offset.heading)/180.*M_PI);
    pos->y += target.offset.distance * sinf((target.pos.course + target.offset.heading)/180.*M_PI);
    pos->z += target.offset.height;

    // Return the course as heading (FIXME)
    *heading = target.pos.course;

    return true;
  }

  return false;
}

/**
 * Get the current target velocity (NED)
 */
bool target_get_vel(struct NedCoor_f *vel) {

  /* When we have a valid target_pos message, state ned is initialized and no timeout */
  if(target.pos.valid && state.ned_initialized_i && (target.pos.recv_time+target.target_pos_timeout) > get_sys_time_msec()) {
    // Calculate baed on ground speed and course
    vel->x = target.pos.ground_speed * cosf(target.pos.course/180.*M_PI);
    vel->y = target.pos.ground_speed * sinf(target.pos.course/180.*M_PI);
    vel->z = -target.pos.climb;

    return true;
  }

  return false;
}
