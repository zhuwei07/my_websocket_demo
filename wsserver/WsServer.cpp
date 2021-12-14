#include "libwebsockets.h"
#include <signal.h>
#include <string.h>

static volatile int exit_sig = 0;
#define MAX_PAYLOAD_SIZE  10 * 1024

void sighdl( int sig ) {
    lwsl_notice( "%d traped", sig );
    exit_sig = 1;
}

/**
 * 会话上下文对象，结构根据需要自定义
 */
struct session_data {
    int msg_count;
    unsigned char buf[LWS_PRE + MAX_PAYLOAD_SIZE];
    int len;
    bool bin;
    bool fin;
};

static int websocket_write_back(struct lws *wsi_in, char *str, int str_size_in)   //此函数从别处借来，再此非常感谢
{
    if (str == NULL || wsi_in == NULL)
        return -1;

    int n;
    int len;
    unsigned char *out = NULL;

    if (str_size_in < 1)
        len = strlen(str);
    else
        len = str_size_in;

    out = (unsigned char *)malloc(sizeof(unsigned char) * (LWS_SEND_BUFFER_PRE_PADDING + len + LWS_SEND_BUFFER_POST_PADDING));
    //* setup the buffer*/  
    memcpy(out + LWS_SEND_BUFFER_PRE_PADDING, str, len);  //要发送的数据从此处拷贝
    //* write out*/  
    n = lws_write(wsi_in, out + LWS_SEND_BUFFER_PRE_PADDING, len, LWS_WRITE_TEXT);  //lws的发送函数
    lwsl_notice("Server Send message.\n");

  //  printf("[websocket_write_back] %s\n", str);  
    //* free the buffer*/  
    free(out);

    return n;
}

static int protocol_my_callback( struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len ) {
    switch ( reason ) {
        case LWS_CALLBACK_ESTABLISHED:       // 当服务器和客户端完成握手后
            lwsl_notice("Client connect!\n");
            break;
        case LWS_CALLBACK_RECEIVE:           // 当接收到客户端发来的帧以后
            break;
        case LWS_CALLBACK_SERVER_WRITEABLE:   // 当此连接可写时
            //lws_write( wsi, &data->buf[ LWS_PRE ], data->len, LWS_WRITE_TEXT );
            //// 下面的调用允许在此连接上接收数据
            //lws_rx_flow_control( wsi, 1 );
            websocket_write_back(wsi, "hello client", 0);
            Sleep(1000);
            break;
        default:
            break;
    }
    // 回调函数最终要返回0，否则无法创建服务器
    return 0;
}

/**
 * 支持的WebSocket子协议数组
 * 子协议即JavaScript客户端WebSocket(url, protocols)第2参数数组的元素
 * 你需要为每种协议提供回调函数
 */
struct lws_protocols protocols[] = {
    {
        //协议名称，协议回调，接收缓冲区大小
        "ws", protocol_my_callback, sizeof( struct session_data ), MAX_PAYLOAD_SIZE,
    },
    {
        NULL, NULL,   0 // 最后一个元素固定为此格式
    }
};
 
int main(int argc,char **argv)
{
    // 信号处理函数
    signal( SIGTERM, sighdl );
 
    struct lws_context_creation_info ctx_info = { 0 };
    ctx_info.port = 8000;
    ctx_info.iface = NULL; // 在所有网络接口上监听
    ctx_info.protocols = protocols;
    ctx_info.gid = -1;
    ctx_info.uid = -1;

    ctx_info.ssl_ca_filepath = NULL;
    ctx_info.ssl_cert_filepath = NULL;
    ctx_info.ssl_private_key_filepath = NULL;
    ctx_info.options = 0;
    
    struct lws_context *context = lws_create_context(&ctx_info);
    while ( !exit_sig ) {
        lws_callback_on_writable_all_protocol(context, &protocols[0]); //激活写数据的case
        lws_service(context, 1000);
    }
    lws_context_destroy(context);

    return 0;
}
