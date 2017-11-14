//
//  Keyboard.hpp
//  Clock Signal
//
//  Created by Thomas Harte on 10/10/2017.
//  Copyright © 2017 Thomas Harte. All rights reserved.
//

#ifndef Machines_Oric_Keyboard_hpp
#define Machines_Oric_Keyboard_hpp

#include "../KeyboardMachine.hpp"
#include "../Utility/Typer.hpp"

namespace Oric {

enum Key: uint16_t {
	Key3			= 0x0000 | 0x80,	KeyX			= 0x0000 | 0x40,	Key1			= 0x0000 | 0x20,
	KeyV			= 0x0000 | 0x08,	Key5			= 0x0000 | 0x04,	KeyN			= 0x0000 | 0x02,	Key7			= 0x0000 | 0x01,
	KeyD			= 0x0100 | 0x80,	KeyQ			= 0x0100 | 0x40,	KeyEscape		= 0x0100 | 0x20,
	KeyF			= 0x0100 | 0x08,	KeyR			= 0x0100 | 0x04,	KeyT			= 0x0100 | 0x02,	KeyJ			= 0x0100 | 0x01,
	KeyC			= 0x0200 | 0x80,	Key2			= 0x0200 | 0x40,	KeyZ			= 0x0200 | 0x20,	KeyControl		= 0x0200 | 0x10,
	Key4			= 0x0200 | 0x08,	KeyB			= 0x0200 | 0x04,	Key6			= 0x0200 | 0x02,	KeyM			= 0x0200 | 0x01,
	KeyQuote		= 0x0300 | 0x80,	KeyBackSlash	= 0x0300 | 0x40,
	KeyMinus		= 0x0300 | 0x08,	KeySemiColon	= 0x0300 | 0x04,	Key9			= 0x0300 | 0x02,	KeyK			= 0x0300 | 0x01,
	KeyRight		= 0x0400 | 0x80,	KeyDown			= 0x0400 | 0x40,	KeyLeft			= 0x0400 | 0x20,	KeyLeftShift	= 0x0400 | 0x10,
	KeyUp			= 0x0400 | 0x08,	KeyFullStop		= 0x0400 | 0x04,	KeyComma		= 0x0400 | 0x02,	KeySpace		= 0x0400 | 0x01,
	KeyOpenSquare	= 0x0500 | 0x80,	KeyCloseSquare	= 0x0500 | 0x40,	KeyDelete		= 0x0500 | 0x20,	KeyFunction		= 0x0500 | 0x10,
	KeyP			= 0x0500 | 0x08,	KeyO			= 0x0500 | 0x04,	KeyI			= 0x0500 | 0x02,	KeyU			= 0x0500 | 0x01,
	KeyW			= 0x0600 | 0x80,	KeyS			= 0x0600 | 0x40,	KeyA			= 0x0600 | 0x20,
	KeyE			= 0x0600 | 0x08,	KeyG			= 0x0600 | 0x04,	KeyH			= 0x0600 | 0x02,	KeyY			= 0x0600 | 0x01,
	KeyEquals		= 0x0700 | 0x80,										KeyReturn		= 0x0700 | 0x20,	KeyRightShift	= 0x0700 | 0x10,
	KeyForwardSlash	= 0x0700 | 0x08,	Key0			= 0x0700 | 0x04,	KeyL			= 0x0700 | 0x02,	Key8			= 0x0700 | 0x01,

	KeyNMI			= 0xfffd,
};

struct KeyboardMapper: public KeyboardMachine::Machine::KeyboardMapper {
	uint16_t mapped_key_for_key(Inputs::Keyboard::Key key);
};

struct CharacterMapper: public ::Utility::CharacterMapper {
	uint16_t *sequence_for_character(char character);
};

};

#endif /* KeyboardMapper_hpp */