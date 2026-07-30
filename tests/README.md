# Testing

Currently, golden tests are being done by comparing outputs of the tokenizer
and the parser.

## Adding a new test case

Create an input file in `[TYPE]/[TEST_CASE_NAME].txt`.

- `TYPE` is one of `tokenizer` and `parser`.
- `TEST_CASE_NAME` should only consist of lowercase letters and underscores.

Then run:

``` shell
$ ./generate_test.bash [path to the CMake build directory] [TYPE] [TEST_CASE_NAME]
```

This generates an output file in `[TYPE]/[TEST_CASE_NAME].expected.txt`.

