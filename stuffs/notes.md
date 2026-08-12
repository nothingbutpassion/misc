# Python
## pytest
- Install
  ```shell
  pip install pytest
  ```
- Write testcase
  ```py
  # NOTES: function name starts with `test_`
  def test_add() {
    # Use built-in `assert` to check
    assert 1+1 == 2, "Error: 1+1 != 2"
  }
  ```
- Run testcase
  ```shell
  cd <python-testcase-dir>
  pytest -v
  ```
