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
   Project name : QtZeroConf Example
   File name    : window.cpp
   Created      : 3 November 2015
   Author(s)    : Jonathan Bagg
---------------------------------------------------------------------------------------------------
   Example app to demonstrate service publishing and service discovery
---------------------------------------------------------------------------------------------------
**************************************************************************************************/
#include <QVBoxLayout>
#include <QPushButton>
#include <QTableWidgetItem>
#include <QNetworkInterface>
#include <QHeaderView>
#include "window.h"

#ifdef Q_OS_IOS
	#define	OS_NAME		"iOS"
#elif defined(Q_OS_MAC)
	#define	OS_NAME		"Mac"
#elif defined(Q_OS_ANDROID)
	#define	OS_NAME		"Android"
#elif defined(Q_OS_LINUX)
	#define	OS_NAME		"Linux"
#elif defined(Q_OS_WIN)
	#define	OS_NAME		"Windows"
#elif defined(Q_OS_FREEBSD)
	#define	OS_NAME		"FreeBSD"
#else
	#define	OS_NAME		"Some OS"
#endif

mainWindow::mainWindow()
{
	publishEnabled = 0;
	buildGUI();

    connect(&zeroConf, SIGNAL(serviceAdded(QZeroConfService )), this, SLOT(addService(QZeroConfService )));
    connect(&zeroConf, SIGNAL(serviceRemoved(QZeroConfService )), this, SLOT(removeService(QZeroConfService )));

    zeroConf.startBrowser("_qtzeroconf_test._tcp");
	startPublish();
}

void mainWindow::buildGUI()
{
	QPushButton *button;
	QVBoxLayout *layout = new QVBoxLayout;

	button = new QPushButton(tr(" Enable Publish "));
	button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	layout->addWidget(button);
	layout->setAlignment(button, Qt::AlignHCenter);
	connect(button, SIGNAL(clicked()), this, SLOT(startPublish()));

	button = new QPushButton(tr(" Disable Publish "));
	button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	layout->addWidget(button);
	layout->setAlignment(button, Qt::AlignHCenter);
	connect(button, SIGNAL(clicked()), this, SLOT(stopPublish()));

	table.verticalHeader()->hide();
	table.horizontalHeader()->hide();
	table.setColumnCount(2);
	layout->addWidget(&table);
	//table.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	QWidget *widget = new QWidget;
	widget->setLayout(layout);
	setCentralWidget(widget);
	show();
}

QString mainWindow::buildName(void)
{
    QString name;

	QList<QNetworkInterface> list = QNetworkInterface::allInterfaces(); // now you have interfaces list

	name = list.last().hardwareAddress();
	name.remove(":");
	name.remove(0, 6);
	name+= ')';
	name.prepend("Qt ZeroConf Test - " OS_NAME " (");
    return name;
}

// ---------- Service Publish ----------

void mainWindow::startPublish()
{
	if (publishEnabled)
		return;
	publishEnabled = 1;

	zeroConf.clearServiceTxtRecords();
	zeroConf.addServiceTxtRecord("Qt", "the best!");
	zeroConf.addServiceTxtRecord("ZeroConf is nice too");
	zeroConf.startServicePublish(buildName().toUtf8(), "_qtzeroconf_test._tcp", "local", 11437);
}

void mainWindow::stopPublish()
{
	if (!publishEnabled)
		return;
	publishEnabled = 0;
	zeroConf.stopServicePublish();
}

// ---------- Discovery Callbacks ----------

void mainWindow::addService(QZeroConfService zcs)
{
	qint32 row;
	QTableWidgetItem *cell;

	row = table.rowCount();
	table.insertRow(row);
    cell = new QTableWidgetItem(zcs.name());
	table.setItem(row, 0, cell);
    cell = new QTableWidgetItem(zcs.ip().toString());
	table.setItem(row, 1, cell);
	table.resizeColumnsToContents();
	#if !(defined(Q_OS_IOS) || defined(Q_OS_ANDROID))
	setFixedSize(table.horizontalHeader()->length() + 60, table.verticalHeader()->length() + 100);
	#endif
}

void mainWindow::removeService(QZeroConfService zcs)
{
	qint32 i, row;
	QTableWidgetItem *nameItem, *ipItem;

    QList <QTableWidgetItem*> search = table.findItems(zcs.name(), Qt::MatchExactly);
	for (i=0; i<search.size(); i++) {
		nameItem = search.at(i);
		row = nameItem->row();
		ipItem = table.item(row, 1);
        if (table.item(row, 1)->text() == zcs.ip().toString()) {		// match ip address in case of dual homed systems
			delete nameItem;
			delete ipItem;
			table.removeRow(row);
		}
	}
}
