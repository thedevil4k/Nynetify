Name: nynetify
Version: 1.0.0
Release: 1
Summary: YouTube Audio Player
License: GPL-3.0
Group: Applications/Multimedia
URL: https://github.com/anomalyco/Nynetify
BuildArch: x86_64
AutoReqProv: no
Requires: python3

%description
Self-contained YouTube audio streaming application. Bundles mpv and yt-dlp.

%files
/opt/nynetify/bin/Nynetify
/opt/nynetify/bin/mpv
/opt/nynetify/bin/yt-dlp
/opt/nynetify/nynetify
/usr/bin/nynetify
%dir /opt/nynetify/lib/
/opt/nynetify/lib/*.so*
%doc

%post
chmod +x /opt/nynetify/bin/* /opt/nynetify/nynetify

%clean
rm -rf %{buildroot}
