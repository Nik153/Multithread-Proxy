#include "headers.h"

using namespace std;

void exit_handler(int sig);
void exit_with(const char * mess);
void init_server_socket();
void * client_thread ( void * client_ );
void * downloader_thread ( void * downloader_ );

void handle_new_connection();

int server_socket_fd;

pthread_mutex_t cache_mutex;
map < string , Buffer * > cache;

int port_to_bind;

int main(int argc, char** argv)
{
    cout << "Starting proxy!" << endl;

    signal(SIGINT, exit_handler);
    signal(SIGKILL, exit_handler);
    signal(SIGTERM, exit_handler);
    signal(SIGABRT, exit_handler);

    struct rlimit rl;

    getrlimit (RLIMIT_AS, &rl);
    rl.rlim_cur = 1024 * 1024 * 1024;
    rl.rlim_max = 1024 * 1024 * 1024;
    setrlimit (RLIMIT_AS, &rl);;

    pthread_mutex_init( &cache_mutex, NULL );

    if (argc != 2)
        exit_with("Usage: ./a.out port_to_bind");

    port_to_bind = atoi(argv[1]);

    cout << "Initing server socket." << endl;

    init_server_socket();

    cout << "Starting main loop." << endl;

    for (;;)
    {
        handle_new_connection();
    }

    return 0;
}

void init_server_socket()
{
    server_socket_fd = socket(PF_INET, SOCK_STREAM, 0);

    if(server_socket_fd < 0)
        exit_with ( "socket error." );

    int enable = 1;
    if ( setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) )
        exit_with (  "setsockopt error." );

    struct sockaddr_in server_sockaddr;
    server_sockaddr.sin_family = PF_INET;
    server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_sockaddr.sin_port = htons( port_to_bind );

    int res = bind(server_socket_fd, (struct sockaddr * ) &server_sockaddr, sizeof(server_sockaddr));
    if(res < 0)
        exit_with("bind error.");

    res = listen(server_socket_fd, 1024);
    if(res)
        exit_with("listen error.");
}

void handle_new_connection()
{
    struct sockaddr_in client_sockaddr_in;
    int size_of_address = sizeof ( sockaddr_in );
    int client_socket_fd = accept ( server_socket_fd, ( struct sockaddr * ) &client_sockaddr_in, ( socklen_t * ) &size_of_address );

    if ( client_socket_fd < 0 )
    {
        cout << "accept error." << endl;
        cout << "CANT HANDLE CLIENT" << endl;
        return;
    }

    Client * client = new Client ( client_socket_fd );

    pthread_t thread;
    if ( pthread_create ( &thread, NULL, client_thread, ( void * ) client ) )
        perror ( "pthread_create error" );

    cout << "CLIENT ACCEPTED" << endl;
    return;
}

void * client_thread (void * client_ )
{
    Client * client = ( Client * ) client_;

    while ( ! client -> is_closing() && ! client -> is_request_complete() )
    {
        client -> do_receive();
        if ( client -> is_request_complete() )
        {
            if ( ! client -> is_request_good() )
                client -> send_bad_request();
            else
            {
                char * uri = client -> get_uri();
                string s_uri(uri);
                Buffer * buffer;

                pthread_mutex_lock ( &cache_mutex );

                if ( ! cache.count( s_uri ) )
                {
                    buffer = new Buffer;
                    cache [s_uri] = buffer;
                    Downloader * downloader = new Downloader ( uri, buffer );
                    client -> toss_request_to_downloader ( downloader );
                    pthread_t thread;
                    if ( pthread_create ( &thread, NULL, downloader_thread, ( void * ) downloader ) )
                    {
                        perror ( "pthred error." );
                        client -> send_bad_request();
                    }
                }
                else
                    buffer = cache [s_uri];

              pthread_mutex_unlock ( &cache_mutex );

              client -> set_read_from ( buffer );

                 }
        }
    }

    while ( ! client -> is_closing())
    {
        client -> read_from_cache();
    }

    cout << "DONE WITH CLIENT" << endl;
    delete client;

    return (NULL);
}

void * downloader_thread ( void * downloader_ )
{
    Downloader * downloader = ( Downloader * ) downloader_;

    while ( ! downloader -> is_closing() && ! downloader -> is_request_sent() )
        downloader -> do_send();
    while ( ! downloader -> is_closing() )
        downloader -> do_receive();
    delete downloader;

    return (NULL);
}

void exit_with(const char * mess)
{
    perror ( mess );
    cout << endl;
    exit_handler(153);
}

void exit_handler (int sig)
{
    cout << endl << "Exiting with code: " << sig << endl;

    for ( auto it = cache.begin(); it != cache.end() ; )
    {
        delete it -> second;
        it = cache.erase(it);
    }

    pthread_mutex_destroy ( &cache_mutex );

    close(server_socket_fd);

    exit(EXIT_SUCCESS);
}
