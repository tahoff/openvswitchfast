#ifndef INCREMENT_COOKIE_H
#define INCREMENT_COOKIE_H 1

#include "compiler.h"
#include "ofp-errors.h"

struct ds;
struct flow;
struct flow_wildcards;
struct ofpbuf;
struct ofpact_increment_cookie;
struct ofputil_flow_mod;
struct nx_action_increment_cookie;

/* NXAST_INCREMENT_COOKIE helper functions.
 *
 * See include/openflow/nicira-ext.h for NXAST_INCREMENT_COOKIE specification.
 */

enum ofperr increment_cookie_from_openflow(
    const struct nx_action_increment_cookie *,
    struct ofpbuf *ofpacts);
enum ofperr increment_cookie_check(const struct ofpact_increment_cookie *,
                                   const struct flow *);
void increment_cookie_to_nxast(const struct ofpact_increment_cookie *,
                               struct ofpbuf *openflow);

uint64_t increment_cookie_execute(
    const struct ofpact_increment_cookie *, struct flow *);

char *increment_cookie_parse(char *, struct ofpbuf *ofpacts) WARN_UNUSED_RESULT;
void increment_cookie_format(const struct ofpact_increment_cookie *,
                             struct ds *);
uint64_t get_cookie_val(void);

#endif /* increment_cookie.h */
