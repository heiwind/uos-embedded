#ifndef __UOS_ERRNO_H__
#define __UOS_ERRNO_H__


//* look posix/errors.h - to see code mutches 

enum sock_error_state{
      SESOCKANY         = (~0xfful) //A limit of sock errors values area: any error < ESOCKANY  
    , SECONNABORTED     = SESOCKANY+103    //A connection has been aborted.
    , SENOTCONN         = SESOCKANY+107    //socket is associated with a connection-oriented protocol and has not been connected 
    , SEINVAL           = SESOCKANY+22     //Socket is not listening for connections
    , SEAGAIN           = SESOCKANY+11     //The socket is marked nonblocking and no connections are present to be accepted.
    , SENOMEM           = SESOCKANY+12     //
    , SENOBUFS          = SESOCKANY+105    //
    , SEBADF            = SESOCKANY+9      //The descriptor is invalid.
    , SEBADFD           = SESOCKANY+77     //* File descriptor in bad state */
    //, SENOTSOCK                  //The descriptor references a file, not a socket.
    , SENFILE           = SESOCKANY+23     //The system limit on the total number of open files has been reached.
    , SENETUNREACH      = SESOCKANY+101    //* Network is unreachable
    , SETIMEDOUT        = SESOCKANY+110
};
typedef enum sock_error_state sock_error;

#define SEANYERROR(p)   (~((unsigned)p) <= 0xff)


#endif //__UOS_ERRNO_H__
