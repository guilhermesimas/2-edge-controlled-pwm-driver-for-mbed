/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef MBED_PWMDOUBLEOUT_API_H
#define MBED_PWMDOUBLEOUT_API_H

#include "device.h"

#if DEVICE_PWMDOUBLEOUT

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pwmdoubleout_s pwmdoubleout_t;

void pwmdoubleout_init         ( pwmdoubleout_t* obj, PinName pin );
void pwmdoubleout_free         ( pwmdoubleout_t* obj );

void  pwmdoubleout_write       ( pwmdoubleout_t* obj, float percent );
float pwmdoubleout_read        ( pwmdoubleout_t* obj );

void pwmdoubleout_dephase      ( pwmdoubleout_t* obj, float percent );

void pwmdoubleout_period       ( pwmdoubleout_t* obj, float seconds );
void pwmdoubleout_period_ms    ( pwmdoubleout_t* obj, int ms );
void pwmdoubleout_period_us    ( pwmdoubleout_t* obj, int us );

void pwmdoubleout_freq_khz     ( pwmdoubleout_t* obj, int khz );

void pwmdoubleout_pulsewidth   ( pwmdoubleout_t* obj, float seconds );
void pwmdoubleout_pulsewidth_ms( pwmdoubleout_t* obj, int ms );
void pwmdoubleout_pulsewidth_us( pwmdoubleout_t* obj, int us );

#ifdef __cplusplus
}
#endif

#endif

#endif
