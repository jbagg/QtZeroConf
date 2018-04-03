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
   File name    : bonjour.cpp
   Created      : 20 July 2015
   Author(s)    : Jonathan Bagg
---------------------------------------------------------------------------------------------------
   Wrapper for Apple's Bonjour library for use on Windows, MACs and iOS
---------------------------------------------------------------------------------------------------
**************************************************************************************************/
#include "qzeroconf.h"
#include "bonjour_p.h"

QZeroConfPrivate::QZeroConfPrivate(QZeroConf *parent)
{
	pub = parent;
	dnssRef = NULL;
	browser = NULL;
	resolver = NULL;
	bs = NULL;
	browserSocket = NULL;
	resolverSocket = NULL;
	addressSocket = NULL;
}

void QZeroConfPrivate::bsRead()
{
	DNSServiceErrorType err = DNSServiceProcessResult(dnssRef);
	if (err != kDNSServiceErr_NoError) {
		cleanUp(dnssRef);
		emit pub->error(QZeroConf::serviceRegistrationFailed);
	}
}

void QZeroConfPrivate::browserRead()
{
	DNSServiceErrorType err = DNSServiceProcessResult(browser);
	if (err != kDNSServiceErr_NoError) {
		cleanUp(browser);
		emit pub->error(QZeroConf::browserFailed);
	}
}

void QZeroConfPrivate::resolverRead()
{
	DNSServiceErrorType err = DNSServiceProcessResult(resolver);
	if (err != kDNSServiceErr_NoError)
		cleanUp(resolver);
}

void QZeroConfPrivate::resolve(void)
{
	DNSServiceErrorType err;

	err = DNSServiceResolve(&resolver, kDNSServiceFlagsTimeout, work.head().interfaceIndex(), work.head().name().toUtf8(), work.head().type().toUtf8(), work.head().domain().toUtf8(), (DNSServiceResolveReply) resolverCallback, this);
	if (err == kDNSServiceErr_NoError) {
		int sockfd = DNSServiceRefSockFD(resolver);
		if (sockfd == -1) {
			cleanUp(resolver);
		}
		else {
			resolverSocket = new QSocketNotifier(sockfd, QSocketNotifier::Read, this);
			connect(resolverSocket, SIGNAL(activated(int)), this, SLOT(resolverRead()));
		}
	}
	else {
		cleanUp(resolver);
	}
}

void DNSSD_API QZeroConfPrivate::registerCallback(DNSServiceRef, DNSServiceFlags, DNSServiceErrorType errorCode, const char *, const char *, const char *, void *userdata)
{
	QZeroConfPrivate *ref = static_cast<QZeroConfPrivate *>(userdata);

	if (errorCode == kDNSServiceErr_NoError) {
		emit ref->pub->servicePublished();
	}
	else {
		ref->cleanUp(ref->dnssRef);
		emit ref->pub->error(QZeroConf::serviceRegistrationFailed);
	}
}

void DNSSD_API QZeroConfPrivate::browseCallback(DNSServiceRef, DNSServiceFlags flags,
		quint32 interfaceIndex, DNSServiceErrorType err, const char *name,
		const char *type, const char *domain, void *userdata)
{
	QString key;
	QZeroConfService zcs;
	QZeroConfPrivate *ref = static_cast<QZeroConfPrivate *>(userdata);

	//qDebug() << name;
	if (err == kDNSServiceErr_NoError) {
		key = name + QString::number(interfaceIndex);
		if (flags & kDNSServiceFlagsAdd) {
			if (!ref->pub->services.contains(key)) {
				zcs.setName(name);
				zcs.setType(type);
				zcs.setDomain(domain);
				zcs.setInterfaceIndex(interfaceIndex);
				if (!ref->work.size()) {
					ref->work.enqueue(zcs);
					ref->resolve();
				}
				else
					ref->work.enqueue(zcs);
			}
		}
		else if (ref->pub->services.contains(key)) {
			zcs = ref->pub->services[key];
			ref->pub->services.remove(key);
			emit ref->pub->serviceRemoved(zcs);
		}
	}
	else {
		ref->cleanUp(ref->browser);
		emit ref->pub->error(QZeroConf::browserFailed);
	}
}

void DNSSD_API QZeroConfPrivate::resolverCallback(DNSServiceRef, DNSServiceFlags,
		quint32 interfaceIndex, DNSServiceErrorType err, const char *,
		const char *hostName, quint16 port, quint16 txtLen,
		const char * txtRecord, void *userdata)
{
	QZeroConfPrivate *ref = static_cast<QZeroConfPrivate *>(userdata);

	if (err != kDNSServiceErr_NoError) {
		ref->cleanUp(ref->resolver);
		return;
	}

	qint16 recLen;
	while (txtLen > 0)		// add txt records
	{
		recLen = txtRecord[0];
		txtRecord++;
		QByteArray avahiText((const char *)txtRecord, recLen);
		QList<QByteArray> pair = avahiText.split('=');
		if (pair.size() == 2)
			ref->work.head().appendTxt(pair.at(0), pair.at(1));
		else
			ref->work.head().appendTxt(pair.at(0));

		txtLen-= recLen + 1;
		txtRecord+= recLen;
	}
	ref->work.head().setHost(hostName);
	ref->work.head().setPort(qFromBigEndian<quint16>(port));
	err = DNSServiceGetAddrInfo(&ref->resolver, kDNSServiceFlagsForceMulticast, interfaceIndex, ref->protocol, hostName, (DNSServiceGetAddrInfoReply) addressReply, ref);
	if (err == kDNSServiceErr_NoError) {
		int sockfd = DNSServiceRefSockFD(ref->resolver);
		if (sockfd == -1) {
			ref->cleanUp(ref->resolver);
		}
		else {
			ref->addressSocket = new QSocketNotifier(sockfd, QSocketNotifier::Read, ref);
			connect(ref->addressSocket, SIGNAL(activated(int)), ref, SLOT(resolverRead()));
		}
	}
	else {
		ref->cleanUp(ref->resolver);
	}
}

void DNSSD_API QZeroConfPrivate::addressReply(DNSServiceRef sdRef,
		DNSServiceFlags flags, quint32 interfaceIndex,
		DNSServiceErrorType err, const char *hostName,
		const struct sockaddr* address, quint32 ttl, void *userdata)
{
	Q_UNUSED(interfaceIndex);
	Q_UNUSED(sdRef);
	Q_UNUSED(ttl);
	Q_UNUSED(hostName);

	QZeroConfPrivate *ref = static_cast<QZeroConfPrivate *>(userdata);

	if (err == kDNSServiceErr_NoError) {
		if ((flags & kDNSServiceFlagsAdd) != 0) {
			QHostAddress hAddress(address);
			if (hAddress.protocol() == QAbstractSocket::IPv6Protocol)
				ref->work.head().setIpv6(hAddress);
			else
				ref->work.head().setIp(hAddress);

			QString key = ref->work.head().name() + QString::number(interfaceIndex);
			if (!ref->pub->services.contains(key)) {
				ref->pub->services.insert(key, ref->work.head());
				emit ref->pub->serviceAdded(ref->work.head());
			}
			else
				emit ref->pub->serviceUpdated(ref->work.head());

		}
		if (!(flags & kDNSServiceFlagsMoreComing))
			ref->cleanUp(ref->resolver);
	}
	else
		ref->cleanUp(ref->resolver);
}

void QZeroConfPrivate::cleanUp(DNSServiceRef toClean)
{
	if (!toClean)
		return;
	if (toClean == resolver) {
		if (addressSocket) {
			delete addressSocket;
			addressSocket = NULL;
		}
		if (resolverSocket) {
			delete resolverSocket;
			resolverSocket = NULL;
		}
		if(!work.isEmpty())
			work.dequeue();
		if (work.size())
			resolve();
	}
	else if (toClean == browser) {
		browser = NULL;
		if (browserSocket) {
			delete browserSocket;
			browserSocket = NULL;
		}
		QMap<QString, QZeroConfService >::iterator i;
		for (i = pub->services.begin(); i != pub->services.end(); i++) {
			emit pub->serviceRemoved(*i);
		}
		pub->services.clear();
	}
	else if (toClean == dnssRef) {
		dnssRef = NULL;
		if (bs) {
			delete bs;
			bs = NULL;
		}
	}

	DNSServiceRefDeallocate(toClean);
}

QZeroConf::QZeroConf(QObject *parent) : QObject (parent)
{
	pri = new QZeroConfPrivate(this);
	qRegisterMetaType<QZeroConfService>("QZeroConfService");
}

QZeroConf::~QZeroConf()
{
	pri->cleanUp(pri->dnssRef);
	pri->cleanUp(pri->browser);
	pri->cleanUp(pri->resolver);
	delete pri;
}

void QZeroConf::startServicePublish(const char *name, const char *type, const char *domain, quint16 port)
{
	DNSServiceErrorType err;

	if (pri->dnssRef) {
		emit error(QZeroConf::serviceRegistrationFailed);
		return;
	}

	err = DNSServiceRegister(&pri->dnssRef, NULL, NULL,
			name,
			type,
			domain,
			NULL,
			qFromBigEndian<quint16>(port),
			pri->txt.size(), pri->txt.data(),
			(DNSServiceRegisterReply) QZeroConfPrivate::registerCallback, pri);

	if (err == kDNSServiceErr_NoError) {
		int sockfd = DNSServiceRefSockFD(pri->dnssRef);
		if (sockfd == -1) {
			pri->cleanUp(pri->dnssRef);
			emit error(QZeroConf::serviceRegistrationFailed);
		}
		else {
			pri->bs = new QSocketNotifier(sockfd, QSocketNotifier::Read, this);
			connect(pri->bs, SIGNAL(activated(int)), pri, SLOT(bsRead()));
		}
	}
	else {
		pri->cleanUp(pri->dnssRef);
		emit error(QZeroConf::serviceRegistrationFailed);
	}
}

void QZeroConf::stopServicePublish(void)
{
	pri->cleanUp(pri->dnssRef);
}

bool QZeroConf::publishExists(void)
{
	if (pri->dnssRef)
		return true;
	else
		return false;
}

void QZeroConf::addServiceTxtRecord(QString nameOnly)
{
	pri->txt.append((quint8) nameOnly.size());
	pri->txt.append(nameOnly.toUtf8());
}

void QZeroConf::addServiceTxtRecord(QString name, QString value)
{
	name.append("=");
	name.append(value);
	addServiceTxtRecord(name);
}

void QZeroConf::clearServiceTxtRecords()
{
	pri->txt.clear();
}

void QZeroConf::startBrowser(QString type, QAbstractSocket::NetworkLayerProtocol protocol)
{
	DNSServiceErrorType err;

	if (pri->browser) {
		emit error(QZeroConf::browserFailed);
		return;
	}

	switch (protocol) {
		case QAbstractSocket::IPv4Protocol: pri->protocol = kDNSServiceProtocol_IPv4; break;
		case QAbstractSocket::IPv6Protocol: pri->protocol = kDNSServiceProtocol_IPv6; break;
		case QAbstractSocket::AnyIPProtocol: pri->protocol = kDNSServiceProtocol_IPv4 | kDNSServiceProtocol_IPv6; break;
		default: pri->protocol = kDNSServiceProtocol_IPv4; break;
	};

	err = DNSServiceBrowse(&pri->browser, 0, 0, type.toUtf8(), 0, (DNSServiceBrowseReply) QZeroConfPrivate::browseCallback, pri);
	if (err == kDNSServiceErr_NoError) {
		int sockfd = DNSServiceRefSockFD(pri->browser);
		if (sockfd == -1) {
			pri->cleanUp(pri->browser);
			emit error(QZeroConf::browserFailed);
		}
		else {
			pri->browserSocket = new QSocketNotifier(sockfd, QSocketNotifier::Read, this);
			connect(pri->browserSocket, SIGNAL(activated(int)), pri, SLOT(browserRead()));
		}
	}
	else {
		pri->cleanUp(pri->browser);
		emit error(QZeroConf::browserFailed);
	}
}

void QZeroConf::stopBrowser(void)
{
	pri->cleanUp(pri->browser);
}

bool QZeroConf::browserExists(void)
{
	if (pri->browser)
		return true;
	else
		return false;
}
