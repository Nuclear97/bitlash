//////////////////////////////////////////////////////////////////
//
//	radio_virtuawire.cpp:	Virtual Wire Radio Interface for Bitlash
//
//	Copyright (C) 2011 by Bill Roy
//
//	This library is free software; you can redistribute it and/or
//	modify it under the terms of the GNU Lesser General Public
//	License as published by the Free Software Foundation; either
//	version 2.1 of the License, or (at your option) any later version.
//
//	This library is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//	Lesser General Public License for more details.
//
//	You should have received a copy of the GNU Lesser General Public
//	License along with this library; if not, write to the Free Software
//	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
//////////////////////////////////////////////////////////////////
//

#include "WProgram.h"
#include "bitlash.h"
#include "../../libraries/bitlash/src/bitlash.h"
#include "parfait.h"
#include "pkt.h"

#if defined(RADIO_VIRTUALWIRE)
#include "VirtualWire.h"


////////////////////////////////////
// Turn this on to enable debug spew
#define RADIO_DEBUG


// Radio Layer User Functions - stubbed out
numvar func_rfget(void) 	{ return 0; }
numvar func_rfset(void) 	{ return 0; }
numvar func_setfreq(void) 	{ return 0; }
numvar func_degf(void) 		{ return 0; }



/////////////////////////////////////
// 
// Initialize the radio interface
//
void init_radio(void) {
	vw_set_ptt_pin(10);		// defaults to 10
	vw_set_rx_pin(11);		// defaults to 11
	vw_set_tx_pin(12);		// defaults to 12
	vw_setup(2000);			// set up for 2000 bps
	vw_rx_start();			// start the rx
}


////////////////////////////////////////
//
// Address handling
//
//	No address handling for VirtualWire
//	These stubs are required for the API
//
void rf_set_rx_address(char *my_address) {;}
void rf_set_tx_address(char *to_address) {;}

////////////////////////////////////////
//
//	Return TRUE when the radio has a packet for us
//
byte rx_pkt_ready(void) {
	return vw_have_message();
}


// declare the packet logging function
void log_packet(char tag, pkt_t *pkt, byte length);


//////////////////////
// rx_fetch_pkt: read packet into buffer if available
//
//	returns number of bytes transferred to pkt, 0 if no data available
//
byte rx_fetch_pkt(pkt_t *pkt) {

	// Does the radio have business for us?
	if (!vw_have_message()) return 0;

	//	"If a message is available (good checksum or not), copies up to *len octets to buf. 
	//	Returns true if there was a message and the checksum was good."
	byte length = RF_PACKET_SIZE;
	if (!vw_get_message((uint8_t *) pkt, &length)) return 0;
	
	// If the packet length is bigger than our buffer We Have A Situation.
	// Clip to our max packet size, silently discarding the excess.
	//
	if (length > RF_PACKET_SIZE) {
		// todo: log here
		length = RF_PACKET_SIZE;
	}
	rx_packet_count++;						// got one? count it

#if defined(RADIO_DEBUG)
	log_packet('R', pkt, length);
#endif

	return length;
}


//////////////////////
//
// tx_send_packet: transmit a data packet
//
//	length: length of raw packet including header
//	pkt:	pointer to packet
//
void tx_send_pkt(pkt_t *pkt, uint8_t length) {

#if defined(RADIO_DEBUG)
	log_packet('T', pkt, length);
#endif

	vw_wait_tx();	// wait for tx idle
	vw_send((uint8_t *) pkt, length);		// todo: handle error here
	tx_packet_count++;
}




//////////
// rf_log_packet: Hex dump the packet to the console
//
byte rf_logbytes;

// a function handler to expose the control
numvar func_rflog(void) { rf_logbytes = getarg(1); }

void lpb(byte b) {
	if (b == '\\') {
		spb('\\');
		spb('\\');
	}
	else if ((b >= ' ') && (b <= 0x7f)) spb(b);
	else {
		spb('\\');
		if (b == 0xd) spb('r');
		else if (b == 0xa) spb('n');
		else {
			spb('x');
			if (b < 0x10) spb('0');
			spb(b);
		}
	}
}

// todo: rewrite as printf()

void log_packet(char tag, pkt_t *pkt, byte length) {
	if (rf_logbytes) {
		int i = 0;
		int last = (length < rf_logbytes) ? length : rf_logbytes;
		spb('[');
		spb(tag); 
		sp("X ");
		printInteger(length, 0); spb(' ');
		printInteger(pkt->type, 0); spb(' ');
		printInteger(pkt->sequence, 0); spb(' ');
		while (i < last-RF_PACKET_HEADER_SIZE) {
			lpb(pkt->data[i++]);
		}
		spb(']');
		speol();
	}
}


#endif	// defined(RADIO_VIRTUALWIRE)