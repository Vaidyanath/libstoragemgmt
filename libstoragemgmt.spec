Name:           libstoragemgmt
Version:        0.0.20
Release:        1%{?dist}
Summary:        Storage array management library
Group:          System Environment/Libraries
License:        LGPLv2+
URL:            http://sourceforge.net/projects/libstoragemgmt/
Source0:        http://sourceforge.net/projects/libstoragemgmt/files/Alpha/libstoragemgmt-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  yajl-devel libxml2-devel pywbem check-devel m2crypto glib2-devel python-paramiko
Requires:       pywbem m2crypto %{name}-python python-paramiko

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

%package        python
Summary:        Python client libraries and plug-in support for %{name}
Group:          System Environment/Libraries
Requires:       %{name} = %{version}-%{release}
BuildArch:      noarch

%description	python
The %{name}-python package contains python client libraries as
well as python framework support and open source plug-ins written in python.

%prep
%setup -q

%build
#The version.py gets created so set the .py to match .py.in times
UpdateTimestamps(){
	touch -r lsm/lsm/version.py.in lsm/lsm/version.py
}

#Tell the install program to preserve file date/timestamps
%configure --disable-static INSTALL_DATA="\${INSTALL} -p"

UpdateTimestamps

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

%files python
%defattr(-,root,root,-)
#Python library files
%{python_sitelib}/*

%changelog
* Fri May 24 2013 Tony Asleson <tasleson@redhat.com> 0.0.20-1
- Python library files now in separate rpm
- Additional debug for plug-ins when exceptions occur
- iSCSI CHAP support modified to handle both inbound and outbound authentication
- VOLUME_THIN Added as new capability flag
- IBM V7000 storage array support
- NFS export support for targetd
- EXPORT_CUSTOM_PATH added capability flag

* Sat Apr 20 2013 Tony Asleson <tasleson@redhat.com> 0.0.19-1
- Improved E-Series array support
- Ontap plug-in: improve performance with many Volumes
- lsmcli: Number of corrections on handling unit specifiers
- lsmcli: Correct stack track when stdout is written to while closed
- Fix build to work with automake >= 1.12

* Thu Mar 7 2013 Tony Asleson <tasleson@redhat.com> 0.0.18-1
- lsmd: Re-written in C
- Simplify fs_delete
- Corrections for C client against Python plugin
- Testing: Run cross language unit test too
- Initial FS support for targetd plugin
- Fix multi-arch python issues which prevent py and compiled py files
  from being identical on different arches

* Thu Jan 31 2013 Tony Asleson <tasleson@redhat.com> 0.0.17-1
- Inconsistency corrections between C and Python API
- Source code documentation updates
- NexentaStor plug-in has been added

* Wed Jan 2 2013 Tony Asleson <tasleson@redhat.com> 0.0.16-1
- lsmcli: Add confirmation prompt for data loss operations
- lsmcli: Display enumerated values as text
- lsmcli: Exit with 7 for --job-status when not complete
- Fixed URI example to reference an existing plug-in
- lsmcli: Retrieve plug-in desc. and version (lsmcli --plugin-info)
- simc: Implement CHAP auth function (no-op)
- lsmcli: Change check for determining if lsmd is running
- Disable mirroring for SMI-S as it needs some re-work

* Mon Nov 19 2012 Tony Asleson <tasleson@redhat.com> 0.0.15-1
- Pool parameter is optional when replicating a volume
- Code improvements(Memory leak fix, lsmcli checks if lsmd is running)
- Source code documentation updates
- Ability to override simulator data storage location
- make check target added to run unit tests

* Fri Oct 19 2012 Tony Asleson <tasleson@redhat.com> 0.0.14-1
- test/cmdline.py added to automatically test what an array supports
- Bug fixes (local plug-in execution, smi-s delete clone, code warnings)
- targetd: (uri syntax consistency change, initialization code change)
- Pool id added to volume information
- lsmcli: Added --replicate-volume-range-block-size <system id> to retrieve
  replicated block size

* Fri Sep 28 2012 Tony Asleson (Red Hat) <tasleson@redhat.com> 0.0.13-1
- targetD Feature adds/fixes for initiators, init_granted_to_volume,
  volumes_accessible_by_init, initiator_grant, initiator_revoke
- SMI-S added compatibility with CIM_StorageConfigurationService
- SMI-S bug fixes/changes to support XIV arrays (Basic functionality verified)
- SMI-S Proxy layer added to allow different internal implementations of smi-s
  client
- Added missing version information for C plug-in API
- lsmcli URI can be stored in file .lsmcli in users home directory

* Fri Sep 07 2012 Tony Asleson (Red Hat) <tasleson@redhat.com> 0.0.12-1
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
