mkdir -p ~/.config/systemd/user/
mkdir -p ~/.bin/vc/
cp out/volume_controller ~/.bin/vc/
cp volume_controller.service ~/.config/systemd/user/

systemctl --user enable volume_controller.service
systemctl --user start volume_controller.service
