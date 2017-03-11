
QZeroConf is a Qt wrapper class for ZeroConf libraries across various platforms.

* Windows (requires iTunes or [Apple Print Services](https://support.apple.com/kb/DL999))
* Mac
* Linux
* Android
* iOS

QZeroConf wraps avahi-client on Linux, avahi-core on Android, and dnssd on Mac, iOS and Windows.

### Building

QZeroConf can be built directly into your project if your project is [LGPL3](http://www.gnu.org/licenses/lgpl-3.0.en.html) compatible.  If your project is closed source, you can build QZeroConf as a dynamic library and link against it.

#### Building into your project

1. Clone or download QZeroConf.  If you download, unzip.
2. Copy the qtzeroconf directory to be under your project's directory.
3. Include the qtzeroconf.pri file in your projects .pro file

    include(qtzeroconf/qtzeroconf.pri)

#### Compiling as a dynamic library

1. Clone or download QZeroConf.  If you download, unzip.
2. Enter the qtzeroconf directory, run qmake and then make.

### API

#### Service Publishing

(See the example included with the source)

1) Include header

```c++
#include "qzeroconf.h"
```
2) Create an instance of QZeroConf

```c++
QZeroConf zeroConf;
```
It is recommend, but not required, that you connect a slot to QZeroConf's error() signal and servicePublished() signal.

3) Call startServicePublish() with the name, type, domain and port of your service.

```c++
zeroConf.startServicePublish("Test", "_test._tcp", "local", 12345);
```
QZeroConf will emit servicePublished() if successful, or the error() signal if registration fails.

Service publishing can be stopped by calling stopServicePublish().
Only one service can be published per instance of QZeroConf.

#### Service Discovery

(See the example included with the source)

1) Include header

```c++
#include "qzeroconf.h"
```
2) Create an instance of QZeroConf

```c++
QZeroConf zeroConf;
```
It is recommend, but not required, that you connect a slot to QZeroConf's error() signal.

3) Connect a slot to QZeroConf's serviceAdded() signal.  When serviceAdded() is emitted, it passes a pointer to the QZeroConfService recently discovered.

4) Optionally connect a slot to QZeroConf's serviceRemoved() signal to received status when the service is unpublished.   ServiceRemoved() passes a pointer to the service being removed.  **Do not make delayed connections with ServiceRemoved() signal.**

5) Call startBrowser() with the type of the service to browse for and optionally the protocol to use.

```c++
startBrowser("_test._tcp");
```
If you are browsing for services published using both ipv4 and ipv6 ( QAbstractSocket::AnyIPProtocol) you should also connect a slot to QzeroConf's serviceUpdated() signal.  When the IP address of the first protocol is resolved,  serviceAdded() is emitted, when the IP address of the second protocol is resolved,  serviceUpdated() is emitted.

Only one browser can be in use per instance of QzeroConf.

### Build Dependencies

Qt5

On Linux, avahi-client-dev and avahi-common-dev