Name:       tel-plugin-dbus_tapi
Summary:    dbus-tapi plugin for telephony
Version:    0.1.10
Release:    1
Group:      System/Libraries
License:    Apache
Source0:    tel-plugin-dbus_tapi-%{version}.tar.gz
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

%build
cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix}
make %{?jobs:-j%jobs}

%post
/sbin/ldconfig

%postun -p /sbin/ldconfig

%install
rm -rf %{buildroot}
%make_install

%files
%defattr(-,root,root,-)
#%doc COPYING
%{_libdir}/telephony/plugins/*
%{_prefix}/etc/dbus-1/system.d/*
