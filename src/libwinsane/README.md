# libwinsane: Quick-and-easy sanity for Windows applications

Build `libwinsane.o` using `make`, then link it into any program to
provide some sanity on Windows. Specifically:

* All command line arguments (i.e. `argv`) will be encoded with UTF-8.

* Functions accepting paths (i.e. `fopen`) will support UTF-8, such as
  file names passed as command line arguments.

* All environment variables (i.e. `getenv`) will be encoded with UTF-8.

* The Windows console will correctly decode the program's UTF-8 output.

* `stdin` and `stdout` will be in binary mode, no text translation.

In other words, the C runtime will behave almost as it does on every other
platform. Windows may not need to be a special support case, either for
Unicode or binary handling. Many command line programs not designed for
Windows will simply work correctly when linked with libwinsane.

Caveat: Despite every indication otherwise, Windows does [not yet support
reading UTF-8 input from a console][in], so programs still cannot accept
UTF-8 keyboard input. For now that continues remaining a special case
requiring [special handling][pw].

## FAQ

Q: Why an object file and not a static library?

A: Constructors and resources do not work in static libraries because
they're not referenced by the program.

* * *

Q: Why not a shared library?

A: The embedded resource won't work from a shared library.

* * *

Q: Why not a C source file, even if it means inline assembly, that users
can `#include`?

A: Only `windres` can produce the specific, 32-bit static relocation used
to build the resource `.rsrc` tree. Trust me, I tried!


[in]: https://github.com/microsoft/terminal/issues/4551#issuecomment-585487802
[pw]: https://nullprogram.com/blog/2020/05/04/
