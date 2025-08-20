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
