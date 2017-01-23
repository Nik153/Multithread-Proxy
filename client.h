#ifndef CLIENT_H
#define CLIENT_H


class Client
{
public:
    Client( int socket_fd );
    ~Client();

    int get_socket_fd();

    void do_receive();

    bool is_request_complete();

    void send_bad_request();

    bool is_request_good();
    char * get_uri();

    void toss_request_to_downloader ( Downloader * downloader );

    void read_from_cache();

    void set_closing();
    bool is_closing();
    void set_read_from(Buffer * buffer);

private:
    int socket_fd;

    Buffer * request;
    Buffer * read_from;

    char * buff;

    bool request_complete;
    bool to_close;

    int reading_offset;
};

#endif
