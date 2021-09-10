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
   File name    : QZeroConfNsdManager.java
   Created      : 10 Spetember 2021
   Author(s)    : Michael Zanetti
---------------------------------------------------------------------------------------------------
   NsdManager wrapper for use on Android devices
---------------------------------------------------------------------------------------------------
**************************************************************************************************/

package qtzeroconf;

import java.util.Map;

import android.util.Log;

import android.content.Context;

import android.net.nsd.NsdServiceInfo;
import android.net.nsd.NsdManager;

public class QZeroConfNsdManager {

	public static native void onServiceResolvedJNI(int id, String name, String type, String hostname, String address, int port, Map<String, byte[]> txtRecords);
	public static native void onServiceRemovedJNI(int id, String name);
	public static native void onBrowserStateChangedJNI(int id, boolean running, boolean error);
	public static native void onPublisherStateChangedJNI(int id, boolean running, boolean error);
	public static native void onServiceNameChangedJNI(int id, String newName);

	private static String TAG = "QZeroConfNsdManager";
	private int id;
	private Context context;
	private NsdManager nsdManager;
	private NsdManager.DiscoveryListener discoveryListener;
	private NsdManager.RegistrationListener registrationListener;
	private String registrationName; // The original service name that was given for registration, it might change on collisions

	public QZeroConfNsdManager(int id, Context context) {
		super();
		this.id = id;
		this.context = context;

		nsdManager = (NsdManager)context.getSystemService(Context.NSD_SERVICE);
		discoveryListener = initializeDiscoveryListener();
		registrationListener = initializeRegistrationListener();
    }

	public void registerService(String name, String type, int port, Map<String, String> txtRecords) {
		registrationName = name;

		NsdServiceInfo serviceInfo = new NsdServiceInfo();
		serviceInfo.setServiceName(name);
		serviceInfo.setServiceType(type);
		serviceInfo.setPort(port);
		for (Map.Entry<String, String> entry: txtRecords.entrySet()) {
			serviceInfo.setAttribute(entry.getKey(), entry.getValue());
		}

		try {
			nsdManager.registerService(serviceInfo, NsdManager.PROTOCOL_DNS_SD, registrationListener);
		} catch (IllegalArgumentException e) {
			Log.w(TAG, "Error registering service: " + e.toString());
			onPublisherStateChangedJNI(id, false, true);
		}
	}

	public void unregisterService() {
		nsdManager.unregisterService(registrationListener);
	}

	public void discoverServices(String serviceType) {
		nsdManager.discoverServices(serviceType, NsdManager.PROTOCOL_DNS_SD, discoveryListener);
	}

	public void stopServiceDiscovery() {
		nsdManager.stopServiceDiscovery(discoveryListener);
	}

	private NsdManager.DiscoveryListener initializeDiscoveryListener() {
		return new NsdManager.DiscoveryListener() {

			@Override
			public void onDiscoveryStarted(String regType) {
				QZeroConfNsdManager.onBrowserStateChangedJNI(id, true, false);
			}

			@Override
			public void onServiceFound(NsdServiceInfo service) {
				nsdManager.resolveService(service, initializeResolveListener());
			}

			@Override
			public void onServiceLost(NsdServiceInfo serviceInfo) {
				QZeroConfNsdManager.onServiceRemovedJNI(id, serviceInfo.getServiceName());
			}

			@Override
			public void onDiscoveryStopped(String serviceType) {
				QZeroConfNsdManager.onBrowserStateChangedJNI(id, false, false);
			}

			@Override
			public void onStartDiscoveryFailed(String serviceType, int errorCode) {
				QZeroConfNsdManager.onBrowserStateChangedJNI(id, false, true);
			}

			@Override
			public void onStopDiscoveryFailed(String serviceType, int errorCode) {
				QZeroConfNsdManager.onBrowserStateChangedJNI(id, false, true);
			}
		};
	}

	private NsdManager.ResolveListener initializeResolveListener() {
		return new NsdManager.ResolveListener() {

			@Override
			public void onResolveFailed(NsdServiceInfo serviceInfo, int errorCode) {
				Log.d(TAG, "Resolving failed for: " + serviceInfo.getServiceName() + " " + serviceInfo.getServiceType() + ": " + errorCode);
			}

			@Override
			public void onServiceResolved(NsdServiceInfo serviceInfo) {
				QZeroConfNsdManager.onServiceResolvedJNI(id,
					serviceInfo.getServiceName(),
					serviceInfo.getServiceType(),
					serviceInfo.getHost().getHostName(),
					serviceInfo.getHost().getHostAddress(),
					serviceInfo.getPort(),
					serviceInfo.getAttributes()
				);
			}
		};
	}

	public NsdManager.RegistrationListener initializeRegistrationListener() {
		return new NsdManager.RegistrationListener() {

			@Override
			public void onServiceRegistered(NsdServiceInfo serviceInfo) {
				QZeroConfNsdManager.onPublisherStateChangedJNI(id, true, false);
				if (!serviceInfo.getServiceName().equals(registrationName)) {
					onServiceNameChangedJNI(id, serviceInfo.getServiceName());
				}
			}

			@Override
			public void onRegistrationFailed(NsdServiceInfo serviceInfo, int errorCode) {
				QZeroConfNsdManager.onPublisherStateChangedJNI(id, false, true);
			}

			@Override
			public void onServiceUnregistered(NsdServiceInfo arg0) {
				QZeroConfNsdManager.onPublisherStateChangedJNI(id, false, false);
			}

			@Override
			public void onUnregistrationFailed(NsdServiceInfo serviceInfo, int errorCode) {
				QZeroConfNsdManager.onPublisherStateChangedJNI(id, false, true);
			}
		};
	}
}
