# ShadeTabler
Shade Table Generator by Todi / Tulou.

Developed and distributed under the terms of the [MIT license](./LICENSE).

## Building

In a Unix-like environment simply `make` the binary.

## Usage

	shadetabler -o <output directory> <additional options> <input files...>

Available options are:

	-o, --output                    Output directory. (Required)
	-v, --verbose                   Verbose output.
	-f, --force                     Overwrite existing files, never prompt.
	-c, --colors [2-256]            Number of colors saved in the output file. (Default 256)
	-s, --shades [2-256]            Number of shades in shade table. (Default 64)
	-t, --type [light|dark|both]    Number of shades in shade table. (Default "light")
	-h, --hq                        High quality quantization quality.
	-r, --reserve                   Reserve colors for black and white.
	-l, --light                     Light shade table filename. (Default "shadetable_light.png")
	-d, --dark                      Dark shade table filename. (Default "shadetable_dark.png")

Examples:

	shadetabler -v -r -c 256 -s 64 texture1.png -o ./shadetables
	shadetabler texture1.png texture2.png texture3.png -o ./shadetables
	shadetabler -f -c 64 -s 32 -r -hq texture1.png -d shadetable.png -o ./shadetables

## Acknowledgments
ShadeTabler uses the following libraries:

* [LodePNG](https://lodev.org/lodepng/) by Lode Vandevenne
* [ExoQuant](https://github.com/exoticorn/exoquant/) by Dennis Ranke
* [libcwalk](https://github.com/likle/cwalk/) by Leonard Iklé
