#include <QMutexLocker>
#include "qzeroconfservice.h"


class QZeroConfServiceData : public QSharedData
{
public:
	QString			name;
	QString			type;
	QString			domain;
	QString			host;
	QHostAddress	ip;
	QHostAddress	ipv6;
	quint32			interfaceIndex;
	quint16			port = 0;
	QMap			<QByteArray, QByteArray> txt;
	QMutex			lock;

};

QZeroConfService::QZeroConfService() : data(new QZeroConfServiceData)
{

}

QZeroConfService::QZeroConfService(const QZeroConfService &rhs) : data(rhs.data)
{

}

QZeroConfService &QZeroConfService::operator=(const QZeroConfService &rhs)
{
	if (this != &rhs)
		data.operator=(rhs.data);
	return *this;
}

QZeroConfService::~QZeroConfService()
{

}

QString QZeroConfService::name() const
{
	return data->name;
}

void QZeroConfService::setName(const QString &name)
{
	data->name = name;
}

QString QZeroConfService::type() const
{
	return data->type;
}

void QZeroConfService::setType(const QString &type)
{
	data->type = type;
}

QString QZeroConfService::domain() const
{
	return  data->domain;
}

void QZeroConfService::setDomain(const QString &domain)
{
	data->domain = domain;
}

QString QZeroConfService::host() const
{
	return data->host;
}

void QZeroConfService::setHost(const QString &host)
{
	data->host = host;
}

QHostAddress QZeroConfService::ip() const
{
	QMutexLocker locker(&data->lock);
	return data->ip;
}

void QZeroConfService::setIp(QHostAddress &ip)
{
	QMutexLocker locker(&data->lock);
	data->ip = ip;
}

QHostAddress QZeroConfService::ipv6() const
{
	QMutexLocker locker(&data->lock);
	return data->ipv6;
}

void QZeroConfService::setIpv6(const QHostAddress &ipv6)
{
	QMutexLocker locker(&data->lock);
	data->ipv6 = ipv6;
}

quint32 QZeroConfService::interfaceIndex() const
{
	return data->interfaceIndex;
}

void QZeroConfService::setInterfaceIndex(const quint32 &interfaceIndex)
{
	data->interfaceIndex = interfaceIndex;
}

quint16 QZeroConfService::port() const
{
	return data->port;
}

void QZeroConfService::setPort(const quint16 port)
{
	data->port = port;
}

QMap<QByteArray, QByteArray> QZeroConfService::txt() const
{
	return data->txt;
}

void QZeroConfService::setTxt(const QMap<QByteArray, QByteArray> txt)
{
	data->txt = txt;
}

void QZeroConfService::appendTxt(QByteArray idx, QByteArray val)
{
	data->txt[idx] = val;
}

bool QZeroConfService::operator==(const QZeroConfService &rhs) const
{
	return this->name() == rhs.name() && (this->ip() == rhs.ip() || this->ipv6() == rhs.ipv6());
}

QDebug operator<<(QDebug debug, const QZeroConfService &service)
{
	QDebugStateSaver saver(debug);
	debug.nospace() << "Zeroconf Service: " + service.name() + " @ " + service.host() + " ("+ service.ip().toString()  + ":" + QString::number( service.port()) + ")";
	return debug.maybeSpace();
}
