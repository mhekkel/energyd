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

#include <mcfp/mcfp.hpp>

#include <pqxx/pqxx>

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
