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
PwmDoubleOut waveA ( p25 );

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
 * LCD print control bit1-DA | bit2-DB | bit3-PH | bit4-FQ
 */

std::atomic<uint8_t> flagModified;

/*
 * Unit of increment for each parameter.
 */

std::atomic<uint32_t> FQ_INC;
std::atomic<uint32_t> DA_INC;
std::atomic<uint32_t> DB_INC;
std::atomic<uint32_t> PH_INC;

/**
 * Functions that triggers via interrupt when the encoder is turned
 */

void trigger() {
	int decoderMultiplier = 1;
	//FInd out the way it is turning, so as to know wether to increment or decrement
	if ( decoderIn.read() == 0 ) {
		decoderMultiplier = -1;
	}
	//DIfferent case for each row (which represents each value)
	uint8_t flag = 0;
	switch ( row.load() ) {
	case 0:	{
		//DutyCycleA
		int32_t dA = dutyCycleA.load();
		dA += DA_INC.load() * decoderMultiplier;
		int32_t fq = freqKhz.load();
		//Make sure it stays inbound
		if ( dA > fq ) {
			dA = fq;
		} else if ( dA < DUTY_CYCLE_MIN ) {
			dA = DUTY_CYCLE_MIN;
		}
		dutyCycleA.store( dA );
		waveA.set_duty_cycle( dA );
		flag |= ( 1 << 0 );
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
		flag |= ( 1 << 1 );
		break;
	}
	case 2:	{
		//dephase
		int32_t ph = dephase.load();
		ph += PH_INC.load() * decoderMultiplier;
		int32_t fq = freqKhz.load();
		if ( ph >= fq ) {
			ph -= fq;
		} else if ( ph < DEPHASE_MIN ) {
			ph += fq;
		}
		dephase.store( ph );
		waveB.set_dephase( ph );
		flag |= ( 1 << 2 );
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
		//rewrite dutyCycles and dephase in order to maintain consistency
		uint32_t dA = dutyCycleA.load();
		if ( dA > fq ) {
			dutyCycleA.store( fq );
			dA = fq;
		}
		uint32_t dB = dutyCycleB.load();
		if ( dB > fq ) {
			dutyCycleB.store( fq );
			dB = fq;
		}
		uint32_t ph = dephase.load();
		if ( ph >= fq ) {
			dephase.store( 0 );
			ph = 0;
		}

		waveA.set_duty_cycle( dA );
		waveB.set_dephase( ph );
		waveB.set_duty_cycle( dB );
		flag |= 0x0F;
		break;
	}
	}
	//Debounce for encoder
	flagModified.store( flag );
	wait( 0.05f );
	while ( knob.read() != 0 );
	wait( 0.05f );
}

/**
 * Debounce function for the mechanical buttons
 */

void debounce( DigitalIn in ) {
	wait( 0.01f );
	while ( in.read() == 0 );
	wait( 0.01f );
}

int main() {
	//Initialize modified flag
	flagModified.store( FALSE );
	//Set buttons mode as pullUp
	rowinc.mode( PullUp );
	rowdec.mode( PullUp );
	colinc.mode( PullUp );
	coldec.mode( PullUp );
	//Initialize the atomic values
	freqKhz.store( FREQ_INIT );
	dutyCycleA.store( DUTY_CYCLE_INIT );
	dutyCycleB.store( DUTY_CYCLE_INIT );
	dephase.store( DEPHASE_INIT );
	//Initialize the increment values
	FQ_INC.store( 1 );
	DA_INC.store( 1 );
	DB_INC.store( 1 );
	PH_INC.store( 1 );
	//Initializing waves
	waveB.set_freq( freqKhz.load() );
	waveA.set_duty_cycle( dutyCycleA.load() );
	waveB.set_duty_cycle( dutyCycleB.load() );
	waveB.set_dephase( dephase.load() );
	//Setting up the interrupt on the encoder
	knob.rise( &trigger );
	//Seeting up the LCD
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
	//Initializing rows and collumns
	row.store( 0 );
	col.store( COL_OFFSET + 3 );
	//Setting cursor's horizontal position for each row
	int rowpos [4] = {COL_OFFSET + 3, COL_OFFSET + 3, COL_OFFSET + 3, COL_OFFSET + 3};
	/**
	 * Main loop. Check if any button is pushed in order to modify row/collumn
	 */
	while ( 1 ) {
		if ( rowinc.read() == 0 ) {
			uint32_t temp = row.load();
			temp = ( temp + 1 ) % 4;
			row.store( temp );
			lcd.moveCursor( rowpos[temp], temp );
			debounce( rowinc );
		}
		if ( rowdec.read() == 0 ) {
			uint32_t temp = row.load();
			temp = ( temp - 1 ) % 4;
			row.store( temp );
			lcd.moveCursor( rowpos[temp], temp );
			debounce( rowdec );
		}
		//When altering the collumn we are altering the INC variables
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
			debounce( colinc );
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
			debounce( coldec );
		}
		// If there was any modification to any wave, the LCD will be updated.
		uint8_t flag = flagModified.load();
		if ( flag ) {
			flagModified.store( FALSE );
			//Since everything is rewritten, the cls() might be unneccessary
			uint32_t fq = freqKhz.load();
			if ( flag & ( 1 << 0 ) ) {
				//PRINT DA
				lcd.locate( 0, 0 );
				uint32_t dA = dutyCycleA.load();
				lcd.printf( DA_PRINT DA_REF_PRINT, dA, 100 * ( ( float )dA / fq ) );
			}
			if ( flag & ( 1 << 1 ) ) {
				//PRINT DB
				uint32_t dB = dutyCycleB.load();
				lcd.locate( 0, 1 );
				lcd.printf( DB_PRINT DB_REF_PRINT, dB, 100 * ( ( float )dB / fq ) );
			}
			if ( flag & ( 1 << 2 ) ) {
				//PRINT PH
				uint32_t ph = dephase.load();
				lcd.locate( 0, 2 );
				lcd.printf( PH_PRINT PH_REF_PRINT, ph, 100 * ( ( float )ph / fq ) );
			}
			if ( flag & ( 1 << 3 ) ) {
				//PRINT FQ
				lcd.locate( 0, 3 );
				lcd.printf( FQ_PRINT FQ_REF_PRINT, fq, 96000 / fq );
			}
			//Move cursor back to original position
			lcd.moveCursor( rowpos[row.load()], row.load() );
		}
	}
}
