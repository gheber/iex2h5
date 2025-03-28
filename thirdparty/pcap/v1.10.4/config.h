#ifndef LIBPCAP_CONFIG_H
#define LIBPCAP_CONFIG_H

#define _U_ __attribute__((unused))
#define HAVE_STDINT_H 1
#define HAVE_SYS_SOCKET_H 1
#define HAVE_NETINET_IN_H 1
#define HAVE_UNISTD_H 1
#define PCAP_SUPPORT_LINUX 1
#define PCAP_SUPPORT_USB 1
#define PCAP_SUPPORT_BPF 1
#define HAVE_STRERROR 1
#define HAVE_STRLCPY 1
#define HAVE_STRLCAT 1

#define HAVE_SOCKLEN_T 0

#define PACKAGE_VERSION "v1.10.4"
#endif // LIBPCAP_CONFIG_H
