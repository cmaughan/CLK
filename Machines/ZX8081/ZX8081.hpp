//
//  ZX8081.hpp
//  Clock Signal
//
//  Created by Thomas Harte on 04/06/2017.
//  Copyright © 2017 Thomas Harte. All rights reserved.
//

#ifndef ZX8081_hpp
#define ZX8081_hpp

#include "../../Configurable/Configurable.hpp"
#include "../ConfigurationTarget.hpp"
#include "../CRTMachine.hpp"
#include "../KeyboardMachine.hpp"

namespace ZX8081 {

/// @returns The options available for a ZX80 or ZX81.
std::vector<std::unique_ptr<Configurable::Option>> get_options();

class Machine:
	public CRTMachine::Machine,
	public ConfigurationTarget::Machine,
	public KeyboardMachine::Machine,
	public Configurable::Device {
	public:
		virtual ~Machine();

		static Machine *ZX8081(const StaticAnalyser::Target &target_hint);

		virtual void set_tape_is_playing(bool is_playing) = 0;
};

}

#endif /* ZX8081_hpp */
