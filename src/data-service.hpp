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

#include <pqxx/pqxx>

#include <chrono>
#include <memory>
#include <string>

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

// --------------------------------------------------------------------

class DataService_v2
{
  public:
	static DataService_v2 &instance();

	void store(const P1Opname &opname);

	void reset_connection();

  private:

	DataService_v2();

	pqxx::connection &get_connection();

	std::string m_connection_string;

	static std::unique_ptr<DataService_v2> s_instance;
	static thread_local std::unique_ptr<pqxx::connection> s_connection;
};