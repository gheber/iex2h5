
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
import os
import numpy as np
from matplotlib import pyplot as plt
from utils import get_prices, remove_nans, clamp_outliers, logreturns
import matplotlib.pyplot as plt
plt.ion() 
%matplotlib inline 

path = os.path.expanduser("~/rts.h5")                  # HDF5 container path
X, names, days = get_prices(path, 10, sanitize_data=False)  # Load prices without cleaning
num_nans = np.isnan(X).sum()                           # Count missing values
X = remove_nans(X)                                     # Clean NaNs in-place
X = clamp_outliers(X, max_jump=0.1)                    # Clip large jumps (>±10%)
plt.plot(X[:, 1])                                      # Visualize one asset
plt.savefig("plot.png")

X, names, days = get_prices(path, 10, sanitize_data=True)   # Fully cleaned load
R = logreturns(X, mode="by_day", T=len(days))               # Compute intra-day log returns
m, n = R.shape
mu = np.mean(R, axis=0)                                     # Mean return per asset
R_tilde = R - np.ones((m, 1)) @ mu.reshape(1, -1)           # De-mean each column
```

These functions will clean up outliers and prepare data for downstream analysis like log-return modeling, PCA, or factor decomposition.
