#ifndef LEARN_LEARN_H
#define LEARN_LEARN_H 1

#include "compiler.h"
#include "ofp-errors.h"

struct ds;
struct flow;
struct flow_wildcards;
struct ofpbuf;
struct ofpact_learn_learn;
struct ofputil_flow_mod;
struct nx_action_learn_learn;

/* NXAST_LEARN_LEARN helper functions.
 *
 * See include/openflow/nicira-ext.h for NXAST_LEARN_LEARN specification.
 */

enum ofperr learn_learn_from_openflow(const struct nx_action_learn_learn *,
                                      struct ofpbuf *ofpacts);
enum ofperr learn_learn_check(const struct ofpact_learn_learn *,
                              struct flow *);
void learn_learn_to_nxast(const struct ofpact_learn_learn *,
                           struct ofpbuf *openflow);

void learn_learn_execute(const struct ofpact_learn_learn *, const struct flow *,
                         struct ofputil_flow_mod *, struct ofpbuf *ofpacts);
void learn_learn_mask(const struct ofpact_learn_learn *, struct flow_wildcards *);

char *learn_learn_parse(char *, struct ofpbuf *ofpacts) WARN_UNUSED_RESULT;
void learn_learn_format(const struct ofpact_learn_learn *, struct ds *);

#endif /* learn_learn.h */
