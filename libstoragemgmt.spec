Name:           libstoragemgmt
Version:        0.0.12
Release:        1%{?dist}
Summary:        Storage array management library
Group:          System Environment/Libraries
License:        LGPLv2+
URL:            http://sourceforge.net/projects/libstoragemgmt/
Source0:        http://sourceforge.net/projects/libstoragemgmt/files/Alpha/libstoragemgmt-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  yajl-devel libxml2-devel pywbem check-devel m2crypto glib2-devel
Requires:       pywbem m2crypto

%if 0%{?fedora}
BuildRequires:  systemd-units
Requires: initscripts
Requires(post): systemd-units
Requires(preun): systemd-units
Requires(postun): systemd-units
%endif

%description
The libStorageMgmt library will provide a vendor agnostic open source storage
application programming interface (API) that will allow management of storage
arrays.  The library includes a command line interface for interactive use and
scripting (command lsmcli).  The library also has a daemon that is used for
executing plug-ins in a separate process (lsmd).

%package        devel
Summary:        Development files for %{name}
Group:          Development/Libraries
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description    devel
The %{name}-devel package contains libraries and header files for
developing applications that use %{name}.


%prep
%setup -q

%build
%configure --disable-static
make %{?_smp_mflags}


%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}
find %{buildroot} -name '*.la' -exec rm -f {} ';'

%if 0%{?fedora}
install -d -m755 %{buildroot}/%{_unitdir}
install -m644 packaging/daemon/libstoragemgmt.service %{buildroot}/%{_unitdir}/libstoragemgmt.service

#tempfiles.d configuration for /var/run
mkdir -p %{buildroot}%{_sysconfdir}/tmpfiles.d
install -m 0644 packaging/daemon/lsm-tmpfiles.conf %{buildroot}%{_sysconfdir}/tmpfiles.d/%{name}.conf
%else
#Need these to exist at install so we can start the daemon
mkdir -p %{buildroot}/etc/rc.d/init.d
install packaging/daemon/libstoragemgmtd %{buildroot}/etc/rc.d/init.d/libstoragemgmtd
%endif

#Need these to exist at install so we can start the daemon
mkdir -p %{buildroot}%{_localstatedir}/run/lsm/ipc

%clean
rm -rf %{buildroot}

%pre
getent group libstoragemgmt >/dev/null || groupadd -r libstoragemgmt
getent passwd libstoragemgmt >/dev/null || \
    useradd -r -g libstoragemgmt -d /var/run/lsm -s /sbin/nologin \
    -c "daemon account for libstoragemgmt" libstoragemgmt

%post
/sbin/ldconfig
if [ $1 -eq 1 ]; then
%if 0%{?fedora}
    /bin/systemctl enable libstoragemgmt.service >/dev/null 2>&1 || :
%else
    /sbin/chkconfig --add libstoragemgmtd
%endif
fi

%preun
if [ $1 -eq 0 ]; then
%if 0%{?fedora}
    # On uninstall (not upgrade), disable and stop the units
    /bin/systemctl --no-reload disable libstoragemgmt.service >/dev/null 2>&1 || :
    /bin/systemctl stop libstoragemgmt.service >/dev/null 2>&1 || :
%else
    /etc/rc.d/init.d/libstoragemgmtd stop > /dev/null 2>&1 || :
    /sbin/chkconfig --del libstoragemgmtd
%endif
fi

%postun
/sbin/ldconfig
/bin/systemctl daemon-reload >/dev/null 2>&1 || :
if [ $1 -ge 1 ] ; then
%if 0%{?fedora}
    # On upgrade (not uninstall), optionally, restart the daemon
    /bin/systemctl try-restart libstoragemgmt.service >/dev/null 2>&1 || :
%else
    #Restart the daemond
    /etc/rc.d/init.d/libstoragemgmtd restart  >/dev/null 2>&1 || :
%endif
fi

%files
%defattr(-,root,root,-)
%doc README COPYING.LIB
%{_mandir}/man1/lsmcli.1*
%{_mandir}/man1/lsmd.1*
%{_libdir}/*.so.*
%{_bindir}/*

#Python library files
%{python_sitelib}/*

%if 0%{?fedora}
%{_unitdir}/*
%endif

%dir %attr(0755, libstoragemgmt, libstoragemgmt) %{_localstatedir}/run/lsm/
%dir %attr(0755, libstoragemgmt, libstoragemgmt) %{_localstatedir}/run/lsm/ipc

%if 0%{?fedora}
%config(noreplace) %{_sysconfdir}/tmpfiles.d/%{name}.conf
%else
%attr(0755, root, root) /etc/rc.d/init.d/libstoragemgmtd
%endif

%files devel
%defattr(-,root,root,-)
%{_includedir}/*
%{_libdir}/*.so
%{_libdir}/pkgconfig/libstoragemgmt.pc

%changelog
* Fri Sep 07 2012 Tony Asleson (Red Hat) <tasleson@redhat.com>
- SMI-S plug-in enhancements (Detach before delete, bug fixes for eSeries)
- Added version specifier for non-opaque structs in plug-in callback interface
- Documentation updates (doxygen, man pages)
- Ontap plug-in: support timeout values
- lsmcli, return back async. values other than volumes when using --job-status

* Mon Aug 13 2012 Tony Asleson <tasleson@redhat.com> 0.0.11-1
- SMI-S fixes and improvements (WaitForCopyState, _get_class_instance)
- Methods for arrays that don't support access groups to grant access
  for luns to initiators etc.
- ISCSI Chap authentication
- System level status field for overall array status
- targetd updates for mapping targets to initiators
- Simulator updates (python & C)
- Removed tog-pegasus dependency (SMI-S is python plug-in)
- Removed lsmVolumeStatus as it was implemented and redundant
- initscript, check for /var/run and create if missing

* Fri Jul 20 2012 Tony Asleson <tasleson@redhat.com> 0.0.10-1
- Numerous updates and re-name for plug-in targetd_lsmplugin
- targetd_lsmplugin included in release
- Memory leak fixes and improved unit tests
- Initial capability query support, implemented for all plug-ins
- Flags variable added to API calls, (Warning: C API/ABI breakage, python
  unaffected)
- Bug fixes for NetApp ontap plug-in
- SMI-S bug fixes (initiator listing and replication, mode and sync types)
- Added ability to specify mirroring async or sync for replication
- Added version header file to allow client version header checks
- Simulator plug-in written in C, simc_lsmplugin is available

* Tue Jun 12 2012 Tony Asleson <tasleson@redhat.com> 0.0.9-1
- Initial checkin of lio plug-in
- System filtering via URI (smispy)
- Error code mapping (ontap)
- Fixed build so same build tarball is used for all binaries

* Mon Jun 4 2012 Tony Asleson <tasleson@redhat.com> 0.0.8-1
- Make building of SMI-S CPP plugin optional
- Add pkg-config file
- SMIS: Fix exception while retrieving Volumes
- SMIS: Fix exception while retrieving Volumes
- lsm: Add package imports
- Make Smis class available in lsm python package
- Add option to disable building C unit test
- Make simulator classes available in lsm python package
- Make ontap class available in lsm python package
- Changes to support building on Fedora 17 (v2)
- Spec. file updates from feedback from T. Callaway (spot)
- F17 linker symbol visibility correction
- Remove unneeded build dependencies and cleaned up some warnings
- C Updates, client C library feature parity with python

* Fri May 11 2012 Tony Asleson <tasleson@redhat.com> 0.0.7-1
- Bug fix for smi-s constants
- Display formatting improvements
- Added header option for lsmcli
- Improved version handling for builds
- Made terminology consistent
- Ability to list visibility for access groups and volumes
- Simulator plug-in fully supports all block operations
- Added support for multiple systems with a single plug-in instance

* Fri Apr 20 2012 Tony Asleson <tasleson@redhat.com> 0.0.6-1
- Documentation improvements (man & source code)
- Support for access groups
- Unified spec files Fedora/RHEL
- Package version auto generate
- Rpm target added to make
- Bug fix for missing optional property on volume retrieval (smispy plug-in)

* Fri Apr 6 2012 Tony Asleson <tasleson@redhat.com> 0.0.5-1
- Spec file clean-up improvements
- Async. operation added to lsmcli and ability to check on job status
- Sub volume replication support
- Ability to check for child dependencies on VOLUMES, FS and files
- SMI-S Bug fixes and improvements

* Mon Mar 26 2012 Tony Asleson <tasleson@redhat.com> 0.0.4-1
- Restore from snapshot
- Job identifiers string instead of integer
- Updated license address

* Wed Mar 14 2012 Tony Asleson <tasleson@redhat.com> 0.0.3-1
- Changes to installer, daemon uid, gid, /var/run/lsm/*
- NFS improvements and bug fixes
- Python library clean up (rpmlint errors)

* Sun Mar 11 2012 Tony Asleson <tasleson@redhat.com> 0.0.2-1
- Added NetApp native plugin

* Mon Feb 6 2012 Tony Asleson <tasleson@redhat.com>  0.0.1alpha-1
- Initial version of package
