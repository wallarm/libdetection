Name: libdetection1
Version: 1.2.2
Release: wallarm3.4+1%{?dist}
Summary: Signature-free approach library to detect attacks

License: BSD
URL: https://github.com/wallarm/libdetection

Source: libdetection-%{version}.tar.gz

BuildRequires: /usr/bin/gcc
BuildRequires: cmake
BuildRequires: CUnit-devel
BuildRequires: bison >= 3.0
BuildRequires: re2c
BuildRequires: libwallarmmisc-devel

Group: System Environment/Libraries

%description
Extendable library for detection syntaxes by formal notations. Can be used to
detect injections and commanding attacks such as SQLi and others. Does not
require attacks samples to learn.


%package devel
Summary: Development files for library to detect attacks
Group: Development/Libraries
Requires: libdetection1 = %{version}-%{release}
Requires: libwallarmmisc-devel
Conflicts: libdetection-devel

%description devel
Extendable library for detection syntaxes by formal notations. Can be used to
detect injections and commanding attacks such as SQLi and others. Does not
require attacks samples to learn.

%package static
Summary: Static libraries to detect attacks
Group: Development/Libraries

%description static
Extendable library for detection syntaxes by formal notations. Can be used to
detect injections and commanding attacks such as SQLi and others. Does not
require attacks samples to learn.

%package utils
Summary: Utilities for signature-free approach library to detect attacks
Group: Development/Tools
Requires: libdetection1 = %{version}-%{release}
Conflicts: libdetection-utils

%description utils
Extendable library for detection syntaxes by formal notations. Can be used to
detect injections and commanding attacks such as SQLi and others. Does not
require attacks samples to learn.

%prep
%setup -c

%build
./config -DCMAKE_INSTALL_PREFIX:PATH=%{_prefix} -DCMAKE_INSTALL_LIBDIR=%{_libdir} -DENABLE_SHARED=1 -DENABLE_STATIC=1 -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON
%{__make} -C build

%install
%{__make} -C build install DESTDIR=%{buildroot}

%define __os_install_post \
    /usr/lib/rpm/brp-compress; \
    /usr/lib/rpm/brp-strip; \
    /usr/lib/rpm/brp-strip-comment-note; \
%{nil}

%files
%{_libdir}/*.so.*

%files devel
%{_libdir}/*.so
%{_includedir}/detect/

%files static
%{_libdir}/*.a

%files utils
%{_bindir}/libdetection_perf

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig
