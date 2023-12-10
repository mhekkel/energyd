/*-
 * SPDX-License-Identifier: BSD-2-Clause
 * 
 * Copyright (c) 2023 Maarten L. Hekkelman
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "p1-reader.hpp"

#include <iostream>
#include <regex>

// std::unique_ptr<P1_Lezer> P1_Lezer::s_instance;

// P1_Lezer &P1_Lezer::instance()
// {
// 	if (not s_instance)
// 		s_instance.reset(new P1_Lezer);
// 	return *s_instance;
// }

// P1_Lezer::P1_Lezer()
// {

// }

const std::regex kReadRX(R"(^1-0:([12])\.8\.([12])\((\d{6})\.(\d{3})\*kWh\))", std::regex::multiline);

inline uint16_t update_crc(uint16_t crc, char ch)
{
	const uint16_t polynomial = 0xa001;
	const uint8_t byte = ch;

	uint16_t result = crc ^ byte;

	for (uint8_t bit = 8; bit > 0; --bit)
	{
		if (result & 1)
			result = (result >> 1) ^ polynomial;
		else
			result = (result >> 1);
	}

	return result;
}



P1_Waarden lees_p1(boost::asio::io_context &io_context)
{
	P1_Waarden result{};

	boost::asio::serial_port p1(io_context.get_executor());

	boost::system::error_code ec;
	p1.open("/dev/ttyUSB0", ec);
	if (ec)
		std::cerr << "Opening failed: " << ec.message() << '\n';

	p1.set_option(boost::asio::serial_port::baud_rate(115200), ec);
	if (ec)
		std::cerr << "Error setting baud rate: " << ec.message() << '\n';

	enum
	{
		START,
		HEADER,
		IDENT0,
		IDENT1,
		IDENT2,
		IDENT3,
		IDENT4,
		DATA = 100,
		CHECKSUM,
		DONE
	} state = START;

	uint16_t crc, check_crc;
	std::string header, ident, message, crc_s, datagram;

	while (state != DONE)
	{
		boost::asio::streambuf data;
		boost::asio::streambuf::mutable_buffers_type bufs = data.prepare(512);

		auto n = p1.read_some(bufs, ec);

		if (n == 0 or ec)
			break;
		
		data.commit(n);

		while (data.in_avail() > 0)
		{
			int ch_i = data.sbumpc();
			if (ch_i == std::char_traits<char>::eof())
				break;
			
			char ch = std::char_traits<char>::to_char_type(ch_i);

			if (state != CHECKSUM)
				crc = update_crc(crc, ch);

			datagram += ch;

			switch (state)
			{
				case START:
					if (ch == '/')
					{
						state = HEADER;
						crc = update_crc(0, ch);
						header = { ch };
						datagram = { ch };
					}
					break;
				
				case HEADER:
					header += ch;

					if (header.length() >= 4)
					{
						if (ch == '5')
						{
							state = IDENT0;
							ident.clear();
						}
					}
					break;
				
				case IDENT0:
					if (ch == '\r')
						state = IDENT1;
					else
						ident += ch;
					break;
				
				case IDENT1:
					if (ch == '\n')
						state = IDENT2;
					else
						state = START;
					break;

				case IDENT2:
					if (ch == '\r')
						state = IDENT3;
					else
						state = START;
					break;

				case IDENT3:
					if (ch == '\n')
					{
						state = DATA;
						message.clear();
					}
					else
						state = START;
					break;

				case DATA:
					if (ch == '!')
					{
						state = CHECKSUM;
						crc_s.clear();
					}
					else
						message += ch;
					break;
				
				case CHECKSUM:
					crc_s += ch;
					if (crc_s.length() == 6)
					{
						if (crc_s[4] != '\r' or crc_s[5] != '\n')
						{
							std::cerr << "Unexpected end of message\n";
							state = START;
							break;
						}

						auto test = std::stol(crc_s, nullptr, 16);

						if (test != crc)
						{
							std::cerr << "CRC did not match\n";
							state = START;
							break;
						}
						
						auto begin = std::sregex_iterator(message.begin(), message.end(), kReadRX);
						auto end = std::sregex_iterator();
						
						for (std::sregex_iterator i = begin; i != end; ++i)
						{
							std::smatch m = *i;
							
							double v = stod(m[3]) + stod(m[4]) / 1000.0;

							if (m[1] == "1")
								if (m[2] == "2")
									result.verbruik_hoog = v;
								else
									result.verbruik_laag = v;
							else if (m[1] == "2")
								if (m[2] == "2")
									result.levering_hoog = v;
								else
									result.levering_laag = v;
						}
				
						state = DONE;
					}
					break;
			}
		}
	}

	return result;
}
