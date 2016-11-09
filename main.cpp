#include "mbed.h"
#include "test_env.h"
#include "rtos.h"
#include "TextLCD.h"
#include "PwmDoubleOut.h"

/*
 * The stack size is defined in cmsis_os.h mainly dependent on the underlying toolchain and
 * the C standard library. For GCC, ARM_STD and IAR it is defined with a size of 2048 bytes
 * and for ARM_MICRO 512. Because of reduce RAM size some targets need a reduced stacksize.
 */
#if (defined(TARGET_STM32L053R8) || defined(TARGET_STM32L053C8)) && defined(TOOLCHAIN_GCC)
#define STACK_SIZE DEFAULT_STACK_SIZE/2
#elif (defined(TARGET_STM32F030R8) || defined(TARGET_STM32F070RB)) && defined(TOOLCHAIN_GCC)
#define STACK_SIZE DEFAULT_STACK_SIZE/2
#elif (defined(TARGET_STM32F030R8)) && defined(TOOLCHAIN_IAR)
#define STACK_SIZE DEFAULT_STACK_SIZE/2
#else
#define STACK_SIZE DEFAULT_STACK_SIZE
#endif

#define PWM_CYCLE_VALUE 100
#define PWM_CYCLE_VALUE_HALF 50

#define PWM_1 0
#define PWM_4 6

#define SBIT_CNTEN 0
#define SBIT_PWMEN 2

#define SBIT_PWMMR0R 1

#define SBIT_PCPWM1 6

#define SBIT_LEN0 0
#define SBIT_LEN1 1
#define SBIT_LEN3 3
#define SBIT_LEN4 4

#define PCLK_PWM1 12
#define SCLK_CLK 01


#define SBIT_PWMENA1 9
#define SBIT_PWMENA4 12

#define SBIT_PWM_DE_4 4

// AnalogIn freqIn( p19 );
// AnalogIn dephaseIn( p20 );
// PwmOut led1 ( p26 );
PwmDoubleOut led2 ( p23 );
// PwmOut led2 ( LED3 );
DigitalIn dutyinc( p21 );
DigitalIn dutydec( p22 );
DigitalIn dephinc( p13 );
DigitalIn dephdec( p14 );
TextLCD lcd( p15, p16, p17, p18, p19, p20 );


int main() {

	float dutyCycle = 0.5f;
	float dephase = 0.0f;
	dutyinc.mode( PullUp );
	dutydec.mode( PullUp );
	dephinc.mode( PullUp );
	dephdec.mode( PullUp );

	led2.period_us( 1 );
	led2.dephase( dephase );
	led2.write( dutyCycle );
	// led1.period_us( 1 );
	// led1.write( dutyCycle );

	while ( 1 ) {
		if ( dutyinc.read() == 0 ) {
			dutyCycle += 0.01f;
			wait( 0.1f );
			led2.write( dutyCycle );
			lcd.cls();
			lcd.printf( "DutyCycle:<%.1f>", 100 * led2.read() );
			wait( .1f );
		}
		if ( dutydec.read() == 0 ) {
			dutyCycle -= 0.01f;
			led2.write( dutyCycle );
			lcd.cls();
			lcd.printf( "DutyCycle:<%.1f>", 100 * led2.read() );
			wait( .1f );
		}
		if ( dephinc.read() == 0 ) {
			dephase += 0.1f;
			led2.dephase( dephase );
			led2.write( dutyCycle );
			// led1.write(  );
			// led2.write( dephaseIn.read() );
		}
		if ( dephdec.read() == 0 ) {
			dephase -= 0.1f;
			led2.dephase( dephase );
			led2.write( dutyCycle );
			// led1.write(  );
			// led2.write( dephaseIn.read() );
		}
	}
}
