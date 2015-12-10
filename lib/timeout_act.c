#include <config.h>

#include "timeout_act.h"

#include "byte-order.h"
#include "dynamic-string.h"
#include "learn.h"
#include "match.h"
#include "meta-flow.h"
#include "nx-match.h"
#include "ofp-actions.h"
#include "ofp-errors.h"
#include "ofp-parse.h"
#include "ofp-util.h"
#include "ofpbuf.h"
#include "ofproto/ofproto.h"
#include "ofproto/ofproto-provider.h"
#include "openflow/openflow.h"
#include "unaligned.h"

void
timeout_act_to_nxast(const struct ofpact_timeout_act *act,
                     struct ofpbuf *out)
{
    // TODO - TEST!

    /* -- ORIGINAL - Error in check, but otherwise good -- */
    struct nx_action_timeout_act *nta;
    size_t start_ofs = out->size;
    unsigned int remainder;
    unsigned int len;
    fprintf(stderr, "timeout_act_to_nxast called\n");

    nta = ofputil_put_NXAST_TIMEOUT_ACT(out);
    nta->ofpacts_len = htons(act->ofpacts_len);
    //out->size -= sizeof nta->ofpacts;

    fprintf(stderr, "nta->ofpacts_len=%u act->ofpacts_len=%u\n",
            nta->ofpacts_len, act->ofpacts_len);
    
    ofpacts_put_openflow10(act->ofpacts, act->ofpacts_len, out);
    //ofpbuf_put(out, act->ofpacts, act->ofpacts_len);
 
    if ((out->size - start_ofs) % 8) {
        ofpbuf_put_zeros(out, 8 - (out->size - start_ofs) % 8);
    }
  
    nta = ofpbuf_at(out, start_ofs, sizeof *nta);
    nta->len = htons(out->size - start_ofs);
  
    int i;
    fprintf(stderr, "...... ");
    for (i = 0; i < act->ofpacts_len; i++) {
        fprintf(stderr, "%d ", *(((char *) act->ofpacts) + i));
    }
    fprintf(stderr, "......\n");

    /*
    struct nx_action_timeout_act *nta;
    size_t start_ofs = out->size;
    unsigned int remainder;
    unsigned int len;
    fprintf(stderr, "timeout_act_to_nxast called %u\n", act->ofpacts_len);

    nta = ofputil_put_NXAST_TIMEOUT_ACT(out);
    nta->ofpacts_len = htons(act->ofpacts_len);
    //out->size -= sizeof nta->ofpacts;

    fprintf(stderr, "nta->ofpacts_len=%u act->ofpacts_len=%u\n",
            nta->ofpacts_len, act->ofpacts_len);
    fprintf(stderr, "act->ofpacts %p\n", act->ofpacts);
    ofpbuf_put(out, act->ofpacts, act->ofpacts_len);
 
    if ((out->size - start_ofs) % 8) {
        ofpbuf_put_zeros(out, 8 - (out->size - start_ofs) % 8);
    }
  
    nta = ofpbuf_at(out, start_ofs, sizeof *nta);
    nta->len = htons(out->size - start_ofs);
  
    int i;
    fprintf(stderr, "...... ");
    for (i = 0; i < act->ofpacts_len; i++) {
        fprintf(stderr, "%d ", *(((char *) (nta + 1)) + i));
    }
    fprintf(stderr, "......\n");
    */
}

enum ofperr
timeout_act_from_openflow(const struct nx_action_timeout_act *nta,
                          struct ofpbuf *out)
{
    // TODO - TEST

    /* -- ORIGINAL - Works, but error in check -- */
    struct ofpact_timeout_act *timeout_act;
    struct ofpact *ofpacts;
    unsigned int length;
    struct ofpbuf timeout_ofpacts;
    ofpbuf_init(&timeout_ofpacts, 32);
    struct ofpbuf converted_timeout_ofpacts;
    ofpbuf_init(&converted_timeout_ofpacts, 32);

    fprintf(stderr, "-- timeout_act_from_openflow called --\n");
    
    //length = ntohs(nta->len) - offsetof(struct nx_action_timeout_act, ofpacts);
    //timeout_act = ofpact_put(out, ofpact_timeout_act, 
    //                        offsetof(struct ofpact_timeout_act, ofpacts) + length);
    timeout_act = ofpact_put_TIMEOUT_ACT(out);
    //timeout_act->ofpacts_len = ntohs(nta->ofpacts_len);
    ofpacts = ofpbuf_put_zeros(out, ntohs(nta->ofpacts_len));
    ofpbuf_put(&timeout_ofpacts, nta + 1, timeout_act->ofpacts_len);

    timeout_act = out->l2;
    timeout_act->ofpacts_len = ntohs(nta->ofpacts_len);
    ofpbuf_put(&timeout_ofpacts, nta + 1, timeout_act->ofpacts_len);
    ofpacts_pull_openflow10(&timeout_ofpacts, timeout_act->ofpacts_len, &converted_timeout_ofpacts); 
    
    
    //ofpbuf_put(&timeout_ofpacts, nta + 1, timeout_act->ofpacts_len);
    
    timeout_act->ofpacts = ofpbuf_steal_data(&converted_timeout_ofpacts); 
    //timeout_act->ofpacts = ofpbuf_steal_data(&timeout_ofpacts); 
    //memcpy(ofpacts, nta + 1, timeout_act->ofpacts_len); 
    
    ofpact_update_len(out, &timeout_act->ofpact);

    fprintf(stderr, "timeout_act_from_openflow timeout_act=%p out=%p\n", timeout_act, out); 
    fprintf(stderr, "timeout_act_from_openflow length=%u nta->ofpacts_len=%u nta->ofpacts=%p\n",
            length, ntohs(nta->ofpacts_len), nta + 1);
    
   int i;
   fprintf(stderr, "...... ");
   for (i = 0; i < timeout_act->ofpacts_len; i++) {
       fprintf(stderr, "%d ", *(((char *) timeout_act->ofpacts) + i));
   }
   fprintf(stderr, "......\n");

    
    fprintf(stderr, "timeout_act->ofpact.len = %u\n", timeout_act->ofpact.len);
    fprintf(stderr, "timeout_act->ofpacts_len=%u out->size=%u nta->len=%u\n",
            timeout_act->ofpacts_len, out->size, ntohs(nta->len));
    fprintf(stderr, "-- timeout_act_from_openflow returning timeout_act->ofpacts%p --\n",
            timeout_act->ofpacts);

    return 0;

    /* NOT WORKING ...
    struct ofpact_timeout_act *timeout_act;
    struct ofpact *ofpacts;
    int i;
    unsigned int length;
    struct ofpbuf timeout_ofpacts;

    fprintf(stderr, "-- timeout_act_from_openflow called %u --\n", ntohs(nta->ofpacts_len));
    

    //length = ntohs(nta->len) - offsetof(struct nx_action_timeout_act, ofpacts);
    //timeout_act = ofpact_put(out, ofpact_timeout_act, 
    //                        offsetof(struct ofpact_timeout_act, ofpacts) + length);
    timeout_act = ofpact_put_TIMEOUT_ACT(out);
    //timeout_act->ofpacts_len = ntohs(nta->ofpacts_len);

    ofpbuf_init(&timeout_ofpacts, 32);
    
    timeout_act = out->l2;
    timeout_act->ofpacts_len = ntohs(nta->ofpacts_len);
    
    //ofpacts = ofpbuf_put_zeros(out, ntohs(nta->ofpacts_len));
    ofpbuf_put(&timeout_ofpacts, nta + 1, timeout_act->ofpacts_len);
    //fprintf(stderr, "timeout_act=%p ofpacts=%p\n");
    
    //memcpy(ofpacts, (nta + 1), timeout_act->ofpacts_len); 
    //ofpacts = ofpbuf_put(out, nta + 1, timeout_act->ofpacts_len);

    timeout_act->ofpacts = ofpbuf_steal_data(&timeout_ofpacts);
    fprintf(stderr, "...... ");
    for (i = 0; i < timeout_act->ofpacts_len; i++) {
        fprintf(stderr, "%d ", *(((char *) timeout_act->ofpacts) + i));
    }
    fprintf(stderr, "......\n");

    ofpact_update_len(out, &timeout_act->ofpact);

    fprintf(stderr, "timeout_act_from_openflow timeout_act=%p out=%p\n", timeout_act, out); 
    fprintf(stderr, "timeout_act_from_openflow length=%u nta->ofpacts_len=%u nta->ofpacts=%p timeout_act->ofpacts=%p\n",
            length, ntohs(nta->ofpacts_len), nta + 1, timeout_act->ofpacts);
   
   //((char *) timeout_act->ofpacts)[0] = 123;
   //for (i = 0; i < timeout_act->ofpacts_len; i++) {
   //    ((char *) timeout_act->ofpacts)[i] = ((char *) (nta + 1))[i];
   //}

   fprintf(stderr, "...... ");
   for (i = 0; i < timeout_act->ofpacts_len; i++) {
       fprintf(stderr, "%d ", (((char *) timeout_act->ofpacts)[i]));
   }
   fprintf(stderr, "......\n");


   fprintf(stderr, "...... ");
   for (i = 0; i < timeout_act->ofpacts_len; i++) {
       fprintf(stderr, "%d ", *(((char *) (nta + 1)) + i));
   }
   fprintf(stderr, "......\n");

   fprintf(stderr, "timeout_act->ofpact.len = %u\n", timeout_act->ofpact.len);
   fprintf(stderr, "timeout_act->ofpacts_len=%u out->size=%u nta->len=%u\n",
           timeout_act->ofpacts_len, out->size, ntohs(nta->len));
   fprintf(stderr, "-- timeout_act_from_openflow returning timeout_act->ofpacts%p --\n",
           timeout_act->ofpacts);

   return 0;
   */
}

enum ofperr timeout_act_check(const struct ofpact_timeout_act *act,
                              struct flow *flow) 
{
    // TODO - TEST
    fprintf(stderr, "~~~~ timeout_act_check %u %p ~~~~ \n", 
            act->ofpacts_len, act->ofpacts);
    int i;
    fprintf(stderr, "......\n");
    for (i = 0; i < act->ofpacts_len; i++) {
        fprintf(stderr, "%d ", *(((char *) act->ofpacts) + i));
    }
    fprintf(stderr, "......\n");
    
    if (act->ofpacts && act->ofpacts_len > 0) {
       return ofpacts_check(act->ofpacts, act->ofpacts_len, flow, OFPP_MAX, 0);
    }
    return 0; 
}

void timeout_act_format(const struct ofpact_timeout_act *act, struct ds *s)
{
    int i;
    const struct ofpact *a;
    ds_put_cstr(s, "timeout_act(");

    fprintf(stderr, "timeout_act_format act->ofpact=%p act->ofpacts_len=%u\n",
            act->ofpacts, act->ofpacts_len);
    
    OFPACT_FOR_EACH (a, act->ofpacts, act->ofpacts_len) {
        fprintf(stderr, "timeout_act_format LOOP %p\n", a);
        ofpact_format(a, s);
        ds_put_char(s, ',');
        break;
    }

    ds_put_char(s, ')');
}

/* 
* Actions taken cannot assume that there's a packet, because there isn't one.
*/
/*void timeout_act_execute(const struct ofpact_timeout_act *act,
                         struct flow *flow, struct rule *rule)
{
    struct ofproto *ofproto;
    struct ofpact_learn *learn;
    struct ofpact *a;
    struct ofputil_flow_mod fm;
    ofproto = rule->ofproto;
 
    struct ofpbuf ofpacts_buf;
    uint64_t ofpacts_stub[1024 / 8];

    if (act->ofpacts && act->ofpacts_len > 0) {
        int i;
        for (i = 0; i < act->ofpacts_len; i++) {
            a = &act->ofpacts[i];
            switch (a->type) {
                case OFPACT_LEARN:
                    ofpbuf_use_stub(&ofpacts_buf, ofpacts_stub, sizeof ofpacts_stub);

                    // Create flow_mod
                    learn = ofpact_get_LEARN(a);

                    if (!learn->learn_on_timeout) {
                        continue;
                    }

                    // Populate fm with the learn attributes
                    // TODO - 3rd arguement is ofpact*, but function takes in ofpbuf
                    //ovs_mutex_unlock(&ofproto_mutex);
                    timeout_learn_execute(learn, &fm, &ofpacts_buf);
                    // ofproto_flow_mod(struct ofproto *ofproto, struct ofputil_flow_mod *fm)
                    ofproto_flow_mod(ofproto, &fm);
                    //ovs_mutex_trylock(&ofproto_mutex);
                    break;
                case OFPACT_TIMEOUT_ACT:
                    timeout_act_execute(ofpact_get_TIMEOUT_ACT(a), flow, rule);
                    break;
                case OFPACT_REG_LOAD:
                    nxm_execute_reg_load(ofpact_get_REG_LOAD(a), flow);
                    break;
                    // Do nothing in the default case, because it isn't supported.
                case OFPACT_CONTROLLER:
                case OFPACT_OUTPUT:
                case OFPACT_ENQUEUE:
                case OFPACT_OUTPUT_REG:
                case OFPACT_BUNDLE:
                case OFPACT_SET_VLAN_VID:
                case OFPACT_SET_VLAN_PCP:
                case OFPACT_STRIP_VLAN:
                case OFPACT_PUSH_VLAN:
                case OFPACT_SET_ETH_SRC:
                case OFPACT_SET_ETH_DST:
                case OFPACT_SET_IPV4_SRC:
                case OFPACT_SET_IPV4_DST:
                case OFPACT_SET_IPV4_DSCP:
                case OFPACT_SET_L4_SRC_PORT:
                case OFPACT_SET_L4_DST_PORT:
                case OFPACT_REG_MOVE:

                case OFPACT_STACK_PUSH:
                case OFPACT_STACK_POP:
                case OFPACT_DEC_TTL:
                case OFPACT_SET_MPLS_TTL:
                case OFPACT_DEC_MPLS_TTL:
                case OFPACT_PUSH_MPLS:
                case OFPACT_POP_MPLS:
                case OFPACT_SET_TUNNEL:
                case OFPACT_SET_QUEUE:
                case OFPACT_POP_QUEUE:
                case OFPACT_FIN_TIMEOUT:
                case OFPACT_RESUBMIT:
                case OFPACT_MULTIPATH:
                case OFPACT_NOTE:
                case OFPACT_EXIT:
                case OFPACT_SAMPLE:
                case OFPACT_METER:
                case OFPACT_CLEAR_ACTIONS:
                case OFPACT_WRITE_METADATA:
                case OFPACT_GOTO_TABLE:
                    break;
            }
        }
    }
}
*/

static char * WARN_UNUSED_RESULT
timeout_act_parse__(char *orig, char *arg, struct ofpbuf *ofpacts) {
    /* -- ORIGINAL - Works, but error in check -- */
    struct ofpact_timeout_act *act;
    char *act_str = xstrdup(orig);

    fprintf(stderr, "timeout_act_parse__ called: %s \n", arg);

    struct ofpbuf timeout_ofpacts;
    char *error;
    unsigned int len;
 
    act = ofpact_put_TIMEOUT_ACT(ofpacts);
    
    ofpbuf_init(&timeout_ofpacts, 32);
    enum ofputil_protocol usable_protocols;
    //usable_protocols = OFPUTIL_P_ANY;
    //usable_protocols = OFPUTIL_P_OF10_STD | OFPUTIL_P_OF10_STD_TID;
    usable_protocols = OFPUTIL_P_OF10_STD_TID;
    

    error = str_to_inst_ofpacts(act_str, &timeout_ofpacts, &usable_protocols);
    if (error) {
        ofpbuf_uninit(&timeout_ofpacts);
        free(act_str);
        return error;
    }
    
    len = timeout_ofpacts.size;
    fprintf(stderr, "timeout_act_parse__ len=%u timeout_ofpacts.size=%u\n",
            len, timeout_ofpacts.size);
    
    ofpbuf_put_zeros(ofpacts, len);
    
    //fprintf(stderr, "act->ofpacts=%p\n", act->ofpacts);
    //memcpy(act->ofpacts, timeout_ofpacts.data, len);

    act = ofpacts->l2;
    act->ofpacts_len += len;

    act->ofpacts = ofpbuf_steal_data(&timeout_ofpacts);
    int i;
    fprintf(stderr, "...... ");
    for (i = 0; i < len; i++) {
        fprintf(stderr, "%d ", *(((char *) act->ofpacts) + i));
    }
    fprintf(stderr, "......\n");

    ofpact_update_len(ofpacts, &act->ofpact);
    
    fprintf(stderr, "timeout_act_parse__ act=%p ofpacts=%p\n", act, act->ofpacts);
    fprintf(stderr, "timeout_act_parse__ timeout_ofpacts.size=%u \n", len);

    free(act_str);
    return NULL;

    /*
    struct ofpact_timeout_act *act;
    char *act_str = xstrdup(orig);

    fprintf(stderr, "timeout_act_parse__ called: %s \n", arg);

    struct ofpbuf timeout_ofpacts;
    char *error;
    unsigned int len;
 
    act = ofpact_put_TIMEOUT_ACT(ofpacts);
    
    ofpbuf_init(&timeout_ofpacts, 32);
    enum ofputil_protocol usable_protocols;
    usable_protocols = OFPUTIL_P_ANY;

    len = ofpacts->size;
    error = str_to_inst_ofpacts(act_str, &timeout_ofpacts, &usable_protocols);
    if (error) {
        fprintf(stderr, "PARSE ERROR WOMP WOMP\n");
        ofpbuf_uninit(&timeout_ofpacts);
        free(act_str);
        return error;
    }
    len = timeout_ofpacts.size;
    //act->ofpacts_len += len;

    //len = timeout_ofpacts.size;
    //ofpbuf_put(ofpacts, timeout_ofpacts.data, len);
    
    ofpbuf_put_zeros(ofpacts, len);
    
    //memcpy(act->ofpacts, ofpbuf_steal_data(&timeout_ofpacts), len);
    //memcpy(act->ofpacts, &timeout_ofpacts.data, len);
    //act->ofpacts = ofpbuf_put(ofpacts, ofpbuf_steal_data(&timeout_ofpacts), len);

    act = ofpacts->l2;
    act->ofpacts_len += len;
    act->ofpacts = ofpbuf_steal_data(&timeout_ofpacts);
    
    fprintf(stderr, "timeout_act_parse__ len=%u act->ofpacts_len=%u act->ofpacts=%p\n",
            len, act->ofpacts_len, act->ofpacts);

    int i;
    fprintf(stderr, "...... ");
    for (i = 0; i < len; i++) {
        fprintf(stderr, "%d ", *(((char *) act->ofpacts) + i));
    }
    fprintf(stderr, "......\n");

    ofpact_update_len(ofpacts, &act->ofpact);
    
    fprintf(stderr, "timeout_act_parse__ act=%p ofpacts=%p act->ofpact_len=%u\n",
            act, act->ofpacts, act->ofpacts_len);
    fprintf(stderr, "timeout_act_parse__ len=%u \n", len);

    free(act_str);
    return NULL;
    */
}

char * WARN_UNUSED_RESULT
timeout_act_parse(char *arg, struct ofpbuf *ofpacts) {
    char *orig = xstrdup(arg);
    char *error = timeout_act_parse__(orig, arg, ofpacts);
    free(orig);
    return error;
}
