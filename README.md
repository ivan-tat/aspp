# aspp

Simple assembler source file preprocessor. It processes source's file dependencies and produces a text line to be used in makefiles.

## License

Public domain (see [UNLICENSE](UNLICENSE) file).

## Authors

See [AUTHORS](AUTHORS) file.

## Dependencies

This project uses these external works:

* GNU Core utilities
* GNU Bash
* GNU grep
* GNU sed

## Usage

```
Usage:
    aspp [options] [filename ...] [options]
                    
Options (GCC-compatible):
-h, --help      show this help and exit
-E              preprocess
-I <path>       include directory
-M[M]           output autodepend make rule
-MF <file>      autodepend output name
-MT <target>    autodepend target name (can be specified multiple times)
```

## Links

* [GNU Operating System](https://www.gnu.org/)
* [GNU Core Utilities](https://www.gnu.org/software/coreutils/) ([package](https://pkgs.org/download/coreutils))
* [GNU Make](https://www.gnu.org/software/make/) - utility for directing compilation ([package](https://pkgs.org/download/make))
* [GNU Bash](https://www.gnu.org/software/bash/) - GNU Bourne Again SHell ([package](https://pkgs.org/download/bash))
* [GNU grep](https://www.gnu.org/software/grep) - a non-interactive command-line utility to search for text in files ([package](https://pkgs.org/download/grep))
* [GNU sed](https://www.gnu.org/software/sed) - a non-interactive command-line text editor ([package](https://pkgs.org/download/sed))
* [GNU Compiler Collection](https://www.gnu.org/software/gcc/) ([package](https://pkgs.org/download/gcc))
* [GNU Standards](http://savannah.gnu.org/projects/gnustandards) - GNU coding and package maintenance standards ([package](https://pkgs.org/download/gnu-standards))
