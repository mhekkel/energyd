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

#include "data-service.hpp"
#include "sessy-service.hpp"
#include "https-client.hpp"

#include <mcfp/mcfp.hpp>

#include <zeep/json/parser.hpp>

#include <iostream>

// --------------------------------------------------------------------

std::unique_ptr<SessyService> SessyService::s_instance;

SessyService &SessyService::init(boost::asio::io_context &io_context)
{
	s_instance.reset(new SessyService(io_context));
	return *s_instance;
}

SessyService &SessyService::instance()
{
	if (not s_instance)
		throw std::logic_error("No instance yet!");

	return *s_instance;
}

SessyService::SessyService(boost::asio::io_context &io_context)
	: m_io_context(io_context)
{
	m_current = read();
	m_thread = std::thread(std::bind(&SessyService::run, this));
}

std::vector<SessySOC> SessyService::get_soc() const
{
	return read();
}

void SessyService::run()
{
	using namespace std::literals;

	using namespace date;
	using namespace std::chrono;

	using quarters = std::chrono::duration<int64_t, std::ratio<60 * 15>>;

	for (;;)
	{
		try
		{
			auto now = std::chrono::system_clock::now();
			std::this_thread::sleep_until(ceil<quarters>(now));

			m_current = read();

			for (auto &soc : m_current)
				DataService_v2::instance().store(soc);
		}
		catch (const std::exception &e)
		{
			std::cerr << e.what() << '\n';
		}
	}
}

std::vector<SessySOC> SessyService::read() const
{
	auto &config = mcfp::config::instance();

	std::vector<SessySOC> result;

	for (int sessy_nr = 1; sessy_nr <= 6; ++sessy_nr)
	{
		std::error_code ec;

		std::string url = config.get("sessy-" + std::to_string(sessy_nr), ec);

		if (ec)
			continue;

		auto rep = simple_request(url);

		if (rep.get_status() != zeep::http::ok)
		{
			std::cerr << "Failed to fetch sessy power status for sessy " << sessy_nr << '\n';
			continue;
		}

		zeep::json::element rep_j;
		zeep::json::parse_json(rep.get_content(), rep_j);

		SessySOC soc;
		from_element(rep_j, soc);
		soc.nr = sessy_nr;
		result.emplace_back(std::move(soc));
	}

	return result;
}
