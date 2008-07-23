#ifndef RCS_ERROR_H
#define RCS_ERROR_H

class rscerror {
    std::string msg;
    std::string sysmsg;
    std::string param;
    int errnum;
public:
    explicit rscerror( const char *msg_p ) : msg(msg_p)
    {
    }
    explicit rscerror( const char *msg_p, int error, const char *param_p="" ) : msg(msg_p),
                                                                                sysmsg(strerror(error)),
                                                                                param(param_p),
                                                                                errnum(error)
    {
    }

    std::string error() const {
        std::string ret(msg);
        if( param.length()!=0 )
            ret+="("+param+")";
        if( sysmsg.length()!=0 )
            ret+=": "+sysmsg;

        return ret;
    }
    int errornum() const {
        return errnum;
    }
};

class delayed_error : public rscerror
{
public:
    delayed_error() : rscerror("Exit code delayed from previous errors")
    {
    }
};

#define EXCEPT_CLASS rscerror

#endif // RCS_ERROR_H
