Name:           downtimed
Version:        1.0
Release:        1%{?dist}
Summary:         Monitors operating system downtime, shutdowns and crashes on the monitored host itself and to keep a record of such events

License:        Simplified BSD
URL:            https://dist.epipe.com/%{name}/
Source0:        https://github.com/snabb/%{name}/archive/version-%{version}.tar.gz

BuildRequires: gcc make automake autoconf 
%{?systemd_requires}
BuildRequires: systemd

%description
A program for monitoring operating system downtime, uptime, shutdowns and crashes and for keeping record of such events

%prep

%setup -n %{name}-version-%{version}
#%autosetup -n %{name}-version-%{version}

#%setup -q


%build
autoreconf
%configure
make %{?_smp_mflags}


%install
mkdir -p %{buildroot}/var/lib/downtimed
%make_install
%{__mkdir} -p %{buildroot}%{_unitdir}
%{__install} -m644 startup-scripts/downtimed.service \
    %{buildroot}%{_unitdir}/%{name}.service

%post
%systemd_post downtimed.service

%preun
%systemd_preun downtimed.service

%postun
%systemd_postun_with_restart downtimed.service

%files
%license LICENSE
%{_sbindir}/downtimed
%{_bindir}/downtimes
%{_bindir}/downtime
%{_unitdir}/%{name}.service
%dir /var/lib/downtimed
%doc NEWS README.md
/usr/share/man/man1/downtime.1.gz
/usr/share/man/man1/downtimes.1.gz
/usr/share/man/man8/downtimed.8.gz


%changelog
* Tue Jun 19 2018 Jeremy Van Veelen <techgooroo@gmail.com>
- 
