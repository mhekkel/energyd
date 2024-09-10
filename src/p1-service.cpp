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

#include "p1-service.hpp"

#include <mcfp/mcfp.hpp>

#include <zeep/value-serializer.hpp>

#include <date/date.h>

#include <iostream>
#include <regex>

std::unique_ptr<P1Service> P1Service::s_instance;

P1Service &P1Service::init(boost::asio::io_context &io_context)
{
	s_instance.reset(new P1Service(io_context));
	return *s_instance;
}

P1Service &P1Service::instance()
{
	if (not s_instance)
		throw std::logic_error("No instance yet!");

	return *s_instance;
}

P1Service::P1Service(boost::asio::io_context &io_context)
	: m_io_context(io_context)
{
	auto &config = mcfp::config::instance();

	m_device_string = config.get("p1-device");

	if (std::filesystem::exists(m_device_string))
	{
		std::tie(m_opname, std::ignore) = read();
		m_thread = std::thread(std::bind(&P1Service::run, this));
	}
}

void P1Service::run()
{
	using namespace std::literals;

	using namespace date;
	using namespace std::chrono;

	using quarters = std::chrono::duration<int64_t, std::ratio<60 * 15>>;

	auto now = std::chrono::system_clock::now();
	auto next = ceil<quarters>(now);

	for (;;)
	{
		try
		{
			auto [opname, status] = read();

			{
				std::unique_lock lock(m_mutex);
				m_opname = opname;
				m_status = status;
			}

			now = std::chrono::system_clock::now();
			if (now < next)
				continue;

			DataService_v2::instance().store(m_opname);

			next = ceil<quarters>(now);
		}
		catch (const std::exception &e)
		{
			std::cerr << e.what() << '\n';
		}
	}
}

const std::regex
	kReadRX(R"(^1-0:(1|2)\.(7|8)\.(0|1|2)\((\d{2,6})\.(\d{3})\*kWh?\))", std::regex::multiline);


// totals:
// 1-0:1.8.1(009936.986*kWh)
// 1-0:1.8.2(008101.080*kWh)
// 1-0:2.8.1(003108.322*kWh)
// 1-0:2.8.2(007566.688*kWh)

// current
// 1-0:1.7.0(00.184*kW)
// 1-0:2.7.0(00.169*kW)

P1Opname P1Service::get_current() const
{
	std::unique_lock lock(m_mutex);
	return m_opname;
}

P1Status P1Service::get_status() const
{
	std::unique_lock lock(m_mutex);
	return m_status;
}

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

std::tuple<P1Opname, P1Status> P1Service::read() const
{
	P1Opname opname{};
	P1Status status{};

	boost::asio::serial_port p1(m_io_context.get_executor());

	boost::system::error_code ec;
	p1.open(m_device_string, ec);
	if (ec)
		throw std::system_error(ec, "Error opening serial device");

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

	uint16_t crc= 0;
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
							crc = 0;
							break;
						}

						auto begin = std::sregex_iterator(message.begin(), message.end(), kReadRX);
						auto end = std::sregex_iterator();

						for (std::sregex_iterator i = begin; i != end; ++i)
						{
							std::smatch m = *i;

							double v = stod(m[4]) + stod(m[5]) / 1000.0;

							if (m[2] == "7")
							{
								if (m[1] == "1")
									status.power_consumed = v;
								else if (m[1] == "2")
									status.power_produced = v;
							}
							else if (m[2] == "8")
							{
								if (m[1] == "1")
								{
									if (m[3] == "2")
										opname.verbruik_hoog = v;
									else
										opname.verbruik_laag = v;
								}
								else if (m[1] == "2")
								{
									if (m[3] == "2")
										opname.levering_hoog = v;
									else
										opname.levering_laag = v;
								}
							}
						}

						state = DONE;
						crc = 0;
					}
					break;

				default:
					break;
			}
		}
	}

	return { opname, status };
}
