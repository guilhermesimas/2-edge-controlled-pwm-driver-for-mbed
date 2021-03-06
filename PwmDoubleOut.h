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
#ifndef MBED_PWMDOUBLEOUT_H
#define MBED_PWMDOUBLEOUT_H

#include "platform.h"

#if DEVICE_PWMDOUBLEOUT
#include "pwmdoubleout_api.h"

namespace mbed {

/** A pulse-width modulation digital output
 *
 * Example
 * @code
 * // Fade a led on.
 * #include "mbed.h"
 *su
 * PwmOut led(LED1);
 *
 * int main() {
 *     while(1) {
 *         led = led + 0.01;
 *         wait(0.2);
 *         if(led == 1.0) {
 *             led = 0;
 *         }
 *     }
 * }
 * @endcode
 *
 * @note
 *  On the LPC1768 and LPC2368, the PWMs all share the same
 *  period - if you change the period for one, you change it for all.
 *  Although routines that change the period maintain the duty cycle
 *  for its PWM, all other PWMs will require their duty cycle to be
 *  refreshed.
 */
class PwmDoubleOut {

public:

	/** Create a PwmOut connected to the specified pin
	 *
	 *  @param pin PwmOut pin to connect to
	 */
	PwmDoubleOut( PinName pin ) {
		pwmdoubleout_init( &_pwm, pin );
	}

	/** Set the ouput dephase, specified as a percentage (float)
	 *
	 *  @param value A floating-point value representing the output dephase,
	 *    specified as a percentage. The value should lie between
	 *    0.0f (representing no dephase) and 1.0f (representing one full cycle dephase).
	 *    Values outside this range will be saturated to 0.0f or 1.0f.
	 */
	void dephase( float value ) {
		pwmdoubleout_dephase( &_pwm, value );
	}
	/** Set the ouput dephase, specified as a percentage (float)
	 *
	 *  @param value A floating-point value representing the output dephase,
	 *    specified as a percentage. The value should lie between
	 *    0.0f (representing no dephase) and 1.0f (representing one full cycle dephase).
	 *    Values outside this range will be saturated to 0.0f or 1.0f.
	 */
	void set_dephase( int value ) {
		pwmdoubleout_set_dephase( &_pwm, value );
	}

	/** Set the ouput duty-cycle, specified as the register value (int)
	 *
	 *  @param value A floating-point value representing the output duty-cycle,
	 *    specified as a percentage. The value should lie between
	 *    0.0f (representing on 0%) and 1.0f (representing on 100%).
	 *    Values outside this range will be saturated to 0.0f or 1.0f.
	 */
	void write( float value ) {
		pwmdoubleout_write( &_pwm, value );
	}
	/** Set the ouput duty-cycle, specified as a percentage (float)
		 *
		 *  @param value A floating-point value representing the output duty-cycle,
		 *    specified as a percentage. The value should lie between
		 *    0.0f (representing on 0%) and 1.0f (representing on 100%).
		 *    Values outside this range will be saturated to 0.0f or 1.0f.
		 */
	void set_duty_cycle( int value ) {
		pwmdoubleout_set_duty_cycle( &_pwm, value );
	}

	/** Return the current output duty-cycle setting, measured as a percentage (float)
	 *
	 *  @returns
	 *    A floating-point value representing the current duty-cycle being output on the pin,
	 *    measured as a percentage. The returned value will lie between
	 *    0.0f (representing on 0%) and 1.0f (representing on 100%).
	 *
	 *  @note
	 *  This value may not match exactly the value set by a previous <write>.
	 */
	float read() {
		return pwmdoubleout_read( &_pwm );
	}

	/** Set the PWM period, specified in seconds (float), keeping the duty cycle the same.
	 *
	 *  @note
	 *   The resolution is currently in microseconds; periods smaller than this
	 *   will be set to zero.
	 */
	void period( float seconds ) {
		pwmdoubleout_period( &_pwm, seconds );
	}

	/** Set the PWM period, specified in milli-seconds (int), keeping the duty cycle the same.
	 */
	void period_ms( int ms ) {
		pwmdoubleout_period_ms( &_pwm, ms );
	}

	/** Set the PWM period, specified in micro-seconds (int), keeping the duty cycle the same.
	 */
	void period_us( int us ) {
		pwmdoubleout_period_us( &_pwm, us );
	}
	/** Set the PWM frequency, specified in khz (int), keeping the duty cycle the same.
	 */
	void freq_khz( int khz ) {
		pwmdoubleout_freq_khz( &_pwm, khz );
	}
	/** Set the PWM frequency, specified in khz (int), keeping the duty cycle the same.
	 */
	void set_freq( int value ) {
		pwmdoubleout_set_freq( &_pwm, value );
	}

	/** Set the PWM pulsewidth, specified in seconds (float), keeping the period the same.
	 */
	void pulsewidth( float seconds ) {
		pwmdoubleout_pulsewidth( &_pwm, seconds );
	}

	/** Set the PWM pulsewidth, specified in milli-seconds (int), keeping the period the same.
	 */
	void pulsewidth_ms( int ms ) {
		pwmdoubleout_pulsewidth_ms( &_pwm, ms );
	}

	/** Set the PWM pulsewidth, specified in micro-seconds (int), keeping the period the same.
	 */
	void pulsewidth_us( int us ) {
		pwmdoubleout_pulsewidth_us( &_pwm, us );
	}

#ifdef MBED_OPERATORS
	/** A operator shorthand for write()
	 */
	PwmDoubleOut& operator= ( float value ) {
		write( value );
		return *this;
	}

	PwmDoubleOut& operator= ( PwmDoubleOut& rhs ) {
		write( rhs.read() );
		return *this;
	}

	/** An operator shorthand for read()
	 */
	operator float() {
		return read();
	}
#endif

protected:
	pwmdoubleout_t _pwm;
};

} // namespace mbed

#endif

#endif
