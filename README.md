# IEX2H5: IEX TOPS Dataset to HDF5 Converter

A high-performance C++ utility for converting [IEX Transport Protocol (IEX-TP)][101] packet captures into structured HDF5 datasets, suitable for financial analytics, scientific computation, and time-series processing.

---

## 📦 Installation

1. **Install Intel oneAPI** (for Intel compilers and MKL)  
   Download from: [Intel oneAPI Base Toolkit](https://www.intel.com/content/www/us/en/developer/tools/oneapi/base-toolkit-download.html)

2. **Install required libraries**

```bash
sudo apt install libgtest-dev libhdf5-dev libboost-program-options-dev \
    libboost-system-dev libgoogle-glog-dev libgoogle-perftools-dev libcpprest-dev libpcap-dev pigz
# intall Howard Hinnant's date library
git clone https://github.com/HowardHinnant/date.git && cd date
cmake -DBUILD_TZ_LIB=ON . && make && sudo make install
```
# Example Usage: Convert IEX TOPS Dataset
```
steven@jupyter:~/projects/iex2h5/src$ ./iex2h5 --help
IEX2H5 converts IEX TOPS Datasets to HDF5 Format

iex2h5 is a specialized tool for importing IEX TOPS datasets into the HDF5 data format,
enabling efficient  storage and analysis of large  financial datasets. HDF5 is a widely
used file format for handling large, complex, and hierarchical data, supported by major
programming languages including Julia, Python, MATLAB, C, C++, and Node.js.

This application allows users to convert captured packet data streams (e.g., DEEP/TOPS)
into structured HDF5 datasets for advanced analytics and seamless integration into
scientific, engineering, and financial workflows.

Usage: ./iex2h5 [--help] [--time-interval VAR] [--start VAR] [--stop VAR] [--output VAR] [--rts-path VAR] [--instruments-path VAR] [--trading-days-path VAR] [--gzip VAR] [--command VAR]

Optional arguments:
  -h, --help           shows help message 
  --time-interval      temporal interval in seconds, irts stream is converted into [nargs=0..1] [default: 10]
  --start              lower bound in UTC, considers events only after [nargs=0..1] [default: "14:30:00"]
  --stop               upper bound in UTC, considers events only before [nargs=0..1] [default: "21:00:00"]
  -o, --output         path to the HDF5 container [nargs=0..1] [default: "./iex.h5"]
  --rts-path           hdf5-group/directory for regular time interval index [nargs=0..1] [default: "/time.txt"]
  --instruments-path   hdf5-group/directory for listed [symbols|assets|financial] instruments [nargs=0..1] [default: "/instruments.txt"]
  --trading-days-path  hdf5-group/directory for active trading days [nargs=0..1] [default: "/trading_days.txt"]
  -g, --gzip           0-9 0 for no compression, 9 for highest [nargs=0..1] [default: 0]
  -c, --command        init  - intitialises hdf5 container with retrieved symbols from irts/stream
                       irts  - saves captured events as irts stream
                       rts   - converts irts to rts
                       index - scans and rebuilds trading day index
                       
 [nargs=0..1] [default: "rts"]


example:
   unpigz -c tops.pcap.gz | ./iex2h5 -g 9 --time-interval 10 --command init
   for file in repo/*.pcap.gz; do unpigz -c ${file} | iex2h5 -g 9 --command rts -o ${HOME}/iex.h5;

Copyright © <2017-2025> Varga Consulting, Toronto, ON, info@vargaconsulting.ca
```
