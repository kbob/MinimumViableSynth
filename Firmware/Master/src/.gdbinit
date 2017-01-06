# # First Discovery board
# target extended-remote /dev/cu.usbmodemBED9D9E1

# # Second Discovery board
target extended-remote /dev/cu.usbmodemBED6C011

# Black Magic Probe Mini, v1
# target extended-remote /dev/cu.usbmodemBDEAA9F1

monitor swdp_scan
attach 1

# set target-async                on
set confirm                     off
set mem inaccessible-by-default off

define lc
  load
  continue
end
define lr
  load
  run
end
