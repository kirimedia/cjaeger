Name:           cjaeger 
Version:        %{version} 
%define _rel    1
Release:        %{_rel}%{version_suffix}
Source0:        cjaeger-%{current_datetime}.tar.gz
BuildRoot:      %{_tmppath}/cjaeger-%{current_datetime}
Summary:        Jaeger client c 
License:        Apache-2.0
%if 0%{?el7:1}
BuildRequires:  devtoolset-7-binutils
BuildRequires:  devtoolset-7-gcc
%else
BuildRequires:  gcc
%endif
BuildRequires:  cmake3
BuildRequires:  jaeger-client-cpp
Requires:       jaeger-client-cpp

%description
Jaeger client c

%define prefix /usr

%prep
[ "%{buildroot}" != "/" ] && rm -fr %{buildroot}
%setup -b 0 -n cjaeger-%{current_datetime}
%if 0%{?el7:1}
source /opt/rh/devtoolset-7/enable
%endif
mkdir build
cd build
cmake3 -DCMAKE_INSTALL_PREFIX=%{prefix} .. 

%build
cd build
make

%install
cd build
make DESTDIR=%{buildroot} PREFIX=%{prefix} MULTILIB=%{multilib} install

%clean
[ "%{buildroot}" != "/" ] && rm -rf %{buildroot}

%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%files
%{prefix}/include/*
%{prefix}/lib64/*
