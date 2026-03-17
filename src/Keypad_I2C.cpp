/*
||
|| @file Keypad_I2C.h
|| @version 2.0 - PCF8575 support added by Paul Williamson
|| @author G. D. (Joe) Young, ptw
|| @contact "G. D. (Joe) Young" <jyoung@islandnet.com>
||
|| @description
|| | Keypad_I2C provides an interface for using matrix keypads that
|| | are attached with I2C port expanders. It supports multiple keypads,
|| | user selectable pins, and user defined keymaps.
|| #
||
|| @license
|| | This library is free software; you can redistribute it and/or
|| | modify it under the terms of the GNU Lesser General Public
|| | License as published by the Free Software Foundation; version
|| | 2.1 of the License.
|| |
|| | This library is distributed in the hope that it will be useful,
|| | but WITHOUT ANY WARRANTY; without even the implied warranty of
|| | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
|| | Lesser General Public License for more details.
|| |
|| | You should have received a copy of the GNU Lesser General Public
|| | License along with this library; if not, write to the Free Software
|| | Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
|| #
||
*/

#include "Keypad_I2C.h"

// Let the user define a keymap - assume the same row/column count as defined in constructor
void Keypad_I2C::begin(char *userKeymap) {
    Keypad::begin(userKeymap);
	Wire.begin();
	pinState = pinState_set( );
}


// Initialize I2C
void Keypad_I2C::begin(void) {
	//Wire.begin();
//	pinState = 0xff;
	pinState = pinState_set( );
}

// Initialize I2C
void Keypad_I2C::begin(byte address) {
	i2caddr = address;
	Wire.begin(address);
//	pinState = 0xff;
	pinState = pinState_set( );
}

// Initialize I2C
void Keypad_I2C::begin(int address) {
	i2caddr = address;
	Wire.begin(address);
//	pinState = 0xff;
	pinState = pinState_set( );
}


void Keypad_I2C::pin_write(byte pinNum, boolean level) {
	word mask = 1<<pinNum;
	if( level == HIGH ) {
		pinState |= mask;
	} else {
		pinState &= ~mask;
	}
	port_write( pinState );
} // I2CxWrite( )


int Keypad_I2C::pin_read(byte pinNum) {
	word mask = 0x1<<pinNum;
	Wire.requestFrom((int)i2caddr, (int)i2cwidth);
	word pinVal = Wire.read( );
	if (i2cwidth > 1) {
		pinVal |= Wire.read( ) << 8;
	} 
	pinVal &= mask;
	if( pinVal == mask ) {
		return 1;
	} else {
		return 0;
	}
}

void Keypad_I2C::port_write( word i2cportval ) {
	Wire.beginTransmission((int)i2caddr);
	Wire.write( i2cportval & 0x00FF);
	if (i2cwidth > 1) {
		Wire.write( i2cportval >> 8 );
	}
	Wire.endTransmission();
	pinState = i2cportval;
} // port_write( )

word Keypad_I2C::pinState_set( ) {
	Wire.requestFrom( (int)i2caddr, (int)i2cwidth );
	pinState = Wire.read( );
	if (i2cwidth > 1) {
		pinState |= Wire.read( ) << 8;
	}
	return pinState;
} // set_pinState( )


/*
|| @changelog
|| |
|| | 2.0 2013-08-31 - Paul Williamson : Added i2cwidth parameter for PCF8575 support
|| |
|| | 1.0 2012-07-12 - Joe Young : Initial Release
|| #
*/
