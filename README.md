Overview
========

Websmsbox is a special Kannel box that sits between bearerbox and smsbox and 
allows plugins to execute and manipulate messages in both directions.

Websmsbox behaves similar to other Kannel boxes and share a compatible
configuration file format and command line options.


Requirements
============
Before you can compile websmsbox and plugins you need a Kannel version installed on your OS with the -fPIC flag set.

To do this, you can just follow the commands below:

```
svn checkout https://svn.kannel.org/gateway/trunk kannel-svn
cd kannel-svn
CFLAGS="-fPIC" ./configure --prefix=/usr/local/kannel
make
```

And finally, as root:

```
make bindir=/usr/local/kannel install
```

Installation
============
Please read the INSTALL file for further instructions. If in a hurry, the quick
explanation is:

```
./bootstrap
CFLAGS="-ldl" ./configure --prefix=/usr/local/kannel --with-kannel-dir=/usr/local/kannel
make
```

And finally, as root:

```
make bindir=/usr/local/kannel install
```

Included plugins
============
For example purposes I have included an HTTP plugin which can intercept all messages to and from Kannel and forward them to an HTTP URL. In the headers will be included all the parameters of the message, which can then be changed by the HTTP server or simply reject the message.

Please see [contrib/plugin-http/websmsbox_http.conf](https://github.com/sah-anshu/kannel-websmsbox/blob/master/contrib/plugin-http/websmsbox_http.conf) as well as [example/websmsbox.conf.example](https://github.com/sah-anshu/kannel-websmsbox/blob/master/example/websmsbox.conf.example) for an example of how to configure.

There is also a PHP script example showing how to reject messages as well as modify parameters of the message structures in [contrib/plugin-http/http-processor.php](https://github.com/sah-anshu/kannel-websmsbox/blob/master/contrib/plugin-http/http-processor.php)

You need to have a compiled version of Kannel available in order to compile
websmsbox.

The Userguide has also valuable information about the install and configuration
steps.

Help
====

The best to ask for help is on Kannel's mailing lists.

Please visit Kannel's site for more information:

http://www.kannel.org/
http://www.websmspanel.com
