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
## OpenCV
- Set VideoCapture format
    ```shell
    # NOTES: first use v4l2-ctl to list supported formats
    # v4l2-ctl -d /dev/video0 --list-formats-ext
    ```
    ```py
    import cv2
    cap = cv2.VideoCapture(0)
    cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc(*'MJPG'))
    # controls whether captured frames are converted to RGB/BGR (default is 1)
    cap.set(cv2.CAP_PROP_CONVERT_RGB, 0)
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)
    w = cap.get(cv2.CAP_PROP_FRAME_WIDTH)
    h = cap.get(cv2.CAP_PROP_FRAME_HEIGHT)
    print(f"size: {w}x{h}")
    while True:
    ret, img = cap.read()
    if not ret:
        break
    # need coverting from jpeg to brg
    img = cv2.imdecode(img, cv2.IMREAD_COLOR)
    cv2.imshow("img", img)
    key = cv2.waitKey(30)
    if key in [27, ord('q')]:
        break
```

## Node.js
- **Downlaod/Install**
    ```bash
    # Download and install nvm:
    curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.40.8/install.sh | bash
    # in lieu of restarting the shell
    \. "$HOME/.nvm/nvm.sh"
    # Download and install Node.js:
    nvm install 24
    # Verify the Node.js version:
    node -v # Should print "v24.22.0".
    # Verify npm version:
    npm -v # Should print "11.19.1".
    ```
- **nvm** is a cross-platform Node.js version manager.
    ```bash
    
    # Note: <version> refers to any version-like string nvm understands. This includes:
    # - full or partial version numbers, starting with an optional "v" (0.10, v0.1.2, v1)
    # - default (built-in) aliases: node, stable, unstable, iojs, system
    # - custom aliases you define with `nvm alias foo`
    
    nvm --help
    nvm --version                         # Print out the installed version of nvm
    nvm list                              # List 
    nvm install [<version>]               # Ex: nvm install 24.22.0
    nvm uinstall [<version>]              # 
    nvm deactivate                        # Undo effects of `nvm` on current shell
    
    nvm install 8.0.0                     # Install a specific version number
    nvm use 8.0                           # Use the latest available 8.0.x release
    nvm run 6.10.3 app.js                 # Run app.js using node 6.10.3
    nvm exec 4.8.3 node app.js            # Run `node app.js` with the PATH pointing to node 4.8.3
    nvm alias default 8.1.0               # Set default node version on a shell
    nvm alias default node                # Always default to the latest available node version on a shell
    
    nvm install node                      # Install the latest available version
    nvm use node                          # Use the latest version
    nvm install --lts                     # Install the latest LTS version
    nvm use --lts                         # Use the latest LTS version
    ```


