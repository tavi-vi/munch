I wrote this out of a fascination with the [Blit terminal's diagnostic program](https://github.com/Alhadis/Research-Unix-v8/blob/389623b76d5b6e195361f0705b1826b00ae14d19/blit/diag/diag.c#L43), which implements a variant of the munching squares program. I decided to keep adding features to make it fun to explore the patterns it produces.

# Controls
`space` pauses

`left`/`right` change the pattern to the previous or next

`up`/`down` change the speed in small increments

`page up`/`page down` change the speed in large increments

`home` returns the speed to the default

`r`/`f` change the direction of the algorithm to reverse or forward, respectively

`d` prints some debug information to `stdout`

# Building
You only need GCC, GNU Make, and SDL2 to build. `make` will build and put the executable at `release/munch`. If you have Nix with flake support, you can just run `nix build`.
