Summary:	libsmb enables access to SMB shares
Summary(fr):	libsmb permet d'acc�der aux partages SMB
Summary(pl):	libsmb pozwala na dost�p do plik�w z zasob�w SMB
Name:		libsmb
Version:	1.0pre	
Release:	2
Copyright:	GPL
Group:		Libraries
Group(fr):	Biblioth�ques
Group(pl):	Biblioteki
Source:		http://www-eleves.iie.cnam.fr/brodu/smblib/libsmb/%{name}-%{version}.tar.gz
URL:		http://www-eleves.iie.cnam.fr/~brodu/smblib/
BuildRoot:	/tmp/%{name}-%{version}-root

%description
libsmb enables access to SMB shares in any C++ program under Unix.
Access to files over SMB is done using member functions acting and
named as their standard equivalent (open, read, opendir, etc.).

%description -l fr
libsmb permet l'acc�s aux partages SMB dans n'importe quel programme en C++
sous Unix. L'acc�s � des fichiers en utilisant le protocole SMB est effectu�
gr�ce aux fonctions membres de SMBIO fonctionnant comme leurs homologues
standards (open, read, opendir, etc.).

%description -l pl
libsmb umo�liwia dost�p do zasob�w SMB z poziomu program�w C++.

%package devel
Summary:	Library and headers for libsmb development
Summary(fr):	Biblioth�que et en-t�tes pour le d�veloppement avec smblib
Summary(pl):	Biblioteka i pliki nag��wkowe
Group:		Development/Libraries
Group(fr):	Developpement/Biblioth�ques
Group(pl):	Programowanie/Biblioteki
Requires:	%{name} = %{version}

%description devel
Library and headers for libsmb development.

%description devel -l fr
Biblioth�que et en-t�tes pour le d�veloppement avec smblib

%description devel -l pl
Biblioteka i pliki nag��wkowe do programowania z wykorzystaniem libsmb.

%package static
Summary:        Static library for libsmb development
Summary(fr):	Biblioth�que statique pour le d�veloppement avec smblib
Summary(pl):    Biblioteka statyczna
Group:          Development/Libraries
Group(pl):      Programowanie/Biblioteki
Requires:       %{name}-devel = %{version}

%description static
Static library for libsmb development.

%description static -l fr
Biblioth�que statique pour le d�veloppement avec smblib

%description static -l pl
Biblioteka statyczna do programowania z wykorzystaniem libsmb.

%prep
%setup -q

%build
CXXFLAGS="$RPM_OPT_FLAGS" \
LDFLAGS="-s" \
./configure \
	--target=%{_target_platform} \
	--host=%{_host} \
	--prefix=%{_prefix} 

make

%install
rm -rf $RPM_BUILD_ROOT

make install \
	DESTDIR=$RPM_BUILD_ROOT

gzip -9nf README AUTHORS NEWS ChangeLog \
	$RPM_BUILD_ROOT%{_mandir}/man1/* 

%post   -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(644,root,root,755)
%doc *.gz
%attr(755,root,root) %{_bindir}/*
%attr(755,root,root) %{_libdir}/lib*.so.*.*
%{_mandir}/man1/*

%files devel
%doc doc/*.html doc/*.c++
%defattr(644,root,root,755)
%attr(755,root,root) %{_libdir}/lib*.so
%attr(755,root,root) %{_libdir}/lib*.la
#%{_includedir}/*

%files static
%defattr(644,root,root,755)
%{_libdir}/lib*.a

%changelog
* Sun May 16 1999 Nicolas Brodu <brodu@iie.cnam.fr>
  [19990516-2]
- added french translations

* Sun May 16 1999 Artur Frysiak <wiget@pld.org.pl>
  [19990516-1]
- initial version  
