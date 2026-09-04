# Software
## VSCode 
**Switch Themes**: File - Preference - Themes - Color Theme


## Python
**pytest**
- Install: `pip install pytest`
- Write testcase: function name starts with `test_xxx()`, use built-in `assert` to check
- Run: cd to python script dir, then run `pytest -v`

**pip source**
- [tsinghua] (https://mirrors.tuna.tsinghua.edu.cn/pypi/web/simple)

## Matlab

### Basics
- Help for cmd
    ```matlab
    help imread
    help plot
    ```
- Print format string
  ```matlab
  fprintf("1 + 1 = %d\n", 1 + 1)
  ```
- Use `;` to prevent output
- `.asv` files are `auto-save` files created by matlab's auto-save feature
- Index start with 1
- `A(i:j)`, where `j` is included
- Multiple assignments is supported by function returns and `deal()` function
  ```matlab
  [value, index] = max([3, 1, 4, 1, 5]);  % value=5, index=5
  [a, b] = deal(1, 2)                     % a = 1, b = 2
  ```
- `...` is **continuation operator**, used to break a long line of code into multiple lines.

### Image processing
```matlab
A = imread("input.png")             % load image
imshow(A)                           % show image
imwrite(A, "output.png")            % save image

B = imresize(A, scale)              % resize with scale
B = imresize(A, scale, "bicubic")   % resize with method "bilinear", "bicubic" ...
B = imresize(A, [rows, cols])       % resize with specified size 
```

## ROS2
**jazzy**
- [Install](https://docs.ros.org/en/jazzy/Installation/Alternatives/Ubuntu-Development-Setup.html)
- Uninstall
  ```shell
  rm -rf ~/ros2_jazzy
  ```

## OpenGL
- Version
  ```shell
  glxinfo | grep -E 'OpenGL\s+(core|version|vendor)'
  ```




