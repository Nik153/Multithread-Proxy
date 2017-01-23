#ifndef DOWNLOADER_H
#define DOWNLOADER_H


class Downloader
{
public:
    Downloader(char *uri , Buffer *buffer);
    ~Downloader();

    int get_socket_fd();

    void set_request ( Buffer * request );
    bool is_request_sent();

    void do_send();
    void do_receive();

    bool is_closing();

private:
    int socket_fd;

    bool to_close;
    bool request_sent;

    int sending_offset;

    char * uri;
    char * buff;
    Buffer * request;
    Buffer * page;

    bool is_ok;
};

#endif
