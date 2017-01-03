# target extended-remote /dev/cu.usbmodemBED9D9E1
target extended-remote /dev/cu.usbmodemBED6C011
monitor swdp_scan
attach 1

set confirm                        off
set memory inaccessible-by-default off
set mi-async                       on

define lc
  load
  continue
end
define lr
  load
  run
end
