# Development Guide

This page documents the development workflow and setup for contributors to `iex2h5`.

## Local Development Environment

To build and test `iex2h5`, the following tools are required:

### System Dependencies

```bash
sudo apt install build-essential cmake libhdf5-dev pigz
pipx install mkdocs
pipx inject mkdocs mkdocs-material
```
SPDX-License-Identifier: MIT
### Shipped with the Project

The following third-party libraries are bundled under `thirdparty/` and integrated directly via CMake. Each is provided under its original license, as noted below.

| Library             | Version | Purpose                                          | License                                    |
| ------------------- | ------- | ------------------------------------------------ | ------------------------------------------ |
| **h5cpp**           | v1.10.8 | Modern C++17 interface to HDF5                   | [MIT](licenses/h5cpp)            |
| **hdf5**            | v1.14.3 | C interface and binary format for storing scientific data | [BSD-like](licenses/hdf5)         |
| **libzng**          | v2.1.6   | Optimized zlib-compatible compression library     | [Zlib](licenses/libzng)                      |
| **armadillo**       | v14.0.1 | Linear algebra library (BLAS/LAPACK backend)     | [Apache 2.0](licenses/armadillo) |
| **argparse**        | v3.1.0  | Header-only argument parser for C++11/14         | [MIT](licenses/argparse)         |
| **date**            | v3.0.1  | C++ date/time parsing utilities                  | [MIT](licenses/date)             |
| **fmt**             | v11.2.0 | C++ fmt parsing utilities                        | [MIT](licenses/fmt)             |
| **doctest**         | v2.4.11 | Lightweight C++ testing framework                | [MIT](licenses/doctest)          |
| **hiredis**         | v1.2.0  | C client for Redis, used for low-level Redis I/O | [BSD 3-Clause](licenses/hiredis) |
| **redis-plus-plus** | v1.3.11 | C++ wrapper over hiredis for modern Redis usage  | [Apache 2.0](licenses/rediscpp)  |


>  **Note:** All third-party libraries are integrated via `thirdparty/CMakeLists.txt`, and are **built as part of the project**. No system-wide installation or external dependency resolution is required.


##  Git Workflow
We follow a **GitHub issue-driven**, rebase-based workflow with a clean commit history and explicit issue linkage.

```bash
# 1. Create a descriptive GitHub issue
gh issue create --title "Implement XYZ" --body "Detailed motivation and scope..."

# 2. Create a branch from the issue (issue number will be assigned, e.g., 42)
gh issue develop 42  # creates and checks out '42-username-some-title'

# 3. Make commits with standardized format
git commit -m "[#42]:svarga:feature, implement XYZ parser"

# 4. Rebase interactively against release to keep history clean
git fetch origin
git rebase -i origin/release

# 5. Push with safety
git push --force-with-lease
````

### Commit Message Format

```
[#issue_number]:author:category, description
```

Examples:

* `[ #42 ]:svarga:feature, add support for IEX DEEP v105`
* `[ #37 ]:svarga:refactor, simplify tick decoding logic`
* `[ #58 ]:svarga:docs, add consumer module explanation`

Accepted categories: `feature`, `fix`, `refactor`, `docs`, `test`, `infra`, `perf`, etc.

>  All changes **must** be linked to an issue — even small doc fixes — to maintain traceability and changelog generation.

### 🌿 Branch Layout

* `main`: Reserved for stable releases.
* `staging`: All features are merged here after CI passes.
* `feature/*`, `fix/*`: Temporary branches for PRs and isolated work.

##  CI & GitHub Pages

### Continuous Integration

GitHub Actions runs on every push to `staging` and `release`. The pipeline includes:

- Compilation and testing (if present)
- Badge status generation
- Documentation build with MkDocs
- Deployment to the `gh-pages` branch

CI status badges are visible on the `README.md` and documentation front page.

### GitHub Pages

- Deployed from the `gh-pages` branch
- Built using `mkdocs build`
- Automatically published after successful CI run

## MkDocs Local Development

You can preview and edit the documentation locally.

### Option 1: With `pipx` (recommended for CLI tools)

```bash
pipx install mkdocs
pipx inject mkdocs mkdocs-material
````

If you don’t have `pipx`:
```bash
python3 -m pip install --user pipx
python3 -m pipx ensurepath
```

### Option 2: Local virtual environment

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install mkdocs mkdocs-material
```

### Live Preview

```bash
mkdocs serve
```

Then open: [http://127.0.0.1:8000](http://127.0.0.1:8000)

### Project Layout

```
docs/
├── index.md
├── usage.md
├── file-format.md
├── development.md
└── spec/*.pdf
```

### Custom Theme Overrides

Partial HTML for header/footer lives in:

```
docs/overrides/partials/
├── header.html
└── footer.html
```

Customize these to match your branding and style.

## Contribution Guidelines

- Follow C++23 idioms and naming conventions.
- Use `snake_case` and postfix types with `_t`.
- Ensure all changes compile cleanly with `-Wall -Wextra -Werror`.
- Document any public-facing or structural changes under `docs/`.


```cpp
struct my_type_t {
    std::string status(...){
        ....
    }
    void on_day_begin(time_point day) try {
        std::cout << status("▫ {}", day) << std::flush;   // newly created irts dataset
    } catch(const h5::error::io::dataset::create& err) {
        std::cout << status("▪ {}", day) << std::flush;   // overwrite existing dataset
    } catch(...) {
        std::cout << status("○ {}", day) << std::flush;   // some other problem
    }
    void on_heart_beat(time_point time) {
        std::cout << status(" {:r}", time) << std::flush; // where {:r} reset line/print into same line
    }
    void on_day_end(time_point day) try {
        std::cout << status(" ✓") << std::endl;           // succesful day closure
    } catch(...) {
        std::cout << status(" ✗") << std::endl;           // failed day closure
    }
    std::string state;
}



```