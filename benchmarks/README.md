# Benchmarks

Implementation of the language benchmarks: https://benchmarksgame-team.pages.debian.net/benchmarksgame/index.html

nbody.luci and nbody2.luci implement the same benchmark but slightly different.  nbody2.luci uses the `array_double` function to have a hard cast into an array with knownly only doubles.  The interpreter can then take a different path that works more optimized.  The benchmark does not show much difference, as the size of the arrays is too small make a difference.

