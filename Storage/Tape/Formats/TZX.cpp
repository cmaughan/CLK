//
//  TZX.cpp
//  Clock Signal
//
//  Created by Thomas Harte on 16/07/2017.
//  Copyright © 2017 Thomas Harte. All rights reserved.
//

#include "TZX.hpp"

using namespace Storage::Tape;

namespace {
const unsigned int StandardTZXClock = 3500000;
const unsigned int TZXClockMSMultiplier = 3500;
}

TZX::TZX(const char *file_name) :
	file_(file_name),
	current_level_(false) {

	// Check for signature followed by a 0x1a
	if(!file_.check_signature("ZXTape!")) throw ErrorNotTZX;
	if(file_.get8() != 0x1a) throw ErrorNotTZX;

	// Get version number
	uint8_t major_version = file_.get8();
	uint8_t minor_version = file_.get8();

	// Reject if an incompatible version
	if(major_version != 1 || minor_version > 20)  throw ErrorNotTZX;

	virtual_reset();
}

void TZX::virtual_reset() {
	clear();
	set_is_at_end(false);
	file_.seek(0x0a, SEEK_SET);

	// This is a workaround for arguably dodgy ZX80/ZX81 TZXs; they launch straight
	// into data but both machines require a gap before data begins. So impose
	// an initial gap, in the form of a very long wave.
	current_level_ = false;
	post_gap(500);
}

void TZX::get_next_pulses() {
	while(empty()) {
		uint8_t chunk_id = file_.get8();
		if(file_.eof()) {
			set_is_at_end(true);
			return;
		}

//		printf("TZX %ld\n", ftell(file_));
		switch(chunk_id) {
			case 0x10:	get_standard_speed_data_block();	break;
			case 0x11:	get_turbo_speed_data_block();		break;
			case 0x12:	get_pure_tone_data_block();			break;
			case 0x13:	get_pulse_sequence();				break;
			case 0x14:	get_pure_data_block();				break;
			case 0x19:	get_generalised_data_block();		break;
			case 0x20:	get_pause();						break;

			case 0x21:	ignore_group_start();				break;
			case 0x22:	ignore_group_end();					break;
			case 0x23:	ignore_jump_to_block();				break;
			case 0x24:	ignore_loop_start();				break;
			case 0x25:	ignore_loop_end();					break;
			case 0x26:	ignore_call_sequence();				break;
			case 0x27:	ignore_return_from_sequence();		break;
			case 0x28:	ignore_select_block();				break;

			case 0x30:	ignore_text_description();			break;
			case 0x31:	ignore_message_block();				break;
			case 0x33:	get_hardware_type();				break;

			default:
				// In TZX each chunk has a different way of stating or implying its length,
				// so there is no route past an unimplemented chunk.
				printf("!!Unknown %02x!!", chunk_id);
				set_is_at_end(true);
			return;
		}
	}
}

void TZX::get_generalised_data_block() {
	uint32_t block_length = file_.get32le();
	long endpoint = file_.tell() + static_cast<long>(block_length);
	uint16_t pause_after_block = file_.get16le();

	uint32_t total_pilot_symbols = file_.get32le();
	uint8_t maximum_pulses_per_pilot_symbol = file_.get8();
	uint8_t symbols_in_pilot_table = file_.get8();

	uint32_t total_data_symbols = file_.get32le();
	uint8_t maximum_pulses_per_data_symbol = file_.get8();
	uint8_t symbols_in_data_table = file_.get8();

	get_generalised_segment(total_pilot_symbols, maximum_pulses_per_pilot_symbol, symbols_in_pilot_table, false);
	get_generalised_segment(total_data_symbols, maximum_pulses_per_data_symbol, symbols_in_data_table, true);
	post_gap(pause_after_block);

	// This should be unnecessary, but intends to preserve sanity.
	file_.seek(endpoint, SEEK_SET);
}

void TZX::get_generalised_segment(uint32_t output_symbols, uint8_t max_pulses_per_symbol, uint8_t number_of_symbols, bool is_data) {
	if(!output_symbols) return;

	// Construct the symbol table.
	struct Symbol {
		uint8_t flags;
		std::vector<uint16_t> pulse_lengths;
	};
	std::vector<Symbol> symbol_table;
	for(int c = 0; c < number_of_symbols; c++) {
		Symbol symbol;
		symbol.flags = file_.get8();
		for(int ic = 0; ic < max_pulses_per_symbol; ic++) {
			symbol.pulse_lengths.push_back(file_.get16le());
		}
		symbol_table.push_back(symbol);
	}

	// Hence produce the output.
	FileHolder::BitStream stream = file_.get_bitstream(false);
	int base = 2;
	int bits = 1;
	while(base < number_of_symbols) {
		base <<= 1;
		bits++;
	}
	for(std::size_t c = 0; c < output_symbols; c++) {
		uint8_t symbol_value;
		int count;
		if(is_data) {
			symbol_value = stream.get_bits(bits);
			count = 1;
		} else {
			symbol_value = file_.get8();
			count = file_.get16le();
		}
		if(symbol_value > number_of_symbols) {
			continue;
		}
		Symbol &symbol = symbol_table[symbol_value];

		while(count--) {
			// Mutate initial output level.
			switch(symbol.flags & 3) {
				case 0: break;
				case 1: current_level_ ^= true;	break;
				case 2: current_level_ = true;	break;
				case 3: current_level_ = false;	break;
			}

			// Output waves.
			for(auto length : symbol.pulse_lengths) {
				if(!length) break;
				post_pulse(length);
			}
		}
	}
}

void TZX::get_standard_speed_data_block() {
	DataBlock data_block;
	data_block.length_of_pilot_pulse = 2168;
	data_block.length_of_sync_first_pulse = 667;
	data_block.length_of_sync_second_pulse = 735;
	data_block.data.length_of_zero_bit_pulse = 855;
	data_block.data.length_of_one_bit_pulse = 1710;
	data_block.data.number_of_bits_in_final_byte = 8;

	data_block.data.pause_after_block = file_.get16le();
	data_block.data.data_length = file_.get16le();
	if(!data_block.data.data_length) return;

	uint8_t first_byte = file_.get8();
	data_block.length_of_pilot_tone = (first_byte < 128) ? 8063  : 3223;
	file_.seek(-1, SEEK_CUR);

	get_data_block(data_block);
}

void TZX::get_turbo_speed_data_block() {
	DataBlock data_block;
	data_block.length_of_pilot_pulse = file_.get16le();
	data_block.length_of_sync_first_pulse = file_.get16le();
	data_block.length_of_sync_second_pulse = file_.get16le();
	data_block.data.length_of_zero_bit_pulse = file_.get16le();
	data_block.data.length_of_one_bit_pulse = file_.get16le();
	data_block.length_of_pilot_tone = file_.get16le();
	data_block.data.number_of_bits_in_final_byte = file_.get8();
	data_block.data.pause_after_block = file_.get16le();
	data_block.data.data_length = file_.get24le();

	get_data_block(data_block);
}

void TZX::get_data_block(const DataBlock &data_block) {
	// Output pilot tone.
	for(unsigned int c = 0; c < data_block.length_of_pilot_tone; c++) {
		post_pulse(data_block.length_of_pilot_pulse);
	}

	// Output sync pulses.
	post_pulse(data_block.length_of_sync_first_pulse);
	post_pulse(data_block.length_of_sync_second_pulse);

	get_data(data_block.data);
}

void TZX::get_data(const Data &data) {
	// Output data.
	for(unsigned int c = 0; c < data.data_length; c++) {
		uint8_t next_byte = file_.get8();

		unsigned int bits = (c != data.data_length-1) ? 8 : data.number_of_bits_in_final_byte;
		while(bits--) {
			unsigned int pulse_length = (next_byte & 0x80) ? data.length_of_one_bit_pulse : data.length_of_zero_bit_pulse;
			next_byte <<= 1;

			post_pulse(pulse_length);
			post_pulse(pulse_length);
		}
	}

	// Output gap.
	post_gap(data.pause_after_block);
}

void TZX::get_pure_tone_data_block() {
	uint16_t length_of_pulse = file_.get16le();
	uint16_t nunber_of_pulses = file_.get16le();

	while(nunber_of_pulses--) post_pulse(length_of_pulse);
}

void TZX::get_pure_data_block() {
	Data data;
	data.length_of_zero_bit_pulse = file_.get16le();
	data.length_of_one_bit_pulse = file_.get16le();
	data.number_of_bits_in_final_byte = file_.get8();
	data.pause_after_block = file_.get16le();
	data.data_length = file_.get24le();

	get_data(data);
}

void TZX::get_pulse_sequence() {
	uint8_t number_of_pulses = file_.get8();
	while(number_of_pulses--) {
		post_pulse(file_.get16le());
	}
}

void TZX::get_pause() {
	uint16_t duration = file_.get16le();
	if(!duration) {
		// TODO (maybe): post a 'pause the tape' suggestion
	} else {
		post_gap(duration);
	}
}

// MARK: - Output

void TZX::post_pulse(unsigned int length) {
	post_pulse(Storage::Time(length, StandardTZXClock));
}

void TZX::post_gap(unsigned int milliseconds) {
	if(!milliseconds) return;
	if(milliseconds > 1 && !current_level_) {
		post_pulse(Storage::Time(TZXClockMSMultiplier, StandardTZXClock));
		post_pulse(Storage::Time((milliseconds - 1u)*TZXClockMSMultiplier, StandardTZXClock));
	} else {
		post_pulse(Storage::Time(milliseconds*TZXClockMSMultiplier, StandardTZXClock));
	}
}

void TZX::post_pulse(const Storage::Time &time) {
	emplace_back(current_level_ ? Tape::Pulse::High : Tape::Pulse::Low, time);
	current_level_ ^= true;
}

// MARK: - Flow control; currently ignored

void TZX::ignore_group_start() {
	printf("Ignoring TZX group\n");
	uint8_t length = file_.get8();
	file_.seek(length, SEEK_CUR);
}

void TZX::ignore_group_end() {
}

void TZX::ignore_jump_to_block() {
	uint16_t target = file_.get16le();
	(void)target;
	printf("Ignoring TZX jump\n");
}

void TZX::ignore_loop_start() {
	uint16_t number_of_repetitions = file_.get16le();
	(void)number_of_repetitions;
	printf("Ignoring TZX loop\n");
}

void TZX::ignore_loop_end() {
}

void TZX::ignore_call_sequence() {
	uint16_t number_of_entries = file_.get16le();
	file_.seek(number_of_entries * sizeof(uint16_t), SEEK_CUR);
	printf("Ignoring TZX call sequence\n");
}

void TZX::ignore_return_from_sequence() {
	printf("Ignoring TZX return from sequence\n");
}

void TZX::ignore_select_block() {
	uint16_t length_of_block = file_.get16le();
	file_.seek(length_of_block, SEEK_CUR);
	printf("Ignoring TZX select block\n");
}

// MARK: - Messaging

void TZX::ignore_text_description() {
	uint8_t length = file_.get8();
	file_.seek(length, SEEK_CUR);
	printf("Ignoring TZX text description\n");
}

void TZX::ignore_message_block() {
	uint8_t time_for_display = file_.get8();
	uint8_t length = file_.get8();
	file_.seek(length, SEEK_CUR);
	(void)time_for_display;
	printf("Ignoring TZX message\n");
}

void TZX::get_hardware_type() {
	// TODO: pick a way to retain and communicate this.
	uint8_t number_of_machines = file_.get8();
	file_.seek(number_of_machines * 3, SEEK_CUR);
	printf("Ignoring TZX hardware types (%d)\n", number_of_machines);
}
