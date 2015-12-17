/*
 * Copyright (c) 2011, 2012, 2013 Nicira, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <config.h>

#include "increment_cookie.h"

#include "byte-order.h"
#include "dynamic-string.h"
#include "match.h"
#include "meta-flow.h"
#include "nx-match.h"
#include "ofp-actions.h"
#include "ofp-errors.h"
#include "ofp-util.h"
#include "ofpbuf.h"
#include "openflow/openflow.h"
#include "ovs-atomic.h"
#include "unaligned.h"
#include <unistd.h>

static atomic_ullong cookie_count = ATOMIC_VAR_INIT(0); 

/* Converts 'nic' into a "struct ofpact_increment_cookie" and appends that 
 * struct to 'ofpacts'.  Returns 0 if successful, otherwise an OFPERR_*. */
enum ofperr
increment_cookie_from_openflow(const struct nx_action_increment_cookie *nic,
                               struct ofpbuf *ofpacts)
{
    fprintf(stderr, "increment_cookie_from_openflow called\n");
    struct ofpact_increment_cookie *incr_cookie;

    incr_cookie = ofpact_put_INCREMENT_COOKIE(ofpacts);
    fprintf(stderr, "increment_cookie_from_openflow returning\n");
    return 0;
}

/* Checks that 'incr_cookie' is a valid action on 'flow'.  Returns 0 always. */
enum ofperr
increment_cookie_check(const struct ofpact_increment_cookie *incr_cookie,
                       const struct flow *flow)
{
    fprintf(stderr, "increment_cookie_check called\n");
    struct match match;
    match_init_catchall(&match);

    fprintf(stderr, "increment_cookie_check returning\n");
    return 0;
}

/* Converts 'learn' into a "struct nx_action_learn" and appends that action to
 * 'ofpacts'. */
void
increment_cookie_to_nxast(const struct ofpact_increment_cookie *incr_cookie,
                          struct ofpbuf *openflow)
{
    struct nx_action_increment_cookie *nic;
    fprintf(stderr, "increment_cookie_to_nxast called\n");

    nic = ofputil_put_NXAST_INCREMENT_COOKIE(openflow);
    fprintf(stderr, "increment_cookie_to_nxast returning\n");
}

/* Increments a global shared value for a cookie, and then returns the value */
uint64_t
increment_cookie_execute(const struct ofpact_increment_cookie *incr_cookie,
                         struct flow *flow)
{
    fprintf(stderr, "increment_cookie_execute called\n");
    // Increment cookie value
    unsigned long long int orig;
    atomic_add(&cookie_count, 1, &orig);
    
    fprintf(stderr, "increment_cookie_execute returning\n");
    return (uint64_t) orig; 
}

/* Returns NULL if successful, otherwise a malloc()'d string describing the
 * error.  The caller is responsible for freeing the returned string. */
static char * WARN_UNUSED_RESULT
increment_cookie_parse__(char *orig, char *arg, struct ofpbuf *ofpacts)
{
    struct ofpact_increment_cookie *incr_cookie;
    fprintf(stderr, "increment_cookie_parse__ called\n");

    incr_cookie = ofpact_put_INCREMENT_COOKIE(ofpacts);

    //ofpact_update_len(ofpacts, &incr_cookie->ofpact);
    fprintf(stderr, "increment_cookie_parse__ returning\n");
}

/* Parses 'arg' as a set of arguments to the "increment_cookie" action and 
 * appends a matching OFPACT_INCREMENT_COOKIE action to 'ofpacts'. 
 * ovs-ofctl(8) describes the format parsed.
 *
 * Returns NULL if successful, otherwise a malloc()'d string describing the
 * error.  The caller is responsible for freeing the returned string.
 *
 * If 'flow' is nonnull, then it should be the flow from a struct match that is
 * the matching rule for the learning action.  This helps to better validate
 * the action's arguments.
 *
 * Modifies 'arg'. */
char * WARN_UNUSED_RESULT
increment_cookie_parse(char *arg, struct ofpbuf *ofpacts)
{
    char *orig = xstrdup(arg);
    char *error = increment_cookie_parse__(orig, arg, ofpacts);
    free(orig);
    return error;
}

/* Appends a description of 'increment_cookie' to 's',
 * in the format that ovs-ofctl(8) describes. */
void
increment_cookie_format(const struct ofpact_increment_cookie *incr_cookie,
                        struct ds *s)
{
    struct match match;
    fprintf(stderr, "increment_cookie_format called\n");

    match_init_catchall(&match);

    ds_put_cstr(s, "increment_cookie()");
    fprintf(stderr, "increment_cookie_format returning\n");
}
