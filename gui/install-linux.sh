mkdir -p ~/.local/share/applications
cp -r out/volumecontroller-gui-linux-x64/ ~/.bin/vc/
cp vcgui_icon.png ~/.bin/vc/volumecontroller-gui-linux-x64/

sed -E "s/%USER%/`whoami`/g" vc_gui.desktop > ~/.local/share/applications/vc_gui.desktop