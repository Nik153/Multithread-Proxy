#include "headers.h"

Buffer::Buffer()
{
    content = ( char * ) malloc ( BUFF_SIZE + 1 );
    content[BUFF_SIZE] = '\0';
    capacity = empty_space = BUFF_SIZE;
    filled_space = 0;
    complete = false;

    pthread_mutex_init ( &buffer_mutex, NULL );
    pthread_cond_init ( &buffer_cond, NULL );

    std::cout << "CREATING BUFFER" <<std::endl;
}

Buffer::~Buffer()
{
    if ( content )
    {
        free ( content );
        content = NULL;
    }

    pthread_mutex_destroy( &buffer_mutex );
    pthread_cond_destroy( &buffer_cond );
    std::cout << "DESTROYING BUFFER" <<std::endl;
}

void Buffer::write_to_buffer ( char * message, int message_len )
{
    pthread_mutex_lock ( &buffer_mutex );
    if ( message_len > empty_space )
        reallocate ( message_len );

    memcpy ( content + filled_space, message, message_len );

    empty_space -= message_len;
    filled_space += message_len;

    pthread_mutex_unlock ( &buffer_mutex );
    pthread_cond_broadcast( &buffer_cond );
}

void Buffer::reallocate ( int space_needed )
{
    do {
        empty_space += capacity;
        capacity *= 2;
    } while ( space_needed > empty_space );
    content = ( char * ) realloc ( content, capacity + 1 );
    content[capacity] = '\0';
}

int Buffer::get_from_buffer ( int offset, char * buff, int buff_len )
{
    pthread_mutex_lock ( &buffer_mutex );

    if ( offset >= filled_space )
        pthread_cond_wait( &buffer_cond, &buffer_mutex );
    if ( buff_len > filled_space - offset )
        buff_len = filled_space - offset;
    memcpy ( buff, content + offset, buff_len);

    pthread_mutex_unlock ( &buffer_mutex );

    return buff_len;
}

bool Buffer::is_request_complete()
{
    pthread_mutex_lock ( &buffer_mutex );

    bool result = false;

    if (strstr(content,"\r\n\r\n"))
        result = complete = true;

    pthread_mutex_unlock ( &buffer_mutex );

    return result;
}

bool Buffer::is_request_good()
{
    pthread_mutex_lock ( &buffer_mutex );

    bool result = strstr ( content, "GET" ) && strstr ( content, "HTTP/1.0")  ;

    pthread_mutex_unlock ( &buffer_mutex );

    return result;
}

char * Buffer::get_uri()
{
    pthread_mutex_lock ( &buffer_mutex );

    char * uri_start = strstr ( content, "http://" ) + strlen ( "http://" );
    char * uri_end = strstr ( content, "HTTP/1.0" );
    int uri_len = uri_end - uri_start;
    char * uri = ( char * ) malloc ( uri_len + 1 );
    strncpy ( uri, uri_start, uri_len );
    uri[uri_len] = '\0';

    pthread_mutex_unlock ( &buffer_mutex );

    return uri;
}

bool Buffer::is_complete()
{
    pthread_mutex_lock ( &buffer_mutex );

    bool _complete = complete;

    pthread_mutex_unlock ( &buffer_mutex );

    return _complete;
}

void Buffer::set_complete()
{
    pthread_mutex_lock ( &buffer_mutex );

    complete = true;

    pthread_mutex_unlock ( &buffer_mutex );
}

int Buffer::get_size()
{
    pthread_mutex_lock ( &buffer_mutex );

    int _filled_space = filled_space;

    pthread_mutex_unlock ( &buffer_mutex );

    return _filled_space;
}
