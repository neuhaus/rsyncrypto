#ifndef RCS_ERROR_H
#define RCS_ERROR_H

class rscerror {
    TSTRING msg;
    TSTRING sysmsg;
    TSTRING param;
    int errnum;
public:
    explicit rscerror( const TSTRING &msg_p ) : msg(msg_p)
    {
    }
    explicit rscerror( const TSTRING &msg_p, int error, const TSTRING &param_p=_T("") ) : msg(msg_p),
                                                                                sysmsg(a2t(strerror(error))),
                                                                                param(param_p),
                                                                                errnum(error)
    {
    }

    TSTRING error() const {
        TSTRING ret(msg);
        if( param.length()!=0 )
            ret+=_T("(")+param+_T(")");
        if( sysmsg.length()!=0 )
            ret+=_T(": ")+sysmsg;

        return ret;
    }
    int errornum() const {
        return errnum;
    }
};

class delayed_error : public rscerror
{
public:
    delayed_error() : rscerror(_T("Exit code delayed from previous errors"))
    {
    }
};

#define EXCEPT_CLASS rscerror

#endif // RCS_ERROR_H
