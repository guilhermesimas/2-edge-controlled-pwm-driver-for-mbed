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
#include "mbed_assert.h"
#include "pwmdoubleout_api.h"
#include "cmsis.h"
#include "pinmap.h"

#define TCR_CNT_EN       0x00000001
#define TCR_RESET        0x00000002

//  PORT ID, PWM ID, Pin function
static const PinMap PinMap_PWM[] = {
	{P1_18, PWM_1, 2},
	{P1_20, PWM_2, 2},
	{P1_21, PWM_3, 2},
	{P1_23, PWM_4, 2},
	{P1_24, PWM_5, 2},
	{P1_26, PWM_6, 2},
	{P2_0 , PWM_1, 1},
	{P2_1 , PWM_2, 1},
	{P2_2 , PWM_3, 1},
	{P2_3 , PWM_4, 1},
	{P2_4 , PWM_5, 1},
	{P2_5 , PWM_6, 1},
	{P3_25, PWM_2, 3},
	{P3_26, PWM_3, 3},
	{NC, NC, 0}
};

__IO uint32_t *PWMDOUBLE_MATCH[] = {
	&( LPC_PWM1->MR0 ),
	&( LPC_PWM1->MR1 ),
	&( LPC_PWM1->MR2 ),
	&( LPC_PWM1->MR3 ),
	&( LPC_PWM1->MR4 ),
	&( LPC_PWM1->MR5 ),
	&( LPC_PWM1->MR6 )
};

#define TCR_PWM_EN       0x00000008

static unsigned int pwm_clock_mhz;

void pwmdoubleout_init( pwmdoubleout_t* obj, PinName pin ) {
	// determine the channel
	PWMName pwm = ( PWMName )pinmap_peripheral( pin, PinMap_PWM );
	MBED_ASSERT( pwm != ( PWMName )NC );

	obj->pwm = pwm;
	obj->MRA = PWMDOUBLE_MATCH[pwm - 1];
	obj->MRB = PWMDOUBLE_MATCH[pwm ];

	// ensure the power is on
	LPC_SC->PCONP |= 1 << 6;

	// ensure clock to /4
	LPC_SC->PCLKSEL0 &= ~( 0x3 << 12 );   // pclk = /4
	LPC_PWM1->PR = 0;                     // no pre-scale


	LPC_PWM1->MCR = 1 << 1; // reset TC on match 0

	// enable the specific PWM output
	// set double edge mode
	LPC_PWM1->PCR |=  1 << ( 8 + pwm ) | ( 1 << ( pwm ) ) ;

	pwm_clock_mhz = SystemCoreClock / 4000000;

	//Initialize MRA to 0
	*obj->MRA = 0;
	*obj->MRB = 0;
	LPC_PWM1->LER |= ( 1 << ( obj->pwm - 1 ) ) | ( 1 << ( obj->pwm ) );

	// default to 20ms: standard for servos, and fine for e.g. brightness control
	pwmdoubleout_period_ms( obj, 20 );
	pwmdoubleout_dephase( obj, 0.0f );
	pwmdoubleout_write    ( obj, 0 );

	// Wire pinout
	pinmap_pinout( pin, PinMap_PWM );
}

void pwmdoubleout_free( pwmdoubleout_t* obj ) {
	// [TODO]
}

void pwmdoubleout_dephase      ( pwmdoubleout_t* obj, float percent ) {
	if ( percent < 0.0f ) {
		percent = 0.0;
	} else if ( percent > 1.0f ) {
		percent = 1.0;
	}
	//for debuggig purposes
	// *obj->MRA = 0;
	uint32_t mra = *obj->MRA;
	uint32_t mrb = *obj->MRB;
	uint32_t mr0 = LPC_PWM1->MR0;

	// set channel match to percentage
	uint32_t v = ( uint32_t )( ( float )( LPC_PWM1->MR0 ) * percent );

	// workaround for PWM1[1] - Never make it equal MR0, else we get 1 cycle dropout
	if ( v == LPC_PWM1->MR0 ) {
		v++;
	}
	// get duty cycle to preserve it
	float dutyCycle = pwmdoubleout_read( obj );

	*obj->MRA = v;

	*obj->MRB = v + ( ( uint32_t )( dutyCycle * ( float )( LPC_PWM1->MR0 ) ) );
	//wraparound
	if ( *obj->MRB >= LPC_PWM1->MR0 ) {
		*obj->MRB = *obj->MRB - LPC_PWM1->MR0;
	}

	// accept on next period start
	LPC_PWM1->LER |= ( 1 << obj->pwm ) | ( 1 << ( obj->pwm - 1 ) );
}

void pwmdoubleout_write( pwmdoubleout_t* obj, float value ) {
	if ( value < 0.0f ) {
		value = 0.0;
	} else if ( value > 1.0f ) {
		value = 1.0;
	}

	// set channel match to percentage
	uint32_t v = ( uint32_t )( ( float )( LPC_PWM1->MR0 ) * value );

	// workaround for PWM1[1] - Never make it equal MR0, else we get 1 cycle dropout
	if ( v == LPC_PWM1->MR0 ) {
		v++;
	}

	*obj->MRB = *obj->MRA + v;
	//wraparound
	if ( *obj->MRB >= LPC_PWM1->MR0 ) {
		*obj->MRB = *obj->MRB - LPC_PWM1->MR0;
	}

	// accept on next period start
	LPC_PWM1->LER |= 1 << obj->pwm;
}

float pwmdoubleout_read( pwmdoubleout_t* obj ) {
	float v = ( float )( *obj->MRB - *obj->MRA ) / ( float )( LPC_PWM1->MR0 );
	//workaround for MRB < MRA
	if ( v < 0 ) {
		v = v + 1.0f;
	}
	return ( v > 1.0f ) ? ( 1.0f ) : ( v );
}

void pwmdoubleout_period( pwmdoubleout_t* obj, float seconds ) {
	pwmdoubleout_period_us( obj, seconds * 1000000.0f );
}

void pwmdoubleout_period_ms( pwmdoubleout_t* obj, int ms ) {
	pwmdoubleout_period_us( obj, ms * 1000 );
}

// Set the PWM period, keeping the duty cycle the same.
void pwmdoubleout_period_us( pwmdoubleout_t* obj, int us ) {
	// calculate number of ticks
	uint32_t ticks = pwm_clock_mhz * us;

	// set reset
	LPC_PWM1->TCR = TCR_RESET;

	// set the global match register
	LPC_PWM1->MR0 = ticks;

	// Scale the pulse width to preserve the duty ratio
	if ( LPC_PWM1->MR0 > 0 ) {
		*obj->MRA = ( *obj->MRA * ticks ) / LPC_PWM1->MR0;
		*obj->MRB = ( *obj->MRB * ticks ) / LPC_PWM1->MR0;
	}

	// set the channel latch to update value at next period start
	// LPC_PWM1->LER |= ( 1 << 0 ) | ( 1 << ( obj->pwm - 1 ) ) | ( 1 << obj->pwm );
	LPC_PWM1->LER |=  1 << 0  ;

	// enable counter and pwm, clear reset
	LPC_PWM1->TCR = TCR_CNT_EN | TCR_PWM_EN;
}

void pwmdoubleout_pulsewidth( pwmdoubleout_t* obj, float seconds ) {
	pwmdoubleout_pulsewidth_us( obj, seconds * 1000000.0f );
}

void pwmdoubleout_pulsewidth_ms( pwmdoubleout_t* obj, int ms ) {
	pwmdoubleout_pulsewidth_us( obj, ms * 1000 );
}

void pwmdoubleout_pulsewidth_us( pwmdoubleout_t* obj, int us ) {
	// calculate number of ticks
	uint32_t v = pwm_clock_mhz * us;

	// workaround for PWM1[1] - Never make it equal MR0, else we get 1 cycle dropout
	if ( v == LPC_PWM1->MR0 ) {
		v++;
	}

	// set the match register value
	*obj->MRB = *obj->MRA + v;

	// set the channel latch to update value at next period start
	LPC_PWM1->LER |= 1 << obj->pwm;
}
