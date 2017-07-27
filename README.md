# gr-fsk4
Custom GNU Radio \(Companion\) blocks for demodulating 4L-FSK (4 Level Frequency-Shift Keying) signals, and APCO-25 and RDLAP protocol processing.

This module is the updated version of [gr-fsk4](http://www.qsl.net/kb9mwr/projects/dv/apco25/GnuradioFourLevelFSK.pdf)
\([code](http://www.qsl.net/kb9mwr/projects/dv/apco25/gr-fsk4-22Apr08.tar.gz)\) for GNU Radio 3.7+. The code in this repository borrows heavily from various GNU Radio examples and components which fall under the GNU General Public License. All additional modifications and code also fall under GNU General Public License.

## Usage
Compiling & Installing:

```sh
$ cd gr-fsk4
$ mkdir build    # We're currently in the module's top directory
$ cd build/
$ cmake ../      # Tell CMake that all its config files are one dir up
$ make           # And start building (should work after the previous section)
$ make install   # Install blocks in GNU Radio
```

More info about out-of-tree modules: https://wiki.gnuradio.org/index.php/OutOfTreeModules

More documentation: http://www.qsl.net/kb9mwr/projects/dv/apco25/GnuradioFourLevelFSK.pdf
