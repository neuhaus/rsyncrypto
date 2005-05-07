#ifndef RANDOM_H
#define RANDOM_H

class random {
public:
    static void rand(void *buff, size_t size)
    {
        if( !RAND_bytes(static_cast<unsigned char *>(buff), size) )
            throw rscerror("No random entropy for key and IV");
    }
    static unsigned char rand()
    {
        unsigned char ret;
        rand( &ret, sizeof(ret) );

        return ret;
    }
};

#endif // RANDOM_H