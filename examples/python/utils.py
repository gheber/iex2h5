import numpy as np
import h5py
import os


def median_filter(X: np.ndarray, w: int = 3):
    T, N = X.shape
    half = w // 2
    for j in range(N):
        col = X[:, j]
        for t in range(half, T - half):
            window = col[t - half:t + half + 1]
            if abs(col[t] - np.median(window)) > 3 * np.std(window):
                col[t] = np.median(window)
    return X

def clamp_outliers(X: np.ndarray, max_jump: float = 0.1):
    T, N = X.shape
    for j in range(N):
        col = X[:, j]
        for t in range(1, T):
            r = (col[t] - col[t - 1]) / col[t - 1]
            if abs(r) > max_jump:
                col[t] = col[t - 1]
    return X

def remove_nans(X: np.ndarray):
    m, n = X.shape
    keep = np.ones(n, dtype=bool)
    for j in range(n):
        x = X[:, j]
        if np.all(np.isnan(x)):
            keep[j] = False
            continue
        # extend edges
        f = np.argmax(~np.isnan(x))
        l = len(x) - 1 - np.argmax(~np.isnan(x[::-1]))
        if f > 0:
            x[:f] = x[f]
        if l < m - 1:
            x[l + 1:] = x[l]
        # interpolate interior NaN runs
        i = f
        while i <= l:
            if not np.isnan(x[i]):
                i += 1
                continue
            i0 = i - 1
            j2 = i
            while j2 <= l and np.isnan(x[j2]):
                j2 += 1
            i1 = j2
            if i1 >= m:
                break
            a, b = x[i0], x[i1]
            length = i1 - i0
            for k in range(1, length):
                x[i0 + k] = a + (b - a) * (k / length)
            i = i1 + 1
    return X[:, keep]

def get_prices(path, N, sanitize_data=True):
    with h5py.File(path, 'r') as rts:
        dates = list(rts["/trading_days.txt"])
        instruments = list(rts["/instruments.txt"])
        D = len(dates)
        I_full = len(instruments)

        # rank instruments by average trade count
        stats = rts["/stats"]
        STATS = np.zeros((D, I_full), dtype=np.float32)
        for d, date in enumerate(dates):
            cnt = rts[f"/stats/{date.decode()}/trade_count"][()]
            STATS[d, :len(cnt)] = cnt

        μ = np.mean(STATS, axis=0)
        picks = np.argsort(μ)[::-1][:min(N, I_full)]

        # infer T from first day
        sample = rts[f"/rts/trade/{dates[0].decode()}"]
        T = sample.shape[1]
        X = np.empty((D * T, len(picks)), dtype=np.float32)
        for d, date in enumerate(dates):
            mat = rts[f"/rts/trade/{date.decode()}"][()]
            X[d*T:(d+1)*T, :] = mat[picks,:].T
        if sanitize_data:
            X = remove_nans(X)
            X = clamp_outliers(X)

        picks_names = [instruments[i].decode() if isinstance(instruments[i], bytes) else instruments[i] for i in picks]
        date_strs = [d.decode() if isinstance(d, bytes) else d for d in dates]
        return X, picks_names, date_strs

def logreturns(X: np.ndarray, mode='continuous', T=None):
    eps = np.finfo(X.dtype).eps
    X_clipped = np.maximum(X, eps)
    
    if mode == 'continuous':
        return np.diff(np.log(X_clipped), axis=0).astype(np.float32)

    elif mode == 'by_day':
        if T is None:
            raise ValueError("Must provide T for mode='by_day'")
        if X.shape[0] % T != 0:
            raise ValueError("Number of rows not divisible by T")

        D = X.shape[0] // T
        N = X.shape[1]
        R = np.empty((D * (T - 1), N), dtype=np.float32)
        for d in range(D):
            lo = d * T
            hi = (d + 1) * T
            block = X[lo:hi, :]
            num = np.maximum(block[1:, :], eps)
            den = np.maximum(block[:-1, :], eps)
            R[d*(T-1):(d+1)*(T-1), :] = np.log(num / den)
        return R
    else:
        raise ValueError("mode must be 'continuous' or 'by_day'")
