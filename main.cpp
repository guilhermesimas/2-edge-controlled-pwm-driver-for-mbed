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

#define PERIOD_INC 1
#define DUTY_CYCLE_INC 0.01
#define DEPHASE_INC 0.01

#define TRUE 1
#define FALSE 0

#define DA_ROW 0
#define DA_COL 4

#define DB_ROW 1
#define DB_COL 4

#define F_ROW 0
#define F_COL 4

#define P_ROW 0
#define P_COL 4


PwmDoubleOut led2 ( p23 );
InterruptIn knob( p13 );
PwmOut led1 ( p26 );
DigitalIn modeinc( p21 );
DigitalIn modedec( p22 );
// DigitalIn dephinc( p13 );
DigitalIn decoderIn( p14 );
TextLCD lcd( p15, p16, p17, p18, p19, p20 , TextLCD::LCD20x4 );

int periodUs = 2;
float dutyCycleA = 0.5f;
float dutyCycleB = 0.5f;
float dephase = 0.25f;
/*
 * Flag to select what the interrupt should do
 */

char flag = 0;

/*
Cursor control
*/
int row = DA_ROW;
int col = DA_COL;
/*
Value control
*/
float dcInc = 10;
float dpInc = 10;




void trigger() {
	int decoderMultiplier = 1;
	if ( decoderIn.read() == 0 ) {
		decoderMultiplier = -1;
	}
	switch ( flag ) {
	case 0:
		//Freq
		periodUs += PERIOD_INC * decoderMultiplier;
		led2.period_us( periodUs );
		break;
	case 1:
		//DutyCycleA
		dutyCycleA += DUTY_CYCLE_INC * decoderMultiplier;
		led1.write( dutyCycleA );
		break;
	case 2:
		//DutyCycleB
		dutyCycleB += DUTY_CYCLE_INC * decoderMultiplier;
		led2.write( dutyCycleB );
		break;
	case 3:
		//dephase
		dephase += DEPHASE_INC * decoderMultiplier;
		led2.dephase( dephase );
		break;
	}
}

int main() {

	// dutyinc.mode( PullUp );
	// dutydec.mode( PullUp );
	modeinc.mode( PullUp );
	modedec.mode( PullUp );

	led1.period_us( periodUs );
	led1.write( dutyCycleA );
	led2.write( dutyCycleB );
	led2.dephase( dephase );

	knob.rise( &trigger );
	// lcd.cls();
	// lcd.printf( "Duty cycle A:\n<%03.1f>%%\n", dutyCycleA * 100 );
	lcd.printf( "dA:<%03.1f>%%\ndB:<%03.1f>%%", dutyCycleA * 100,
	            dutyCycleB * 100 );
	// lcd.locate( 0, 0 );
	lcd.setCursor( TRUE );
	lcd.locate( 0, 0 );

	while ( 1 ) {
		if ( modeinc.read() == 0 ) {
			flag = ( flag + 1 ) % 4;
			lcd.cls();
			lcd.printf( "Mode <%d>", flag );
			wait( .5f );
		}
		if ( modedec.read() == 0 ) {
			flag = ( flag - 1 ) % 4;
			lcd.cls();
			lcd.printf( "Mode <%d>", flag );
			wait( .5f );
		}
	}
}
