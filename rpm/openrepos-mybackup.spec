Name:           openrepos-mybackup

Summary:        Backup manager
Version:        1.0.6
Release:        1
License:        BSD
URL:            https://github.com/monich/harbour-mybackup
Source0:        %{name}-%{version}.tar.gz

Requires:       jolla-vault >= 1.0.0
Requires:       sailfishsilica-qt5
Requires:       qt5-qtsvg-plugin-imageformat-svg
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(dconf)
BuildRequires:  pkgconfig(sailfishapp)
BuildRequires:  pkgconfig(mlite5)
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  qt5-qttools-linguist

%{!?qtc_qmake5:%define qtc_qmake5 %qmake5}
%{!?qtc_make:%define qtc_make make}
%{?qtc_builddir:%define _builddir %qtc_builddir}

%description
Allows to add more stuff to the backup.

%if "%{?vendor}" == "chum"
Categories:
 - Utility
Icon: https://raw.githubusercontent.com/monich/harbour-mybackup/master/icons/harbour-mybackup.svg
Screenshots:
- https://home.monich.net/chum/openrepos-mybackup/screenshots/screenshot-001.png
- https://home.monich.net/chum/openrepos-mybackup/screenshots/screenshot-002.png
- https://home.monich.net/chum/openrepos-mybackup/screenshots/screenshot-003.png
- https://home.monich.net/chum/openrepos-mybackup/screenshots/screenshot-004.png
- https://home.monich.net/chum/openrepos-mybackup/screenshots/screenshot-005.png
- https://home.monich.net/chum/openrepos-mybackup/screenshots/screenshot-006.png
Url:
  Homepage: https://openrepos.net/content/slava/my-backup
%endif

%prep
%setup -q -n %{name}-%{version}

%build
%qtc_qmake5 CONFIG+=openrepos harbour-mybackup.pro
%qtc_make %{?_smp_mflags}

%install
rm -rf %{buildroot}
%qmake5_install

desktop-file-install --delete-original \
  --dir %{buildroot}%{_datadir}/applications \
   %{buildroot}%{_datadir}/applications/*.desktop

%postun
if [ "$1" == 0 ] ; then
  for d in $(getent passwd | cut -d: -f6) ; do
    if [ "$d" != "" ] && [ "$d" != "/" ] && [ -d "$d/.local/share/openrepos-mybackup" ] ; then
      rm -fr "$d/.local/share/openrepos-mybackup" ||:
    fi
  done
fi

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
%{_datadir}/jolla-vault/units/*.json
