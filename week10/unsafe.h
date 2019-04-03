/**
 * Created by ilya on 4/3/19.
 */

#ifndef NETWORKS_LABS_UNSAFE_H
#define NETWORKS_LABS_UNSAFE_H

#include "node.h"

#define EZ(o1) ((o1) == 0)
#define NEZ(o1) ((o1) != 0)

#define DCLi(name,type,value) type name = (value);
#define T_ND(v_ptr) ((node_t*) v_ptr)
#define T_C$(v_ptr) ((char*) v_ptr)

#define _S(s,v,l,f) send(s,v,l,f);

#endif //NETWORKS_LABS_UNSAFE_H
