#include "mbed.h"
#include "test_env.h"
#include "rtos.h"
/*
 * Auxiliary implemented libraries
 */
#include "TextLCD.h"
#include "PwmDoubleOut.h"
/*
 * C++ lib for atomic operations
 */
#include <atomic>

/*
 * Initial value that will be loaded to each parameter
 */

static constexpr auto FREQ_INIT = 192; // 500KHz
static constexpr auto DUTY_CYCLE_INIT = FREQ_INIT / 2; //50%
static constexpr auto DEPHASE_INIT = FREQ_INIT / 4; //25%

/*
 * Limits for values
 */
static constexpr auto FREQ_MAX = 1000;
static constexpr auto DUTY_CYCLE_MAX = 1000;
static constexpr auto DEPHASE_MAX = 1000;

static constexpr auto FREQ_MIN = 0;
static constexpr auto DUTY_CYCLE_MIN = 0;
static constexpr auto DEPHASE_MIN = 0;

/*
 * Limits for cursor
 */

static constexpr auto COL_OFFSET = 4; //first digit is at 4,Y
static constexpr auto COL_LIM = 8; // 8th column is out of bounds

/*
 * Strings format for printf
 */

#define DA_PRINT "dA:<%04d>"
#define DB_PRINT "dB:<%04d>"
#define PH_PRINT "Ph:<%04d>"
#define FQ_PRINT "Fq:<%04d>"
#define DA_REF_PRINT "=%04.1f%%"
#define DB_REF_PRINT "=%04.1f%%"
#define PH_REF_PRINT "=%04.1f%%"
#define FQ_REF_PRINT "=%04dKHz"

#define TRUE 1
#define FALSE 0

InterruptIn knob( p14 );
DigitalIn decoderIn( p13 );

PwmDoubleOut waveB ( p23 );
PwmDoubleOut waveA ( p26 );

DigitalIn rowinc( p12 );
DigitalIn rowdec( p21 );

DigitalIn coldec( p22 );
DigitalIn colinc( p11 );



TextLCD lcd( p15, p16, p17, p18, p19, p20 , TextLCD::LCD20x4 );

std::atomic<uint32_t> freqKhz;
std::atomic<uint32_t> dutyCycleA;
std::atomic<uint32_t> dutyCycleB;
std::atomic<uint32_t> dephase;

/*
 * Cursor control
 */

std::atomic<uint32_t> row;
std::atomic<uint32_t> col;

/*
 * LCD print control
 */

std::atomic<uint8_t> flagModified;

/*
 * Unit of increment for each parameter.
 */

std::atomic<uint32_t> FQ_INC;
std::atomic<uint32_t> DA_INC;
std::atomic<uint32_t> DB_INC;
std::atomic<uint32_t> PH_INC;



void trigger() {
	int decoderMultiplier = 1;
	if ( decoderIn.read() == 0 ) {
		decoderMultiplier = -1;
	}
	switch ( row.load() ) {
	case 0:	{
		//DutyCycleA
		int32_t dA = dutyCycleA.load();
		dA += DA_INC.load() * decoderMultiplier;
		int32_t fq = freqKhz.load();
		if ( dA > fq ) {
			dA = fq;
		} else if ( dA < DUTY_CYCLE_MIN ) {
			dA = DUTY_CYCLE_MIN;
		}
		dutyCycleA.store( dA );
		waveA.set_duty_cycle( dA );
		break;
	}
	case 1:	{
		//DutyCycleB
		int32_t dB = dutyCycleB.load();
		dB += DB_INC.load() * decoderMultiplier;
		int32_t fq = freqKhz.load();
		if ( dB > fq ) {
			dB = fq;
		} else if ( dB < DUTY_CYCLE_MIN ) {
			dB = DUTY_CYCLE_MIN;
		}
		dutyCycleB.store( dB );
		waveB.set_duty_cycle( dB );
		break;
	}
	case 2:	{
		//dephase
		int32_t ph = dephase.load();
		ph += PH_INC.load() * decoderMultiplier;
		int32_t fq = freqKhz.load();
		if ( ph > fq ) {
			ph = fq;
		} else if ( ph < DEPHASE_MIN ) {
			ph = DEPHASE_MIN;
		}
		dephase.store( ph );
		waveB.set_dephase( ph );
		break;
	}
	case 3:	{
		//Freq
		int32_t fq = freqKhz.load();
		int32_t old_fq = fq;
		fq += FQ_INC.load() * decoderMultiplier;
		if ( fq > FREQ_MAX ) {
			fq = FREQ_MAX;
		} else if ( fq < FREQ_MIN ) {
			fq = FREQ_MIN;
		}
		freqKhz.store( fq );
		waveB.set_freq( fq );
		//rewrite dutyCycles to maintain consistency
		// uint32_t dA = dutyCycleA.load();
		// dA = dA * fq / old_fq;
		// dutyCycleA.store( dA );
		// uint32_t dB = dutyCycleB.load();
		// dB = dB * fq / old_fq;
		// dutyCycleA.store( dB );
		// uint32_t ph = dephase.load();
		// ph = ph * fq / old_fq;
		// dephase.store( ph );

		// waveA.set_duty_cycle( dA );
		// waveB.set_dephase( ph );
		// waveB.set_duty_cycle( dB );
		break;
	}
	}
	flagModified.store( TRUE );
	wait( 0.1f );
}

int main() {

	rowinc.mode( PullUp );
	rowdec.mode( PullUp );
	colinc.mode( PullUp );
	coldec.mode( PullUp );

	freqKhz.store( FREQ_INIT );
	dutyCycleA.store( DUTY_CYCLE_INIT );
	dutyCycleB.store( DUTY_CYCLE_INIT );
	dephase.store( DEPHASE_INIT );

	FQ_INC.store( 1 );
	DA_INC.store( 1 );
	DB_INC.store( 1 );
	PH_INC.store( 1 );

	waveB.set_freq( freqKhz.load() );
	waveA.set_duty_cycle( dutyCycleA.load() );
	waveB.set_duty_cycle( dutyCycleB.load() );
	waveB.set_dephase( dephase.load() );

	knob.rise( &trigger );
	lcd.setCursor( TRUE );
	uint32_t dA = dutyCycleA.load();
	uint32_t dB = dutyCycleB.load();
	uint32_t ph = dephase.load();
	uint32_t fq = freqKhz.load();
	lcd.printf(
	    DA_PRINT DA_REF_PRINT "\n" DB_PRINT DB_REF_PRINT "\n" PH_PRINT
	    PH_REF_PRINT "\n" FQ_PRINT FQ_REF_PRINT,
	    dA , 100 * ( ( float )dA / fq ),
	    dB , 100 * ( ( float )dB / fq ),
	    ph , 100 * ( ( float )ph / fq ),
	    fq , 96000 / fq );

	lcd.moveCursor( COL_OFFSET + 3, 0 );

	row.store( 0 );
	col.store( COL_OFFSET + 3 );
	/*
	 * Cursor horizontal position
	 */

	int rowpos [4] = {COL_OFFSET + 3, COL_OFFSET + 3, COL_OFFSET + 3, COL_OFFSET + 3};
	while ( 1 ) {
		if ( rowinc.read() == 0 ) {
			uint32_t temp = row.load();
			temp = ( temp + 1 ) % 4;
			row.store( temp );
			lcd.moveCursor( rowpos[temp], temp );
			wait( .5f );
		}
		if ( rowdec.read() == 0 ) {
			uint32_t temp = row.load();
			temp = ( temp - 1 ) % 4;
			row.store( temp );
			lcd.moveCursor( rowpos[temp], temp );
			wait( .5f );
		}
		if ( colinc.read() == 0 ) {
			uint32_t temp = rowpos[row.load()];
			if ( temp != COL_LIM - 1 ) {

				temp++;

				rowpos[row.load()] = temp;
				uint32_t rowTemp = row.load();
				lcd.moveCursor( temp, rowTemp );
				switch ( rowTemp ) {
				case 0: {
					uint32_t temp = DA_INC.load() / 10;
					DA_INC.store( temp );
					break;
				}

				case 1: {
					uint32_t temp = DB_INC.load() / 10;
					DB_INC.store( temp );
					break;
				}

				case 2: {
					uint32_t temp = PH_INC.load() / 10;
					PH_INC.store( temp );
					break;
				}
				case 3: {
					uint32_t temp = FQ_INC.load() / 10;
					FQ_INC.store( temp );
					break;
				}

				}
			}
			wait( .5f );
		}
		if ( coldec.read() == 0 ) {
			uint32_t temp = rowpos[row.load()];
			if ( temp != COL_OFFSET ) {

				temp--;
				rowpos[row.load()] = temp;
				uint32_t rowTemp = row.load();
				lcd.moveCursor( temp, rowTemp );
				switch ( rowTemp ) {
				case 0: {
					uint32_t temp = DA_INC.load() * 10;
					DA_INC.store( temp );
					break;
				}

				case 1: {
					uint32_t temp = DB_INC.load() * 10;
					DB_INC.store( temp );
					break;
				}

				case 2: {
					uint32_t temp = PH_INC.load() * 10;
					PH_INC.store( temp );
					break;
				}
				case 3: {
					uint32_t temp = FQ_INC.load() * 10;
					FQ_INC.store( temp );
					break;
				}

				}
			}
			wait( .5f );
		}
		if ( flagModified.load() ) {
			lcd.cls();
			uint32_t dA = dutyCycleA.load();
			uint32_t dB = dutyCycleB.load();
			uint32_t ph = dephase.load();
			uint32_t fq = freqKhz.load();
			lcd.printf(
			    DA_PRINT DA_REF_PRINT "\n" DB_PRINT DB_REF_PRINT "\n" PH_PRINT
			    PH_REF_PRINT "\n" FQ_PRINT FQ_REF_PRINT,
			    dA , 100 * ( ( float )dA / fq ),
			    dB , 100 * ( ( float )dB / fq ),
			    ph , 100 * ( ( float )ph / fq ),
			    fq , 96000 / fq );
			lcd.moveCursor( rowpos[row.load()], row.load() );
			flagModified.store( FALSE );
		}
	}
}
