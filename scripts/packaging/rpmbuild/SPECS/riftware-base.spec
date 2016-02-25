# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

#%%define _topdir           %{_rift_root}/scripts/packaging/rpmbuild/
%define _topdir           %{_rift_root}/.install/rpmbuild/
%define name              %{_rpmname}
#%%define release           1%{?dist}
%define release           %{_buildnum}%{?dist}
%define version           %{_version}
%define buildroot         %{_topdir}/%{name}-%{version}-root
%define DST_RIFT_ROOT     %{_dst_rift_root}
%define RIFT_ROOT         %{_rift_root}

%define _binaries_in_noarch_packages_terminate_build   0 	# http://winzter143.blogspot.com/2011/11/linux-arch-dependent-binaries-in-noarch.html

Name:     	%{name}
Version:  	%{version}
Release:  	%{release}
Summary:        RIFT.ware

License:        Apache
URL:            http://www.riftio.com
Source0:        %{name}-%{version}.tar.gz


#BuildArch: noarch
BuildRequires: yum

Requires: riftware-release rift_scripts
#Requires(post): info
#Requires(preun): info


%description
The %{name} program is a RIFT.ware package.


# This reads the sources and patches in the source directory %_sourcedir. It unpackages the sources to a subdirectory underneath the build directory %_builddir and applies the patches.
%prep

# This compiles the files underneath the build directory %_builddir. This is often implemented by running some variation of "./configure && make".
%build
echo "BuildDir: %_builddir"

#%setup


# This reads the files underneath the build directory %_builddir and writes to a directory underneath the build root directory %_buildrootdir. The files that are written are the files that are supposed to be installed when the binary package is installed by an end-user. 
# Beware of the weird terminology: The build root directory is not the same as the build directory. This is often implemented by running "make install".
#
# this is a little wonky now but it works
#
%install
echo "buildroot: %{buildroot}"
echo "pwd: `pwd`";
echo "running CMD: cp -Rpv ../SOURCES/%{name}-%{version}/* %{buildroot}/"
#cp -Rpv ../SOURCES/%{name}-%{version}/* %{buildroot}/
cp -Rpv %{RIFT_ROOT}/.install/rpmbuild/SOURCES/%{name}-%{version}/* %{buildroot}/ 

#helper 
cp -p %{RIFT_ROOT}/scripts/packaging/services/%{name}/prepare_lp.sh  %{buildroot}/%{DST_RIFT_ROOT}/

exit 0; # http://stackoverflow.com/questions/30317213/how-to-remove-pyo-anc-pyc-from-an-rpm

%post

# remove /usr/rift symlink if it exists which comes from internal labmode
# moved to scripts/rpm/rift-scripts.spec in RIFT-10842

# move old rift/scripts if they exist
SDIR="/home/rift/scripts"

if [ -d "$SDIR" ]; then 
  if [ -L "$SDIR" ]; then
    # it is a symlink
    rm "$SDIR"
  else
    # it's a directory (from old rpm)
    rm -Rfv "$SDIR"
  fi
fi

# then link rift scripts
ln -s /usr/rift/scripts $SDIR


# use a glob* here like
# /usr/X11R6/man/man1/xtoolwait.*
# or a directory ???
# like %{DST_RIFT_ROOT}/demos ???
# or use an external list of files
# %files -f xconfig_files.txt

# filelist test
#%files -f %{_filelist}

%files
/%{DST_RIFT_ROOT}/


%clean
#rm -rf $RPM_BUILD_ROOT
#rm -rvf $RPM_BUILD_ROOT

# we may need some of this stuff down the road ?!?
# doc AUTHORS ChangeLog NEWS README THANKS TODO
# license COPYING

%changelog
* Thu Nov 12 2015 <Nate.Hudson@riftio.com> 4.0
- Initial version of the package

