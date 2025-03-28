#!/bin/bash
set -e

if [[ -z "$1" ]]; then
    echo "Usage: $0 <path-to-libpcap-directory>"
    exit 1
fi

PCAP_DIR="$1"

if [[ ! -d "$PCAP_DIR" ]]; then
    echo "Error: Directory '$PCAP_DIR' does not exist."
    exit 1
fi

cd "$PCAP_DIR"

rm -rf fad-glifc.c dlpisubs.c dlpisubs.h \
 pcap-config.1 pcap-dll.rc  mkdep nomkdep install-sh doc testprogs *.md *.3pcap arcnet.h atmuni31.h cmake lbl missing msdos rpcap-protocol.* pcap-rpcap* pcap-npf.c pcap-nit.c pcap-null.c pcap-dos.* pcap-dbus.* pcap-dlpi.c pcap-libdlpi.c pcap-septel.* \
pcap-sita.* pcap-sita.html pcap-snf.* pcap-snit.c pcap-snoop.c pcap-pf.c pcap-dag.* pcap-dpdk.* pcap-netmap.* pcap-tc.* pcap-rdmasniff.* pcap-haiku.cpp rpcapd TODO CHANGES CMakeLists.txt \
scanner.l configure* Makefile* aclocal* config.* *.in *.am m4 tests ChmodBPF chmod_bpf org.tcpdump.chmod_bpf.plist