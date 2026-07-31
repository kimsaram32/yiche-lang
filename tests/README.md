# Testing

Golden tests are done by comparing outputs of the components.

Currently, the component types are `tokenizer` and `parser`.

## Adding a new test case

1. Create an input file.

The file path depends on the component type.

- For `tokenizer`: `tokenizer/[TEST_CASE_NAME].txt`
- For `parser`: `parser/[TEST_CASE_NAME].yiche`

`TEST_CASE_NAME` should only consist of lowercase letters and underscores.

2. Regenerate the output files.

Run the custom target for the component type after adding the input file.

``` shell
$ cmake --build [path to the CMake build directory] --target generate_[TYPE]_outputs
```

This generates output files in `[TYPE]/[TEST_CASE_NAME].expected.txt`.

