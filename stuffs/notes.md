# Python
## pytest
- Install
  ```shell
  pip install pytest
  ```
- Write testcase
  ```py
  # NOTES: function name starts with "test_"
  def test_add() {
    assert 1+1 == 2, "Error: 1+1 != 2"
  }
  ```
- Run testcase
  ```shell
  cd <python-test-script-dir>
  pytest -v
  ```
