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
