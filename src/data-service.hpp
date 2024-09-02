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

#pragma once

#include <zeep/nvp.hpp>

#include <date/date.h>
#include <pqxx/pqxx>

#include <chrono>
#include <memory>
#include <string>
#include <thread>

struct P1Opname
{
	std::chrono::system_clock::time_point tijd;
	float verbruik_hoog, verbruik_laag, levering_hoog, levering_laag;

	template <typename Archive>
	void serialize(Archive &ar, unsigned long version)
	{
		ar & zeep::make_nvp("tijd", tijd)
		   & zeep::make_nvp("verbruik_hoog", verbruik_hoog)
		   & zeep::make_nvp("verbruik_laag", verbruik_laag)
		   & zeep::make_nvp("levering_hoog", levering_hoog)
		   & zeep::make_nvp("levering_laag", levering_laag);
	}
};

struct P1Status
{
	std::string status;
	std::string state;
	float total_power;
	float power_consumed;
	float power_produced;

	template <typename Archive>
	void serialize(Archive &ar, unsigned long version)
	{
		ar & zeep::make_nvp("status", status)
		   & zeep::make_nvp("state", state)
		   & zeep::make_nvp("total_power", total_power)
		   & zeep::make_nvp("power_consumed", power_consumed)
		   & zeep::make_nvp("power_produced", power_produced);
	}
};

struct Sessy
{
	float state_of_charge;
	float power;
	float power_setpoint;
	std::string system_state;
	std::string system_state_details;
	float frequency;

	template <typename Archive>
	void serialize(Archive &ar, unsigned long version)
	{
		ar & zeep::make_nvp("state_of_charge", state_of_charge)
			 & zeep::make_nvp("power", power)
			 & zeep::make_nvp("power_setpoint", power_setpoint)
			 & zeep::make_nvp("system_state", system_state)
			 & zeep::make_nvp("system_state_details", system_state_details)
			 & zeep::make_nvp("frequency", frequency);
	}
};

struct RenewableEnergy
{
	float voltage_rms;
	float current_rms;
	float power;

	template <typename Archive>
	void serialize(Archive &ar, unsigned long version)
	{
		ar & zeep::make_nvp("voltage_rms", voltage_rms)
		   & zeep::make_nvp("current_rms", current_rms)
		   & zeep::make_nvp("power", power);
	}
};

struct SessySOC
{
	int nr;
	std::string status;
	Sessy sessy;
	RenewableEnergy phase[3];

	template <typename Archive>
	void serialize(Archive &ar, unsigned long version)
	{
		ar & zeep::make_nvp("nr", nr)
		   & zeep::make_nvp("status", status)
		   & zeep::make_nvp("sessy", sessy)
		   & zeep::make_nvp("renewable_energy_phase1", phase[0])
		   & zeep::make_nvp("renewable_energy_phase2", phase[1])
		   & zeep::make_nvp("renewable_energy_phase3", phase[2]);
	}
};

// --------------------------------------------------------------------

struct GrafiekPunt
{
	std::chrono::system_clock::time_point tijd;
	float zon;
	float batterij;
	float verbruik;
	float levering;
	float laad_niveau;

	template <typename Archive>
	void serialize(Archive &ar, unsigned long version)
	{
		ar & zeep::make_nvp("tijd", tijd)
		   & zeep::make_nvp("zon", zon)
		   & zeep::make_nvp("batterij", batterij)
		   & zeep::make_nvp("verbruik", verbruik)
		   & zeep::make_nvp("levering", levering)
		   & zeep::make_nvp("laad_niveau", laad_niveau);
	}
};

// --------------------------------------------------------------------

class DataService_v2
{
  public:
	static DataService_v2 &instance();

	void store(const P1Opname &opname);
	void store(const SessySOC &soc);
	void store(const GrafiekPunt &pt);

	void reset_connection();

	std::vector<GrafiekPunt> grafiekVoorDag(date::year_month_day dag, std::chrono::minutes resolutie);

  private:

	DataService_v2();

	pqxx::connection &get_connection();

	void run();

	std::string m_connection_string;

	std::thread m_thread;
	std::mutex m_mutex;
	bool m_read_only;

	static std::unique_ptr<DataService_v2> s_instance;
	static thread_local std::unique_ptr<pqxx::connection> s_connection;
};