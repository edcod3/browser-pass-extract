# browser-pass-extract
Extract locally stored browser passwords 

## Compilation

Make sure `libsqlite3-dev` and `openssl` is installed. 

Then compile the binary with gcc: \
`gcc ./src/linux/main.c -lsqlite3 -lcrypto -o browser-pass-extract`.

## Usage

`./main [-f file]`
Dynamic extraction of the _Login Data_ file is currently not fully implemented.
