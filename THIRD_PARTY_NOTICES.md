# Third-Party Notices

TIDEGLASS uses the following third-party software. The corresponding license
texts are included in the [`licenses/`](licenses/) directory.

## Direct dependencies

| Software | Version | Purpose | License file | Changes |
|---|---:|---|---|---|
| [Assimp](https://github.com/assimp/assimp) | 6.0.4 | FBX/GLB and other 3D model loading | [`assimp-LICENSE.txt`](licenses/assimp-LICENSE.txt) | None |
| [GLEW](https://github.com/nigels-com/glew) | 2.3.1 | OpenGL extension loading | [`glew-LICENSE.txt`](licenses/glew-LICENSE.txt) | None |
| [GLFW](https://github.com/glfw/glfw) | 3.4 | Window, OpenGL context and input handling | [`glfw3-LICENSE.txt`](licenses/glfw3-LICENSE.txt) | None |
| [GLM](https://github.com/g-truc/glm) | 1.0.3 | Vector, matrix and 3D mathematics | [`glm-LICENSE.txt`](licenses/glm-LICENSE.txt) | None |
| [stb](https://github.com/nothings/stb) | 2024-07-29 | Image loading | [`stb-LICENSE.txt`](licenses/stb-LICENSE.txt) | None |
| [Dear ImGui](https://github.com/ocornut/imgui) | 1.92.7 | In-game user interface | [`IMGUI-LICENSE.txt`](licenses/IMGUI-LICENSE.txt) | Integrated into the game source |
| [JSON for Modern C++](https://github.com/nlohmann/json) | 3.12.0 | JSON parsing | [`NLOHMANN-JSON-LICENSE.txt`](licenses/NLOHMANN-JSON-LICENSE.txt) | None |

## Transitive dependencies

The vcpkg dependency graph also contains the following components. Their
original copyright and license notices are included without modification in
the `licenses/` directory.

- EGL Registry
- jhasse-poly2tri
- kubazip
- minizip
- OpenGL and OpenGL Registry
- Clipper/polyclipping
- pugixml
- RapidJSON
- utfcpp
- zlib

## Build tooling

vcpkg and its CMake helper ports are used to restore and build dependencies.
They are build tools and are not bundled as part of the game executable.

This notice does not replace the license text of any component. If this list
and an included license text conflict, the included license text controls.
