# Testing

Golden tests are done by comparing outputs of the components.

Currently, the component types are `tokenizer` and `parser`.

## Adding a new test case

1. Create an input file.

The file path depends on the component type.

- For `tokenizer`: `tokenizer/[TEST_CASE_NAME].txt`
- For `parser`: `parser/[TEST_CASE_NAME].yiche`

`TEST_CASE_NAME` should only consist of lowercase letters and underscores.

2. Generate the output file.

The executables for the running the test cases must have been built via CMake.

``` shell
$ ./[TYPE]_generate_output.bash [path to the CMake build directory] [path to the input file]
```

This generates an output file in `[TYPE]/[TEST_CASE_NAME].expected.txt`.

