Name: libdetection1
Version: 1.2.1
Release: 3
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
./config -DCMAKE_INSTALL_PREFIX:PATH=%{_prefix} -DCMAKE_INSTALL_LIBDIR=%{_libdir} -DENABLE_SHARED=1 -DENABLE_STATIC=1
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

%changelog
* Thu Dec 05 2019 Dmitry Murzin <dmurzin@wallarm.com> - 1.2.1-3
- Rebuilt for CentOS 8 (NODE-1790)

* Thu Nov 21 2019 Dmitry Murzin <dmurzin@wallarm.com> - 1.2.1-2
- Bump version

* Mon Nov 12 2019 Alexander Drozdov <adrozdov@wallarm.com> - 1.2.1-1
- NODE-2552: update version to force upgrade from Node 2.12 to Node 2.14

* Mon Nov 11 2019 Alexander Drozdov <adrozdov@wallarm.com> - 1.2.0-3
- NODE-2552: Internal rebuild

* Mon Jul 29 2019 Dmitry Murzin <dmurzin@wallarm.com> - 1.2.0-2
- Added: build for debian buster (Closes: NODE-1977)

* Mon Jul 22 2019 Oleg Trifonov <otrifonov@wallarm.com> - 1.2.0-1
- NODE-2044: Internal rebuild

* Wed Jul 10 2019 Oleg Trifonov <otrifonov@wallarm.com> - 1.1.0-2
- NODE-1948: Internal rebuild

* Mon Mar 18 2019 Oleg Trifonov <otrifonov@wallarm.com> 1.1.0-1
- Improved SQLi attack detection (Closes: NODE-1445)
* Tue Feb 13 2018 Alexey Temnikov <atemnikov@wallarm.com> 1.0.0-2
- fixed packages
* Mon Feb 12 2018 Alexey Temnikov <atemnikov@wallarm.com> 1.0.0-1
- removed libavl2 dependency (Closes: NODE-1131)
* Mon Dec 25 2017 Alexey Temnikov <atemnikov@wallarm.com> 0.2.1-1
- Fixed crashes with incorrect data (Closes: NODE-1081)
* Wed Apr 12 2017 Alexander Golovko <ag@wallarm.com> 0.2.0-4
- Fix devel package dependencies.
* Wed Apr 12 2017 Alexander Golovko <ag@wallarm.com> 0.2.0-2
- Initial release.
