%define major 3
%define minor 0
%define patchlevel 1

Name:           tel-plugin-dbus_tapi
Version:        %{major}.%{minor}.%{patchlevel}
Release:        1
License:        Apache-2.0
Summary:        dbus-tapi plugin for telephony
Group:          System/Libraries
Source0:        tel-plugin-dbus_tapi-%{version}.tar.gz
Source1001: 	tel-plugin-dbus_tapi.manifest
BuildRequires:  cmake
BuildRequires:  python
BuildRequires:  python-xml
BuildRequires:  pkgconfig(appsvc)
BuildRequires:  pkgconfig(aul)
BuildRequires:  pkgconfig(gio-2.0)
BuildRequires:  pkgconfig(gio-unix-2.0)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(gobject-2.0)
BuildRequires:  pkgconfig(libtzplatform-config)
BuildRequires:  pkgconfig(security-server)
BuildRequires:  pkgconfig(tcore)
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description
dbus-tapi plugin for telephony

%prep
%setup -q
cp %{SOURCE1001} .

%build
versionint=$[%{major} * 1000000 + %{minor} * 1000 + %{patchlevel}]
cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix} -DVERSION=$versionint
make %{?jobs:-j%jobs}

%cmake . \
-DTZ_SYS_USER_GROUP=%TZ_SYS_USER_GROUP

%post
/sbin/ldconfig

%postun -p /sbin/ldconfig

%install
rm -rf %{buildroot}
%make_install
mkdir -p %{buildroot}/usr/share/license
cp LICENSE %{buildroot}/usr/share/license/%{name}

%files
%manifest %{name}.manifest
%defattr(-,root,root,-)
#%doc COPYING
%{_libdir}/telephony/plugins/*
%{_prefix}/etc/dbus-1/system.d/*
/usr/share/license/%{name}
