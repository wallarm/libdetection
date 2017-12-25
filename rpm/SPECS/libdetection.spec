Name: libdetection
Version: 0.2.2
Release: 5
Summary: Signature-free approach library to detect attacks

License: BSD
URL: https://github.com/wallarm/libdetection

Source: libdetection-%{version}.tar.gz

BuildRequires: /usr/bin/gcc
BuildRequires: cmake
BuildRequires: CUnit-devel
BuildRequires: bison >= 3.0
BuildRequires: re2c
BuildRequires: libavl2-devel

Group: System Environment/Libraries

%description
Extendable library for detection syntaxes by formal notations. Can be used to
detect injections and commanding attacks such as SQLi and others. Does not
require attacks samples to learn.


%package devel
Summary: Development files for library to detect attacks
Group: Development/Libraries
Requires: libdetection = %{version}-%{release}
Requires: libavl2-devel

%description devel
Extendable library for detection syntaxes by formal notations. Can be used to
detect injections and commanding attacks such as SQLi and others. Does not
require attacks samples to learn.

%package utils
Summary: Utilities for signature-free approach library to detect attacks
Group: Development/Tools
Requires: libdetection = %{version}-%{release}

%description utils
Extendable library for detection syntaxes by formal notations. Can be used to
detect injections and commanding attacks such as SQLi and others. Does not
require attacks samples to learn.

%prep
%setup -c

%build
./config -DCMAKE_INSTALL_PREFIX:PATH=%{_prefix} -DCMAKE_INSTALL_LIBDIR=%{_libdir}
%{__make} -C build

%install
%{__make} -C build install DESTDIR=%{buildroot}

%files
%{_libdir}/*.so.*

%files devel
%{_libdir}/*.so
%{_includedir}/detect/

%files utils
%{_bindir}/libdetection_perf

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%changelog
* Mon Dec 25 2017 Alexey Temnikov <atemnikov@wallarm.com> 0.2.2-1
- no changes yet
* Mon Dec 25 2017 Alexey Temnikov <atemnikov@wallarm.com> 0.2.1-1
- Fixed crashes with incorrect data (Closes: NODE-1081)
* Wed Apr 12 2017 Alexander Golovko <ag@wallarm.com> 0.2.0-4
- Fix devel package dependencies.
* Wed Apr 12 2017 Alexander Golovko <ag@wallarm.com> 0.2.0-2
- Initial release.
