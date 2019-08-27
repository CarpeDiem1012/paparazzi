/*
 * Copyright (C) 2015
 *
 * This file is part of Paparazzi.
 *
 * Paparazzi is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * Paparazzi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Paparazzi; see the file COPYING.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/**
 * @file modules/ctrl/ctrl_module_outerloop_demo.h
 * @brief example empty controller
 *
 */

#include "modules/ctrl/ctrl_module_outerloop_demo.h"
#include "state.h"
#include "subsystems/radio_control.h"
#include "firmwares/rotorcraft/stabilization.h"
#include "firmwares/rotorcraft/stabilization/stabilization_attitude.h"
#include "firmwares/rotorcraft/stabilization/stabilization_attitude_rc_setpoint.h"
#include "autopilot.h"
#include "modules/qpoas/qpoas.h"
#include <stdio.h>
// Own Variables

struct ctrl_module_demo_struct {
  // RC Inputs
  struct Int32Eulers rc_sp;
  // Output command
  struct Int32Eulers cmd;
} ctrl;

// Settings
float comode_time = 0;


////////////////////////////////////////////////////////////////////
// Call our controller
// Implement own Horizontal loops
void guidance_h_module_init(void)
{
}

void guidance_h_module_enter(void)
{
  // Store current heading
  ctrl.cmd.psi = stateGetNedToBodyEulers_i()->psi;

  optimal_enter();

  // Convert RC to setpoint
  stabilization_attitude_read_rc_setpoint_eulers(&ctrl.rc_sp, autopilot.in_flight, false, false);
}

void guidance_h_module_read_rc(void)
{
  stabilization_attitude_read_rc_setpoint_eulers(&ctrl.rc_sp, autopilot.in_flight, false, false);
}

float roll = 0.0;
float pitch = 0.0;

void guidance_h_module_run(bool in_flight)
{
  // YOUR NEW HORIZONTAL OUTERLOOP CONTROLLER GOES HERE
  // ctrl.cmd = CallMyNewHorizontalOuterloopControl(ctrl);
  dronerace_get_cmd(&roll, &pitch);

  // printf("roll: %f \t pitch: %f \n", roll, pitch);
  
  ctrl.cmd.phi = ANGLE_BFP_OF_REAL(roll);
  ctrl.cmd.theta = ANGLE_BFP_OF_REAL(pitch);
  
  stabilization_attitude_set_rpy_setpoint_i(&(ctrl.cmd));
  stabilization_attitude_run(in_flight);

  // Alternatively, use the indi_guidance and send AbiMsgACCEL_SP to it instead of setting pitch and roll
}

#if 1
void guidance_v_module_init(void)
{
  // initialization of your custom vertical controller goes here
}

// Implement own Vertical loops
void guidance_v_module_enter(void)
{
  // your code that should be executed when entering this vertical mode goes here
}

#define KP_ALT 0.4
#define KD_ALT 0.05
#define HOVERTHRUST 0.55
void guidance_v_module_run(bool in_flight)
{ 
  struct NedCoor_f *optipos = stateGetPositionNed_f();
  struct NedCoor_f *optivel = stateGetSpeedNed_f();
  struct FloatEulers *att = stateGetNedToBodyEulers_f();

  // Altitude control 
  float z_cmd = -1.5; 
  float z_measured  = optipos->z;
  float zv_measured = optivel->z;
  
  float thrust_cmd = -(KP_ALT *(z_cmd - z_measured) - KD_ALT * zv_measured) + HOVERTHRUST /  (cosf(att->phi * 0.8)*cosf(att->theta * 0.8));

  // float nominal = radio_control.values[RADIO_THROTTLE];

  stabilization_cmd[COMMAND_THRUST] = thrust_cmd*9125;
}
#endif