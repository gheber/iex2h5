using HDF5, Dates, Statistics, StatsBase

function median_filter!(P::AbstractMatrix{<:AbstractFloat}, w::Int=3)
    T, N = size(P)
    half = div(w,2)
    @inbounds for j in 1:N
        col = @view P[:, j]
        for t in (1+half):(T-half)
            window = col[(t-half):(t+half)]
            if abs(col[t] - median(window)) > 3 * std(window)
                col[t] = median(window)
            end
        end
    end
    return P
end
function clamp_outliers!(P::AbstractMatrix{<:AbstractFloat}; max_jump=0.2)
    T, N = size(P)
    @inbounds for j in 1:N
        col = @view P[:, j]
        for t in 2:T
            r = (col[t] - col[t-1]) / col[t-1]
            if abs(r) > max_jump
                col[t] = col[t-1] # replace with prev price
            end
        end
    end
    return P
end
function remove_nans!(P::AbstractMatrix{T}) where {T<:AbstractFloat}
    m, n = size(P)
    keep = trues(n)  # drop columns that are all-NaN
    @inbounds for j in 1:n
        x = @view P[:, j]
        if all(isnan, x)
            keep[j] = false
            continue
        end
        f = findfirst(!isnan, x)::Int
        l = findlast(!isnan,  x)::Int

        # extend edges
        if f > 1; x[1:f-1] .= x[f]; end
        if l < m; x[l+1:end] .= x[l]; end

        # interpolate interior NaN runs
        i = f
        while i <= l
            if !isnan(x[i]); i += 1; continue; end
            i₀ = i - 1
            j₂ = i
            while j₂ <= l && isnan(x[j₂]); j₂ += 1; end
            i₁ = j₂                       # first non-NaN after the run
            a, b = x[i₀], x[i₁]
            len = i₁ - i₀
            @simd for k in 1:len-1
                x[i₀ + k] = a + (b - a) * (k/len)
            end
            i = i₁ + 1
        end
    end
    return P[:, keep]   # drop all-NaN columns
end

function get_prices(path::AbstractString, N::Integer; sanitize_data::Bool=true)
    rts = h5open(path, "r")
    try
        dates        = read(rts["/trading_days.txt"])
        instruments  = read(rts["/instruments.txt"])
        D, I_full    = length(dates), length(instruments)

        # --- rank instruments by avg trade_count over days ---
        stats = rts["/stats"]
        STATS = zeros(Float32, D, I_full)
        for (d, date) in enumerate(dates)
            cnt = vec(read(stats[string(date)]["trade_count"]))
            n   = length(cnt)
            @inbounds STATS[d, 1:n] = cnt
        end
        μ = vec(mean(STATS, dims=1))
        picks = sortperm(μ, rev=true)[1:min(N, I_full)]

        # --- infer T from first day, stack all days ---
        trades     = rts["/rts/trade"]
        sample_mat = read(trades[string(dates[1])])          # T×I_full
        T, _       = size(sample_mat)
        X = Matrix{Float32}(undef, D*T, length(picks))

        for (d, date) in enumerate(dates)
            mat = read(trades[string(date)])                 # T×I_full (Float32)
            @inbounds X[(d-1)*T+1 : d*T, :] = mat[:, picks]  # take top-N cols
        end
        if sanitize_data
            remove_nans!(X)
            clamp_outliers!(X)
        end
        return X, instruments[picks], dates
    finally
        close(rts)
    end
end

function logreturns(X::AbstractMatrix; mode::Symbol=:continuous, T::Union{Int,Nothing}=nothing)
    Tval = eltype(X)
    tiny = eps(Tval)

    if mode === :continuous
        @views return Float32.(diff(log.(max.(X, tiny)); dims=1))
    elseif mode === :by_day
        T === nothing && throw(ArgumentError("Provide T for :by_day"))
        size(X,1) % T != 0 && throw(ArgumentError("Rows not divisible by T"))

        D, N = size(X,1) ÷ T, size(X,2)
        R = Matrix{Float32}(undef, D*(T-1), N)

        @inbounds @views for d in 1:D
            lo = (d-1)*T + 1; hi = d*T
            block = X[lo:hi, :]             # <- no @view here

            num_v = block[2:end, :]         # views thanks to @views
            den_v = block[1:end-1, :]

            num = max.(num_v, tiny)
            den = max.(den_v, tiny)

            R[(d-1)*(T-1)+1 : d*(T-1), :] .= Float32.(log.(num ./ den))
        end
        return R
    else
        throw(ArgumentError("mode must be :continuous or :by_day"))
    end
end
