#include <stdlib.h>
#include <string.h>
#include <pcap/pcap.h>

void pcap_freecode(struct bpf_program *fp) {
    if (fp && fp->bf_insns) {
        free(fp->bf_insns);
        fp->bf_insns = NULL;
        fp->bf_len = 0;
    }
}

void sappend(char **strp, const char *s) {
    size_t old_len = *strp ? strlen(*strp) : 0;
    size_t add_len = strlen(s);
    *strp = realloc(*strp, old_len + add_len + 1);
    memcpy(*strp + old_len, s, add_len + 1);
}
