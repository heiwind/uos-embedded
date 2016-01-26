#include <runtime/lib.h>
#include <net/ip.h>
#include <net/arpa/inet.h>
#include <stream/stream.h>

static char iptoasn_buf[16];

char*   inet_iptoasn( ip_addr ip, char *cp, unsigned cplen){
    if (cp == NULL){
        cp = iptoasn_buf;
        cplen = sizeof(iptoasn_buf); 
    }
    snprintf((unsigned char *)cp, cplen, "%u.%u.%u.%u"
                , (unsigned)ip.ucs[0]
                , (unsigned)ip.ucs[1]
                , (unsigned)ip.ucs[2]
                , (unsigned)ip.ucs[3]
            );
    return cp;
}

int     inet_atoip(const char *cp, ip_addr *ip){
    unsigned node[4];
    int ok = sscanf((const unsigned char *)cp, "%u.%u.%u.%u", &node[0], &node[1], &node[2], &node[3]);
    if (ok == 4){
        unsigned n = node[0]|node[1]|node[2]|node[3];
        if (n < 0xff){
            n = node[3];
            n = (n<<8)|node[2];
            n = (n<<8)|node[1];
            n = (n<<8)|node[0];
            ip->val = n;
            return n;
        }
    }
    return 0;
}
