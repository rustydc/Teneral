SUMMARY
=======

Teneral is a proof-of-concept command-line interface for the web.

>  Ten-er-al (adj.): The condition of an arthropod shortly after shedding 
>                     its former shell, when the animal is free to grow.


DESCRIPTION
===========

'Terminal emulators' are designed to emulate the DEC VT100 or VT220 hardware
terminals, released in 1978.  The goal of Teneral is to demonstrate that we 
can do better by instead basing them on modern web technologies.

Each command executed via Teneral is displayed in its own output window.
These windows can be minimized, closed, resized, and scrolled individually.
The output window border color indicates whether the command is still running
(bright green), completed successfully (green), or failed (red).  Because each
command has its own window, there's no need to move jobs between background
and foreground.

Teneral uses modern web features like websockets and data-URIs, so a recent
standards compliant browser is recommended.

As a quick, hacky demo: any command ending in `.jpg` will be interpreted
and displayed as a JPEG.


COMPILING
=========

Just run `make` in `src/`.

(Requires libEV, libJSON, and OpenSSL.  To use `gcc`, edit Makefile.)


USAGE
=====

Start the server up on localhost, port 10002:

    ./tnrld


FUTURE WORK
===========

 * Collaborative sessions
 * Save and restore sessions
 * Allow HTML/JS output from commands
 * Display standard error separately
 * Check credentials via PAM
 * SSL (For now, proxy it behind Apache)
 * More I/O redirection


HACKING
=======

 * tnrld.c
   - Main entry point, starts the daemon.
 * http.c
   - Handle HTTP requests and responses; serve static files.
 * socket.c
   - Callbacks to read from and write to sockets.
 * websocket.c
   - Upgrade a socket to a websocket.
 * processes.c
   - Execute processes via fork/exec, set up events.
 * messages.c
   - JSON encoding/decoding.
 * writebuf.c
   - A buffer of data waiting to be written.
 * ilist.c
   - Sorted array-list.  Used to track PIDs, sockets, etc.

 * static/index.html
   - HTML for the client interface.
 * static/tnrl.js
   - Javascript for the client UI.
 * static/tnrl.css
   - UI styling.


BUGS
====

 * Current path display is hardcoded.
 * `cd` not implemented.


LICENSE
=======

(C) 2012, Rusty Mellinger.  Released under GPLv3, see COPYING.
