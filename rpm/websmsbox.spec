%define version cvs
%define release _%(echo svn-r | cut -d- -f2)

Summary: DB-Based Kannel Box for message queueing.
Name: websmsbox
Version: %{version}
Release: %{release}
License: Kannel
Group: System Environment/Daemons
URL: http://www.kannel.org/~aguerrieri/
Source0: websmsbox-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
BuildRequires: bison, byacc, flex
BuildRequires: kannel-devel, openssl-devel, zlib-devel
BuildRequires: 

%description
Websmsbox is a special Kannel box that sits between bearerbox and smsbox and uses
a database queue to store and forward messages.

Messages are queued on a configurable table (defaults to send_sms) and moved to
another table (defaults to sent_sms) afterwards.

You can also manually insert messages into the send_sms table and they will be sent
and moved to the sent_sms table as well. This allows for fast and easy injection
of large amounts of messages into Kannel.

%prep
rm -rf %{buildroot}
%setup -q

%build
%configure
%{__make}

%install
%{__rm} -rf %{buildroot}
%makeinstall
%{__mkdir_p} %{buildroot}%{_sbindir}
%{__mkdir_p} %{buildroot}%{_var}/log/kannel/
%{__mkdir_p} %{buildroot}%{_var}/spool/kannel/

%{__install} -D -m 0640 example/websmsbox.conf.example %{buildroot}%{_sysconfdir}/kannel/websmsbox.conf

strip %{buildroot}%{_sbindir}/websmsbox

%clean
%{__rm} -rf %{buildroot}

#%pre

#%post

#%preun

#%postun


%files
%defattr(-, root, root, 0755)
%doc AUTHORS COPYING ChangeLog NEWS README UPGRADE
%attr(0640, kannel, kannel) %config(noreplace) %{_sysconfdir}/kannel/websmsbox.conf
%{_sbindir}/*
%attr(0750, kannel, kannel) %dir %{_var}/log/kannel/
%attr(0750, kannel, kannel) %dir %{_sysconfdir}/kannel/


%changelog
* Wed Nov 19 2008 Donald Jackson <djackson at kannel dot org>
- First RPM version for Websmsbox
