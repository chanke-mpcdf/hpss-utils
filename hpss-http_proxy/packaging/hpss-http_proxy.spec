# norootforbuild  

Name:           hpss-http_proxy
Group:          Productivity/Archiving/Backup
Summary:        Exposes the HPSS API over HTTP
Version:        0.1 
Release:        3 
License:        UNDISCLOSED
Source0:        hpss-http_proxy.tbz2
BuildRoot:      %{_tmppath}/%{name}-%{version}-build  
BuildRequires:  hpss-client-devel
BuildRequires:  doxygen
BuildRequires:  ksh
BuildRequires:  zlib
BuildRequires:  libevent-devel  > 2
BuildRequires:  krb5-devel 
Requires:       hpss-client = 7.4
Provides:      	hpss-http_proxy

%description
Simple webserver which exposes the HPSS client API over HTTP.
Authentication to HPSS is done at startup.

%prep
%setup -q  -T -b 0 -n %{name}

%build
make 
 
%install
mkdir -p $RPM_BUILD_ROOT/usr/bin
cp src/hpss-http_proxy $RPM_BUILD_ROOT/usr/bin
mkdir -p $RPM_BUILD_ROOT/usr/share/hpss-http_proxy
cp README $RPM_BUILD_ROOT/usr/share/hpss-http_proxy

%clean
rm -rf $RPM_BUILD_ROOT  

%files
%defattr(-, root, root)
/usr/bin/hpss-http_proxy
/usr/share/hpss-http_proxy

%changelog
