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
 * Duty Cycle will be stored in int and will only be evaluated as float when necessary.
 * This facilitates atomic operation and speeds up the process
 */
static constexpr auto DUTY_CYCLE_PRECISION = 1000;
static constexpr auto DEPHASE_PRECISION = 1000;
static constexpr auto DUTY_CYCLE_MULT = 1.0f / DUTY_CYCLE_PRECISION;
static constexpr auto DEPHASE_MULT = 1.0f / DEPHASE_PRECISION;

/*
 * Initial value that will be loaded to each parameter
 */

static constexpr auto FREQ_INIT = 1000; // 1MHz
static constexpr auto DUTY_CYCLE_INIT = DUTY_CYCLE_PRECISION / 2; //50%
static constexpr auto DEPHASE_INIT = DEPHASE_PRECISION / 4; //25%


/*
 * Limits for cursor
 */

static constexpr auto COL_OFFSET = 4; //first digit is at 4,Y
static constexpr auto COL_LIM = 8; // 8th column is out of bounds

/*
 * Strings format for printf
 */

#define DA_PRINT "dA:<%04.1f>%%"
#define DB_PRINT "db:<%04.1f>%%"
#define PH_PRINT "Ph:<%04.1f>%%"
#define FQ_PRINT "Fq:<%04d>KHz"

#define TRUE 1
#define FALSE 0

InterruptIn knob( p13 );
DigitalIn decoderIn( p14 );

PwmDoubleOut waveB ( p23 );
PwmOut waveA ( p26 );

DigitalIn rowinc( p21 );
DigitalIn rowdec( p22 );

DigitalIn coldec( p11 );
DigitalIn colinc( p12 );



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
		dutyCycleA.fetch_add( DA_INC.load() * decoderMultiplier );
		waveA.write( dutyCycleA.load() * DUTY_CYCLE_MULT );
		break;
	}
	case 1:	{
		//DutyCycleB
		dutyCycleB.fetch_add( DB_INC.load() * decoderMultiplier );
		waveB.write( dutyCycleB.load() * DUTY_CYCLE_MULT );
		break;
	}
	case 2:	{
		//dephase
		dephase.fetch_add( PH_INC.load() * decoderMultiplier );
		waveB.dephase( dephase.load() *DEPHASE_MULT );
		break;
	}
	case 3:	{
		//Freq
		freqKhz.fetch_add( FQ_INC.load() * decoderMultiplier );
		waveB.freq_khz( freqKhz.load() );
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

	waveB.freq_khz( freqKhz.load() );
	waveA.write( dutyCycleA.load() * DUTY_CYCLE_MULT );
	waveB.write( dutyCycleB.load() * DUTY_CYCLE_MULT );
	waveB.dephase( dephase.load() * DEPHASE_MULT );

	knob.rise( &trigger );
	lcd.setCursor( TRUE );

	lcd.printf( DA_PRINT"\n"DB_PRINT"\n"PH_PRINT"\n"FQ_PRINT,
	            dutyCycleA.load() * DUTY_CYCLE_MULT * 100 ,
	            dutyCycleB.load() * DUTY_CYCLE_MULT * 100 ,
	            dephase.load()*DEPHASE_MULT * 100 ,
	            freqKhz.load() );

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
				if ( row.load() != 3 && temp == COL_OFFSET + 1 ) {
					temp = COL_OFFSET + 3;
				} else {
					temp++;
				}
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
				if ( row.load() != 3 && temp == COL_OFFSET + 3 ) {
					temp = COL_OFFSET + 1;
				} else {
					temp--;
				}
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
			lcd.printf( DA_PRINT"\n"DB_PRINT"\n"PH_PRINT"\n"FQ_PRINT,
			            dutyCycleA.load() * DUTY_CYCLE_MULT * 100 ,
			            dutyCycleB.load() * DUTY_CYCLE_MULT * 100 ,
			            dephase.load()*DEPHASE_MULT * 100 ,
			            freqKhz.load() );
			lcd.moveCursor( rowpos[row.load()], row.load() );
			flagModified.store( FALSE );
		}
	}
}
