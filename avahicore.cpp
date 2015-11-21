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
   File name    : avahicore.cpp
   Created      : 9 September 2015
   Author(s)    : Jonathan Bagg
---------------------------------------------------------------------------------------------------
   Avahi-core wrapper for use in embedded Linux systems (Android)
---------------------------------------------------------------------------------------------------
**************************************************************************************************/
//#include <avahi-qt4/qt-watch.h>	//
#include "avahi-qt/qt-watch.h"		// fixme don't depend on avahi-qt until it supports Qt5
#include <avahi-core/core.h>
#include <avahi-core/publish.h>
#include <avahi-core/lookup.h>
#include <avahi-common/simple-watch.h>
#include <QCoreApplication>
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
		ready = 0;
		registerWaiting = 0;

		poll = avahi_qt_poll_get();
		if (!poll) {
			return;
		}

		avahi_server_config_init(&config);
		config.publish_workstation = 0;

		server = avahi_server_new(poll, &config, serverCallback, this, &error);
		if (!server) {
			return;
		}
	}

	static void serverCallback(AvahiServer *, AvahiServerState state, AVAHI_GCC_UNUSED void * userdata)
	{
		QZeroConfPrivate *ref = static_cast<QZeroConfPrivate *>(userdata);
		switch (state) {
			case AVAHI_SERVER_RUNNING:
				ref->ready = 1;
                if (ref->registerWaiting) {
                    ref->registerWaiting = 0;
                    ref->registerService(ref->name.toUtf8(), ref->type.toUtf8(), ref->domain.toUtf8(), ref->port);
				}
				break;
			case AVAHI_SERVER_COLLISION:
				break;
			case AVAHI_SERVER_REGISTERING: break;
			case AVAHI_SERVER_FAILURE:
				break;
			case AVAHI_SERVER_INVALID: break;
		}
	}

	static void groupCallback(AvahiServer *, AvahiSEntryGroup *g, AvahiEntryGroupState state, AVAHI_GCC_UNUSED void *userdata)
	{
		QZeroConfPrivate *ref = static_cast<QZeroConfPrivate *>(userdata);
		switch (state) {
			case AVAHI_ENTRY_GROUP_ESTABLISHED:
				emit ref->pub->servicePublished();
				break;
			case AVAHI_ENTRY_GROUP_COLLISION:
				avahi_s_entry_group_free(g);
				ref->group = NULL;
				emit ref->pub->error(QZeroConf::serviceNameCollision);
				break;
			case AVAHI_ENTRY_GROUP_FAILURE :
				avahi_s_entry_group_free(g);
				ref->group = NULL;
				emit ref->pub->error(QZeroConf::serviceRegistrationFailed);
				break;
			case AVAHI_ENTRY_GROUP_UNCOMMITED: break;
			case AVAHI_ENTRY_GROUP_REGISTERING: break;
		}
	}

	static void browseCallback(
			AvahiSServiceBrowser *,
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
				avahi_s_service_resolver_new(ref->server, interface, protocol, name, type, domain, AVAHI_PROTO_UNSPEC, AVAHI_LOOKUP_USE_MULTICAST, resolveCallback, ref);
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
	    AvahiSServiceResolver *r,
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
				zcs->interface = interface;
				zcs->port = port;
				ref->pub->services.insert(key, zcs);
			}

			char a[AVAHI_ADDRESS_STR_MAX];
			avahi_address_snprint(a, sizeof(a), address);
			// fixme maybe add type, and txt records
			if (protocol == AVAHI_PROTO_INET6)
				zcs->ipv6 = QHostAddress(a);
			else if (protocol == AVAHI_PROTO_INET)
				zcs->ip = QHostAddress(a);

			if (newRecord)
				emit ref->pub->serviceAdded(zcs);
			else
				emit ref->pub->serviceUpdated(zcs);
		}
		avahi_s_service_resolver_free(r);
	}

	void broswerCleanUp(void)
	{
		if (!browser)
			return;

		QMap<QString, QZeroConfService *>::iterator i;
		for (i = pub->services.begin(); i != pub->services.end(); i++)
			delete *i;
		pub->services.clear();

		avahi_s_service_browser_free(browser);
		browser = NULL;
	}

	void registerService(const char *name, const char *type, const char *domain, quint16 port)
	{
		qint32 ret;
		group = avahi_s_entry_group_new(server, QZeroConfPrivate::groupCallback, this);
		if (!group) {
			pub->emit error(QZeroConf::serviceRegistrationFailed);
			return;
		}

		ret = avahi_server_add_service(server, group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, AVAHI_PUBLISH_UPDATE, name, type, domain, NULL, port, NULL);
		if (ret < 0) {
			avahi_s_entry_group_free(group);
			group = NULL;
			pub->emit error(QZeroConf::serviceRegistrationFailed);
			return;
		}

		ret = avahi_s_entry_group_commit(group);
		if (ret < 0) {
			avahi_s_entry_group_free(group);
			group = NULL;
			pub->emit error(QZeroConf::serviceRegistrationFailed);
		}
	}

	QZeroConf *pub;
	const AvahiPoll *poll;
	AvahiServer *server;
	AvahiServerConfig config;
	AvahiSEntryGroup *group;
	AvahiSServiceBrowser *browser;
	bool ready, registerWaiting;
	QString name, type, domain;
	qint32 port;
};


QZeroConf::QZeroConf()
{
	pri = new QZeroConfPrivate(this);
}

QZeroConf::~QZeroConf()
{
	pri->broswerCleanUp();
	avahi_server_config_free(&pri->config);
	if (pri->server)
		avahi_server_free(pri->server);
	delete pri;
}

void QZeroConf::startServicePublish(const char *name, const char *type, const char *domain, quint16 port)
{
	if (pri->group) {
		emit error(QZeroConf::serviceRegistrationFailed);
		return;
	}
	if (pri->ready)
		pri->registerService(name, type, domain, port);
	else {
		pri->registerWaiting = 1;
		pri->name = name;
		pri->type = type;
		pri->domain = domain;
		pri->port = port;
	}
}

void QZeroConf::stopServicePublish(void)
{
	if (pri->group) {
		avahi_s_entry_group_free(pri->group);
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

	pri->browser = avahi_s_service_browser_new(pri->server, AVAHI_IF_UNSPEC, avahiProtocol, type.toUtf8(), NULL, AVAHI_LOOKUP_USE_MULTICAST, QZeroConfPrivate::browseCallback, pri);
	if (!pri->browser)
		emit error(QZeroConf::browserFailed);
}

void QZeroConf::stopBrowser(void)
{
	pri->broswerCleanUp();
}
