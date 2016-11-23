#include "mbed.h"
#include "test_env.h"
#include "rtos.h"
#include "TextLCD.h"
#include "PwmDoubleOut.h"
#include <atomic>


#define FREQ_INC 1
#define DUTY_CYCLE_INC 0.01
#define DEPHASE_INC 0.01

#define TRUE 1
#define FALSE 0

#define DA_ROW 0
#define DA_COL 4

#define DB_ROW 1
#define DB_COL 4

#define F_ROW 2
#define F_COL 4

// #define P_ROW 3
#define P_COL 4

static constexpr auto P_ROW = 0;


PwmDoubleOut waveB ( p23 );
InterruptIn knob( p13 );
PwmOut waveA ( p26 );
DigitalIn modeinc( p21 );
DigitalIn modedec( p22 );
DigitalIn decoderIn( p14 );
TextLCD lcd( p15, p16, p17, p18, p19, p20 , TextLCD::LCD20x4 );

std::atomic<uint32_t> freqKhz = 1000;
std::atomic<float> dutyCycleA = 0.5f;
std::atomic<float> dutyCycleB = 0.5f;
std::atomic<float> dephase = 0.25f;
/*
 * Flag to select what the interrupt should do
 */

std::atomic<uint8_t> flag;

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
	switch ( flag.load() ) {
	case 0:
		//Freq
		// freqKhz += FREQ_INC * decoderMultiplier;
		freqKhz.fetch_add( FREQ_INC * decoderMultiplier );
		waveB.freq_khz( freqKhz.load() );
		break;
	case 1:
		//DutyCycleA
		// dutyCycleA += DUTY_CYCLE_INC * decoderMultiplier;
		dutyCycleA.fetch_add( DUTY_CYCLE_INC * decoderMultiplier );
		waveA.write( dutyCycleA.load() );
		break;
	case 2:
		//DutyCycleB
		// dutyCycleB += DUTY_CYCLE_INC * decoderMultiplier;
		dutyCycleB.fetch_add( DUTY_CYCLE_INC * decoderMultiplier );
		waveB.write( dutyCycleB.load() );
		break;
	case 3:
		//dephase
		// dephase += DEPHASE_INC * decoderMultiplier;
		dephase.fetch_add( DUTY_CYCLE_INC * decoderMultiplier );
		waveB.dephase( dephase.load() );
		break;
	}
}

int main() {

	// dutyinc.mode( PullUp );
	// dutydec.mode( PullUp );
	modeinc.mode( PullUp );
	modedec.mode( PullUp );

	waveB.freq_khz( freqKhz.load() );
	waveA.write( dutyCycleA.load() );
	waveB.write( dutyCycleB.load() );
	waveB.dephase( dephase.load() );

	knob.rise( &trigger );
	// lcd.cls();
	// lcd.printf( "Duty cycle A:\n<%03.1f>%%\n", dutyCycleA * 100 );
	lcd.printf( "dA:<%03.1f>%%\ndB:<%03.1f>%%", dutyCycleA.load() * 100,
	            dutyCycleB.load() * 100 );
	lcd.printf( "\nPh:<%03.1f>%%\nFq:<%4d>KHz", dephase.load(), freqKhz.load() );
	// lcd.locate( 0, 0 );
	lcd.setCursor( TRUE );
	lcd.locate( 0, 0 );
	lcd.locate( 1, 1 );


	while ( 1 ) {
		if ( modeinc.read() == 0 ) {
			uint8_t temp = flag.load();
			flag.store( ( temp + 1 ) % 4 );
			lcd.cls();
			// lcd.printf( "Mode <%d>", flag );
			wait( .5f );
		}
		if ( modedec.read() == 0 ) {
			uint8_t temp = flag.load();
			flag.store( ( temp - 1 ) % 4 );
			lcd.cls();
			// lcd.printf( "Mode <%d>", flag );
			wait( .5f );
		}
	}
}
