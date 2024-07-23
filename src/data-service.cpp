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
#include "p1-service.hpp"
#include "sessy-service.hpp"

#include <mcfp/mcfp.hpp>

#include <iostream>

// --------------------------------------------------------------------

std::unique_ptr<DataService_v2> DataService_v2::s_instance;
thread_local std::unique_ptr<pqxx::connection> DataService_v2::s_connection;

// --------------------------------------------------------------------

DataService_v2 &DataService_v2::instance()
{
	if (not s_instance)
		s_instance.reset(new DataService_v2);
	return *s_instance;
}

DataService_v2::DataService_v2()
{
	auto &config = mcfp::config::instance();

	m_connection_string = config.get("databank");

	// try it
	pqxx::transaction tx(get_connection());

	// start collecting thread

	m_thread = std::thread(std::bind(&DataService_v2::run, this));
}

pqxx::connection &DataService_v2::get_connection()
{
	if (not s_connection)
		s_connection.reset(new pqxx::connection(m_connection_string));
	return *s_connection;
}

void DataService_v2::reset_connection()
{
	s_connection.reset(nullptr);
}

void DataService_v2::store(const P1Opname &opname)
{
	bool first_reset = true;
	do
	{
		try
		{
			pqxx::transaction tx(get_connection());

			tx.exec("INSERT INTO p1_opname (verbruik_hoog, verbruik_laag, levering_hoog, levering_laag) VALUES (" +
					tx.quote(opname.verbruik_hoog) + ", " +
					tx.quote(opname.verbruik_laag) + ", " +
					tx.quote(opname.levering_hoog) + ", " +
					tx.quote(opname.levering_laag) + ")");
			tx.commit();
		}
		catch (const pqxx::broken_connection &e)
		{
			if (first_reset)
			{
				first_reset = false;
				reset_connection();
				continue;
			}
			std::cerr << "Failed to write opname: " << e.what() << '\n';
		}
	} while (false);
}

void DataService_v2::store(const SessySOC &soc)
{
	bool first_reset = true;
	do
	{
		try
		{
			pqxx::transaction tx(get_connection());

			tx.exec("INSERT INTO sessy_soc (nr, soc) VALUES (" +
					tx.quote(soc.nr) + ", " +
					tx.quote(soc.sessy.state_of_charge) + ")");
			tx.commit();
		}
		catch (const pqxx::broken_connection &e)
		{
			if (first_reset)
			{
				first_reset = false;
				reset_connection();
				continue;
			}
			std::cerr << "Failed to write opname: " << e.what() << '\n';
		}
	} while (false);
}

// --------------------------------------------------------------------

void DataService_v2::run()
{
	using namespace std::literals;
	using namespace date;
	using namespace std::chrono;

	using two_minutes = std::chrono::duration<int64_t, std::ratio<60 * 2>>;

	auto now = std::chrono::system_clock::now();
	auto next = ceil<two_minutes>(now);

	for (;;)
	{
		std::this_thread::sleep_until(next);

		// auto opname = P1Service::instance().get_current();
		auto status = P1Service::instance().get_status();
		auto sessy = SessyService::instance().get_soc();

		now = std::chrono::system_clock::now();
		next = ceil<two_minutes>(now);

		GrafiekPunt pt
		{
			.tijd = now,
			.zonne_energie = std::accumulate(sessy.begin(), sessy.end(), 0.f, [](float sum, SessySOC &s)
				{ return sum + s.phase[0].power + sum + s.phase[1].power + sum + s.phase[2].power; }),
			.laad_stroom = std::accumulate(sessy.begin(), sessy.end(), 0.f, [](float sum, SessySOC &s)
				{ return sum + s.sessy.power; }),
			.verbruik = status.power_consumed,
			.terug_levering = status.power_produced,
			.laad_niveau = std::accumulate(sessy.begin(), sessy.end(), 0.f, [](float sum, SessySOC &s)
							   { return sum + s.sessy.state_of_charge; }) /
			               sessy.size(),
		};

		if (not m_vandaag.empty())
		{
			auto ymd_now = date::year_month_day(floor<date::days>(now));
			auto ymd_last = date::year_month_day(floor<date::days>(m_vandaag.back().tijd));

			if (ymd_now > ymd_last)
			{
				m_gisteren = std::move(m_vandaag);
				m_vandaag.clear();
			}
		}

		std::unique_lock lock(m_mutex);
		m_vandaag.emplace_back(std::move(pt));
	}
}

std::vector<GrafiekPunt> DataService_v2::grafiekVoorDag(date::year_month_day dag)
{
	std::unique_lock lock(m_mutex);
	return m_vandaag;
}
