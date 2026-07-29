# Testing

Currently, golden tests are being done by comparing outputs of the tokenizer
and the parser.

## Adding a new test case

Test cases are located in `./tokenizer` and `./parser`.

Create an input file in either directory, named `[TEST_CASE_NAME].txt`.
`TEST_CASE_NAME` should only consist of lowercase letters and underscores.

Then run in the same directory:

``` shell
$ ./generate_test.bash [TEST_CASE_NAME].txt
```

This generates an output file named `TEST_CASE_NAME.expected.txt`.

