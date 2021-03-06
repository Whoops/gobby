/* Gobby - GTK-based collaborative text editor
 * Copyright (C) 2008-2010 Armin Burgmeier <armin@arbur.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "core/nodewatch.hpp"

Gobby::NodeWatch::NodeWatch(InfcBrowser* browser,
                            const InfcBrowserIter* iter):
	m_browser(browser), m_iter(*iter)
{
	// Need to have a connection for the browser, otherwise we can't
	// reach that node anyway.

	// TODO: (weak-)ref connection and browser?
	m_connection = infc_browser_get_connection(browser);
	g_assert(m_connection);

	InfXmlConnectionStatus status;
	g_object_get(G_OBJECT(m_connection), "status", &status, NULL);
	g_assert(status == INF_XML_CONNECTION_OPEN);

	m_browser_notify_connection_handler = g_signal_connect(
		browser, "notify::connection",
		G_CALLBACK(on_browser_notify_connection_static), this);
	m_node_removed_handler = g_signal_connect(m_browser, "node-removed",
	                 G_CALLBACK(on_node_removed_static), this);
	m_connection_notify_status_handler = g_signal_connect(
		m_connection, "notify::status",
		G_CALLBACK(on_connection_notify_status_static), this);
}

Gobby::NodeWatch::~NodeWatch()
{
	if(m_browser != NULL)
		reset();
}

void Gobby::NodeWatch::on_browser_notify_connection()
{
	InfXmlConnection* connection = infc_browser_get_connection(m_browser);

	if(connection)
	{
		// Connection changed on-the-fly. This isn't a problem for
		// us if the watched node is still available (though I don't
		// think this can happen currently).
		InfXmlConnectionStatus status;
		g_object_get(G_OBJECT(connection), "status", &status, NULL);
		if(status == INF_XML_CONNECTION_OPEN)
			set_connection(connection);
		else
			connection = NULL;
	}

	if(!connection)
	{
		reset();
		m_signal_node_removed.emit();
	}
}

void Gobby::NodeWatch::on_connection_notify_status()
{
	InfXmlConnectionStatus status;
	g_object_get(G_OBJECT(m_connection), "status", &status, NULL);

	// Connection was closed: Node is no longer reachable
	if(status != INF_XML_CONNECTION_OPEN)
	{
		reset();
		m_signal_node_removed.emit();
	}
}

void Gobby::NodeWatch::on_node_removed(InfcBrowser* browser,
                                       InfcBrowserIter* iter)
{
	g_assert(browser == m_browser);

        InfcBrowserIter test_iter = m_iter;

        do
        {
                if(test_iter.node == iter->node &&
                   test_iter.node_id == iter->node_id)
                {
			reset();
			// Note that we want to allow signal handlers to
			// delete the watch, so make sure we don't access
			// member variables anymore after having emitted this.
			m_signal_node_removed.emit();
                        break;
                }
        } while(infc_browser_iter_get_parent(browser, &test_iter));
}

void Gobby::NodeWatch::reset()
{
	g_assert(m_browser != NULL);

	set_connection(NULL);

	g_signal_handler_disconnect(
		m_browser, m_browser_notify_connection_handler);
	g_signal_handler_disconnect(
		m_browser, m_node_removed_handler);

	m_browser = NULL;
}

void Gobby::NodeWatch::set_connection(InfXmlConnection* connection)
{
	if(m_connection != NULL)
	{
		g_signal_handler_disconnect(
			m_connection, m_connection_notify_status_handler);
	}

	m_connection = connection;

	if(connection != NULL)
	{
		m_connection_notify_status_handler = g_signal_connect(
			m_connection, "notify::status",
			G_CALLBACK(on_connection_notify_status_static), this);
	}
}
