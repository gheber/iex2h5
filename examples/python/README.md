
## 🐍 Python Examples

This project includes Python scripts for exploring and cleaning financial time series stored in HDF5 containers.

### ✅ Step 1: Install Python

Download the latest stable release of **Python 3.10+** from [python.org](https://www.python.org/downloads/) and follow install instructions for your OS.

* **macOS/Linux:** Use your package manager or install via pyenv

  ```bash
  sudo apt install python3 python3-pip  # Debian/Ubuntu
  # or use pyenv: https://github.com/pyenv/pyenv
  ```
* **Windows:** Use the official installer and make sure to check *“Add Python to PATH”*.

Confirm installation:

```bash
python --version
```

### 📦 Step 2: Install Required Packages

Open a terminal and create a virtual environment (optional but recommended):

```bash
python -m venv .venv
source .venv/bin/activate     # macOS/Linux
.venv\Scripts\activate        # Windows
pip install numpy h5py matplotlib # or: pip install -r requirements.txt
```

### ▶️ Step 3: Run the Examples Interactively

1. Open the `clean_and_center_returns.py` script or similar in VS Code
2. Select code blocks and run them with `Shift+Enter` or in the integrated Python terminal
3. Modify and explore the examples as needed

Example usage:

```python
using Statistics, GRUtils
include("utils.jl")

path = joinpath(homedir(), "rts.h5")           # Construct path to RTS HDF5 container in home directory
X, names, days = get_prices(path, 10, sanitize_data=false)  # Load prices for top 10 assets without cleaning
num_nans = count(isnan, X)                     # Count NaN values in the matrix (missing or corrupt data)
remove_nans!(X)                                # Clean in-place by removing NaNs
plot(X[:, 2])                                  # Visualize second asset's price series (for glitch inspection)
clamp_outliers!(X, max_jump=0.1)               # Clip large log-return jumps (> ±10%) — helps deal with bad ticks
# median_filter!(X, 17)                        # Smooth data with a 17-point median filter (optional)
plot(X[:, 2])                                  # The sudden jumps if any is expected to be gone 

X, names, days = get_prices(path, 10, sanitize_data=true)  # Load top 10 assets with full cleaning
R = logreturns(X, mode=:by_day, T=length(days))            # Compute intra-day log returns, strip overnight jumps
m,n = size(R)                                              # Dimensions: days × assets
μ = vec(mean(R, dims=1))                                   # Mean return per asset (n-vector)
R̃ = R .- ones(m) * μ'                                      # De-mean each column (centered returns for PCA/factor modeling)
```

These functions will clean up outliers and prepare data for downstream analysis like log-return modeling, PCA, or factor decomposition.
