# slowhax / waithax

This is Mrrraou's implementation of the slowhax / waithax ARM11 kernel exploit with a "time left and elapsed" display added.
Kind of intended as a reference implementation, sort of based on [Steveice10's memchunkhax2 implementation](https://github.com/Steveice10/memchunkhax2/).
Definitely does not look the cleanest possible, feel free to contribute.

Can only work from 9.0 to 11.1, as the vulnerability was patched on 11.2. Use faster exploits if you can, though.

Only tested on my 10.3 New3DS, but I don't see why it would fail on other consoles. There are no hardcoded addresses in this implementation.

Exploit written in less than a day. Finding the strat took more time.
No one really seemed to care about doing it apparently...

## How to use, for users

Use [the latest release](https://github.com/Mrrraou/waithax/releases) or build the app yourself.

Then, run the .3dsx, wait for the exploit to be done, and run your homebrew of choice that needs elevated privileges (can be yellows8's 3ds_dsiwarehax_installer, for example).

If you are getting a freeze/crash on the Homebrew Launcher when running it, please update your Homebrew Launcher.

## How to use, for developers

Please check the `main.c` file in the `source` folder for information on how to use this implementation in your own application.
If you want to use this as a library, you just need `waithax.c`, `utils.s` and their headers.

Take note that this implementation does not patch the SVC call access table, nor the process PID on its own.
However, an helper method is given to run your own kernel mode code after running the exploit.
This method *sort of* acts like svcBackdoor; but the given code will be run with the SVC-mode stack, instead of the userland caller thread stack.

## Estimated time for running the exploit

Takes around 25 minutes for New3DS (should), and around 1 hour and 10 minutes for Old3DS.

## Credits

 - nedwill/derrek for discovering the vulnerability, they're the real guys here
 - Steveice10 for the memchunkhax2 implementation
 - AuroraWright/TuxSH for Luma3DS and its exception handlers, Subv/cell9 for the SVC access check patches which were extensively used for development
 - TuxSH for finding the KSyncObject address leaking used in memchunkhax2, Kernel11 RE and lots of more stuff
 - if I missed anyone in there please tell me

## License

Copyright (c) 2016 Mrrraou

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
