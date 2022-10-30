# com.c

Generic hooks for com devices. The main COMs used are:

- COM 1: Camera
- COM 2: NFC
- COM 3: Touch
- COM 5-8: LEDs

These are defined in devices/

# files.c

Redirection hooks for files. These are used to redirect a number of locations to relative paths, and for `\\\\.\\driver` files.

# gui.c

Hooks for drawing over the game

# logging.c

Hooks for `[f]printf[_s]`, and event sources.

# network.c

Network hooks used to log when outbound TCP connections are made, and to redirect DNS.

# processes.c

(Disabled) hooks CreateProcess to re-inject ourself into any child processes too

# setupapi.c

Hooks the Setup API functions to inject fake devices (used for amPlatform and amEeprom)

# time.c

Hooks a handful of time-setting functions to NOP them
