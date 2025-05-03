systemctl --user stop volume_controller.service
systemctl --user disable volume_controller.service

rm ~/.bin/vc/volume_controller
rm ~/.bin/vc/appind_lib.so
rm ~/.bin/vc/vc_icon1.png
rm ~/.bin/vc/config.json
rm ~/.config/systemd/user/volume_controller.service