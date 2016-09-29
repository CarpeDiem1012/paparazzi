/*
 * Copyright (C) Shuo Li
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
 * @file "modules/command_level_iros/command_level_iros.c"
 * @author Shuo Li
 * This module is the highest level
 */

#include <stdio.h>
#include "modules/command_level_iros/command_level_iros.h"
#include "firmwares/rotorcraft/autopilot.h"
#include "modules/flight_plan_in_guided_mode/flight_plan_in_guided_mode.h"
#include "modules/flight_plan_in_guided_mode/flight_plan_clock.h"
//#include "modules/computer_vision/fly_through_gate_demo.h"
//#include "modules/stereocam/stereo_gate_position/stereo_gate_position.h"
#include "modules/state_autonomous_race/state_autonomous_race.h"
//#include "modules/replay_commands/replay_commands.h"
#include "modules/computer_vision/snake_gate_detection.h"

#define PI 3.1415926

uint8_t previous_mode;
uint8_t current_mode;

void first_part_logic(void);
void second_part_logic(void);
void third_part_logic(void);
float choose_heading_after_passing_through_gate(void);
float choose_distance_after_gate(void);

int counter_of_step;
enum states_lower_level state_lower_level = WAIT_FOR_DETECTION_CM;
enum states_upper_level state_upper_level = FIRST_PART;

struct parameters_to_be_tuned parameter_to_be_tuned;

void command_init(){
    previous_mode = autopilot_mode;
    current_mode = autopilot_mode;
    states_race.time_to_go_straight = 0;
    states_race.distance_before_gate = 1.5;
    //gate_counter_in_second_part = 0;
    //record_command = 0;

    // here we need to change the angle and distance for passing through the gates
    parameter_to_be_tuned.distance_first_gate = 1;
    parameter_to_be_tuned.distance_second_gate = 1;
    parameter_to_be_tuned.distance_third_gate = 1;
    parameter_to_be_tuned.distance_fourth_gate = 1;
    parameter_to_be_tuned.heading_after_first_gate = PI;
    parameter_to_be_tuned.heading_after_second_gate = PI;
    parameter_to_be_tuned.heading_after_third_gate = PI;
    parameter_to_be_tuned.heading_after_fourth_gate = PI;
}


void command_run() {
  
    current_mode = autopilot_mode;
    if (previous_mode != current_mode)
    {
        counter_autopilot_mode = 0;
        time_autopilot_mode = 0;
        primitive_in_use = NO_PRIMITIVE;
        state_lower_level = TAKE_OFF_CM;
        state_upper_level = FIRST_PART;
        states_race.gate_counter_in_second_part = 0;
        counter_of_step = 0;
        states_race.altitude_is_achieved = 0;  // we need to take off from ground
    }
    if (autopilot_mode != AP_MODE_MODULE) {
        return;
    }

    if(state_upper_level  == FIRST_PART)
    {

        first_part_logic();
    }

    if(state_upper_level  == SECOND_PART)
    {
        second_part_logic();
    }

    if(state_upper_level  == THIRD_PART)
    {
        third_part_logic();
    }
    previous_mode = current_mode;
}








void first_part_logic()
{
    switch (state_lower_level){
        case TAKE_OFF_CM:
            take_off(-1.5);
            if (states_race.altitude_is_achieved == TRUE)
            {
                state_lower_level = HOVER_CM;
                counter_of_step++;
            }
            break;
        case HOVER_CM:
            hover();
            if (time_primitive < 2)
                return;
            if (counter_of_step == 1)
            {
                state_lower_level = GO_STRAIGHT_CM;
            }
            else
            {
                state_lower_level = TURN_CM;
            }

            counter_of_step++;
            break;
        case GO_STRAIGHT_CM:
            go_straight(0.5);
            if (time_primitive < 3)
                return;
                state_lower_level = HOVER_CM;
                counter_of_step++;
            break;
        case TURN_CM:
            change_heading_hover(-90.0/180*PI);
            if (time_primitive < 1)
                return;
            else
            {
                state_lower_level = WAIT_FOR_DETECTION_CM;
                state_upper_level = SECOND_PART;
                counter_of_step++;
            }
            break;

    }
}



void second_part_logic()
{
    // if we pass through 2 gates, we can change to third part
    if ( states_race.gate_counter_in_second_part == 2)
    {
        state_upper_level = THIRD_PART;
        return;
    }
    
    switch (state_lower_level)
    {
        case WAIT_FOR_DETECTION_CM:
            hover();
            if(time_primitive < 1)
                break;
            if(states_race.gate_detected == FALSE)
            {
                hover();
            }
            else
            {
                state_lower_level = ADJUST_POSITION_CM;
            }

            if (time_gate_detected > 3 && states_race.gate_detected == FALSE)
            {
                // gate is lost
                state_lower_level = SEARCH_GATE_CM;
            }
            break;


        case ADJUST_POSITION_CM:
            if (states_race.gate_detected == FALSE && time_gate_detected > 0.5)
            {
                // lost gate
                state_lower_level = WAIT_FOR_DETECTION_CM;
                break;
            }
            if(states_race.ready_pass_through == FALSE)
                adjust_position(-delta_z_gate);
            else
            {
                states_race.distance_before_gate = current_y_gate;
                //state_lower_level = GO_THROUGH_CM;
                state_lower_level = GO_THROUGH_CM;
            }
            break;


        case GO_THROUGH_CM:
            states_race.time_to_go_straight = (states_race.distance_before_gate +
                    choose_distance_after_gate())/(CONSTANT_VELOCITY_STRAIGHT*1.2);
            go_straight(CONSTANT_VELOCITY_STRAIGHT);
            if (time_primitive > states_race.time_to_go_straight)
            {
                state_lower_level = HOVER_CM;
            }
            break;


        case HOVER_CM:
            hover();
            if (time_primitive > 2)
            {
                state_lower_level = TURN_CM;
            }
            break;


        case TURN_CM:
            change_heading_hover(choose_heading_after_passing_through_gate());
            if (states_race.turning == FALSE)
            {
                // turning is finished, go to next gate
                init_pos_filter = 1;
                state_lower_level = WAIT_FOR_DETECTION_CM;
                states_race.gate_counter_in_second_part ++;
            }
            break;


        case SEARCH_GATE_CM:
            search_gate();
            if (states_race.gate_detected == 1 && time_gate_detected > 0.5)
            {
                state_lower_level = WAIT_FOR_DETECTION_CM;
            }
            break;
    }
}

void third_part_logic()
{
    land();
}
















float choose_heading_after_passing_through_gate()
{
    float heading_turn = 0;
    switch (states_race.gate_counter_in_second_part)
    {
        case 0:
            heading_turn =parameter_to_be_tuned.heading_after_first_gate;
            break;
        case 1:
            heading_turn = parameter_to_be_tuned.heading_after_second_gate;
            break;
        case 2:
            heading_turn = parameter_to_be_tuned.heading_after_third_gate;
            break;
        case 3:
            heading_turn = parameter_to_be_tuned.heading_after_fourth_gate;
            break;

    }
    return heading_turn;
}

float choose_distance_after_gate()
{
    float distance_after_gate = 1;
    switch (states_race.gate_counter_in_second_part)
    {
        case 0:
            distance_after_gate = parameter_to_be_tuned.distance_first_gate;
            break;
        case 1:
            distance_after_gate = parameter_to_be_tuned.distance_second_gate;
            break;
        case 2:
            distance_after_gate = parameter_to_be_tuned.distance_third_gate;
            break;
        case 3:
            distance_after_gate = parameter_to_be_tuned.distance_fourth_gate;
            break;
    }
    return distance_after_gate;
}