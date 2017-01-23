#include "headers.h"

using namespace std;

Client::Client( int socket_fd )
{
    this -> socket_fd = socket_fd;
    this -> request = new Buffer;
    this -> to_close = false;
    this -> request_complete = false;
    this -> reading_offset = 0;
    this -> buff = ( char * ) malloc ( BUFF_SIZE + 1 );
    buff[BUFF_SIZE] = '\0';
    std::cout << "CREATING CLIENT" << std::endl;
}

Client::~Client()
{
    close ( socket_fd );
    if ( buff )
    {
        free ( buff );
        buff = NULL;
    }
    if ( request )
    {
        delete request;
        request = NULL;
    }
    read_from = NULL;
    std::cout << "DESTROYING CLIENT" << std::endl;
}

void Client::do_receive()
{
    int received = recv ( socket_fd, buff, BUFF_SIZE, 0 );
    buff[received] = '\0';

    switch ( received ) {
    case -1 :
        perror("recv error.");
        to_close = true;
        break;
    case 0:
//        cout << "REECEIVING COMPLETED" << endl;
        to_close = true;
        break;
    default:
        request -> write_to_buffer ( buff, received );
        if ( request -> is_request_complete() )
            request_complete = true;
    }

}

bool Client::is_request_complete() { return request_complete ; }

bool Client::is_request_good()
{
    return request -> is_request_good();
}

void Client::send_bad_request()
{
    const char * response;

    response = "HTTP/1.0 400 Bad request\r\nConnection: close\r\nContent-Length: 43\r\n\r\n"
               "<h1>My proxy supports only GET and HTTP/1.0</h1>";

    int sent = send ( socket_fd, response, strlen(response), 0 );
    switch (sent) {
    case -1:
        perror ( " send" );
        to_close = true;
        break;
    case 0:
        cout << "SENDING COMPLETE" << endl;
        to_close = true;
        break;
    default:
        to_close = true;
        break;
    }
}

void Client::toss_request_to_downloader( Downloader * downloader )
{
    downloader -> set_request ( request );
    request = NULL;
}

char * Client::get_uri()
{
    return request -> get_uri();
}

void Client::set_read_from(Buffer * buffer)
{
    read_from = buffer;
}

void Client::read_from_cache()
{   
    int got = read_from -> get_from_buffer ( reading_offset, buff, BUFF_SIZE);
    buff[got] = '\0';

    if ( ! got )
    {
        if ( read_from -> is_complete() && reading_offset >= read_from -> get_size())
            to_close = true;
        return;
    }

    int sent = send ( socket_fd, buff, got, 0 );
    switch (sent)
    {
    case -1:
        perror ("send error.");
        to_close = true;
        break;
    case 0:
        std::cout << "SENDING DONE" << endl;
        to_close = true;
        break;
    default:
        reading_offset += got;
        if ( read_from -> is_complete() && reading_offset >= read_from -> get_size())
            to_close = true;
    }

}

bool Client::is_closing() { return to_close; }
