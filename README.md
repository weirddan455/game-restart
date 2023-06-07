# game-restart
This is a simple program that watches /dev/kmsg for an Xid error coming from the Nvidia drivers.
When this happens, it will kill the offending process.  This is useful because these errors often make the DE unresponsive.

A build script is provided "build.sh".  This needs root permissions to assign syslog capabilities to the binary.  If your system can read /dev/ksmg as a normal user, this is not required and the "setcap" line can be ommited from the build script.

This program also accepts arguments to be executed on start and whenever a crash occurs.  With no arguments, it will still watch for crashes and kill offending processes but it will not restart the game.

For example `./restart steam steam://rungameid/570` will launch Dota 2 from Steam and restart it if it crashes.
