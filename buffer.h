#ifndef BUFFER_H
#define BUFFER_H


class Buffer
{
public:
    Buffer();
    ~Buffer();

    void write_to_buffer( char * message, int message_len );
    void reallocate( int space_needed );

    bool is_request_complete();

    bool is_request_good();
    char * get_uri();

    char * parse_first_line();

    void set_complete();
    bool is_complete();

    int get_from_buffer( int offset, char * buff, int buff_len );
    int get_size();

private:
    char * content;
    int filled_space;
    int empty_space;
    int capacity;

    bool complete;

    pthread_mutex_t buffer_mutex;
    pthread_cond_t buffer_cond;
};

#endif
