#ifndef FILELIST_H
#define FILELIST_H

class metadata {
    std::string plainname;
    std::string ciphername;
#ifdef POSIX
    uid_t uid;
    gid_t gid;
    mode_t mode; 
#endif
public:
    static void fill_map( const char *list_filename, bool encrypt );
};

extern std::map<std::string, metadata> filelist;

#endif
