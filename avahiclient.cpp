/**************************************************************************************************
---------------------------------------------------------------------------------------------------
	Copyright (C) 2015  Jonathan Bagg
	This file is part of QtZeroConf.

	QtZeroConf is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	QtZeroConf is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with QtZeroConf.  If not, see <http://www.gnu.org/licenses/>.
---------------------------------------------------------------------------------------------------
   Project name : QtZeroConf
   File name    : avahiclient.cpp
   Created      : 20 July 2015
   Author(s)    : Jonathan Bagg
---------------------------------------------------------------------------------------------------
   Avahi-client wrapper for use in Desktop Linux systems
---------------------------------------------------------------------------------------------------
**************************************************************************************************/
//#include <avahi-qt4/qt-watch.h>	//
#include "avahi-qt/qt-watch.h"		// fixme don't depend on avahi-qt until it supports Qt5
#include <avahi-client/client.h>
#include <avahi-client/publish.h>
#include <avahi-common/error.h>
#include <avahi-client/lookup.h>
#include "qzeroconf.h"

class QZeroConfPrivate
{
public:
	QZeroConfPrivate(QZeroConf *parent)
	{
		qint32 error;

		pub = parent;
		group = NULL;
		browser = NULL;
		poll = avahi_qt_poll_get();
		if (!poll) {
			return;
		}
		client = avahi_client_new(poll, AVAHI_CLIENT_IGNORE_USER_CONFIG, NULL, this, &error);
		if (!client) {
			return;
		}
	}

	static void groupCallback(AvahiEntryGroup *g, AvahiEntryGroupState state, AVAHI_GCC_UNUSED void *userdata)
	{
		qint32 ret;

		QZeroConfPrivate *ref = static_cast<QZeroConfPrivate *>(userdata);
		switch (state) {
			case AVAHI_ENTRY_GROUP_ESTABLISHED:
				emit ref->pub->servicePublished();
				break;
			case AVAHI_ENTRY_GROUP_COLLISION:
				avahi_entry_group_free(g);
				ref->group = NULL;
				emit ref->pub->error(QZeroConf::serviceNameCollision);
				break;
			case AVAHI_ENTRY_GROUP_FAILURE:
				avahi_entry_group_free(g);
				ref->group = NULL;
				emit ref->pub->error(QZeroConf::serviceRegistrationFailed);
				break;
			case AVAHI_ENTRY_GROUP_UNCOMMITED:
				ret = avahi_entry_group_add_service(g, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, AVAHI_PUBLISH_UPDATE, ref->name.toUtf8(), ref->type.toUtf8(), ref->domain.toUtf8(), NULL, ref->port, NULL);
				if (ret < 0) {
					avahi_entry_group_free(g);
					ref->group = NULL;
					emit ref->pub->error(QZeroConf::serviceRegistrationFailed);
					return;
				}

				ret = avahi_entry_group_commit(g);
				if (ret < 0) {
					avahi_entry_group_free(g);
					ref->group = NULL;
					emit ref->pub->error(QZeroConf::serviceRegistrationFailed);
				}
				break;
			case AVAHI_ENTRY_GROUP_REGISTERING: break;
		}
	}

	static void browseCallback(AvahiServiceBrowser *,
			AvahiIfIndex interface,
			AvahiProtocol protocol,
			AvahiBrowserEvent event,
			const char *name,
			const char *type,
			const char *domain,
			AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
			void* userdata)
	{
		QString key;
		QZeroConfPrivate *ref = static_cast<QZeroConfPrivate *>(userdata);

		switch (event) {
		case AVAHI_BROWSER_FAILURE:
			ref->broswerCleanUp();
			emit ref->pub->error(QZeroConf::browserFailed);
			break;
		case AVAHI_BROWSER_NEW:
			avahi_service_resolver_new(ref->client, interface, protocol, name, type, domain, AVAHI_PROTO_UNSPEC, AVAHI_LOOKUP_USE_MULTICAST, resolveCallback, ref);
			break;
		case AVAHI_BROWSER_REMOVE:
			key = name + QString::number(interface);
			if (!ref->pub->services.contains(key))
				return;
			QZeroConfService *zcs;
			zcs = ref->pub->services[key];
			ref->pub->services.remove(key);
			emit ref->pub->serviceRemoved(zcs);
			delete zcs;
			break;
		case AVAHI_BROWSER_ALL_FOR_NOW:
		case AVAHI_BROWSER_CACHE_EXHAUSTED:
			break;
		}
	}

	static void resolveCallback(
	    AvahiServiceResolver *r,
	    AVAHI_GCC_UNUSED AvahiIfIndex interface,
	    AVAHI_GCC_UNUSED AvahiProtocol protocol,
	    AvahiResolverEvent event,
	    const char *name,
	    const char *type,
	    const char *domain,
	    const char *host_name,
	    const AvahiAddress *address,
	    uint16_t port,
	    AvahiStringList *,
	    AvahiLookupResultFlags,
	    AVAHI_GCC_UNUSED void* userdata)
	{
		bool newRecord = 0;
		QZeroConfPrivate *ref = static_cast<QZeroConfPrivate *>(userdata);

		if (event == AVAHI_RESOLVER_FOUND) {
			QString key = name + QString::number(interface);
			QZeroConfService *zcs;
			if (ref->pub->services.contains(key))
				zcs = ref->pub->services[key];
			else {
				newRecord = 1;
				zcs = new QZeroConfService;
				zcs->name = name;
				zcs->type = type;
				zcs->domain = domain;
				zcs->host = host_name;
				zcs->interfaceIndex = interface;
				zcs->port = port;
				ref->pub->services.insert(key, zcs);
			}

			char a[AVAHI_ADDRESS_STR_MAX];
			avahi_address_snprint(a, sizeof(a), address);
			// fixme maybe add type, and txt records
			if (protocol == AVAHI_PROTO_INET6)
				zcs->ipv6 = QHostAddress(a);
			else if (protocol == AVAHI_PROTO_INET)
				zcs->ip = QHostAddress (a);

			if (newRecord)
				emit ref->pub->serviceAdded(zcs);
			else
				emit ref->pub->serviceUpdated(zcs);
		}
		avahi_service_resolver_free(r);
	}

	void broswerCleanUp(void)
	{
		if (!browser)
			return;

		QMap<QString, QZeroConfService *>::iterator i;
		for (i = pub->services.begin(); i != pub->services.end(); i++)
			delete *i;
		pub->services.clear();

		avahi_service_browser_free(browser);
		browser = NULL;
	}

	QZeroConf *pub;
	const AvahiPoll *poll;
	AvahiClient *client;
	AvahiEntryGroup *group;
	QString name, type, domain;
	quint16 port;

	AvahiServiceBrowser *browser;
};


QZeroConf::QZeroConf()
{
	pri = new QZeroConfPrivate(this);
}

QZeroConf::~QZeroConf()
{
	pri->broswerCleanUp();
	if (pri->client)
		avahi_client_free(pri->client);
	delete pri;
}

void QZeroConf::startServicePublish(const char *name, const char *type, const char *domain, quint16 port)
{
	if (pri->group) {
		emit error(QZeroConf::serviceRegistrationFailed);
		return;
	}

	pri->name = name;
	pri->type = type;
	pri->domain = domain;
	pri->port = port;

	pri->group = avahi_entry_group_new(pri->client, QZeroConfPrivate::groupCallback, pri);
	if (!pri->group)
		emit error(QZeroConf::serviceRegistrationFailed);
}

void QZeroConf::stopServicePublish(void)
{
	if (pri->group) {
		avahi_entry_group_free(pri->group);
		pri->group = NULL;
	}
}

void QZeroConf::startBrowser(QString type, QAbstractSocket::NetworkLayerProtocol protocol)
{
 	AvahiProtocol	avahiProtocol;

	if (pri->browser)
		emit error(QZeroConf::browserFailed);

	switch (protocol) {
		case QAbstractSocket::IPv4Protocol: avahiProtocol = AVAHI_PROTO_INET; break;
		case QAbstractSocket::IPv6Protocol: avahiProtocol = AVAHI_PROTO_INET6; break;
		case QAbstractSocket::AnyIPProtocol: avahiProtocol = AVAHI_PROTO_UNSPEC; break;
		default: avahiProtocol = AVAHI_PROTO_INET; break;
	};

	pri->browser = avahi_service_browser_new(pri->client, AVAHI_IF_UNSPEC, avahiProtocol, type.toUtf8(), NULL, AVAHI_LOOKUP_USE_MULTICAST, QZeroConfPrivate::browseCallback, pri);
	if (!pri->browser)
		emit error(QZeroConf::browserFailed);
}

void QZeroConf::stopBrowser(void)
{
	pri->broswerCleanUp();
}
