import os                                              # Filesystem utilities
import h5py                                            # HDF5 reader
import numpy as np                                     # Numerical operations
import matplotlib.pyplot as plt                        # Plotting
from datetime import datetime                          # Time conversion

def unix2datetime_ns(ns_array):                        # Convert UNIX ns timestamps to datetime
    return [datetime.utcfromtimestamp(ns * 1e-9) for ns in ns_array]

path = os.path.expanduser("~/iex.h5")                  # Path to HDF5 file
fd = h5py.File(path, "r")                              # Open file for reading

dates = [d.decode() for d in fd["/trading_days.txt"][:]]       # Read trading day list
instruments = [i.decode() for i in fd["/instruments.txt"][:]]  # Read instrument names

stats = fd["/stats"]                                   # Navigate into stats group
day = dates[-1]                                        # Select last trading day

N = stats[day]["trade_count"][:].astype(int)           # Read trade counts and convert to int
top_indices = np.argsort(N)[::-1]                      # Indices sorted by descending count

cutoff_limit = 10                                      # Number of top instruments to show
top_names = [instruments[i] for i in top_indices[:cutoff_limit]]  # Top instrument names
top_counts = N[top_indices[:cutoff_limit]]             # Top instrument trade counts

ticks = fd[f"/irts/{day}"][:]                          # Load tick events for selected day

i = top_indices[4]                                     # Select 6th most traded instrument

nbids = np.count_nonzero((ticks["contract_id"] == i) & (ticks["flags"] == 0x0001))   # Count BID events
ntrades = np.count_nonzero((ticks["contract_id"] == i) & (ticks["flags"] == 0x0002)) # Count TRADE events
nasks = np.count_nonzero((ticks["contract_id"] == i) & (ticks["flags"] == 0x0004))   # Count ASK events
nall = np.count_nonzero(ticks["contract_id"] == i)                                   # Total events

assert nbids + ntrades + nasks == nall                        # Sanity check

trades = ticks[(ticks["contract_id"] == i) & (ticks["flags"] == 0x0002)]  # Extract trade events

times_ns = trades["time"]                                    # Extract timestamps
times_dt = unix2datetime_ns(times_ns)                        # Convert to datetime
prices = trades["price"]                                     # Extract prices

plt.figure(figsize=(10, 4))                                  # Create figure
plt.plot(times_dt, prices)                                   # Plot prices vs. time
plt.title(f"Trade prices for {instruments[i]}")              # Plot title
plt.xlabel("Time")                                           # X axis label
plt.ylabel("Price")                                          # Y axis label
plt.grid(True)                                               # Add grid
plt.tight_layout()                                           # Improve layout
plt.savefig("plot.png")                                      # Save plot to file

contract_ids = ticks["contract_id"].astype(int)              # Extract all contract IDs
