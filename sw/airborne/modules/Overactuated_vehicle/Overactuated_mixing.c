/*
 * Copyright (C) 2021 A. Mancinelli
 *
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
 * @file "modules/Overactuated_vehicle/Overactuated_vehicle.c"
 * @author Alessandro Mancinelli
 * Control laws for Overactuated Vehicle
 */

#include "Overactuated_mixing.h"
#include <math.h>
#include "subsystems/radio_control.h"
#include "state.h"
#include "subsystems/datalink/telemetry.h"


struct overactuated_mixing_t overactuated_mixing;
struct overactuated_mixing_t overactuated_mixing;

float phi, theta, psi, x, y, z, u, v, w;

// PID and general settings from slider
float P_az_gain = 12.7 ;
float I_az_gain = 1 ;
float D_az_gain = 0.002 ;
float P_el_gain = 12.9 ;
float I_el_gain = 1 ;
float D_el_gain = 0.005 ;
int Deadband_stick = 100;
float Stick_gain_position = 0.1; // Stick to position gain


float desired_x_e = 0;
float desired_y_e = 0;
float lateral_cmd_old = 0;
float longitudinal_cmd_old = 0;

static void send_overactuated_variables( struct transport_tx *trans , struct link_device * dev ) {

// Send telemetry message
    int16_t actuators[8];
    for (uint8_t i = 0; i < 8; i++)
    {
        actuators[i] = (int16_t)overactuated_mixing.commands[i];
    }
    float psi_deg = psi*180/3.14;

    pprz_msg_send_OVERACTUATED_VARIABLES(trans , dev , AC_ID , & x, & y, & psi_deg, & desired_x_e, & desired_y_e, 8, actuators );
}


/**
 * Initialize the motor mixing and calculate the trim values
 */
void overactuated_mixing_init() {
    uint8_t i;
    phi = stateGetNedToBodyEulers_f()->phi;
    theta = stateGetNedToBodyEulers_f()->theta;
    psi = stateGetNedToBodyEulers_f()->psi;
    x = stateGetPositionNed_i()->x;
    y = stateGetPositionNed_i()->y;
    z = stateGetPositionNed_i()->z;
    u = stateGetSpeedNed_i()->x;
    v = stateGetSpeedNed_i()->y;
    w = stateGetSpeedNed_i()->z;

    // Case of Manual direct mode
    if(radio_control.values[RADIO_MODE] < 500 && radio_control.values[RADIO_MODE] > -500)
    {
        // Go trough all the motors and calculate the trim value and set the initial command
        for (i = 0; i < N_ACT; i++) {
            overactuated_mixing.commands[i] = 0;
            if (i % 2 != 0)   //Odd value --> (azimuth angle)
            {
                overactuated_mixing.commands[i] = radio_control.values[RADIO_ROLL];
            } else           //Even value --> (elevation angle)
            {
                overactuated_mixing.commands[i] = radio_control.values[RADIO_PITCH];
            }
            BoundAbs(overactuated_mixing.commands[i], MAX_PPRZ);
        }

        // Reset variables for the position hold to avoid bump.
        desired_x_e = x;
        desired_y_e = y;
    }

    // Case of Position hold mode
    if(radio_control.values[RADIO_MODE] > 500)
    {
        //Calculate the desired position considering the RC input
        if( abs(radio_control.values[RADIO_PITCH]) > Deadband_stick ){
            desired_x_e += radio_control.values[RADIO_PITCH]*Stick_gain_position*-0.0002;
        }
        if( abs(radio_control.values[RADIO_ROLL]) > Deadband_stick ){
            desired_y_e += radio_control.values[RADIO_ROLL]*Stick_gain_position*0.0002;
        }

        float longitudinal_cmd = cos(psi) * (desired_x_e - x) + sin(psi)*(desired_y_e - y);
        float longitudinal_speed = cos(psi) * u + sin(psi)*v;

        float lateral_cmd = cos(psi) * (desired_y_e - y) -sin(psi) * (desired_x_e - x);
        float lateral_speed = cos(psi) * v - sin(psi) * u;

//        // Compute derivative of this term considering 500 Hz as module frequency update
//        float lateral_cmd_der = (lateral_cmd - lateral_cmd_old)*500;
//        float longitudinal_cmd_der = (longitudinal_cmd - longitudinal_cmd_old)*500;
//        lateral_cmd_old = lateral_cmd;
//        longitudinal_cmd_old = longitudinal_cmd;

        //Applying PID to command to get the servo deflection values:
        float azimuth_cmd = 5*(P_az_gain*lateral_cmd - D_az_gain*lateral_speed);
        float elevation_cmd = 5*(P_el_gain*longitudinal_cmd - D_el_gain*longitudinal_speed);

        // Write values to servos
        for (i = 0; i < N_ACT; i++) {
            overactuated_mixing.commands[i] = 0;
            if (i % 2 != 0)   //Odd value --> (azimuth angle)
            {
                overactuated_mixing.commands[i] = azimuth_cmd;
            } else           //Even value --> (elevation angle)
            {
                overactuated_mixing.commands[i] = -elevation_cmd;
            }
            BoundAbs(overactuated_mixing.commands[i], MAX_PPRZ);
        }
    }

    register_periodic_telemetry ( DefaultPeriodic , PPRZ_MSG_ID_OVERACTUATED_VARIABLES , send_overactuated_variables );
}

/*
 * Run the overactuated mixing
 * This depends on the ROLL and PITCH command
 * It also depends on the throttle_curve.collective
 */
void overactuated_mixing_run()
{
    uint8_t i;
    phi = stateGetNedToBodyEulers_f()->phi;
    theta = stateGetNedToBodyEulers_f()->theta;
    psi = stateGetNedToBodyEulers_f()->psi;
    x = stateGetPositionNed_i()->x;
    y = stateGetPositionNed_i()->y;
    z = stateGetPositionNed_i()->z;
    u = stateGetSpeedNed_i()->x;
    v = stateGetSpeedNed_i()->y;
    w = stateGetSpeedNed_i()->z;

    // Case of Manual direct mode
    if(radio_control.values[RADIO_MODE] < 500 && radio_control.values[RADIO_MODE] > -500)
    {
        // Go trough all the motors and calculate the trim value and set the initial command
        for (i = 0; i < N_ACT; i++) {
            overactuated_mixing.commands[i] = 0;
            if (i % 2 != 0)   //Odd value --> (azimuth angle)
            {
                overactuated_mixing.commands[i] = radio_control.values[RADIO_ROLL];
            } else           //Even value --> (elevation angle)
            {
                overactuated_mixing.commands[i] = radio_control.values[RADIO_PITCH];
            }
            BoundAbs(overactuated_mixing.commands[i], MAX_PPRZ);
        }

        // Reset variables for the next mode to avoid bump.
        desired_x_e = x;
        desired_y_e = y;
    }

    // Case of Position hold mode
    if(radio_control.values[RADIO_MODE] > 500)
    {
        //Calculate the desired position considering the RC input
        if( abs(radio_control.values[RADIO_PITCH]) > Deadband_stick ){
            desired_x_e += radio_control.values[RADIO_PITCH]*Stick_gain_position*-0.002;
        }
        if( abs(radio_control.values[RADIO_ROLL]) > Deadband_stick ){
            desired_y_e += radio_control.values[RADIO_ROLL]*Stick_gain_position*0.002;
        }

        float longitudinal_cmd = cos(psi) * (desired_x_e - x) + sin(psi)*(desired_y_e - y);
        float longitudinal_speed = cos(psi) * u + sin(psi)*v;

        float lateral_cmd = cos(psi) * (desired_y_e - y) -sin(psi) * (desired_x_e - x);
        float lateral_speed = cos(psi) * v - sin(psi) * u;

//        // Compute derivative of this term considering 500 Hz as module frequency update
//        float lateral_cmd_der = (lateral_cmd - lateral_cmd_old)*500;
//        float longitudinal_cmd_der = (longitudinal_cmd - longitudinal_cmd_old)*500;
//        lateral_cmd_old = lateral_cmd;
//        longitudinal_cmd_old = longitudinal_cmd;

        // Applying PID to command to get the servo deflection values:
        float azimuth_cmd = 5*(P_az_gain*lateral_cmd - D_az_gain*lateral_speed);
        float elevation_cmd = 5*(P_el_gain*longitudinal_cmd - D_el_gain*longitudinal_speed);

        // Write values to servos
        for (i = 0; i < N_ACT; i++) {
            overactuated_mixing.commands[i] = 0;
            if (i % 2 != 0)   //Odd value --> (azimuth angle)
            {
                overactuated_mixing.commands[i] = azimuth_cmd;
            } else           //Even value --> (elevation angle)
            {
                overactuated_mixing.commands[i] = -elevation_cmd;
            }
            BoundAbs(overactuated_mixing.commands[i], MAX_PPRZ);
        }
    }
}

