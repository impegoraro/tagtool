Summary: Audio Tag Tool is an MP3 and Vorbis tag manager.
Name: tagtool
Version: 0.12.3 
Release: 1 
License: GPL
Group: Sound
Source: http://pwp.netcabo.pt/paol/tagtool/%{name}-%{version}.tar.gz
URL: http://pwp.netcabo.pt/paol/tagtool/
Buildroot: %{_tmppath}/%{name}-root
Packager: Pedro Lopes <paol1976@yahoo.com>
Requires: gtk2 >= 2.4.0, libglade2 >= 2.3.6, libxml2 >= 2.6.0, libvorbis >= 1.0, id3lib >= 3.8.0
BuildRequires: gtk2-devel, libglade2-devel, libxml2-devel, libvorbis-devel, id3lib-devel


%description
Audio Tag Tool is a program to manage the information fields in MP3 and 
Ogg Vorbis files (commonly called 'tags').
It can be used to edit tags one by one, but the most useful features are 
mass tag and mass rename. These are designed to tag or rename hundreds of 
files at once, in any desired format. 
 


%prep
%setup -q

%build
CFLAGS="-g -O2"
%configure
make

%install

rm -rf $RPM_BUILD_ROOT
%makeinstall
#rm -r $RPM_BUILD_ROOT/var/scrollkeeper

%post 

scrollkeeper-update

%postun

scrollkeeper-update

%clean

rm -rf $RPM_BUILD_ROOT

%files
%defattr(-, root, root)
%doc AUTHORS BUGS COPYING ChangeLog NEWS README THANKS TODO
%{_bindir}/%{name}
%{_datadir}/applications/tagtool.desktop
%{_datadir}/tagtool/*
%{_datadir}/pixmaps/TagTool.png
%{_datadir}/locale/*

%changelog
* Fri Jul 7 2006 Michael Kuerschner <m-i.kuerschner@gmx.de>
- Rebuild on FC4

* Wed Nov 3 2004 Nickolay V. Shmyrev <nshmyrev@yandex.ru>
- Update to recent version

* Sun Nov 4 2001 Chris Weyl <cweyl@mindspring.com>
- Autoconfiscated package.
- First spec file.

