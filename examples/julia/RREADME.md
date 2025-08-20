## 🧪 Julia Examples

This project includes Julia scripts for exploring and cleaning financial time series stored in HDF5 containers.

### ✅ Step 1: Install Julia

Download the latest stable release from [julialang.org](https://julialang.org/downloads/) and follow install instructions for your OS.

* **macOS/Linux:** Extract and symlink:

  ```bash
  tar -xvzf julia-1.X.Y-linux-x86_64.tar.gz
  sudo ln -s $PWD/julia-1.X.Y/bin/julia /usr/local/bin/julia
  ```
* **Windows:** Use the installer and make sure to check *“Add Julia to PATH”*.

Confirm installation:

```bash
julia --version
```

---

### 💻 Step 2: Set Up VS Code for Julia

1. Install **[Visual Studio Code](https://code.visualstudio.com/Download)**
2. Launch VS Code and install the **“Julia”** extension by [julialang](https://marketplace.visualstudio.com/items?itemName=julialang.language-julia)
3. Optional but recommended:

   * Set Julia executable path via `Cmd+Shift+P → Julia: Executable Path`
   * Enable **Plot Pane**, **Linting**, and **Inline Evaluation** in settings

---

### 📦 Step 3: Install Required Packages

Open the integrated terminal (`Ctrl+~`), enter the Julia REPL (`julia`) and type:

```julia
using Pkg
Pkg.add(["HDF5", "StatsBase", "GRUtils"])
```

---

### ▶️ Step 4: Run the Examples Interactively

1. Open the `clean_and_center_returns.jl` script or similar in VS Code
2. Select code blocks or lines and run them with `Shift+Enter`
3. Explore and modify the examples interactively

Example usage in the REPL or notebook-style:

```julia
using Statistics, GRUtils
include("utils.jl")

path = joinpath(homedir(), "rts.h5")           # Construct path to RTS HDF5 container in home directory
X, names, days = get_prices(path, 10, sanitize_data=false)  # Load prices for top 10 assets without cleaning
num_nans = count(isnan, X)                     # Count NaN values in the matrix (missing or corrupt data)
remove_nans!(X)                                # Clean in-place by removing NaNs
plot(X[:, 2])                                  # Visualize second asset's price series (for glitch inspection)
clamp_outliers!(X, max_jump=0.2)               # Clip large log-return jumps (> ±20%) — helps deal with bad ticks
# median_filter!(X, 17)                        # Smooth data with a 17-point median filter (optional)
plot(X[:, 2])                                  # The sudden jumps if any is expected to be gone 

X, names, days = get_prices(path, 10, sanitize_data=true)  # Load top 10 assets with full cleaning
R = logreturns(X, mode=:by_day, T=length(days))            # Compute intra-day log returns, strip overnight jumps
m,n = size(R)                                              # Dimensions: days × assets
μ = vec(mean(R, dims=1))                                   # Mean return per asset (n-vector)
R̃ = R .- ones(m) * μ'                                      # De-mean each column (centered returns for PCA/factor modeling)
```

These functions will clean up spurious spikes and prepare the data for downstream processing like log returns or factor models.
