#ifndef NOVA_TCG2_H
#define NOVA_TCG2_H

/* Minimal UEFI/TCG2 event-log evidence parser for NovaOS Stage-4D.
 * The log is evidence, not an authority: replayed PCRs must be compared with
 * live TPM PCRs before the evidence is accepted. */

struct nova_tcg2_evidence {
    unsigned long log_bytes;
    unsigned long event2_count;
    unsigned long pcr7_sha256_events;
    char log_sha256[65];
    char replayed_pcr7_sha256[65];
};

int nova_tcg2_parse_event_log(const unsigned char *log, unsigned long len,
                              struct nova_tcg2_evidence *out);
#ifndef NOVA_TCG2_PARSER_ONLY
int nova_tcg2_collect_evidence(const char *path, struct nova_tcg2_evidence *out);
#endif
const char *nova_tcg2_last_error(void);

#endif
