%bcond_with rest_api

# Use one-line macro for OBS workaround:
#   https://bugzilla.novell.com/show_bug.cgi?id=864323
%{?_with_rest_api: %global with_rest_api 1 }
%{?_without_rest_api: %global with_rest_api 0 }

%define libsoname libstoragemgmt

%if 0%{?suse_version} || 0%{?fedora} >= 15 || 0%{?rhel} >= 7
%define with_systemd 1
%endif

%global libstoragemgmt libstoragemgmt

%if 0%{?suse_version}
%global libstoragemgmt libstoragemgmt1
%endif

%define udev_dir /lib
# Later versions moved /lib to /usr/lib
%if 0%{?fedora} >= 18 || 0%{?rhel} >= 7 || 0%{?suse_version}
%define udev_dir /usr/lib
%endif

%if 0%{?suse_version}
# Use fdupes on openSuSE.
# For Fedora, it will conflict with brp-python-bytecompile
# For RHEL, fdupes is in EPEL repo.
%define do_fdupes 1
%endif

Name:           libstoragemgmt
Version:        1.1.0
Release:        1%{?dist}
Summary:        Storage array management library
Group:          System Environment/Libraries
%if 0%{?suse_version}
License:        LGPL-2.1+
%else
License:        LGPLv2+
%endif
URL:            http://sourceforge.net/projects/libstoragemgmt/
Source0:        http://sourceforge.net/projects/libstoragemgmt/files/libstoragemgmt-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
Requires:       %{libstoragemgmt}-python
BuildRequires:  autoconf automake libtool libxml2-devel check-devel perl
BuildRequires:  openssl-devel
BuildRequires:  python-argparse
BuildRequires:  glib2-devel
# Explicitly require gcc-c++ is for OBS
BuildRequires:  gcc-c++
%if 0%{?suse_version}
BuildRequires:  libyajl-devel
BuildRequires:  python-PyYAML
%else
# Fedora RHEL
BuildRequires:  yajl-devel
BuildRequires:  PyYAML
%endif

%if 0%{?do_fdupes}
BuildRequires:  fdupes
%endif

%if 0%{?rhel} == 6
BuildRequires:  python-ordereddict
%endif

%if 0%{?with_systemd}
BuildRequires:  systemd
Requires(post): systemd
Requires(preun): systemd
Requires(postun): systemd
%endif

%description
The libStorageMgmt library will provide a vendor agnostic open source storage
application programming interface (API) that will allow management of storage
arrays.  The library includes a command line interface for interactive use and
scripting (command lsmcli).  The library also has a daemon that is used for
executing plug-ins in a separate process (lsmd).

%if %{libstoragemgmt} != %{name}
%package        -n %{libstoragemgmt}
Summary:        Storage array management library
Group:          System Environment/Libraries
Requires:       %{libstoragemgmt}-python

%description    -n %{libstoragemgmt}
The libStorageMgmt library will provide a vendor agnostic open source storage
application programming interface (API) that will allow management of storage
arrays.  The library includes a command line interface for interactive use and
scripting (command lsmcli).  The library also has a daemon that is used for
executing plug-ins in a separate process (lsmd).
%endif

%package        devel
Summary:        Development files for %{name}
Group:          Development/Libraries
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description    devel
The %{name}-devel package contains libraries and header files for
developing applications that use %{name}.

%package        -n %{libstoragemgmt}-python
Summary:        Python client libraries and plug-in support for %{libstoragemgmt}
Group:          System Environment/Libraries
Requires:       %{libstoragemgmt} = %{version}-%{release}
BuildArch:      noarch
Requires:       python-argparse
%if 0%{?rhel} == 6
# No way to detect 6.2 yet. Just forcing all RHEL 6 to install
# python-ordereddict just in case.
Requires:       python-ordereddict
%endif

%description    -n %{libstoragemgmt}-python
The %{libstoragemgmt}-python package contains python client libraries as
well as python framework support and open source plug-ins written in python.


%package        -n %{libstoragemgmt}-smis-plugin
Summary:        Files for SMI-S generic array support for %{libstoragemgmt}
Group:          System Environment/Libraries
%if 0%{?suse_version}
BuildRequires:  python-pywbem
Requires:       python-pywbem
%else
BuildRequires:  pywbem
Requires:       pywbem
%endif
Requires:       %{libstoragemgmt}-python = %{version}-%{release}
BuildArch:      noarch

%description    -n %{libstoragemgmt}-smis-plugin
The %{libstoragemgmt}-smis-plugin package contains plug-in for generic SMI-S
array support.


%package        -n %{libstoragemgmt}-netapp-plugin
Summary:        Files for NetApp array support for %{libstoragemgmt}
Group:          System Environment/Libraries
%if 0%{?suse_version}
BuildRequires:  python-M2Crypto
Requires:       python-M2Crypto
%else
BuildRequires:  m2crypto
Requires:       m2crypto
%endif
Requires:       %{libstoragemgmt}-python = %{version}-%{release}
BuildArch:      noarch

%description    -n %{libstoragemgmt}-netapp-plugin
The %{libstoragemgmt}-netapp-plugin package contains plug-in for NetApp array
support.


%package        -n %{libstoragemgmt}-targetd-plugin
Summary:        Files for targetd array support for %{libstoragemgmt}
Group:          System Environment/Libraries
Requires:       %{libstoragemgmt}-python = %{version}-%{release}
BuildArch:      noarch

%description    -n %{libstoragemgmt}-targetd-plugin
The %{libstoragemgmt}-targetd-plugin package contains plug-in for targetd
array support.


%package        -n %{libstoragemgmt}-nstor-plugin
Summary:        Files for NexentaStor array support for %{libstoragemgmt}
Group:          System Environment/Libraries
Requires:       %{libstoragemgmt}-python = %{version}-%{release}
BuildArch:      noarch

%description    -n %{libstoragemgmt}-nstor-plugin
The %{libstoragemgmt}-nstor-plugin package contains plug-in for NexentaStor
array support.

%package        udev
Summary:        Udev files for %{name}
Group:          System Environment/Base

%description    udev
The %{name}-udev package contains udev rules and helper utilities for
uevents generated by the kernel.

%if 0%{?with_rest_api}
%package        -n %{libstoragemgmt}-rest
Summary:        REST API daemon for %{libstoragemgmt}
Group:          System Environment/Daemons
Requires:       %{libstoragemgmt}%{?_isa} = %{version}-%{release}
BuildRequires:  libmicrohttpd-devel
%if 0%{?suse_version}
BuildRequires:  libjson-devel procps
Requires:       libjson0
Requires:       libmicrohttpd10
%else
# Fedora RHEL
BuildRequires:  json-c-devel
Requires:       json-c
Requires:       libmicrohttpd
%endif

%description    -n %{libstoragemgmt}-rest
the %{libstoragemgmt}-rest package contains the http daemon for
%{libstoragemgmt} rest api.
%endif

%prep
%setup -q

%build
./autogen.sh

%configure \
%if 0%{?with_rest_api} != 1
    --without-rest-api \
%endif
    --disable-static

V=1 make %{?_smp_mflags}

%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}
find %{buildroot} -name '*.la' -exec rm -f {} ';'

%if 0%{?with_systemd}
install -d -m755 %{buildroot}/%{_unitdir}
install -m644 packaging/daemon/libstoragemgmt.service \
    %{buildroot}/%{_unitdir}/libstoragemgmt.service

#tempfiles.d configuration for /var/run
mkdir -p %{buildroot}/%{_tmpfilesdir}
install -m 0644 packaging/daemon/lsm-tmpfiles.conf \
    %{buildroot}/%{_tmpfilesdir}/%{name}.conf
%else
#Need these to exist at install so we can start the daemon
mkdir -p %{buildroot}/etc/rc.d/init.d
install packaging/daemon/libstoragemgmtd \
    %{buildroot}/etc/rc.d/init.d/libstoragemgmtd
%endif

#Files for udev handling
mkdir -p %{buildroot}/%{udev_dir}/udev/rules.d
install -m 644 tools/udev/90-scsi-ua.rules \
    %{buildroot}/%{udev_dir}/udev/rules.d/90-scsi-ua.rules
install -m 755 tools/udev/scan-scsi-target \
    %{buildroot}/%{udev_dir}/udev/scan-scsi-target

%if 0%{?with_rest_api}
%if 0%{?with_systemd}
%{__install} -m 0644 packaging/daemon/libstoragemgmt-rest.service \
    %{buildroot}/%{_unitdir}/libstoragemgmt-rest.service
%endif
%endif

# Deduplication
%if 0%{?do_fdupes}
%fdupes -s %{buildroot}/%{python_sitelib}/lsm
%endif

%clean
rm -rf %{buildroot}

%check
if ! make check
then
  cat test/test-suite.log || true
  exit 1
fi

%pre -n %{libstoragemgmt}
if [ $1 -eq 1 ]; then
    #New install.
    getent group libstoragemgmt >/dev/null || groupadd -r libstoragemgmt
    getent passwd libstoragemgmt >/dev/null || \
        useradd -r -g libstoragemgmt -d %{_localstatedir}/run/lsm \
        -s /sbin/nologin \
        -c "daemon account for libstoragemgmt" libstoragemgmt
    #Need these to exist at install so we can start the daemon
    mkdir -p %{_localstatedir}/run/lsm/ipc
    chmod 0755 %{_localstatedir}/run/lsm
    chmod 0755 %{_localstatedir}/run/lsm/ipc
    chown libstoragemgmt:libstoragemgmt %{_localstatedir}/run/lsm
    chown libstoragemgmt:libstoragemgmt %{_localstatedir}/run/lsm/ipc
fi

%post -n %{libstoragemgmt}
/sbin/ldconfig
%if 0%{?with_systemd}
%if 0%{?suse_version}
    %service_add_post libstoragemgmt.service
%else
    %systemd_post libstoragemgmt.service
%endif
%else
    /sbin/chkconfig --add libstoragemgmtd
%endif

%preun -n %{libstoragemgmt}
%if 0%{?with_systemd}
%if 0%{?suse_version}
    %service_del_preun libstoragemgmt.service
%else
    %systemd_preun libstoragemgmt.service
%endif
%else
    /etc/rc.d/init.d/libstoragemgmtd stop > /dev/null 2>&1 || :
    /sbin/chkconfig --del libstoragemgmtd
%endif

%postun -n %{libstoragemgmt}
/sbin/ldconfig
%if 0%{?with_systemd}
%if 0%{?suse_version}
    %service_del_postun libstoragemgmt.service
%else
    %systemd_postun libstoragemgmt.service
%endif
%else
    #Restart the daemond
    /etc/rc.d/init.d/libstoragemgmtd restart  >/dev/null 2>&1 || :
%endif

%if 0%{?with_rest_api}
%post -n %{libstoragemgmt}-rest
%if 0%{?with_systemd}
%if 0%{?suse_version}
    %service_add_post libstoragemgmt-rest.service
%else
    %systemd_post libstoragemgmt-rest.service
%endif
%endif

%preun -n %{libstoragemgmt}-rest
%if 0%{?with_systemd}
%if 0%{?suse_version}
    %service_del_preun libstoragemgmt-rest.service
%else
    %systemd_preun libstoragemgmt-rest.service
%endif
%endif

%postun -n %{libstoragemgmt}-rest
%if 0%{?with_systemd}
%if 0%{?suse_version}
    %service_del_postun libstoragemgmt-rest.service
%else
    %systemd_postun libstoragemgmt-rest.service
%endif
%endif
%endif

# Need to restart lsmd if plugin is new installed or removed.
%post -n %{libstoragemgmt}-smis-plugin
if [ $1 -eq 1 ]; then
    # New install.
    /usr/bin/systemctl try-restart libstoragemgmt.service \
        >/dev/null 2>&1 || :
fi

%postun -n %{libstoragemgmt}-smis-plugin
if [ $1 -eq 0 ]; then
    # Remove
    /usr/bin/systemctl try-restart libstoragemgmt.service \
        >/dev/null 2>&1 || :
fi

# Need to restart lsmd if plugin is new installed or removed.
%post -n %{libstoragemgmt}-netapp-plugin
if [ $1 -eq 1 ]; then
    # New install.
    /usr/bin/systemctl try-restart libstoragemgmt.service \
        >/dev/null 2>&1 || :
fi

%postun -n %{libstoragemgmt}-netapp-plugin
if [ $1 -eq 0 ]; then
    # Remove
    /usr/bin/systemctl try-restart libstoragemgmt.service \
        >/dev/null 2>&1 || :
fi

# Need to restart lsmd if plugin is new installed or removed.
%post -n %{libstoragemgmt}-targetd-plugin
if [ $1 -eq 1 ]; then
    # New install.
    /usr/bin/systemctl try-restart libstoragemgmt.service \
        >/dev/null 2>&1 || :
fi

%postun -n %{libstoragemgmt}-targetd-plugin
if [ $1 -eq 0 ]; then
    # Remove
    /usr/bin/systemctl try-restart libstoragemgmt.service \
        >/dev/null 2>&1 || :
fi

# Need to restart lsmd if plugin is new installed or removed.
%post -n %{libstoragemgmt}-nstor-plugin
if [ $1 -eq 1 ]; then
    # New install.
    /usr/bin/systemctl try-restart libstoragemgmt.service \
        >/dev/null 2>&1 || :
fi

%postun -n %{libstoragemgmt}-nstor-plugin
if [ $1 -eq 0 ]; then
    # Remove
    /usr/bin/systemctl try-restart libstoragemgmt.service \
        >/dev/null 2>&1 || :
fi

%files -n %{libstoragemgmt}
%defattr(-,root,root,-)
%doc README COPYING.LIB
%{_mandir}/man1/lsmcli.1*
%{_mandir}/man1/lsmd.1*
%{_libdir}/*.so.*
%{_bindir}/lsmcli
%{_bindir}/lsmd
%{_bindir}/simc_lsmplugin

%if 0%{?with_systemd}
%{_unitdir}/libstoragemgmt.service
%endif

%if 0%{?with_systemd}
%attr(0644, root, root)  %{_tmpfilesdir}/%{name}.conf
%else
%attr(0755, root, root) /etc/rc.d/init.d/libstoragemgmtd
%endif

%files devel
%defattr(-,root,root,-)
%{_includedir}/*
%{_libdir}/*.so
%{_libdir}/pkgconfig/libstoragemgmt.pc

%files -n %{libstoragemgmt}-python
%defattr(-,root,root,-)
#Python library files
%dir %{python_sitelib}/lsm
%{python_sitelib}/lsm/__init__.*
%dir %{python_sitelib}/lsm/external
%{python_sitelib}/lsm/external/*
%{python_sitelib}/lsm/_client.*
%{python_sitelib}/lsm/_common.*
%{python_sitelib}/lsm/_data.*
%{python_sitelib}/lsm/_iplugin.*
%{python_sitelib}/lsm/_pluginrunner.*
%{python_sitelib}/lsm/_transport.*
%{python_sitelib}/lsm/version.*
%dir %{python_sitelib}/lsm/plugin
%{python_sitelib}/lsm/plugin/__init__.*
%dir %{python_sitelib}/lsm/plugin/sim
%{python_sitelib}/lsm/plugin/sim/__init__.*
%{python_sitelib}/lsm/plugin/sim/simulator.*
%{python_sitelib}/lsm/plugin/sim/simarray.*
%dir %{python_sitelib}/lsm/lsmcli
%{python_sitelib}/lsm/lsmcli/__init__.*
%{python_sitelib}/lsm/lsmcli/data_display.*
%{python_sitelib}/lsm/lsmcli/cmdline.*
%{_bindir}/sim_lsmplugin

%files -n %{libstoragemgmt}-smis-plugin
%defattr(-,root,root,-)
%dir %{python_sitelib}/lsm/plugin/smispy
%{python_sitelib}/lsm/plugin/smispy/__init__.*
%{python_sitelib}/lsm/plugin/smispy/smis.*
%{python_sitelib}/lsm/plugin/smispy/dmtf.*
%{python_sitelib}/lsm/plugin/smispy/utils.*
%{python_sitelib}/lsm/plugin/smispy/smis_common.*
%{python_sitelib}/lsm/plugin/smispy/smis_cap.*
%{python_sitelib}/lsm/plugin/smispy/smis_sys.*
%{python_sitelib}/lsm/plugin/smispy/smis_pool.*
%{python_sitelib}/lsm/plugin/smispy/smis_disk.*
%{python_sitelib}/lsm/plugin/smispy/smis_vol.*
%{python_sitelib}/lsm/plugin/smispy/smis_ag.*
%{_bindir}/smispy_lsmplugin

%files -n %{libstoragemgmt}-netapp-plugin
%defattr(-,root,root,-)
%dir %{python_sitelib}/lsm/plugin/ontap
%{python_sitelib}/lsm/plugin/ontap/__init__.*
%{python_sitelib}/lsm/plugin/ontap/na.*
%{python_sitelib}/lsm/plugin/ontap/ontap.*
%{_bindir}/ontap_lsmplugin

%files -n %{libstoragemgmt}-targetd-plugin
%defattr(-,root,root,-)
%dir %{python_sitelib}/lsm/plugin/targetd
%{python_sitelib}/lsm/plugin/targetd/__init__.*
%{python_sitelib}/lsm/plugin/targetd/targetd.*
%{_bindir}/targetd_lsmplugin

%files -n %{libstoragemgmt}-nstor-plugin
%defattr(-,root,root,-)
%dir %{python_sitelib}/lsm/plugin/nstor
%{python_sitelib}/lsm/plugin/nstor/__init__.*
%{python_sitelib}/lsm/plugin/nstor/nstor.*
%{_bindir}/nstor_lsmplugin

%files udev
%defattr(-,root,root,-)
%{udev_dir}/udev/scan-scsi-target
%{udev_dir}/udev/rules.d/90-scsi-ua.rules

%if 0%{?with_rest_api}
%files -n %{libstoragemgmt}-rest
%defattr(-,root,root,-)
%{_bindir}/lsm_restd
%if 0%{?with_systemd}
%{_unitdir}/libstoragemgmt-rest.service
%endif
%endif

%changelog
* Thu Dec 4 2014 Tony Asleson <tasleson@redhat.com> 1.1.0-1
- Library adds:

  API Constants for new pool element types and plugin changes to support it
  * C constants:
     LSM_POOL_ELEMENT_TYPE_VOLUME_FULL, LSM_POOL_ELEMENT_TYPE_VOLUME_THIN
  * Py constants:
     Pool.ELEMENT_TYPE_VOLUME_FULL, Poll.ELEMENT_TYPE_THIN

  lsmcli:
  * lt - Alias for 'list --type target_ports'
  * Removed --init for volume-mask, it was broken for targetd (the only
    user) and instead of fixing we are going to improve targetd to support
    access groups in the next release

- Numerous code improvements, including a big SMI-S plugin refactor,
  source code documentation corrections

- Bug fix: Use correct default values for anonymous uid/gid in lsmcli
- Bug fix: simc simulator not working for allowable NULL parameters for:
  *  fs_child_dependency
  *  fs_child_dependency_rm
  *  fs_snapshot_restore
- Bug fix: lsm_restd memory leak corrections
- Bug fix: NetApp plugin, correctly set export path when caller specifies
  default in API
- Bug fix: Add file locking to sim plugin to prevent concurrent modification
- Bug fix: Consistently report common error conditions for NO_STATE_CHANGE,
  EXISTS_INITIATOR for all plugins
- Bug fix: Number of bugs addressed in SMI-S plugin including:
  * EMC: Correct error path when replicating a volume with a duplicate
    volume name
  * HDS: Correctly create thinly provisioned volume on thinly provisioned
    pool

* Sun Sep 7 2014 Tony Asleson <tasleson@redhat.com> 1.0.0-1
- Release version 1
- Numerous constants re-naming & removing
- Removed the pool create/delete until things work better,
  esp. WRT SMI-S
- Added checks for initiator ID verification
- Added checks for vpd 0x83 verification
- Simplified error logging (removed domain & level)
- Re-named functions for online,offline -> enable,disable
- Always use objects instead of object ID in function
  params
- Removed individual files from fs snapshot creation
- Add unsupported actions for pools
- lsm_capability_set_n uses a -1 to terminate list
- Volume status removed, replaced with admin state
- Removed ibmiv7k plugin
- Explicitly specify python2
- Error path consistency changes (same error for same condition
  across plug-ins)
- Numerous bug fixes

* Thu Jul 3 2014 Tony Asleson <tasleson@redhat.com> 0.1.0-1
- Release candidate for a 1.0.0 release
- Optional data removed
- Initiator only functions removed
- Pool create from from volumes removed
- Code directory structure updated
- Target port listing added

* Tue Feb 18 2014 Gris Ge <fge@redhat.com> 0.0.24-2
- Introduce a REST daemon(only for systemd yet).
- Allowing enable/disable ibm_v7k plugin via 'rpmbuild --with ibm_v7k' or
  'rpmbuild --without ibm_v7k'.
- Fix the compile warning about data of changelog by changing
  'Thu Nov 27 2013' to 'Wed Nov 27 2013'.

* Thu Jan 30 2014 Tony Asleson <tasleson@redhat.com> 0.0.24-1
- Command line interface (CLI) re-factored and improved to be easier to use
  and more consistent, man pages have been updated
- Command line output now has '-s, --script' for an additional way to output
  information for consumption in scripts
- Command line option '-o' for retrieving optional/extended data for disks &
  pools
- Pool creation/deleting in CLI & python API
- Numerous small bug fixes
- C API, added ability to list disks, list plugins and retrieve optional
  data for disks
- SSL for SMI-S is more stringent on certificate checking for newer
  distributions, new URI option "no_ssl_verify=yes" to disable

* Wed Nov 27 2013 Tony Asleson <tasleson@redhat.com> 0.0.23-1
- Addition of listing disks implemented for SMI-S and Ontap plugins
  (new, not in C library yet)
- Add the ability to list currently installed and usable plug-ins
- Verify return types are correct in python client calls
- Added the ability to retrieve optional data (new, not in C library yet)
- Visibility reductions for python code (somethings were public when should be
  private
- Add calls to create/delete pools (new, not in C library yet)
- Add missing initiator type for SAS
- Improved vpd83 retrieval for SMI-S
- Performance improvements for SMI-S plug-in
- Numerous small bug fixes
- Nstor plugin, additional testing and bug fixes
- lsmd, added call to setgroups and enable full relo and PIE (ASLR) for
  security improvements
- simulator state is now versioned
- SCSI Unit Attention uevent handling

* Mon Aug 12 2013 Tony Asleson <tasleson@redhat.com> 0.0.22-1
- Numerous code improvments/fixes
- BZ 968384
- BZ 990577

* Tue Jul 16 2013 Tony Asleson <tasleson@redhat.com> 0.0.21-1
- Don't include IBM7K plugin for RHEL > 6 missing paramakio
- IEC binary size handling
- Functionality improvements for IBM V7K array
- Workaround for python bug on F19
- Bugfix (BZ 968384)
- Package plug-ins as separately in rpm packages

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
