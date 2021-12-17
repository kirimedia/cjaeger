Name:           cjaeger 
Version:        %{version} 
%define _rel    1
Release:        %{_rel}%{version_suffix}
Source0:        cjaeger-%{current_datetime}.tar.gz
BuildRoot:      %{_tmppath}/cjaeger-%{current_datetime}
Summary:        Jaeger client c 
License:        Apache-2.0
%if 0%{rhel} <= 7
BuildRequires:  devtoolset-7-binutils
BuildRequires:  devtoolset-7-gcc
BuildRequires:  cmake3
%else
BuildRequires:  gcc
BuildRequires:  cmake
%endif
BuildRequires:  jaeger-client-cpp
Requires:       jaeger-client-cpp

%description
Jaeger client c

%prep
[ "%{buildroot}" != "/" ] && rm -fr %{buildroot}
%setup -b 0 -n cjaeger-%{current_datetime}
%if 0%{?el7:1}
source /opt/rh/devtoolset-7/enable
%endif
mkdir build
cd build
%cmake3 $CMAKE_OPTIONS ..

%build
%make_build -C build

%install
%make_install -C build

%clean
[ "%{buildroot}" != "/" ] && rm -rf %{buildroot}

%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%files
%{_includedir}/*
%{_libdir}/*
