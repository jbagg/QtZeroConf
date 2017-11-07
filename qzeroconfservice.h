#ifndef QZEROCONFSERVICE_H
#define QZEROCONFSERVICE_H

#include <QExplicitlySharedDataPointer>
#include <QHostAddress>
#include "qzeroconfglobal.h"

class QZeroConfServiceData;
class QZeroConfPrivate;

class Q_ZEROCONF_EXPORT QZeroConfService
{
	Q_GADGET
	Q_PROPERTY( QString name READ name )
	Q_PROPERTY( QString type READ type )
	Q_PROPERTY( QString domain READ domain )
	Q_PROPERTY( QString host READ host )

friend class QZeroConfPrivate;

public:

	QZeroConfService();
	QZeroConfService(const QZeroConfService &);
	QZeroConfService &operator=(const QZeroConfService &);
	~QZeroConfService();

	QString name() const;
	QString type() const;
	QString domain() const;
	QString host() const;
	QHostAddress ip() const;
	QHostAddress ipv6() const;
	quint32 interfaceIndex() const;
	quint16 port() const;
	QMap <QByteArray, QByteArray> txt() const;

	bool operator==(const QZeroConfService &rhs) const;

private:
	void setName(const QString &name);
	void setType(const QString &type);
	void setDomain(const QString &domain);
	void setHost(const QString &host);
	void setIp(QHostAddress &ip);
	void setIpv6(const QHostAddress &ipv6);
	void setInterfaceIndex(const quint32 &interfaceIndex);
	void setPort(const quint16 port);
	void setTxt(const QMap<QByteArray, QByteArray> txt);
	void appendTxt(QByteArray idx, QByteArray val = "");
	QExplicitlySharedDataPointer<QZeroConfServiceData> data;
};

QDebug operator<<(QDebug debug, const QZeroConfService &service);

Q_DECLARE_METATYPE(QZeroConfService)

#endif // QZEROCONFSERVICE_H
