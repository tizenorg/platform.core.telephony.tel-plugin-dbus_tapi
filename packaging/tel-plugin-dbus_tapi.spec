Name:       tel-plugin-dbus_tapi
Summary:    dbus-tapi plugin for telephony
Version:    0.1.10
Release:    1
Group:      System/Libraries
License:    Apache
Source0:    tel-plugin-dbus_tapi-%{version}.tar.gz
Source1001: packaging/tel-plugin-dbus_tapi.manifest 
Patch0:     0001-Support-sysconfdir-in-the-makefile.patch
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  cmake
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(tcore)
BuildRequires:  pkgconfig(dbus-1)
BuildRequires:  pkgconfig(dbus-glib-1)
BuildRequires:  pkgconfig(gthread-2.0)
BuildRequires:  pkgconfig(aul)
BuildRequires:  pkgconfig(tapi)
BuildRequires:  pkgconfig(security-server)

%description
dbus-tapi plugin for telephony

%prep
%setup -q
%patch0 -p1

%build
cp %{SOURCE1001} .
cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix} -DSYSCONFDIR=%{_sysconfdir}
make %{?_smp_mflags}

%install
%make_install

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%manifest tel-plugin-dbus_tapi.manifest
%{_libdir}/telephony/plugins/*
%{_sysconfdir}/dbus-1/system.d/*
