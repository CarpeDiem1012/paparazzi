/*
 * Copyright (C) 2015 C. De Wagter
 *               2015 Freek van Tienen <freek.v.tienen@gmail.com>
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
 * @file "modules/Overactuated_vehicle/Overactuated_vehicle.h"
 * @author Alessandro Mancinelli
 * Control laws for Overactuated Vehicle
 */

#ifndef OVERACTUATED_MIXING_H
#define OVERACTUATED_MIXING_H

#include "std.h"
#include "paparazzi.h"
#include "generated/airframe.h"


/**
 * DEFINE VEHICLE PROPERTIES
 */
#define N_ACT 8
#define INDI_OUTPUTS 6
#define INDI_NUM_ACT 12

/* Overactuated mixing structure */
struct overactuated_mixing_t {
  int32_t commands[N_ACT];      ///< The output commands
};

extern struct overactuated_mixing_t overactuated_mixing;

// Variables for slider
extern float wind_speed;
extern float P_az_gain;
extern float I_az_gain;
extern float D_az_gain;
extern float P_el_gain;
extern float I_el_gain;
extern float D_el_gain;
extern int Deadband_stick;
extern float Stick_gain_position;
extern bool activate_longitudinal_over;
extern bool activate_lateral_over;
extern bool activate_yaw_over;
extern bool manual_yaw_overactuated;

// For external module
extern bool manual_roll_setpoint;
extern bool manual_pitch_setpoint;
extern int32_t manual_roll_overactuated_module;
extern int32_t manual_pitch_overactuated_module;

/* External used functions */
extern void overactuated_mixing_init(void);
extern void overactuated_mixing_run(pprz_t in_cmd[]);

#endif
