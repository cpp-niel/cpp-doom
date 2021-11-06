# Doom++

The original Doom in C++ (maybe one day at least - for now it's mainly just the level renderer).

## About Doom++

The original Doom was written in C. This is an attempt to take those original algorithms and implement them
reasonably faithfully in C++. Purely as an exercise and a way to get to know how this marvel of the early 
nineties actually works.

The `++` in the name comes from the choice of language. In particular, the `++` is in no
way suggestive of any kind of "increment" over the original Doom. The original is just a breathtaking
piece of work. As a programmer, to look at that original code knowing that 

 - its design and implementation was in large part the work of a single developer
 - it took less than a year with a significant interruption somewhere in the middle to create the Nintendo Wolfenstein port. 
 - it ran on 386 PC hardware

is humbling. There is so much to learn about programming by looking at the Doom source code. That's the motivation 
behind this. To try and learn from the code that was graciously made available to the public and that I should
have looked at many years ago, but for some unfathomable reason didn't.

Some aspects that make the original so great have been ignored in this implementation. So in some ways
this is an inferior implementation. For instance, the original is very careful to limit its memory consumption 
using a pre-allocated memory arena. Doom++ makes no attempt to limit memory consumption and just allocates
directly from the heap. This is far simpler, but it is slower and wasteful and would be an obstacle to
running this implementation on memory restricted hardware.

Another example: the original Doom uses 16:16 fixed point to represent non-integer position and distance values and a 
360° / 2^32 resolution angle representation in unsigned 32-bit integers (note how that elegantly handles the
wrap-around at 360° by overflowing - add any two angles and the result will be a normalized angle). This requires all kinds
of complex bit shifting and look up tables for trigonometric functions. But these were the days
before PCs had dedicated floating point hardware. Optimizations like this were essential to getting
decent performance on a 386. Doom++ just does everything in floating point. An angle, for instance, is a strong type
wrapping a regular floating point value and trigonometry just uses the standard library functions.

## Credits

First and foremost this work is derived from studying the [original Doom sources](https://github.com/id-Software/DOOM)
(see [license](https://github.com/id-Software/DOOM/blob/master/linuxdoom-1.10/DOOMLIC.TXT)).
Most data structures are identical or at least very close to the originals. There are places where I
started with the original sources and morphed them into C++. So if you are familiar with the original code
you might recognize certain comments, function names etc. `english.hpp` is the original `d_englsh.h` file
which contains the English strings used in the game. 

The video and input code uses [SDL 2](https://www.libsdl.org/) and the general approach is lifted from
[crispy-doom](https://github.com/fabiangreffrath/crispy-doom). The icon is also inspired by the crispy-doom /
[chocolate-doom](https://github.com/chocolate-doom/chocolate-doom) icons. The definition of the keys in
`doomkeys.hpp` is (a very slightly modified) `doomkeys.h` from chocolate-doom.

This work uses the [{fmt} library](https://fmt.dev/latest/index.html) (see 
[license](https://github.com/fmtlib/fmt/blob/master/LICENSE.rst)).

The `ranges::to` implementation is by Corentin Jabot and used under the terms of the 
[Boost License](https://www.boost.org/users/license.html).