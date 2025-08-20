using HDF5, Dates, Statistics, StatsBase          # Load core libraries for HDF5 I/O, date/time, stats, and statistical sampling
using GRUtils                                     # Plotting backend

path = joinpath(homedir(), "iex.h5")              # Construct path to HDF5 container in user's home directory
fd = h5open(path, "r")                            # Open HDF5 file for reading
dates = read(fd["/trading_days.txt"])             # Read list of trading days (as strings or dates)
instruments = read(fd["/instruments.txt"])        # Read list of instrument names

stats = fd["/stats"]                              # Navigate into the stats group
day = last(dates)                                 # Select the last available trading day

N = Int.(read(stats[day]["trade_count"]))         # Read trade counts for that day and convert to Int
sort(N, rev=true)                                 # Sort trade counts descending (not assigned here)

cutoff_limit = 10                                 # Show top 10 most active instruments by trade count
top_indices = sortperm(N, rev=true)               # Get permutation of indices sorted by descending trade count
N[top_indices]                                    # Display sorted trade counts

instruments[top_indices][1:10]                    # Show top 10 most traded instrument names
ticks = read(fd["irts/" * day])                   # Read all tick events for the selected day

i = top_indices[3] - 1                            # Select the 3rd most traded instrument (adjusting for 0-based contract_id)

# Count tick types for this instrument (by flags):
nbids   = count(t -> t.contract_id == i && t.flags == 0x0001, ticks)  # Number of BID events
ntrades = count(t -> t.contract_id == i && t.flags == 0x0002, ticks)  # Number of TRADE events
nasks   = count(t -> t.contract_id == i && t.flags == 0x0004, ticks)  # Number of ASK events
nall    = count(t -> t.contract_id == i, ticks)                       # Total number of events for this instrument
nbids + ntrades + nasks == nall                                      # Sanity check: part sums should equal total

A = filter(t -> t.contract_id == i && t.flags == 0x0002, ticks)       # Extract only TRADE events for the instrument

times_ns = [t.time for t in A]                                        # Get trade timestamps in nanoseconds (UInt64 UNIX time)
times_dt = unix2datetime.(times_ns ./ 1e9)                            # Convert nanosecond UNIX timestamps to DateTime
prices = [t.price for t in A]                                         # Extract trade prices

plot(prices)                                                          # Plot the price time series
instruments[i]                                                        # Return instrument name (Julia indexing)

contract_ids = [t.contract_id for t in ticks]                         # Collect all contract IDs for further analysis
close(fd)                                                             # Close the HDF5 file
