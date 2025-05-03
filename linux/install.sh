mkdir -p ~/.config/systemd/user/
mkdir -p ~/.bin/vc/
cp out/volume_controller ~/.bin/vc/
cp out/appind_lib.so ~/.bin/vc/
cp vc_icon1.png ~/.bin/vc/
cp volume_controller.service ~/.config/systemd/user/

systemctl --user enable volume_controller.service
systemctl --user start volume_controller.service
